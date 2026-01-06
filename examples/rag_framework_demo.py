#!/usr/bin/env python3
"""
Vector Studio - RAG & Framework Integration Example
Demonstrates integration with TensorFlow, PyTorch, LangChain, and LlamaIndex
"""

import sys
sys.path.insert(0, '../bindings/python')

def demo_tensorflow_integration():
 """Demonstrate TensorFlow SavedModel integration"""
 print(f"\n{'='*80}")
 print("TENSORFLOW INTEGRATION")
 print(f"{'='*80}")

 print("""
# Load TensorFlow SavedModel for embeddings
from pyvdb.framework import TensorFlowEmbedder

config = TensorFlowEmbedder.Config()
config.model_path = "models/my_encoder/"
config.use_gpu = True

embedder = TensorFlowEmbedder(config)

# Embed text using TensorFlow model
text = "Gold prices surge on inflation fears"
embedding = embedder.embed(text)

# Batch embedding
texts = ["doc1", "doc2", "doc3"]
embeddings = embedder.embed_batch(texts)

# Add to vector database
for emb, txt in zip(embeddings, texts):
 db.add(emb, {"content": txt})

# Export for TensorFlow training
TensorFlowEmbedder.export_for_training(
 vectors=all_vectors,
 labels=all_labels,
 output_path="training_data/tfrecords/"
)
 """)

 print("\nKey Features:")
 print(" Direct TensorFlow SavedModel support")
 print(" GPU acceleration")
 print(" Batch processing for efficiency")
 print(" Export vectors for model training")
 print(" Seamless integration with existing models")

def demo_pytorch_integration():
 """Demonstrate PyTorch TorchScript integration"""
 print(f"\n{'='*80}")
 print("PYTORCH INTEGRATION")
 print(f"{'='*80}")

 print("""
# Load PyTorch TorchScript model
from pyvdb.framework import PyTorchEmbedder

config = PyTorchEmbedder.Config()
config.model_path = "models/my_encoder.pt"
config.device = "cuda"
config.use_half_precision = True # FP16 for speed

embedder = PyTorchEmbedder(config)

# Embed with PyTorch model
embedding = embedder.embed("Test document")

# Export for contrastive learning
from pyvdb.framework import TrainingExporter

exporter = TrainingExporter()
exporter.export_contrastive_pairs(
 positive_pairs=similar_pairs,
 negative_pairs=dissimilar_pairs,
 output_path="training/pairs.jsonl"
)

# Export triplets for metric learning
exporter.export_triplets(
 triplets=[(anchor, positive, negative), ...],
 output_path="training/triplets.jsonl"
)
 """)

 print("\nKey Features:")
 print(" TorchScript model support")
 print(" CUDA/CPU device selection")
 print(" FP16 half-precision inference")
 print(" Export for contrastive/metric learning")
 print(" Triplet mining for embedding training")

def demo_rag_engine():
 """Demonstrate RAG toolkit"""
 print(f"\n{'='*80}")
 print("RAG (RETRIEVAL AUGMENTED GENERATION) ENGINE")
 print(f"{'='*80}")

 print("""
# Configure RAG engine
from pyvdb.framework import RAGEngine

config = RAGEngine.Config()
config.top_k = 5
config.max_context_length = 2048
config.relevance_threshold = 0.7
config.chunking_strategy = "semantic" # semantic, sentence, fixed
config.chunk_size = 512
config.chunk_overlap = 50

rag = RAGEngine(config)

# Chunk long document for indexing
document = load_long_document()
chunks = rag.chunk_document(document)

for chunk in chunks:
 embedding = embedder.embed(chunk)
 db.add(embedding, {"content": chunk})

# Query with RAG
query = "What is the impact of inflation on gold prices?"
query_embedding = embedder.embed(query)
search_results = db.search(query_embedding, k=10)

# Build context from retrieved documents
context = rag.build_context(query, search_results)

# Format prompt for LLM
system_prompt = "You are a helpful financial analyst."
prompt = rag.format_prompt(query, context, system_prompt)

# Send to LLM (e.g., GPT-4, Claude, local LLM)
response = llm.generate(prompt)
print(response)
 """)

 print("\nRAG Workflow:")
 print(" 1. Document Chunking → Split long docs into manageable pieces")
 print(" 2. Vector Indexing → Embed and store chunks")
 print(" 3. Retrieval → Find relevant chunks for query")
 print(" 4. Context Building → Assemble retrieved chunks")
 print(" 5. Prompt Formatting → Create LLM-ready prompt")
 print(" 6. Generation → Get LLM response with context")

 print("\nChunking Strategies:")
 print(" • Fixed: Equal-sized chunks (simple, fast)")
 print(" • Sentence: Split by sentences (preserves meaning)")
 print(" • Paragraph: Split by paragraphs (natural boundaries)")
 print(" • Semantic: Split by semantic similarity (best quality)")
 print(" • Recursive: Hierarchical splitting (flexible)")

def demo_langchain_adapter():
 """Demonstrate LangChain integration"""
 print(f"\n{'='*80}")
 print("LANGCHAIN INTEGRATION")
 print(f"{'='*80}")

 print("""
# Use Vector Studio as LangChain vector store
from pyvdb.framework import LangChainAdapter

config = LangChainAdapter.Config()
config.use_mmr = True # Maximal Marginal Relevance
config.mmr_lambda = 0.5 # Balance relevance vs diversity

langchain = LangChainAdapter(config)

# Standard LangChain operations
docs = langchain.similarity_search("gold prices", k=4)

# With scores
results = langchain.similarity_search_with_score("gold", k=4)

# MMR search for diverse results
diverse_results = langchain.max_marginal_relevance_search(
 query="gold prices",
 k=4, # Final results
 fetch_k=20, # Initial retrieval
 lambda_=0.5 # 0=diversity, 1=relevance
)

# As retriever (returns text only)
texts = langchain.as_retriever("gold prices", k=4)

# Use with LangChain RetrievalQA
from langchain.chains import RetrievalQA
from langchain.llms import OpenAI

qa = RetrievalQA.from_chain_type(
 llm=OpenAI(),
 chain_type="stuff",
 retriever=langchain.as_retriever()
)

answer = qa.run("What factors affect gold prices?")
 """)

 print("\nLangChain Features:")
 print(" Drop-in replacement for LangChain VectorStore")
 print(" All standard LangChain retriever methods")
 print(" MMR (Maximal Marginal Relevance) for diversity")
 print(" Compatible with RetrievalQA, ConversationalRetrievalChain")
 print(" Works with any LangChain LLM")

def demo_llamaindex_adapter():
 """Demonstrate LlamaIndex integration"""
 print(f"\n{'='*80}")
 print("LLAMAINDEX INTEGRATION")
 print(f"{'='*80}")

 print("""
# Use Vector Studio with LlamaIndex
from pyvdb.framework import LlamaIndexAdapter

config = LlamaIndexAdapter.Config()
config.chunk_size = 1024
config.chunk_overlap = 20

llama_index = LlamaIndexAdapter(config)

# Build index from documents
documents = load_documents()
metadata = load_metadata()

llama_index.build_index(documents, metadata)

# Query index
context = llama_index.query(
 "What factors affect gold prices?",
 k=5
)

# Update index dynamically
llama_index.insert("New document about gold", metadata)
llama_index.delete_document(doc_id)

# Persist and reload
llama_index.save("index.llamaindex")
loaded_index = LlamaIndexAdapter.load("index.llamaindex")

# Use with LlamaIndex query engine
from llama_index import VectorStoreIndex

index = VectorStoreIndex.from_vector_store(
 vector_store=llama_index
)

query_engine = index.as_query_engine()
response = query_engine.query("What is gold?")
 """)

 print("\nLlamaIndex Features:")
 print(" Compatible with LlamaIndex VectorStore interface")
 print(" Automatic document chunking")
 print(" Dynamic index updates (insert/delete)")
 print(" Persistence support")
 print(" Works with LlamaIndex query engines")

def demo_document_chunking():
 """Demonstrate advanced document chunking"""
 print(f"\n{'='*80}")
 print("ADVANCED DOCUMENT CHUNKING")
 print(f"{'='*80}")

 print("""
from pyvdb.framework import DocumentChunker, ChunkingStrategy

# Configure chunking
config = DocumentChunker.Config()
config.strategy = ChunkingStrategy.Semantic
config.chunk_size = 512
config.chunk_overlap = 50
config.separator = "\\n\\n"
config.keep_separator = False

chunker = DocumentChunker(config)

# Simple chunking
long_document = load_document()
chunks = chunker.chunk(long_document)

# Chunking with metadata preservation
chunks_with_metadata = chunker.chunk_with_metadata(
 document=long_document,
 base_metadata={"source": "report.pdf", "date": "2026-01-06"}
)

for chunk in chunks_with_metadata:
 print(f"Chunk [{chunk.start_char}:{chunk.end_char}]")
 print(f"Text: {chunk.text}")
 print(f"Metadata: {chunk.metadata}")
 """)

 print("\nChunking Strategies Comparison:")
 print("""
| Strategy | Best For | Quality | Speed |
|------------|-----------------------|---------|-------|
| Fixed | Simple docs, speed | ★★☆☆☆ | ★★★★★ |
| Sentence | General purpose | ★★★☆☆ | ★★★★☆ |
| Paragraph | Structured docs | ★★★★☆ | ★★★☆☆ |
| Semantic | Best quality | ★★★★★ | ★★☆☆☆ |
| Recursive | Hierarchical docs | ★★★★☆ | ★★★☆☆ |
 """)

def demo_training_export():
 """Demonstrate training data export"""
 print(f"\n{'='*80}")
 print("TRAINING DATA EXPORT")
 print(f"{'='*80}")

 print("""
from pyvdb.framework import TrainingExporter, ExportFormat

# Configure exporter
config = TrainingExporter.Config()
config.format = ExportFormat.HuggingFace # or TFRecord, PyTorchDataset, etc.
config.include_metadata = True
config.include_embeddings = True
config.include_text = True

exporter = TrainingExporter(config)

# Export for supervised learning
exporter.export_supervised(
 embeddings=all_embeddings,
 labels=all_labels,
 output_path="training/supervised.parquet"
)

# Export for contrastive learning
exporter.export_contrastive_pairs(
 positive_pairs=similar_doc_pairs,
 negative_pairs=dissimilar_doc_pairs,
 output_path="training/pairs.jsonl"
)

# Export triplets for metric learning
exporter.export_triplets(
 triplets=[(anchor, positive, negative), ...],
 output_path="training/triplets.jsonl"
)

# Export entire database
exporter.export_database(
 database_path="./my_vectors",
 output_path="training/full_database.parquet"
)
 """)

 print("\nExport Formats:")
 print(" • TFRecord: TensorFlow native format")
 print(" • PyTorchDataset: PyTorch Dataset format")
 print(" • HuggingFace: Compatible with Hugging Face datasets")
 print(" • JSONL: JSON Lines (universal)")
 print(" • Parquet: Apache Parquet (efficient, columnar)")

 print("\nTraining Use Cases:")
 print(" • Fine-tune embedding models on domain data")
 print(" • Contrastive learning (similar/dissimilar pairs)")
 print(" • Metric learning (triplet loss)")
 print(" • Supervised classification")
 print(" • Few-shot learning")

def main():
 """Main demonstration"""
 print("""
╔═══════════════════════════════════════════════════════════════╗
║ ║
║ VECTOR STUDIO - RAG & FRAMEWORK INTEGRATION ║
║ ║
║ Seamless integration with TensorFlow, PyTorch, ║
║ LangChain, LlamaIndex, and modern ML frameworks ║
║ ║
╚═══════════════════════════════════════════════════════════════╝
 """)

 print("\n Demonstration Sections:\n")

 # Run demonstrations
 demo_tensorflow_integration()
 demo_pytorch_integration()
 demo_rag_engine()
 demo_langchain_adapter()
 demo_llamaindex_adapter()
 demo_document_chunking()
 demo_training_export()

 # Summary
 print(f"\n{'='*80}")
 print("SUMMARY: Framework Integration Benefits")
 print(f"{'='*80}")
 print("""
 Production-Ready RAG:
 • Complete RAG pipeline (chunking → retrieval → generation)
 • Multiple chunking strategies for different doc types
 • Context optimization for token limits
 • Reranking for improved precision

🤖 Deep Learning Frameworks:
 • TensorFlow SavedModel support
 • PyTorch TorchScript support
 • GPU acceleration
 • Batch processing
 • Export for training

🔗 Ecosystem Integration:
 • LangChain VectorStore adapter
 • LlamaIndex integration
 • Compatible with popular RAG frameworks
 • Drop-in replacement for existing solutions

 Training & Fine-Tuning:
 • Export vectors for model training
 • Contrastive learning pairs
 • Metric learning triplets
 • Multiple export formats (TFRecord, Parquet, JSONL)

 Key Advantages:
 • Use your own custom models
 • Fine-tune on domain-specific data
 • Integrate with existing ML pipelines
 • Production-ready performance
 • No vendor lock-in
 """)

 print("\n All features ready for production use!")
 print("📖 See docs/18_ADVANCED_FEATURES.md for detailed API documentation")

if __name__ == "__main__":
 main()
