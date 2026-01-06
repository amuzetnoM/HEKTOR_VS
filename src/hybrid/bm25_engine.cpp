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
    auto it = impl_->documents.find(id);
    if (it == impl_->documents.end()) {
        return Error("Document not found: " + std::to_string(id));
    }
    
    const auto& doc = it->second;
    
    // Update inverted index
    for (const auto& [term, term_data] : doc.terms) {
        auto& postings = impl_->inverted_index[term];
        postings.erase(
            std::remove_if(postings.begin(), postings.end(),
                [id](const auto& p) { return p.first == id; }),
            postings.end()
        );
        
        // Update document frequency
        impl_->document_frequency[term]--;
        if (impl_->document_frequency[term] == 0) {
            impl_->document_frequency.erase(term);
            impl_->inverted_index.erase(term);
        }
    }
    
    // Update statistics
    impl_->total_terms -= doc.length;
    impl_->documents.erase(it);
    impl_->total_documents--;
    
    if (impl_->total_documents > 0) {
        impl_->avg_doc_length = static_cast<double>(impl_->total_terms) / impl_->total_documents;
    } else {
        impl_->avg_doc_length = 0.0;
    }
    
    return {};
}

Result<void> BM25Engine::update_document(VectorId id, const std::string& content) {
    // Remove old document
    auto remove_result = remove_document(id);
    if (!remove_result) {
        // Document doesn't exist, just add it
        return impl_->add_document(id, content);
    }
    
    // Add new document
    return impl_->add_document(id, content);
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
    std::ofstream file(path, std::ios::binary);
    if (!file) {
        return Error("Failed to open file for writing: " + path);
    }
    
    // Write header
    file << "BM25_ENGINE_V1\n";
    
    // Write configuration
    file << "k1=" << impl_->config.k1 << "\n";
    file << "b=" << impl_->config.b << "\n";
    file << "min_term_length=" << impl_->config.min_term_length << "\n";
    file << "use_stemming=" << (impl_->config.use_stemming ? "1" : "0") << "\n";
    file << "case_sensitive=" << (impl_->config.case_sensitive ? "1" : "0") << "\n";
    
    // Write statistics
    file << "total_documents=" << impl_->total_documents << "\n";
    file << "total_terms=" << impl_->total_terms << "\n";
    file << "avg_doc_length=" << impl_->avg_doc_length << "\n";
    
    // Write documents
    file << "DOCUMENTS_START\n";
    for (const auto& [id, doc] : impl_->documents) {
        // Escape newlines in content
        std::string escaped_content = doc.content;
        size_t pos = 0;
        while ((pos = escaped_content.find('\n', pos)) != std::string::npos) {
            escaped_content.replace(pos, 1, "\\n");
            pos += 2;
        }
        file << id << "\t" << doc.length << "\t" << escaped_content << "\n";
    }
    file << "DOCUMENTS_END\n";
    
    file.close();
    
    LOG_INFO("Saved BM25 engine to: " + path);
    return {};
}

Result<BM25Engine> BM25Engine::load(const std::string& path) {
    std::ifstream file(path);
    if (!file) {
        return Error("Failed to open file for reading: " + path);
    }
    
    std::string line;
    
    // Read header
    std::getline(file, line);
    if (line != "BM25_ENGINE_V1") {
        return Error("Invalid BM25 engine file format");
    }
    
    // Read configuration
    BM25Config config;
    std::unordered_map<std::string, std::string> config_values;
    
    while (std::getline(file, line)) {
        if (line == "DOCUMENTS_START") {
            break;
        }
        
        size_t eq_pos = line.find('=');
        if (eq_pos != std::string::npos) {
            std::string key = line.substr(0, eq_pos);
            std::string value = line.substr(eq_pos + 1);
            config_values[key] = value;
        }
    }
    
    // Parse configuration
    if (config_values.count("k1")) {
        config.k1 = std::stof(config_values["k1"]);
    }
    if (config_values.count("b")) {
        config.b = std::stof(config_values["b"]);
    }
    if (config_values.count("min_term_length")) {
        config.min_term_length = std::stoul(config_values["min_term_length"]);
    }
    if (config_values.count("use_stemming")) {
        config.use_stemming = (config_values["use_stemming"] == "1");
    }
    if (config_values.count("case_sensitive")) {
        config.case_sensitive = (config_values["case_sensitive"] == "1");
    }
    
    // Create engine
    BM25Engine engine(config);
    
    // Read documents
    while (std::getline(file, line)) {
        if (line == "DOCUMENTS_END") {
            break;
        }
        
        std::istringstream iss(line);
        VectorId id;
        size_t length;
        std::string content;
        
        if (iss >> id >> length) {
            iss.get(); // skip tab
            std::getline(iss, content);
            
            // Unescape newlines
            size_t pos = 0;
            while ((pos = content.find("\\n", pos)) != std::string::npos) {
                content.replace(pos, 2, "\n");
                pos += 1;
            }
            
            // Add document
            auto result = engine.add_document(id, content);
            if (!result) {
                return Error("Failed to load document " + std::to_string(id) + ": " + result.error().message);
            }
        }
    }
    
    file.close();
    
    LOG_INFO("Loaded BM25 engine from: " + path);
    return engine;
}

} // namespace hybrid
} // namespace vdb
