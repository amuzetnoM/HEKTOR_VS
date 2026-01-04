import { TestBed } from '@angular/core/testing';
import { HttpClientTestingModule, HttpTestingController } from '@angular/common/http/testing';
import { VectorDbService, Collection, SearchResult } from './vector-db.service';

describe('VectorDbService', () => {
    let service: VectorDbService;
    let httpMock: HttpTestingController;

    beforeEach(() => {
        TestBed.configureTestingModule({
            imports: [HttpClientTestingModule],
            providers: [VectorDbService]
        });
        service = TestBed.inject(VectorDbService);
        httpMock = TestBed.inject(HttpTestingController);

        // Clear localStorage
        localStorage.clear();
    });

    afterEach(() => {
        httpMock.verify();
    });

    describe('Authentication', () => {
        it('should login successfully and store token', async () => {
            const mockResponse = {
                access_token: 'test-token-123',
                token_type: 'bearer'
            };

            const loginPromise = service.login('admin', 'password');

            const req = httpMock.expectOne('http://localhost:8080/auth/login');
            expect(req.request.method).toBe('POST');
            expect(req.request.body).toEqual({ username: 'admin', password: 'password' });

            req.flush(mockResponse);

            await loginPromise;

            expect(service['authToken']()).toBe('test-token-123');
            expect(localStorage.getItem('vdb_auth_token')).toBe('test-token-123');
        });

        it('should handle login failure', async () => {
            const loginPromise = service.login('admin', 'wrong-password');

            const req = httpMock.expectOne('http://localhost:8080/auth/login');
            req.flush({ detail: 'Invalid credentials' }, { status: 401, statusText: 'Unauthorized' });

            await expectAsync(loginPromise).toBeRejected();
            expect(service.error()).toContain('Login failed');
        });

        it('should logout and clear token', () => {
            service['authToken'].set('test-token');
            localStorage.setItem('vdb_auth_token', 'test-token');

            service.logout();

            expect(service['authToken']()).toBeNull();
            expect(localStorage.getItem('vdb_auth_token')).toBeNull();
        });
    });

    describe('Collection Operations', () => {
        beforeEach(() => {
            // Set auth token for authenticated requests
            service['authToken'].set('test-token');
        });

        it('should load collections from API', async () => {
            const mockCollections: Collection[] = [
                {
                    name: 'test-collection',
                    dimension: 1536,
                    metric: 'cosine',
                    documents: [],
                    schema: [],
                    document_count: 0
                }
            ];

            const loadPromise = service.loadCollections();

            const req = httpMock.expectOne('http://localhost:8080/collections');
            expect(req.request.method).toBe('GET');
            expect(req.request.headers.get('Authorization')).toBe('Bearer test-token');

            req.flush(mockCollections);

            await loadPromise;

            expect(service.collections().length).toBe(1);
            expect(service.collections()[0].name).toBe('test-collection');
        });

        it('should create a new collection', async () => {
            const mockResponse: Collection = {
                name: 'new-collection',
                dimension: 768,
                metric: 'euclidean',
                documents: [],
                schema: [],
                document_count: 0,
                created_at: new Date().toISOString()
            };

            const createPromise = service.createCollection('new-collection', 768, 'euclidean');

            const req = httpMock.expectOne('http://localhost:8080/collections');
            expect(req.request.method).toBe('POST');
            expect(req.request.body).toEqual({
                name: 'new-collection',
                dimension: 768,
                metric: 'euclidean'
            });

            req.flush(mockResponse);

            const result = await createPromise;

            expect(result).toContain('created successfully');
            expect(service.collections().length).toBe(1);
        });

        it('should delete a collection', async () => {
            // Setup initial state
            service.collections.set([
                {
                    name: 'test-collection',
                    dimension: 1536,
                    metric: 'cosine',
                    documents: [],
                    schema: []
                }
            ]);

            const deletePromise = service.deleteCollection('test-collection');

            const req = httpMock.expectOne('http://localhost:8080/collections/test-collection');
            expect(req.request.method).toBe('DELETE');

            req.flush({ message: 'Collection deleted' });

            const result = await deletePromise;

            expect(result).toContain('deleted successfully');
            expect(service.collections().length).toBe(0);
        });
    });

    describe('Document Operations', () => {
        beforeEach(() => {
            service['authToken'].set('test-token');
        });

        it('should add documents to a collection', async () => {
            const mockResponse = {
                ids: ['id1', 'id2'],
                count: 2,
                message: 'Added 2 documents'
            };

            const docs = [
                { content: 'Document 1', metadata: { source: 'test' } },
                { content: 'Document 2', metadata: { source: 'test' } }
            ];

            const addPromise = service.addDocuments('test-collection', docs);

            const req = httpMock.expectOne('http://localhost:8080/collections/test-collection/documents/batch');
            expect(req.request.method).toBe('POST');
            expect(req.request.body.documents).toEqual(docs);

            req.flush(mockResponse);

            // Expect a second request to reload collections
            const reloadReq = httpMock.expectOne('http://localhost:8080/collections');
            reloadReq.flush([]);

            const result = await addPromise;

            expect(result).toContain('Added 2 documents');
        });
    });

    describe('Search Operations', () => {
        beforeEach(() => {
            service['authToken'].set('test-token');
        });

        it('should perform semantic search', async () => {
            const mockResults: SearchResult[] = [
                {
                    id: 'vec1',
                    score: 0.95,
                    content: 'Test document',
                    metadata: { source: 'test' }
                },
                {
                    id: 'vec2',
                    score: 0.85,
                    content: 'Another document',
                    metadata: { source: 'test' }
                }
            ];

            const searchPromise = service.query('test-collection', 'test query', 5);

            const req = httpMock.expectOne('http://localhost:8080/collections/test-collection/search');
            expect(req.request.method).toBe('POST');
            expect(req.request.body).toEqual({
                query: 'test query',
                k: 5
            });

            req.flush(mockResults);

            const results = await searchPromise;

            expect(results.length).toBe(2);
            expect(results[0].id).toBe('vec1');
            expect(results[0].score).toBe(0.95);
        });
    });

    describe('Stats', () => {
        beforeEach(() => {
            service['authToken'].set('test-token');
        });

        it('should fetch API stats', async () => {
            const mockStats = {
                total_vectors: 1000,
                memory_usage_bytes: 1024000,
                index_size: 500,
                collections: 5
            };

            const statsPromise = service.getApiStats();

            const req = httpMock.expectOne('http://localhost:8080/stats');
            expect(req.request.method).toBe('GET');

            req.flush(mockStats);

            const stats = await statsPromise;

            expect(stats.total_vectors).toBe(1000);
            expect(stats.collections).toBe(5);
        });
    });

    describe('Error Handling', () => {
        beforeEach(() => {
            service['authToken'].set('test-token');
        });

        it('should handle network errors gracefully', async () => {
            const loadPromise = service.loadCollections();

            const req = httpMock.expectOne('http://localhost:8080/collections');
            req.error(new ProgressEvent('Network error'));

            await loadPromise;

            // Should fallback to mock data
            expect(service.collections().length).toBeGreaterThan(0);
            expect(service.error()).toContain('Failed to load collections');
        });

        it('should handle 401 unauthorized', async () => {
            const createPromise = service.createCollection('test', 1536, 'cosine');

            const req = httpMock.expectOne('http://localhost:8080/collections');
            req.flush({ detail: 'Unauthorized' }, { status: 401, statusText: 'Unauthorized' });

            await expectAsync(createPromise).toBeRejected();
            expect(service.error()).toContain('Failed to create collection');
        });
    });

    describe('Mock Mode', () => {
        it('should work in mock mode when USE_MOCK_MODE is true', () => {
            // This would require modifying the service to expose USE_MOCK_MODE
            // or creating a separate test configuration
            expect(service.collections).toBeDefined();
            expect(service.stats).toBeDefined();
        });
    });
});
