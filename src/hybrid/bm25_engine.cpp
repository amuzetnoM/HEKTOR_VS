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
            return std::unexpected(Error(ErrorCode::InvalidData, "Document already exists"));
        }
        
        auto terms = process_text(content, config);
        if (terms.empty()) {
            return std::unexpected(Error(ErrorCode::InvalidData, "No valid terms in document"));
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
            return std::unexpected(Error(ErrorCode::InvalidInput, "No valid terms in query"));
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
        return std::unexpected(Error(ErrorCode::VectorNotFound, "Document not found: " + std::to_string(id)));
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
        return std::unexpected(Error(ErrorCode::IoError, "Failed to open file for writing: " + path));
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
        return std::unexpected(Error(ErrorCode::IoError, "Failed to open file for reading: " + path));
    }
    
    std::string line;
    
    // Read header
    std::getline(file, line);
    if (line != "BM25_ENGINE_V1") {
        return std::unexpected(Error(ErrorCode::InvalidData, "Invalid BM25 engine file format"));
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
                return std::unexpected(Error(ErrorCode::InvalidData, "Failed to load document " + std::to_string(id) + ": " + result.error().message));
            }
        }
    }
    
    file.close();
    
    LOG_INFO("Loaded BM25 engine from: " + path);
    return engine;
}

Result<std::shared_ptr<BM25Engine>> BM25Engine::load_shared(const std::string& path) {
    auto result = load(path);
    if (!result) {
        return std::unexpected(result.error());
    }
    // Move into heap allocation — Impl is complete here, so move ctor works
    return std::shared_ptr<BM25Engine>(new BM25Engine(std::move(*result)));
}

// ============================================================================
// KeywordExtractor Implementation
// ============================================================================

struct KeywordExtractor::Impl {
    KeywordConfig config;
    std::unordered_map<std::string, uint32_t> document_frequency;
    std::unordered_map<std::string, uint32_t> term_frequency;
    size_t total_documents = 0;
    bool trained = false;
    
    Impl(const KeywordConfig& cfg) : config(cfg) {}
    
    Result<std::vector<Keyword>> extract(const std::string& text) const {
        auto tokens = tokenize(text);
        if (tokens.empty()) {
            return std::vector<Keyword>();
        }
        
        // Count term frequencies and positions
        std::unordered_map<std::string, std::pair<uint32_t, std::vector<size_t>>> term_data;
        for (size_t pos = 0; pos < tokens.size(); ++pos) {
            std::string term = to_lower(tokens[pos]);
            if (term.length() < 2 || is_stop_word(term)) {
                continue;
            }
            term_data[term].first++;
            term_data[term].second.push_back(pos);
        }
        
        // Calculate scores
        std::vector<Keyword> keywords;
        keywords.reserve(term_data.size());
        
        for (const auto& [term, data] : term_data) {
            Keyword kw;
            kw.term = term;
            kw.frequency = data.first;
            kw.positions = data.second;
            
            // Calculate score
            float tf = static_cast<float>(data.first);
            float idf = 1.0f;
            
            if (config.use_tfidf && trained && document_frequency.count(term)) {
                uint32_t df = document_frequency.at(term);
                idf = std::log(static_cast<float>(total_documents + 1) / (df + 1)) + 1.0f;
            }
            
            kw.score = tf * idf;
            
            // Position weighting - terms appearing early get a boost
            if (config.use_position_weight && !data.second.empty()) {
                float pos_weight = 1.0f / (1.0f + data.second[0] / 10.0f);
                kw.score *= (1.0f + pos_weight);
            }
            
            if (kw.score >= config.min_score) {
                keywords.push_back(kw);
            }
        }
        
        // Sort by score
        std::sort(keywords.begin(), keywords.end());
        
        // Limit to max keywords
        if (keywords.size() > config.max_keywords) {
            keywords.resize(config.max_keywords);
        }
        
        return keywords;
    }
    
    Result<void> train(const std::vector<std::string>& documents) {
        document_frequency.clear();
        term_frequency.clear();
        total_documents = documents.size();
        
        for (const auto& doc : documents) {
            auto tokens = tokenize(doc);
            std::unordered_set<std::string> seen_in_doc;
            
            for (const auto& token : tokens) {
                std::string term = to_lower(token);
                if (term.length() < 2 || is_stop_word(term)) {
                    continue;
                }
                
                term_frequency[term]++;
                
                if (!seen_in_doc.count(term)) {
                    document_frequency[term]++;
                    seen_in_doc.insert(term);
                }
            }
        }
        
        trained = true;
        return {};
    }
};

KeywordExtractor::KeywordExtractor(const KeywordConfig& config)
    : impl_(std::make_unique<Impl>(config)) {}

KeywordExtractor::~KeywordExtractor() = default;

Result<std::vector<Keyword>> KeywordExtractor::extract(const std::string& text) const {
    return impl_->extract(text);
}

Result<void> KeywordExtractor::train(const std::vector<std::string>& documents) {
    return impl_->train(documents);
}

Result<void> KeywordExtractor::save(const std::string& path) const {
    std::ofstream file(path, std::ios::binary);
    if (!file) {
        return std::unexpected(Error(ErrorCode::IoError, "Failed to open file for writing: " + path));
    }
    
    file << "KEYWORD_EXTRACTOR_V1\n";
    file << "max_keywords=" << impl_->config.max_keywords << "\n";
    file << "min_score=" << impl_->config.min_score << "\n";
    file << "use_tfidf=" << (impl_->config.use_tfidf ? "1" : "0") << "\n";
    file << "use_position_weight=" << (impl_->config.use_position_weight ? "1" : "0") << "\n";
    file << "total_documents=" << impl_->total_documents << "\n";
    file << "trained=" << (impl_->trained ? "1" : "0") << "\n";
    
    file << "DOCUMENT_FREQUENCY_START\n";
    for (const auto& [term, freq] : impl_->document_frequency) {
        file << term << "\t" << freq << "\n";
    }
    file << "DOCUMENT_FREQUENCY_END\n";
    
    file.close();
    return {};
}

Result<KeywordExtractor> KeywordExtractor::load(const std::string& path) {
    std::ifstream file(path);
    if (!file) {
        return std::unexpected(Error(ErrorCode::IoError, "Failed to open file for reading: " + path));
    }
    
    std::string line;
    std::getline(file, line);
    if (line != "KEYWORD_EXTRACTOR_V1") {
        return std::unexpected(Error(ErrorCode::InvalidData, "Invalid keyword extractor file format"));
    }
    
    KeywordConfig config;
    std::unordered_map<std::string, std::string> values;
    
    while (std::getline(file, line)) {
        if (line == "DOCUMENT_FREQUENCY_START") break;
        size_t eq = line.find('=');
        if (eq != std::string::npos) {
            values[line.substr(0, eq)] = line.substr(eq + 1);
        }
    }
    
    if (values.count("max_keywords")) config.max_keywords = std::stoul(values["max_keywords"]);
    if (values.count("min_score")) config.min_score = std::stof(values["min_score"]);
    if (values.count("use_tfidf")) config.use_tfidf = (values["use_tfidf"] == "1");
    if (values.count("use_position_weight")) config.use_position_weight = (values["use_position_weight"] == "1");
    
    KeywordExtractor extractor(config);
    
    if (values.count("total_documents")) {
        extractor.impl_->total_documents = std::stoul(values["total_documents"]);
    }
    if (values.count("trained")) {
        extractor.impl_->trained = (values["trained"] == "1");
    }
    
    // Load document frequencies
    while (std::getline(file, line)) {
        if (line == "DOCUMENT_FREQUENCY_END") break;
        std::istringstream iss(line);
        std::string term;
        uint32_t freq;
        if (iss >> term >> freq) {
            extractor.impl_->document_frequency[term] = freq;
        }
    }
    
    file.close();
    return extractor;
}

// ============================================================================
// QueryRewriter Implementation
// ============================================================================

struct QueryRewriter::Impl {
    RewriteConfig config;
    std::unordered_map<std::string, std::vector<std::string>> synonyms;
    
    Impl(const RewriteConfig& cfg) : config(cfg) {}
    
    Result<std::string> rewrite(const std::string& query) const {
        auto tokens = tokenize(query);
        if (tokens.empty()) {
            return query;
        }
        
        std::vector<std::string> result_tokens;
        result_tokens.reserve(tokens.size() * 2);
        
        for (const auto& token : tokens) {
            std::string lower_token = to_lower(token);
            result_tokens.push_back(token);
            
            // Add stemmed version
            if (config.add_stemmed_terms) {
                std::string stemmed = stem(lower_token);
                if (stemmed != lower_token && stemmed.length() >= 2) {
                    result_tokens.push_back(stemmed);
                }
            }
            
            // Expand synonyms
            if (config.expand_synonyms) {
                auto it = synonyms.find(lower_token);
                if (it != synonyms.end()) {
                    size_t count = 0;
                    for (const auto& syn : it->second) {
                        if (count >= config.max_expansions) break;
                        result_tokens.push_back(syn);
                        count++;
                    }
                }
            }
        }
        
        // Join tokens back into a string
        std::string result;
        for (size_t i = 0; i < result_tokens.size(); ++i) {
            if (i > 0) result += " ";
            result += result_tokens[i];
        }
        
        return result;
    }
    
    Result<void> add_synonym(const std::string& term, const std::vector<std::string>& syns) {
        std::string lower_term = to_lower(term);
        synonyms[lower_term] = syns;
        return {};
    }
    
    Result<void> load_synonyms(const std::string& path) {
        std::ifstream file(path);
        if (!file) {
            return std::unexpected(Error(ErrorCode::IoError, "Failed to open synonym file: " + path));
        }
        
        std::string line;
        while (std::getline(file, line)) {
            if (line.empty() || line[0] == '#') continue;
            
            std::istringstream iss(line);
            std::string term;
            if (!(iss >> term)) continue;
            
            std::vector<std::string> syns;
            std::string syn;
            while (iss >> syn) {
                syns.push_back(to_lower(syn));
            }
            
            if (!syns.empty()) {
                synonyms[to_lower(term)] = syns;
            }
        }
        
        file.close();
        return {};
    }
};

QueryRewriter::QueryRewriter(const RewriteConfig& config)
    : impl_(std::make_unique<Impl>(config)) {}

QueryRewriter::~QueryRewriter() = default;

Result<std::string> QueryRewriter::rewrite(const std::string& query) const {
    return impl_->rewrite(query);
}

Result<void> QueryRewriter::add_synonym(const std::string& term, const std::vector<std::string>& synonyms) {
    return impl_->add_synonym(term, synonyms);
}

Result<void> QueryRewriter::load_synonyms(const std::string& path) {
    return impl_->load_synonyms(path);
}

} // namespace hybrid
} // namespace vdb
