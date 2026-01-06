// Copyright (c) 2024 Vector Studio
// RAG (Retrieval Augmented Generation) Engine Implementation

#include "vdb/framework_integration.hpp"
#include <algorithm>
#include <sstream>
#include <regex>
#include <numeric>

namespace vdb {

// ============================================================================
// RAGEngine Implementation
// ============================================================================

struct RAGEngine::Impl {
    RAGConfig config;
    
    // Tokenizer helper
    size_t count_tokens(const std::string& text) const {
        // Simple whitespace tokenization (approximate)
        size_t tokens = 0;
        bool in_word = false;
        for (char c : text) {
            if (std::isspace(c)) {
                if (in_word) {
                    tokens++;
                    in_word = false;
                }
            } else {
                in_word = true;
            }
        }
        if (in_word) tokens++;
        return tokens;
    }
    
    // Sentence splitter
    std::vector<std::string> split_sentences(const std::string& text) const {
        std::vector<std::string> sentences;
        std::regex sentence_regex(R"([^.!?]+[.!?]+)");
        
        auto sentences_begin = std::sregex_iterator(text.begin(), text.end(), sentence_regex);
        auto sentences_end = std::sregex_iterator();
        
        for (std::sregex_iterator i = sentences_begin; i != sentences_end; ++i) {
            std::string sentence = (*i).str();
            // Trim whitespace
            sentence.erase(0, sentence.find_first_not_of(" \t\n\r"));
            sentence.erase(sentence.find_last_not_of(" \t\n\r") + 1);
            if (!sentence.empty()) {
                sentences.push_back(sentence);
            }
        }
        
        // Handle remaining text without sentence terminators
        size_t last_pos = 0;
        if (!sentences.empty()) {
            std::smatch last_match = *std::prev(sentences_end);
            last_pos = last_match.position() + last_match.length();
        }
        
        if (last_pos < text.length()) {
            std::string remaining = text.substr(last_pos);
            remaining.erase(0, remaining.find_first_not_of(" \t\n\r"));
            remaining.erase(remaining.find_last_not_of(" \t\n\r") + 1);
            if (!remaining.empty()) {
                sentences.push_back(remaining);
            }
        }
        
        return sentences;
    }
    
    // Fixed-size chunking
    std::vector<std::string> chunk_fixed(const std::string& document) const {
        std::vector<std::string> chunks;
        size_t pos = 0;
        
        while (pos < document.length()) {
            size_t chunk_end = std::min(pos + config.chunk_size, document.length());
            
            // Try to break at word boundary
            if (chunk_end < document.length()) {
                size_t last_space = document.rfind(' ', chunk_end);
                if (last_space != std::string::npos && last_space > pos) {
                    chunk_end = last_space + 1;
                }
            }
            
            chunks.push_back(document.substr(pos, chunk_end - pos));
            
            // Apply overlap
            if (config.chunk_overlap > 0 && chunk_end < document.length()) {
                pos = chunk_end - std::min(config.chunk_overlap, chunk_end - pos);
            } else {
                pos = chunk_end;
            }
        }
        
        return chunks;
    }
    
    // Sentence-based chunking
    std::vector<std::string> chunk_sentence(const std::string& document) const {
        std::vector<std::string> chunks;
        std::vector<std::string> sentences = split_sentences(document);
        
        std::string current_chunk;
        for (const auto& sentence : sentences) {
            if (current_chunk.empty()) {
                current_chunk = sentence;
            } else if (current_chunk.length() + sentence.length() + 1 <= config.chunk_size) {
                current_chunk += " " + sentence;
            } else {
                chunks.push_back(current_chunk);
                current_chunk = sentence;
            }
        }
        
        if (!current_chunk.empty()) {
            chunks.push_back(current_chunk);
        }
        
        return chunks;
    }
    
    // Paragraph-based chunking
    std::vector<std::string> chunk_paragraph(const std::string& document) const {
        std::vector<std::string> chunks;
        std::istringstream stream(document);
        std::string paragraph;
        std::string current_chunk;
        
        while (std::getline(stream, paragraph)) {
            // Skip empty lines
            if (paragraph.empty() || paragraph.find_first_not_of(" \t\r\n") == std::string::npos) {
                continue;
            }
            
            if (current_chunk.empty()) {
                current_chunk = paragraph;
            } else if (current_chunk.length() + paragraph.length() + 2 <= config.chunk_size) {
                current_chunk += "\n\n" + paragraph;
            } else {
                chunks.push_back(current_chunk);
                current_chunk = paragraph;
            }
        }
        
        if (!current_chunk.empty()) {
            chunks.push_back(current_chunk);
        }
        
        return chunks;
    }
    
    // Semantic chunking (simplified - groups similar sentences)
    std::vector<std::string> chunk_semantic(const std::string& document) const {
        // For now, fallback to sentence chunking
        // In a full implementation, this would use embeddings to group semantically similar content
        return chunk_sentence(document);
    }
    
    // Recursive chunking (hierarchical splitting)
    std::vector<std::string> chunk_recursive(const std::string& document) const {
        std::vector<std::string> chunks;
        
        // First try paragraph boundaries
        std::vector<std::string> paragraphs;
        std::istringstream stream(document);
        std::string paragraph;
        
        while (std::getline(stream, paragraph)) {
            if (!paragraph.empty() && paragraph.find_first_not_of(" \t\r\n") != std::string::npos) {
                paragraphs.push_back(paragraph);
            }
        }
        
        // Process each paragraph
        for (const auto& para : paragraphs) {
            if (para.length() <= config.chunk_size) {
                chunks.push_back(para);
            } else {
                // Split into sentences
                auto sentences = split_sentences(para);
                std::string current_chunk;
                
                for (const auto& sent : sentences) {
                    if (sent.length() > config.chunk_size) {
                        // Sentence too long, split by fixed size
                        if (!current_chunk.empty()) {
                            chunks.push_back(current_chunk);
                            current_chunk.clear();
                        }
                        auto fixed_chunks = chunk_fixed(sent);
                        chunks.insert(chunks.end(), fixed_chunks.begin(), fixed_chunks.end());
                    } else if (current_chunk.empty()) {
                        current_chunk = sent;
                    } else if (current_chunk.length() + sent.length() + 1 <= config.chunk_size) {
                        current_chunk += " " + sent;
                    } else {
                        chunks.push_back(current_chunk);
                        current_chunk = sent;
                    }
                }
                
                if (!current_chunk.empty()) {
                    chunks.push_back(current_chunk);
                }
            }
        }
        
        return chunks;
    }
};

RAGEngine::RAGEngine(const RAGConfig& config) : impl_(std::make_unique<Impl>()) {
    impl_->config = config;
}

RAGEngine::~RAGEngine() = default;

Result<std::vector<std::string>> RAGEngine::chunk_document(const std::string& document) const {
    if (document.empty()) {
        return Err<std::vector<std::string>>("Document is empty");
    }
    
    std::vector<std::string> chunks;
    
    if (impl_->config.chunking_strategy == "fixed") {
        chunks = impl_->chunk_fixed(document);
    } else if (impl_->config.chunking_strategy == "sentence") {
        chunks = impl_->chunk_sentence(document);
    } else if (impl_->config.chunking_strategy == "paragraph") {
        chunks = impl_->chunk_paragraph(document);
    } else if (impl_->config.chunking_strategy == "semantic") {
        chunks = impl_->chunk_semantic(document);
    } else if (impl_->config.chunking_strategy == "recursive") {
        chunks = impl_->chunk_recursive(document);
    } else {
        return Err<std::vector<std::string>>("Unknown chunking strategy: " + impl_->config.chunking_strategy);
    }
    
    return Ok(chunks);
}

Result<RAGContext> RAGEngine::build_context(
    const std::string& query,
    const std::vector<QueryResult>& search_results
) const {
    RAGContext context;
    
    // Filter by relevance threshold
    std::vector<QueryResult> filtered;
    for (const auto& result : search_results) {
        if (result.score >= impl_->config.relevance_threshold) {
            filtered.push_back(result);
        }
    }
    
    // Apply reranking if enabled
    std::vector<QueryResult> ranked = filtered;
    if (impl_->config.rerank && !filtered.empty()) {
        // Simple reranking: sort by score (already done by search, but ensure)
        std::sort(ranked.begin(), ranked.end(), 
                  [](const auto& a, const auto& b) { return a.score > b.score; });
    }
    
    // Build context within token limit
    size_t current_tokens = impl_->count_tokens(query);
    std::ostringstream formatted;
    
    for (const auto& result : ranked) {
        // Get document text from metadata if available
        std::string doc_text;
        if (result.metadata && result.metadata->contains("text")) {
            doc_text = result.metadata->at("text");
        } else {
            doc_text = "Document " + std::to_string(result.id);
        }
        
        size_t doc_tokens = impl_->count_tokens(doc_text);
        
        // Check if adding this document would exceed token limit
        if (current_tokens + doc_tokens <= impl_->config.max_context_length) {
            context.retrieved_chunks.push_back(doc_text);
            context.relevance_scores.push_back(result.score);
            context.document_ids.push_back(result.id);
            current_tokens += doc_tokens;
        } else {
            // Try to truncate document to fit
            size_t available_tokens = impl_->config.max_context_length - current_tokens;
            if (available_tokens > 50) { // Minimum meaningful chunk
                // Simple truncation by character count (approximation)
                size_t approx_chars = (available_tokens * doc_text.length()) / doc_tokens;
                std::string truncated = doc_text.substr(0, approx_chars) + "...";
                context.retrieved_chunks.push_back(truncated);
                context.relevance_scores.push_back(result.score);
                context.document_ids.push_back(result.id);
                current_tokens += available_tokens;
            }
            break; // Context full
        }
    }
    
    // Format context
    for (size_t i = 0; i < context.retrieved_chunks.size(); ++i) {
        formatted << "[Document " << (i + 1) << " (score: " 
                  << context.relevance_scores[i] << ")]\n"
                  << context.retrieved_chunks[i] << "\n\n";
    }
    
    context.formatted_context = formatted.str();
    context.total_tokens = current_tokens;
    
    return Ok(context);
}

std::string RAGEngine::format_prompt(
    const std::string& query,
    const RAGContext& context,
    const std::string& system_prompt
) const {
    std::ostringstream prompt;
    
    // Add system prompt if provided
    if (!system_prompt.empty()) {
        prompt << system_prompt << "\n\n";
    }
    
    // Add context
    prompt << "Context:\n" << context.formatted_context << "\n";
    
    // Add query
    prompt << "Question: " << query << "\n\n";
    prompt << "Answer: ";
    
    return prompt.str();
}

Result<std::vector<QueryResult>> RAGEngine::rerank(
    const std::string& query,
    const std::vector<QueryResult>& results
) const {
    // Simple reranking implementation
    // In production, this could use a cross-encoder or more sophisticated model
    
    std::vector<QueryResult> reranked = results;
    
    // Sort by score (most relevant first)
    std::sort(reranked.begin(), reranked.end(),
              [](const auto& a, const auto& b) { return a.score > b.score; });
    
    return Ok(reranked);
}

} // namespace vdb
