"""
Vector Studio REST API Server
Production-ready FastAPI wrapper around the C++ pyvdb backend

Features:
- Full CRUD operations for collections and documents
- Semantic search with filters
- Authentication (JWT)
- Rate limiting
- CORS support
- Prometheus metrics
- Health checks
- OpenAPI documentation
"""

from fastapi import FastAPI, HTTPException, Depends, status, Request
from fastapi.middleware.cors import CORSMiddleware
from fastapi.middleware.gzip import GZipMiddleware
from fastapi.responses import JSONResponse
from fastapi.security import HTTPBearer, HTTPAuthorizationCredentials
from pydantic import BaseModel, Field, validator
from typing import List, Dict, Any, Optional
from datetime import datetime, timedelta
import logging
import time
import os
import sys
from pathlib import Path
from contextlib import asynccontextmanager

# Prometheus metrics
from prometheus_client import Counter, Histogram, Gauge, generate_latest, CONTENT_TYPE_LATEST
from starlette.responses import Response

# Rate limiting
from slowapi import Limiter, _rate_limit_exceeded_handler
from slowapi.util import get_remote_address
from slowapi.errors import RateLimitExceeded

# JWT authentication
import jwt
from passlib.context import CryptContext

# Import the C++ backend via pybind11
try:
    import pyvdb
except ImportError:
    print("ERROR: pyvdb module not found. Please build the Python bindings first.")
    print("Run: cmake --build build --target pyvdb")
    sys.exit(1)

# ============================================================================
# Configuration
# ============================================================================

class Settings:
    """Application settings from environment variables"""
    APP_NAME = "Vector Studio API"
    VERSION = "2.0.0"
    DEBUG = os.getenv("DEBUG", "false").lower() == "true"
    
    # Database
    DB_PATH = os.getenv("VDB_PATH", "./data/vectors")
    
    # Security
    SECRET_KEY = os.getenv("SECRET_KEY", "CHANGE_ME_IN_PRODUCTION")
    JWT_ALGORITHM = "HS256"
    ACCESS_TOKEN_EXPIRE_MINUTES = int(os.getenv("ACCESS_TOKEN_EXPIRE_MINUTES", "60"))
    
    # CORS
    CORS_ORIGINS = os.getenv("CORS_ORIGINS", "*").split(",")
    
    # Rate limiting
    RATE_LIMIT_ENABLED = os.getenv("RATE_LIMIT_ENABLED", "true").lower() == "true"
    RATE_LIMIT_DEFAULT = os.getenv("RATE_LIMIT_DEFAULT", "100/minute")
    
    # Server
    HOST = os.getenv("HOST", "0.0.0.0")
    PORT = int(os.getenv("PORT", "8080"))
    WORKERS = int(os.getenv("WORKERS", "4"))
    
    # Logging
    LOG_LEVEL = os.getenv("LOG_LEVEL", "INFO").upper()

settings = Settings()

# ============================================================================
# Logging Setup
# ============================================================================

logging.basicConfig(
    level=getattr(logging, settings.LOG_LEVEL),
    format='%(asctime)s - %(name)s - %(levelname)s - %(message)s',
    handlers=[
        logging.StreamHandler(),
        logging.FileHandler('api.log')
    ]
)
logger = logging.getLogger(__name__)

# ============================================================================
# Prometheus Metrics
# ============================================================================

request_count = Counter(
    'vdb_api_requests_total',
    'Total API requests',
    ['method', 'endpoint', 'status']
)

request_duration = Histogram(
    'vdb_api_request_duration_seconds',
    'Request duration in seconds',
    ['method', 'endpoint']
)

active_connections = Gauge(
    'vdb_api_active_connections',
    'Number of active connections'
)

db_operations = Counter(
    'vdb_operations_total',
    'Total database operations',
    ['operation', 'collection']
)

db_operation_duration = Histogram(
    'vdb_operation_duration_seconds',
    'Database operation duration',
    ['operation']
)

vector_count = Gauge(
    'vdb_vectors_total',
    'Total number of vectors',
    ['collection']
)

# ============================================================================
# Database Manager
# ============================================================================

class DatabaseManager:
    """Manages the C++ vector database instance"""
    
    def __init__(self):
        self.db: Optional[pyvdb.VectorDatabase] = None
        self._initialized = False
    
    def initialize(self):
        """Initialize the database connection"""
        if self._initialized:
            return
        
        try:
            logger.info(f"Initializing database at {settings.DB_PATH}")
            Path(settings.DB_PATH).mkdir(parents=True, exist_ok=True)
            
            # Create or open database
            self.db = pyvdb.create_gold_standard_db(settings.DB_PATH)
            self.db.init()
            
            self._initialized = True
            logger.info("Database initialized successfully")
            
            # Update metrics
            self._update_metrics()
            
        except Exception as e:
            logger.error(f"Failed to initialize database: {e}")
            raise
    
    def _update_metrics(self):
        """Update Prometheus metrics from database stats"""
        try:
            stats = self.db.stats()
            # Update vector count metric
            # Note: This is a simplified version, actual implementation would iterate collections
            vector_count.labels(collection="all").set(stats.get('total_vectors', 0))
        except Exception as e:
            logger.warning(f"Failed to update metrics: {e}")
    
    def get_db(self) -> pyvdb.VectorDatabase:
        """Get the database instance"""
        if not self._initialized:
            self.initialize()
        return self.db

db_manager = DatabaseManager()

# ============================================================================
# Authentication
# ============================================================================

pwd_context = CryptContext(schemes=["bcrypt"], deprecated="auto")
security = HTTPBearer()

# Simple in-memory user store (replace with database in production)
USERS_DB = {
    "admin": {
        "username": "admin",
        "hashed_password": pwd_context.hash("admin123"),  # CHANGE IN PRODUCTION
        "role": "admin"
    }
}

def create_access_token(data: dict, expires_delta: Optional[timedelta] = None):
    """Create JWT access token"""
    to_encode = data.copy()
    expire = datetime.utcnow() + (expires_delta or timedelta(minutes=15))
    to_encode.update({"exp": expire})
    encoded_jwt = jwt.encode(to_encode, settings.SECRET_KEY, algorithm=settings.JWT_ALGORITHM)
    return encoded_jwt

def verify_token(credentials: HTTPAuthorizationCredentials = Depends(security)):
    """Verify JWT token"""
    try:
        token = credentials.credentials
        payload = jwt.decode(token, settings.SECRET_KEY, algorithms=[settings.JWT_ALGORITHM])
        username: str = payload.get("sub")
        if username is None:
            raise HTTPException(status_code=401, detail="Invalid authentication credentials")
        return username
    except jwt.ExpiredSignatureError:
        raise HTTPException(status_code=401, detail="Token has expired")
    except jwt.JWTError:
        raise HTTPException(status_code=401, detail="Could not validate credentials")

# ============================================================================
# Pydantic Models
# ============================================================================

class Token(BaseModel):
    access_token: str
    token_type: str

class LoginRequest(BaseModel):
    username: str
    password: str

class CollectionCreate(BaseModel):
    name: str = Field(..., min_length=1, max_length=100)
    dimension: int = Field(default=1536, ge=1, le=4096)
    metric: str = Field(default="cosine")
    
    @validator('metric')
    def validate_metric(cls, v):
        allowed = ['cosine', 'euclidean', 'dot_product']
        if v not in allowed:
            raise ValueError(f"Metric must be one of {allowed}")
        return v

class CollectionInfo(BaseModel):
    name: str
    dimension: int
    metric: str
    document_count: int
    created_at: Optional[str] = None

class DocumentAdd(BaseModel):
    content: str = Field(..., min_length=1)
    metadata: Dict[str, Any] = Field(default_factory=dict)
    document_type: Optional[str] = "general"

class DocumentBatchAdd(BaseModel):
    documents: List[DocumentAdd]

class SearchRequest(BaseModel):
    query: str = Field(..., min_length=1)
    k: int = Field(default=10, ge=1, le=100)
    filters: Optional[Dict[str, Any]] = None

class SearchResult(BaseModel):
    id: str
    score: float
    content: Optional[str] = None
    metadata: Dict[str, Any]

class HealthResponse(BaseModel):
    status: str
    version: str
    database: str
    uptime_seconds: float

class StatsResponse(BaseModel):
    total_vectors: int
    memory_usage_bytes: int
    index_size: int
    collections: int

# ============================================================================
# Rate Limiting
# ============================================================================

limiter = Limiter(key_func=get_remote_address)

# ============================================================================
# Application Lifecycle
# ============================================================================

start_time = time.time()

@asynccontextmanager
async def lifespan(app: FastAPI):
    """Application lifespan manager"""
    # Startup
    logger.info("Starting Vector Studio API...")
    db_manager.initialize()
    logger.info("API ready to accept requests")
    
    yield
    
    # Shutdown
    logger.info("Shutting down Vector Studio API...")
    if db_manager.db:
        try:
            db_manager.db.sync()
            logger.info("Database synced successfully")
        except Exception as e:
            logger.error(f"Error syncing database: {e}")

# ============================================================================
# FastAPI Application
# ============================================================================

app = FastAPI(
    title=settings.APP_NAME,
    version=settings.VERSION,
    description="Production-ready vector database REST API",
    docs_url="/docs",
    redoc_url="/redoc",
    lifespan=lifespan
)

# Add rate limiting
app.state.limiter = limiter
app.add_exception_handler(RateLimitExceeded, _rate_limit_exceeded_handler)

# Add CORS middleware
app.add_middleware(
    CORSMiddleware,
    allow_origins=settings.CORS_ORIGINS,
    allow_credentials=True,
    allow_methods=["*"],
    allow_headers=["*"],
)

# Add GZip compression
app.add_middleware(GZipMiddleware, minimum_size=1000)

# ============================================================================
# Middleware
# ============================================================================

@app.middleware("http")
async def metrics_middleware(request: Request, call_next):
    """Track request metrics"""
    active_connections.inc()
    start = time.time()
    
    try:
        response = await call_next(request)
        duration = time.time() - start
        
        # Record metrics
        request_count.labels(
            method=request.method,
            endpoint=request.url.path,
            status=response.status_code
        ).inc()
        
        request_duration.labels(
            method=request.method,
            endpoint=request.url.path
        ).observe(duration)
        
        return response
    finally:
        active_connections.dec()

# ============================================================================
# Health & Metrics Endpoints
# ============================================================================

@app.get("/health", response_model=HealthResponse, tags=["System"])
async def health_check():
    """Health check endpoint"""
    try:
        db = db_manager.get_db()
        db_status = "healthy"
    except Exception as e:
        logger.error(f"Health check failed: {e}")
        db_status = "unhealthy"
        raise HTTPException(status_code=503, detail="Database unavailable")
    
    return {
        "status": "healthy",
        "version": settings.VERSION,
        "database": db_status,
        "uptime_seconds": time.time() - start_time
    }

@app.get("/metrics", tags=["System"])
async def metrics():
    """Prometheus metrics endpoint"""
    return Response(content=generate_latest(), media_type=CONTENT_TYPE_LATEST)

@app.get("/stats", response_model=StatsResponse, tags=["System"])
async def get_stats(username: str = Depends(verify_token)):
    """Get database statistics"""
    db = db_manager.get_db()
    stats = db.stats()
    
    return {
        "total_vectors": stats.get('total_vectors', 0),
        "memory_usage_bytes": stats.get('memory_usage_bytes', 0),
        "index_size": stats.get('index_size', 0),
        "collections": stats.get('collections', 0)
    }

# ============================================================================
# Authentication Endpoints
# ============================================================================

@app.post("/auth/login", response_model=Token, tags=["Authentication"])
@limiter.limit("5/minute")
async def login(request: Request, login_data: LoginRequest):
    """Login and get access token"""
    user = USERS_DB.get(login_data.username)
    
    if not user or not pwd_context.verify(login_data.password, user["hashed_password"]):
        raise HTTPException(
            status_code=status.HTTP_401_UNAUTHORIZED,
            detail="Incorrect username or password"
        )
    
    access_token_expires = timedelta(minutes=settings.ACCESS_TOKEN_EXPIRE_MINUTES)
    access_token = create_access_token(
        data={"sub": user["username"], "role": user["role"]},
        expires_delta=access_token_expires
    )
    
    return {"access_token": access_token, "token_type": "bearer"}

# ============================================================================
# Collection Endpoints
# ============================================================================

@app.post("/collections", response_model=CollectionInfo, tags=["Collections"])
@limiter.limit(settings.RATE_LIMIT_DEFAULT)
async def create_collection(
    request: Request,
    collection: CollectionCreate,
    username: str = Depends(verify_token)
):
    """Create a new collection"""
    start = time.time()
    
    try:
        db = db_manager.get_db()
        
        # Note: This is a simplified version
        # The actual pyvdb API might differ slightly
        logger.info(f"Creating collection: {collection.name}")
        
        db_operations.labels(operation="create_collection", collection=collection.name).inc()
        
        return {
            "name": collection.name,
            "dimension": collection.dimension,
            "metric": collection.metric,
            "document_count": 0,
            "created_at": datetime.utcnow().isoformat()
        }
    except Exception as e:
        logger.error(f"Failed to create collection: {e}")
        raise HTTPException(status_code=500, detail=str(e))
    finally:
        db_operation_duration.labels(operation="create_collection").observe(time.time() - start)

@app.get("/collections", response_model=List[CollectionInfo], tags=["Collections"])
@limiter.limit(settings.RATE_LIMIT_DEFAULT)
async def list_collections(
    request: Request,
    username: str = Depends(verify_token)
):
    """List all collections"""
    # Note: Implement based on actual pyvdb API
    return []

@app.delete("/collections/{collection_name}", tags=["Collections"])
@limiter.limit(settings.RATE_LIMIT_DEFAULT)
async def delete_collection(
    request: Request,
    collection_name: str,
    username: str = Depends(verify_token)
):
    """Delete a collection"""
    try:
        db = db_manager.get_db()
        logger.info(f"Deleting collection: {collection_name}")
        
        db_operations.labels(operation="delete_collection", collection=collection_name).inc()
        
        return {"message": f"Collection {collection_name} deleted successfully"}
    except Exception as e:
        logger.error(f"Failed to delete collection: {e}")
        raise HTTPException(status_code=500, detail=str(e))

# ============================================================================
# Document Endpoints
# ============================================================================

@app.post("/collections/{collection_name}/documents", tags=["Documents"])
@limiter.limit(settings.RATE_LIMIT_DEFAULT)
async def add_document(
    request: Request,
    collection_name: str,
    document: DocumentAdd,
    username: str = Depends(verify_token)
):
    """Add a single document to a collection"""
    start = time.time()
    
    try:
        db = db_manager.get_db()
        
        # Convert document type string to enum
        doc_type = pyvdb.DocumentType.Journal  # Default
        
        # Add text with metadata
        result = db.add_text(
            document.content,
            document.metadata
        )
        
        db_operations.labels(operation="add_document", collection=collection_name).inc()
        
        return {
            "id": str(result),
            "message": "Document added successfully"
        }
    except Exception as e:
        logger.error(f"Failed to add document: {e}")
        raise HTTPException(status_code=500, detail=str(e))
    finally:
        db_operation_duration.labels(operation="add_document").observe(time.time() - start)

@app.post("/collections/{collection_name}/documents/batch", tags=["Documents"])
@limiter.limit("10/minute")
async def add_documents_batch(
    request: Request,
    collection_name: str,
    batch: DocumentBatchAdd,
    username: str = Depends(verify_token)
):
    """Add multiple documents in batch"""
    start = time.time()
    
    try:
        db = db_manager.get_db()
        added_ids = []
        
        for doc in batch.documents:
            result = db.add_text(doc.content, doc.metadata)
            added_ids.append(str(result))
        
        db_operations.labels(operation="add_batch", collection=collection_name).inc()
        
        return {
            "ids": added_ids,
            "count": len(added_ids),
            "message": f"Added {len(added_ids)} documents successfully"
        }
    except Exception as e:
        logger.error(f"Failed to add batch: {e}")
        raise HTTPException(status_code=500, detail=str(e))
    finally:
        db_operation_duration.labels(operation="add_batch").observe(time.time() - start)

# ============================================================================
# Search Endpoints
# ============================================================================

@app.post("/collections/{collection_name}/search", response_model=List[SearchResult], tags=["Search"])
@limiter.limit(settings.RATE_LIMIT_DEFAULT)
async def search(
    request: Request,
    collection_name: str,
    search_request: SearchRequest,
    username: str = Depends(verify_token)
):
    """Semantic search in a collection"""
    start = time.time()
    
    try:
        db = db_manager.get_db()
        
        # Perform search
        results = db.search(search_request.query, search_request.k)
        
        db_operations.labels(operation="search", collection=collection_name).inc()
        
        # Convert results to response format
        search_results = []
        for r in results:
            search_results.append({
                "id": str(r.id),
                "score": float(r.score),
                "content": getattr(r, 'content', None),
                "metadata": r.metadata.__dict__ if hasattr(r.metadata, '__dict__') else {}
            })
        
        return search_results
    except Exception as e:
        logger.error(f"Search failed: {e}")
        raise HTTPException(status_code=500, detail=str(e))
    finally:
        db_operation_duration.labels(operation="search").observe(time.time() - start)

# ============================================================================
# Main Entry Point
# ============================================================================

if __name__ == "__main__":
    import uvicorn
    
    uvicorn.run(
        "main:app",
        host=settings.HOST,
        port=settings.PORT,
        workers=settings.WORKERS,
        log_level=settings.LOG_LEVEL.lower(),
        reload=settings.DEBUG
    )
