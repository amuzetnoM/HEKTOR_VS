#!/usr/bin/env python3
"""
Vector Studio - Hybrid Search Python Example
Demonstrates BM25 and hybrid fusion in Python
"""

import sys

try:
    import pyvdb
except ImportError:
    print("Error: pyvdb module not found. Build and install Python bindings first.")
    print("Run: cd build && pip install .")
    sys.exit(1)

def main():
    print("=" * 60)
    print("Vector Studio - Hybrid Search Python Demo")
    print("=" * 60)
    print()
    
    # ========================================================================
    # Part 1: BM25 Full-Text Search
    # ========================================================================
    print("[1/3] BM25 Full-Text Search")
    print("-" * 60)
    
    # Create BM25 engine
    bm25_config = pyvdb.BM25Config()
    bm25_config.k1 = 1.2
    bm25_config.b = 0.75
    bm25_config.use_stemming = True
    bm25_config.case_sensitive = False
    
    bm25 = pyvdb.BM25Engine(bm25_config)
    
    # Add documents
    documents = [
        (1, "Gold prices surge to new highs on inflation concerns"),
        (2, "Silver rallies as precious metals gain momentum"),
        (3, "Dollar weakness supports gold and silver prices"),
        (4, "Central banks increase gold reserves"),
        (5, "Mining stocks rise with metal prices"),
    ]
    
    print(f"Indexing {len(documents)} documents...")
    for doc_id, content in documents:
        bm25.add_document(doc_id, content)
    
    print(f"  Documents indexed: {bm25.document_count()}")
    print(f"  Unique terms: {bm25.term_count()}")
    print(f"  Average doc length: {bm25.average_document_length():.1f} terms")
    print()
    
    # Search
    query = "gold prices"
    print(f"Searching for: '{query}'")
    results = bm25.search(query, k=5)
    
    print(f"Found {len(results)} results:")
    for i, result in enumerate(results, 1):
        print(f"  {i}. Document {result.id}: score={result.score:.3f}")
        print(f"     Matched terms: {', '.join(result.matched_terms)}")
    print()
    
    # ========================================================================
    # Part 2: Hybrid Search (Mock Vector Results)
    # ========================================================================
    print("[2/3] Hybrid Search with Fusion Methods")
    print("-" * 60)
    
    # For demonstration, create mock vector results
    # In real usage, these would come from actual vector search
    vector_results = [
        pyvdb.QueryResult(id=1, distance=0.1, score=0.9, metadata=None),
        pyvdb.QueryResult(id=3, distance=0.2, score=0.8, metadata=None),
        pyvdb.QueryResult(id=4, distance=0.3, score=0.7, metadata=None),
    ]
    
    # Get lexical results from BM25
    lexical_results = bm25.search(query, k=5)
    
    # Test different fusion methods
    fusion_methods = [
        (pyvdb.FusionMethod.RRF, "RRF (Reciprocal Rank Fusion)"),
        (pyvdb.FusionMethod.WeightedSum, "Weighted Sum"),
        (pyvdb.FusionMethod.CombSUM, "CombSUM"),
        (pyvdb.FusionMethod.CombMNZ, "CombMNZ"),
        (pyvdb.FusionMethod.Borda, "Borda Count"),
    ]
    
    for method, method_name in fusion_methods:
        print(f"\nFusion Method: {method_name}")
        
        config = pyvdb.HybridSearchConfig()
        config.fusion = method
        config.vector_weight = 0.7
        config.lexical_weight = 0.3
        config.rrf_k = 60
        
        hybrid = pyvdb.HybridSearchEngine(config)
        combined = hybrid.combine(vector_results, lexical_results, k=5)
        
        print(f"  Top 3 results:")
        for i, result in enumerate(combined[:3], 1):
            print(f"    {i}. Doc {result.id}: combined={result.combined_score:.3f}, "
                  f"vector={result.vector_score:.3f}, lexical={result.lexical_score:.3f}")
    
    print()
    
    # ========================================================================
    # Part 3: Static Fusion Methods
    # ========================================================================
    print("[3/3] Static Fusion Method Calculations")
    print("-" * 60)
    
    vec_score = 0.8
    lex_score = 0.6
    
    print(f"Given: vector_score={vec_score}, lexical_score={lex_score}")
    print()
    
    ws = pyvdb.HybridSearchEngine.weighted_sum(vec_score, lex_score, 0.7)
    print(f"  Weighted Sum (0.7 weight): {ws:.4f}")
    
    rrf = pyvdb.HybridSearchEngine.reciprocal_rank_fusion(1, 2, 60)
    print(f"  RRF (ranks 1,2, k=60): {rrf:.4f}")
    
    cs = pyvdb.HybridSearchEngine.comb_sum(vec_score, lex_score)
    print(f"  CombSUM: {cs:.4f}")
    
    cmnz = pyvdb.HybridSearchEngine.comb_mnz(vec_score, lex_score, 2)
    print(f"  CombMNZ (2 systems): {cmnz:.4f}")
    
    print()
    print("=" * 60)
    print("Demo Complete!")
    print("=" * 60)

if __name__ == "__main__":
    main()
