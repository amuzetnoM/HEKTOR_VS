// ============================================================================
// VectorDB - Flat Index Implementation
// Brute-force exact search (for small datasets or testing)
// ============================================================================

#include "vdb/index.hpp"
#include "vdb/distance.hpp"
#include <algorithm>
#include <queue>

namespace vdb {

// ============================================================================
// Flat Index - Exact brute-force search
// ============================================================================

class FlatIndex {
public:
    explicit FlatIndex(Dim dimension, DistanceType distance_type = DistanceType::Cosine)
        : dimension_(dimension)
        , distance_type_(distance_type)
    {}
    
    void add(VectorId id, VectorView vector) {
        if (vector.dim() != dimension_) {
            throw std::invalid_argument("Vector dimension mismatch");
        }
        
        vectors_.emplace_back(vector.data(), vector.data() + vector.dim());
        ids_.push_back(id);
    }
    
    std::vector<SearchResult> search(VectorView query, size_t k) const {
        if (query.dim() != dimension_) {
            return {};
        }
        
        // Calculate distances to all vectors
        using ScorePair = std::pair<float, size_t>;
        std::priority_queue<ScorePair, std::vector<ScorePair>, std::greater<>> pq;
        
        for (size_t i = 0; i < vectors_.size(); ++i) {
            float dist;
            switch (distance_type_) {
                case DistanceType::Cosine:
                    dist = cosine_distance(query.data(), vectors_[i].data(), dimension_);
                    break;
                case DistanceType::Euclidean:
                    dist = euclidean_distance(query.data(), vectors_[i].data(), dimension_);
                    break;
                case DistanceType::DotProduct:
                    dist = -dot_product(query.data(), vectors_[i].data(), dimension_);
                    break;
            }
            
            if (pq.size() < k) {
                pq.emplace(dist, i);
            } else if (dist < pq.top().first) {
                pq.pop();
                pq.emplace(dist, i);
            }
        }
        
        // Convert to results
        std::vector<SearchResult> results;
        results.reserve(pq.size());
        
        while (!pq.empty()) {
            auto [dist, idx] = pq.top();
            pq.pop();
            
            SearchResult result;
            result.id = ids_[idx];
            result.distance = dist;
            result.score = (distance_type_ == DistanceType::Cosine) 
                         ? 1.0f - dist 
                         : 1.0f / (1.0f + dist);
            results.push_back(result);
        }
        
        // Reverse to get best first
        std::reverse(results.begin(), results.end());
        
        return results;
    }
    
    bool remove(VectorId id) {
        for (size_t i = 0; i < ids_.size(); ++i) {
            if (ids_[i] == id) {
                ids_.erase(ids_.begin() + i);
                vectors_.erase(vectors_.begin() + i);
                return true;
            }
        }
        return false;
    }
    
    size_t size() const { return ids_.size(); }
    Dim dimension() const { return dimension_; }
    
    void clear() {
        vectors_.clear();
        ids_.clear();
    }

private:
    Dim dimension_;
    DistanceType distance_type_;
    std::vector<std::vector<float>> vectors_;
    std::vector<VectorId> ids_;
};

} // namespace vdb
