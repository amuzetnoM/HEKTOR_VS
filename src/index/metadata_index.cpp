// ============================================================================
// VectorDB - Metadata Index Implementation  
// ============================================================================

#include "vdb/index/metadata_index.hpp"
#include <sstream>
#include <algorithm>
#include <fstream>

namespace vdb {
namespace index {

MetadataIndex::MetadataIndex() = default;
MetadataIndex::~MetadataIndex() = default;

Result<void> MetadataIndex::create_index(const std::string& field, bool is_numeric) {
    if (has_index(field)) {
        return std::unexpected(Error{ErrorCode::InvalidState, "Index already exists"});
    }
    
    InvertedIndex idx;
    idx.is_numeric = is_numeric;
    indices_[field] = std::move(idx);
    
    return {};
}

bool MetadataIndex::has_index(const std::string& field) const {
    return indices_.find(field) != indices_.end();
}

void MetadataIndex::drop_index(const std::string& field) {
    indices_.erase(field);
}

Result<void> MetadataIndex::insert(VectorId id, const Metadata& metadata) {
    for (auto& [field, idx] : indices_) {
        std::string value = get_field_value(metadata, field);
        if (value.empty()) continue;
        
        idx.postings[value].insert(id);
        
        if (idx.is_numeric) {
            try {
                double num_val = std::stod(value);
                idx.numeric_index[num_val].insert(id);
            } catch (...) {}
        }
    }
    
    return {};
}

Result<void> MetadataIndex::update(VectorId id, const Metadata& old_meta,
    const Metadata& new_meta) 
{
    auto remove_res = remove(id, old_meta);
    if (!remove_res) return remove_res;
    
    return insert(id, new_meta);
}

Result<void> MetadataIndex::remove(VectorId id, const Metadata& metadata) {
    for (auto& [field, idx] : indices_) {
        std::string value = get_field_value(metadata, field);
        if (value.empty()) continue;
        
        auto it = idx.postings.find(value);
        if (it != idx.postings.end()) {
            it->second.erase(id);
            if (it->second.empty()) {
                idx.postings.erase(it);
            }
        }
        
        if (idx.is_numeric) {
            try {
                double num_val = std::stod(value);
                auto num_it = idx.numeric_index.find(num_val);
                if (num_it != idx.numeric_index.end()) {
                    num_it->second.erase(id);
                    if (num_it->second.empty()) {
                        idx.numeric_index.erase(num_it);
                    }
                }
            } catch (...) {}
        }
    }
    
    return {};
}

std::set<VectorId> MetadataIndex::query(const FilterCondition& condition) const {
    if (!has_index(condition.field)) {
        return {};
    }
    
    const auto& idx = indices_.at(condition.field);
    
    switch (condition.op) {
        case FilterOp::Equal:
            return query_exact(condition.field, condition.value);
            
        case FilterOp::LessThan:
            if (idx.is_numeric) {
                try {
                    double val = std::stod(condition.value);
                    return query_range(condition.field,
                        std::numeric_limits<double>::lowest(), val, true, false);
                } catch (...) {}
            }
            break;
            
        case FilterOp::LessOrEqual:
            if (idx.is_numeric) {
                try {
                    double val = std::stod(condition.value);
                    return query_range(condition.field,
                        std::numeric_limits<double>::lowest(), val, true, true);
                } catch (...) {}
            }
            break;
            
        case FilterOp::GreaterThan:
            if (idx.is_numeric) {
                try {
                    double val = std::stod(condition.value);
                    return query_range(condition.field, val,
                        std::numeric_limits<double>::max(), false, true);
                } catch (...) {}
            }
            break;
            
        case FilterOp::GreaterOrEqual:
            if (idx.is_numeric) {
                try {
                    double val = std::stod(condition.value);
                    return query_range(condition.field, val,
                        std::numeric_limits<double>::max(), true, true);
                } catch (...) {}
            }
            break;
            
        case FilterOp::Range:
            if (idx.is_numeric) {
                try {
                    double min_val = std::stod(condition.value);
                    double max_val = std::stod(condition.value2);
                    return query_range(condition.field, min_val, max_val, true, true);
                } catch (...) {}
            }
            break;
            
        case FilterOp::In: {
            std::set<VectorId> result;
            for (const auto& val : condition.values) {
                auto ids = query_exact(condition.field, val);
                result.insert(ids.begin(), ids.end());
            }
            return result;
        }
        
        default:
            break;
    }
    
    return {};
}

std::set<VectorId> MetadataIndex::query_and(
    const std::vector<FilterCondition>& conditions) const 
{
    if (conditions.empty()) return {};
    
    std::set<VectorId> result = query(conditions[0]);
    
    for (size_t i = 1; i < conditions.size(); ++i) {
        std::set<VectorId> next = query(conditions[i]);
        std::set<VectorId> intersection;
        std::set_intersection(result.begin(), result.end(),
            next.begin(), next.end(),
            std::inserter(intersection, intersection.begin()));
        result = std::move(intersection);
    }
    
    return result;
}

std::set<VectorId> MetadataIndex::query_or(
    const std::vector<FilterCondition>& conditions) const 
{
    std::set<VectorId> result;
    
    for (const auto& cond : conditions) {
        auto ids = query(cond);
        result.insert(ids.begin(), ids.end());
    }
    
    return result;
}

std::function<bool(VectorId)> MetadataIndex::create_filter(
    const std::vector<FilterCondition>& conditions) const 
{
    auto valid_ids = query_and(conditions);
    
    return [valid_ids](VectorId id) {
        return valid_ids.find(id) != valid_ids.end();
    };
}

std::string MetadataIndex::get_field_value(const Metadata& metadata,
    const std::string& field) const 
{
    if (field == "date") return metadata.date;
    if (field == "type") return std::to_string(static_cast<int>(metadata.type));
    if (field == "asset") return metadata.asset;
    if (field == "bias") return metadata.bias;
    if (field == "source_file") return metadata.source_file;
    if (field == "content_hash") return metadata.content_hash;
    return "";
}

std::set<VectorId> MetadataIndex::query_exact(const std::string& field,
    const std::string& value) const 
{
    auto it = indices_.find(field);
    if (it == indices_.end()) return {};
    
    auto posting_it = it->second.postings.find(value);
    if (posting_it == it->second.postings.end()) return {};
    
    return posting_it->second;
}

std::set<VectorId> MetadataIndex::query_range(const std::string& field,
    double min_val, double max_val, bool min_inclusive, bool max_inclusive) const 
{
    auto it = indices_.find(field);
    if (it == indices_.end() || !it->second.is_numeric) return {};
    
    std::set<VectorId> result;
    
    auto lower = min_inclusive ? 
        it->second.numeric_index.lower_bound(min_val) :
        it->second.numeric_index.upper_bound(min_val);
    
    auto upper = max_inclusive ?
        it->second.numeric_index.upper_bound(max_val) :
        it->second.numeric_index.lower_bound(max_val);
    
    for (auto iter = lower; iter != upper; ++iter) {
        result.insert(iter->second.begin(), iter->second.end());
    }
    
    return result;
}

size_t MetadataIndex::size(const std::string& field) const {
    auto it = indices_.find(field);
    return (it != indices_.end()) ? it->second.postings.size() : 0;
}

size_t MetadataIndex::total_entries() const {
    size_t total = 0;
    for (const auto& [field, idx] : indices_) {
        for (const auto& [value, ids] : idx.postings) {
            total += ids.size();
        }
    }
    return total;
}

size_t MetadataIndex::memory_usage() const {
    size_t usage = 0;
    for (const auto& [field, idx] : indices_) {
        usage += field.size();
        for (const auto& [value, ids] : idx.postings) {
            usage += value.size() + ids.size() * sizeof(VectorId);
        }
        if (idx.is_numeric) {
            usage += idx.numeric_index.size() * (sizeof(double) + sizeof(void*));
        }
    }
    return usage;
}

Result<void> MetadataIndex::save(std::string_view path) const {
    std::ofstream file(std::string(path), std::ios::binary);
    if (!file) {
        return std::unexpected(Error{ErrorCode::IoError, "Failed to open file"});
    }
    
    size_t num_indices = indices_.size();
    file.write(reinterpret_cast<const char*>(&num_indices), sizeof(size_t));
    
    for (const auto& [field, idx] : indices_) {
        size_t field_len = field.size();
        file.write(reinterpret_cast<const char*>(&field_len), sizeof(size_t));
        file.write(field.data(), field_len);
        file.write(reinterpret_cast<const char*>(&idx.is_numeric), sizeof(bool));
        
        size_t num_postings = idx.postings.size();
        file.write(reinterpret_cast<const char*>(&num_postings), sizeof(size_t));
        
        for (const auto& [value, ids] : idx.postings) {
            size_t value_len = value.size();
            file.write(reinterpret_cast<const char*>(&value_len), sizeof(size_t));
            file.write(value.data(), value_len);
            
            size_t num_ids = ids.size();
            file.write(reinterpret_cast<const char*>(&num_ids), sizeof(size_t));
            for (VectorId id : ids) {
                file.write(reinterpret_cast<const char*>(&id), sizeof(VectorId));
            }
        }
    }
    
    return {};
}

Result<MetadataIndex> MetadataIndex::load(std::string_view path) {
    std::ifstream file(std::string(path), std::ios::binary);
    if (!file) {
        return std::unexpected(Error{ErrorCode::IoError, "Failed to open file"});
    }
    
    MetadataIndex idx;
    
    size_t num_indices;
    file.read(reinterpret_cast<char*>(&num_indices), sizeof(size_t));
    
    for (size_t i = 0; i < num_indices; ++i) {
        size_t field_len;
        file.read(reinterpret_cast<char*>(&field_len), sizeof(size_t));
        
        std::string field(field_len, ' ');
        file.read(&field[0], field_len);
        
        InvertedIndex inv_idx;
        file.read(reinterpret_cast<char*>(&inv_idx.is_numeric), sizeof(bool));
        
        size_t num_postings;
        file.read(reinterpret_cast<char*>(&num_postings), sizeof(size_t));
        
        for (size_t j = 0; j < num_postings; ++j) {
            size_t value_len;
            file.read(reinterpret_cast<char*>(&value_len), sizeof(size_t));
            
            std::string value(value_len, ' ');
            file.read(&value[0], value_len);
            
            size_t num_ids;
            file.read(reinterpret_cast<char*>(&num_ids), sizeof(size_t));
            
            std::set<VectorId> ids;
            for (size_t k = 0; k < num_ids; ++k) {
                VectorId id;
                file.read(reinterpret_cast<char*>(&id), sizeof(VectorId));
                ids.insert(id);
            }
            
            inv_idx.postings[value] = std::move(ids);
            
            if (inv_idx.is_numeric) {
                try {
                    double num_val = std::stod(value);
                    inv_idx.numeric_index[num_val] = inv_idx.postings[value];
                } catch (...) {}
            }
        }
        
        idx.indices_[field] = std::move(inv_idx);
    }
    
    return idx;
}

}} // namespace vdb::index
