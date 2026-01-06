// ============================================================================
// VectorDB - BM25 Full-Text Search Engine Implementation
// ============================================================================

#include "vdb/hybrid_search.hpp"
#include "vdb/logging.hpp"
#include <algorithm>
#include <cmath>
#include <sstream>
#include <unordered_map>
#include <unordered_set>
#include <cctype>
#include <fstream>

namespace vdb {
namespace hybrid {

// ============================================================================
// Text Processing Utilities
// ============================================================================

namespace {

std::string to_lower(const std::string& str) {
    std::string result = str;
    std::transform(result.begin(), result.end(), result.begin(),
        [](unsigned char c) { return std::tolower(c); });
    return result;
}

std::vector<std::string> tokenize(const std::string& text) {
    std::vector<std::string> tokens;
    std::string token;
    
    for (char c : text) {
        if (std::isalnum(c) || c == '-' || c == '_') {
            token += c;
        } else if (!token.empty()) {
            tokens.push_back(token);
            token.clear();
        }
    }
    
    if (!token.empty()) {
        tokens.push_back(token);
    }
    
    return tokens;
}

// Simple stemming
std::string stem(const std::string& word) {
    std::string result = word;
    
    if (result.length() > 3) {
        if (result.size() >= 3 && result.substr(result.size() - 3) == "ing") {
            result = result.substr(0, result.length() - 3);
        } else if (result.size() >= 2 && result.substr(result.size() - 2) == "ed") {
            result = result.substr(0, result.length() - 2);
        } else if (result.size() >= 1 && result.back() == 's' && 
                   !(result.size() >= 2 && result.substr(result.size() - 2) == "ss")) {
            result = result.substr(0, result.length() - 1);
        }
    }
    
    return result;
}

const std::unordered_set<std::string> STOP_WORDS = {
    "a", "an", "and", "are", "as", "at", "be", "by", "for", "from",
    "has", "he", "in", "is", "it", "its", "of", "on", "that", "the",
    "to", "was", "were", "will", "with", "this", "but", "they", "have"
};

bool is_stop_word(const std::string& word) {
    return STOP_WORDS.find(word) != STOP_WORDS.end();
}

std::vector<std::string> process_text(const std::string& text, 
                                      const BM25Config& config) {
    std::vector<std::string> processed;
    auto tokens = tokenize(text);
    
    for (auto& token : tokens) {
        if (!config.case_sensitive) {
            token = to_lower(token);
        }
        
        if (token.length() < config.min_term_length) {
            continue;
        }
        
        if (is_stop_word(token)) {
            continue;
        }
        
        if (config.use_stemming) {
            token = stem(token);
        }
        
        processed.push_back(token);
    }
    
    return processed;
}

} // anonymous namespace

// ============================================================================
// BM25Engine Implementation
// ============================================================================

struct BM25Engine::Impl {
    BM25Config config;
    std::unordered_map<VectorId, Document> documents;
    std::unordered_map<std::string, std::vector<std::pair<VectorId, uint32_t>>> inverted_index;
    std::unordered_map<std::string, uint32_t> document_frequency;
    size_t total_documents = 0;
    size_t total_terms = 0;
    double avg_doc_length = 0.0;
    
    Impl(const BM25Config& cfg) : config(cfg) {}
    
    Result<void> add_document(VectorId id, const std::string& content) {
        if (documents.find(id) != documents.end()) {
            return Error("Document already exists");
        }
        
        auto terms = process_text(content, config);
        if (terms.empty()) {
            return Error("No valid terms in document");
        }
        
        Document doc;
        doc.id = id;
        doc.content = content;
        doc.length = terms.size();
        
        std::unordered_map<std::string, uint32_t> term_counts;
        for (size_t pos = 0; pos < terms.size(); ++pos) {
            const auto& term = terms[pos];
            term_counts[term]++;
            doc.terms[term].positions.push_back(pos);
        }
        
        for (const auto& [term, count] : term_counts) {
            doc.terms[term].text = term;
            doc.terms[term].frequency = count;
        }
        
        for (const auto& [term, term_data] : doc.terms) {
            inverted_index[term].push_back({id, term_data.frequency});
            if (term_data.frequency > 0) {
                document_frequency[term]++;
            }
        }
        
        documents[id] = std::move(doc);
        total_documents++;
        total_terms += terms.size();
        avg_doc_length = static_cast<double>(total_terms) / total_documents;
        
        return {};
    }
    
    Result<std::vector<BM25Result>> search(const std::string& query,
                                           size_t k,
                                           float min_score) const {
        if (total_documents == 0) {
            return std::vector<BM25Result>();
        }
        
        auto query_terms = process_text(query, config);
        if (query_terms.empty()) {
            return Error("No valid terms in query");
        }
        
        std::unordered_map<VectorId, float> scores;
        std::unordered_map<VectorId, std::vector<std::string>> matched_terms;
        
        for (const auto& term : query_terms) {
            auto it = inverted_index.find(term);
            if (it == inverted_index.end()) {
                continue;
            }
            
            const auto& postings = it->second;
            uint32_t df = document_frequency.at(term);
            double idf = std::log((total_documents - df + 0.5) / (df + 0.5) + 1.0);
            
            for (const auto& [doc_id, tf] : postings) {
                const auto& doc = documents.at(doc_id);
                double numerator = tf * (config.k1 + 1.0);
                double denominator = tf + config.k1 * (1.0 - config.b + 
                    config.b * doc.length / avg_doc_length);
                double score = idf * (numerator / denominator);
                
                scores[doc_id] += score;
                matched_terms[doc_id].push_back(term);
            }
        }
        
        std::vector<BM25Result> results;
        results.reserve(scores.size());
        
        for (const auto& [doc_id, score] : scores) {
            if (score >= min_score) {
                BM25Result result;
                result.id = doc_id;
                result.score = score;
                result.matched_terms = std::move(matched_terms[doc_id]);
                results.push_back(std::move(result));
            }
        }
        
        std::sort(results.begin(), results.end());
        
        if (results.size() > k) {
            results.resize(k);
        }
        
        return results;
    }
};

BM25Engine::BM25Engine(const BM25Config& config)
    : impl_(std::make_unique<Impl>(config)) {}

BM25Engine::~BM25Engine() = default;

Result<void> BM25Engine::add_document(VectorId id, const std::string& content) {
    return impl_->add_document(id, content);
}

Result<void> BM25Engine::remove_document(VectorId id) {
    return Error("Not implemented yet");
}

Result<void> BM25Engine::update_document(VectorId id, const std::string& content) {
    return Error("Not implemented yet");
}

Result<std::vector<BM25Result>> BM25Engine::search(const std::string& query,
                                                     size_t k,
                                                     float min_score) const {
    return impl_->search(query, k, min_score);
}

size_t BM25Engine::document_count() const {
    return impl_->total_documents;
}

size_t BM25Engine::term_count() const {
    return impl_->inverted_index.size();
}

float BM25Engine::average_document_length() const {
    return static_cast<float>(impl_->avg_doc_length);
}

Result<void> BM25Engine::save(const std::string& path) const {
    return Error("Not implemented yet");
}

Result<BM25Engine> BM25Engine::load(const std::string& path) {
    return Error("Not implemented yet");
}

} // namespace hybrid
} // namespace vdb
