// ============================================================================
// VectorDB - Python Bindings (pybind11)
// ============================================================================

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/functional.h>
#include <pybind11/numpy.h>

#include "vdb/database.hpp"
#include "vdb/ingest.hpp"

namespace py = pybind11;
using namespace vdb;

// ============================================================================
// Helper: Convert numpy array to VectorView
// ============================================================================

VectorView numpy_to_view(py::array_t<float> arr) {
    py::buffer_info buf = arr.request();
    if (buf.ndim != 1) {
        throw std::runtime_error("Expected 1D array");
    }
    return VectorView(static_cast<float*>(buf.ptr), static_cast<Dim>(buf.size));
}

Vector numpy_to_vector(py::array_t<float> arr) {
    py::buffer_info buf = arr.request();
    if (buf.ndim != 1) {
        throw std::runtime_error("Expected 1D array");
    }
    float* ptr = static_cast<float*>(buf.ptr);
    return Vector(std::vector<float>(ptr, ptr + buf.size));
}

py::array_t<float> vector_to_numpy(const Vector& vec) {
    py::array_t<float> result(vec.dim());
    py::buffer_info buf = result.request();
    std::memcpy(buf.ptr, vec.data(), vec.dim() * sizeof(float));
    return result;
}

// ============================================================================
// Module Definition
// ============================================================================

PYBIND11_MODULE(pyvdb, m) {
    m.doc() = "VectorDB - High-performance vector database for Gold Standard";
    
    // Version info
    m.attr("__version__") = py::str(
        std::to_string(VERSION_MAJOR) + "." + 
        std::to_string(VERSION_MINOR) + "." + 
        std::to_string(VERSION_PATCH)
    );
    
    // ========================================================================
    // Enums
    // ========================================================================
    
    py::enum_<DistanceMetric>(m, "DistanceMetric")
        .value("Cosine", DistanceMetric::Cosine)
        .value("L2", DistanceMetric::L2)
        .value("DotProduct", DistanceMetric::DotProduct)
        .export_values();
    
    py::enum_<DocumentType>(m, "DocumentType")
        .value("Journal", DocumentType::Journal)
        .value("Chart", DocumentType::Chart)
        .value("CatalystWatchlist", DocumentType::CatalystWatchlist)
        .value("InstitutionalMatrix", DocumentType::InstitutionalMatrix)
        .value("EconomicCalendar", DocumentType::EconomicCalendar)
        .value("WeeklyRundown", DocumentType::WeeklyRundown)
        .value("ThreeMonthReport", DocumentType::ThreeMonthReport)
        .value("OneYearReport", DocumentType::OneYearReport)
        .value("MonthlyReport", DocumentType::MonthlyReport)
        .value("PreMarket", DocumentType::PreMarket)
        .value("Unknown", DocumentType::Unknown)
        .export_values();
    
    py::enum_<ExecutionProvider>(m, "ExecutionProvider")
        .value("CPU", ExecutionProvider::CPU)
        .value("CUDA", ExecutionProvider::CUDA)
        .value("DirectML", ExecutionProvider::DirectML)
        .value("Auto", ExecutionProvider::Auto)
        .export_values();
    
    // ========================================================================
    // Metadata
    // ========================================================================
    
    py::class_<Metadata>(m, "Metadata")
        .def(py::init<>())
        .def_readwrite("id", &Metadata::id)
        .def_readwrite("type", &Metadata::type)
        .def_readwrite("date", &Metadata::date)
        .def_readwrite("source_file", &Metadata::source_file)
        .def_readwrite("asset", &Metadata::asset)
        .def_readwrite("bias", &Metadata::bias)
        .def_readwrite("gold_price", &Metadata::gold_price)
        .def_readwrite("silver_price", &Metadata::silver_price)
        .def_readwrite("gsr", &Metadata::gsr)
        .def_readwrite("dxy", &Metadata::dxy)
        .def_readwrite("vix", &Metadata::vix)
        .def_readwrite("yield_10y", &Metadata::yield_10y)
        .def("__repr__", [](const Metadata& m) {
            return "<Metadata id=" + std::to_string(m.id) + 
                   " type=" + std::string(document_type_name(m.type)) + 
                   " date='" + m.date + "'>";
        });
    
    // ========================================================================
    // Query Options & Results
    // ========================================================================
    
    py::class_<QueryOptions>(m, "QueryOptions")
        .def(py::init<>())
        .def_readwrite("k", &QueryOptions::k)
        .def_readwrite("ef_search", &QueryOptions::ef_search)
        .def_readwrite("type_filter", &QueryOptions::type_filter)
        .def_readwrite("date_filter", &QueryOptions::date_filter)
        .def_readwrite("date_from", &QueryOptions::date_from)
        .def_readwrite("date_to", &QueryOptions::date_to)
        .def_readwrite("asset_filter", &QueryOptions::asset_filter)
        .def_readwrite("bias_filter", &QueryOptions::bias_filter)
        .def_readwrite("include_metadata", &QueryOptions::include_metadata)
        .def_readwrite("deduplicate_by_date", &QueryOptions::deduplicate_by_date);
    
    py::class_<QueryResult>(m, "QueryResult")
        .def_readonly("id", &QueryResult::id)
        .def_readonly("distance", &QueryResult::distance)
        .def_readonly("score", &QueryResult::score)
        .def_readonly("metadata", &QueryResult::metadata)
        .def("__repr__", [](const QueryResult& r) {
            return "<QueryResult id=" + std::to_string(r.id) + 
                   " score=" + std::to_string(r.score) + ">";
        });
    
    // ========================================================================
    // Database Config
    // ========================================================================
    
    py::class_<DatabaseConfig>(m, "DatabaseConfig")
        .def(py::init<>())
        .def_readwrite("path", &DatabaseConfig::path)
        .def_readwrite("dimension", &DatabaseConfig::dimension)
        .def_readwrite("metric", &DatabaseConfig::metric)
        .def_readwrite("hnsw_m", &DatabaseConfig::hnsw_m)
        .def_readwrite("hnsw_ef_construction", &DatabaseConfig::hnsw_ef_construction)
        .def_readwrite("hnsw_ef_search", &DatabaseConfig::hnsw_ef_search)
        .def_readwrite("max_elements", &DatabaseConfig::max_elements)
        .def_readwrite("text_model_path", &DatabaseConfig::text_model_path)
        .def_readwrite("image_model_path", &DatabaseConfig::image_model_path)
        .def_readwrite("vocab_path", &DatabaseConfig::vocab_path)
        .def_readwrite("provider", &DatabaseConfig::provider)
        .def_readwrite("num_threads", &DatabaseConfig::num_threads)
        .def_readwrite("memory_only", &DatabaseConfig::memory_only)
        .def_readwrite("auto_sync", &DatabaseConfig::auto_sync);
    
    // ========================================================================
    // Index Stats
    // ========================================================================
    
    py::class_<IndexStats>(m, "IndexStats")
        .def_readonly("total_vectors", &IndexStats::total_vectors)
        .def_readonly("dimension", &IndexStats::dimension)
        .def_readonly("memory_usage_bytes", &IndexStats::memory_usage_bytes)
        .def_readonly("index_size_bytes", &IndexStats::index_size_bytes)
        .def_readonly("index_type", &IndexStats::index_type)
        .def("__repr__", [](const IndexStats& s) {
            return "<IndexStats vectors=" + std::to_string(s.total_vectors) + 
                   " dim=" + std::to_string(s.dimension) + 
                   " memory=" + std::to_string(s.memory_usage_bytes / 1024 / 1024) + "MB>";
        });
    
    // ========================================================================
    // VectorDatabase
    // ========================================================================
    
    py::class_<VectorDatabase>(m, "VectorDatabase")
        .def(py::init<const DatabaseConfig&>())
        .def("init", [](VectorDatabase& self) {
            auto result = self.init();
            if (!result) {
                throw std::runtime_error(result.error().message);
            }
        })
        .def("is_ready", &VectorDatabase::is_ready)
        .def("config", &VectorDatabase::config, py::return_value_policy::reference)
        
        // Text operations
        .def("add_text", [](VectorDatabase& self, const std::string& text, 
                            const Metadata& meta) {
            auto result = self.add_text(text, meta);
            if (!result) {
                throw std::runtime_error(result.error().message);
            }
            return *result;
        }, py::arg("text"), py::arg("metadata"))
        
        .def("add_text", [](VectorDatabase& self, const std::string& text,
                            DocumentType type, const std::string& date) {
            auto result = self.add_text(text, type, date);
            if (!result) {
                throw std::runtime_error(result.error().message);
            }
            return *result;
        }, py::arg("text"), py::arg("type"), py::arg("date"))
        
        .def("query_text", [](VectorDatabase& self, const std::string& query,
                              const QueryOptions& options) {
            auto result = self.query_text(query, options);
            if (!result) {
                throw std::runtime_error(result.error().message);
            }
            return *result;
        }, py::arg("query"), py::arg("options") = QueryOptions())
        
        // Simple query interface
        .def("search", [](VectorDatabase& self, const std::string& query, size_t k) {
            QueryOptions opts;
            opts.k = k;
            auto result = self.query_text(query, opts);
            if (!result) {
                throw std::runtime_error(result.error().message);
            }
            return *result;
        }, py::arg("query"), py::arg("k") = 10)
        
        // Image operations
        .def("add_image", [](VectorDatabase& self, const std::string& path,
                             const Metadata& meta) {
            auto result = self.add_image(path, meta);
            if (!result) {
                throw std::runtime_error(result.error().message);
            }
            return *result;
        }, py::arg("path"), py::arg("metadata"))
        
        .def("add_image", [](VectorDatabase& self, const std::string& path) {
            auto result = self.add_image(path);
            if (!result) {
                throw std::runtime_error(result.error().message);
            }
            return *result;
        }, py::arg("path"))
        
        .def("query_image", [](VectorDatabase& self, const std::string& path,
                               const QueryOptions& options) {
            auto result = self.query_image(path, options);
            if (!result) {
                throw std::runtime_error(result.error().message);
            }
            return *result;
        }, py::arg("path"), py::arg("options") = QueryOptions())
        
        // Vector operations (with numpy support)
        .def("add_vector", [](VectorDatabase& self, py::array_t<float> vec,
                              const Metadata& meta) {
            auto result = self.add_vector(numpy_to_view(vec), meta);
            if (!result) {
                throw std::runtime_error(result.error().message);
            }
            return *result;
        }, py::arg("vector"), py::arg("metadata"))
        
        .def("query_vector", [](VectorDatabase& self, py::array_t<float> vec,
                                const QueryOptions& options) {
            auto result = self.query_vector(numpy_to_view(vec), options);
            if (!result) {
                throw std::runtime_error(result.error().message);
            }
            return *result;
        }, py::arg("vector"), py::arg("options") = QueryOptions())
        
        .def("get_vector", [](VectorDatabase& self, VectorId id) -> py::object {
            auto vec = self.get_vector(id);
            if (!vec) {
                return py::none();
            }
            return py::cast(vector_to_numpy(*vec));
        }, py::arg("id"))
        
        // Metadata
        .def("get_metadata", &VectorDatabase::get_metadata, py::arg("id"))
        .def("find_by_date", &VectorDatabase::find_by_date, py::arg("date"))
        .def("find_by_type", &VectorDatabase::find_by_type, py::arg("type"))
        .def("find_by_asset", &VectorDatabase::find_by_asset, py::arg("asset"))
        
        // Deletion
        .def("remove", [](VectorDatabase& self, VectorId id) {
            auto result = self.remove(id);
            if (!result) {
                throw std::runtime_error(result.error().message);
            }
        }, py::arg("id"))
        
        .def("remove_by_date", [](VectorDatabase& self, const std::string& date) {
            auto result = self.remove_by_date(date);
            if (!result) {
                throw std::runtime_error(result.error().message);
            }
            return *result;
        }, py::arg("date"))
        
        // Stats & management
        .def("__len__", &VectorDatabase::size)
        .def("size", &VectorDatabase::size)
        .def("count_by_type", &VectorDatabase::count_by_type, py::arg("type"))
        .def("all_dates", &VectorDatabase::all_dates)
        .def("stats", &VectorDatabase::stats)
        .def("optimize", &VectorDatabase::optimize)
        .def("sync", [](VectorDatabase& self) {
            auto result = self.sync();
            if (!result) {
                throw std::runtime_error(result.error().message);
            }
        })
        .def("compact", [](VectorDatabase& self) {
            auto result = self.compact();
            if (!result) {
                throw std::runtime_error(result.error().message);
            }
        })
        
        // Export
        .def("export_training_data", [](VectorDatabase& self, const std::string& path) {
            auto result = self.export_training_data(path);
            if (!result) {
                throw std::runtime_error(result.error().message);
            }
        }, py::arg("output_path"));
    
    // ========================================================================
    // Ingest
    // ========================================================================
    
    py::class_<IngestStats>(m, "IngestStats")
        .def_readonly("journals_added", &IngestStats::journals_added)
        .def_readonly("charts_added", &IngestStats::charts_added)
        .def_readonly("reports_added", &IngestStats::reports_added)
        .def_readonly("skipped", &IngestStats::skipped)
        .def_readonly("errors", &IngestStats::errors)
        .def_readonly("error_messages", &IngestStats::error_messages)
        .def("__repr__", [](const IngestStats& s) {
            return "<IngestStats journals=" + std::to_string(s.journals_added) +
                   " charts=" + std::to_string(s.charts_added) +
                   " reports=" + std::to_string(s.reports_added) +
                   " errors=" + std::to_string(s.errors) + ">";
        });
    
    py::class_<IngestConfig>(m, "IngestConfig")
        .def(py::init<>())
        .def_readwrite("gold_standard_output", &IngestConfig::gold_standard_output)
        .def_readwrite("include_journals", &IngestConfig::include_journals)
        .def_readwrite("include_charts", &IngestConfig::include_charts)
        .def_readwrite("include_reports", &IngestConfig::include_reports)
        .def_readwrite("overwrite_existing", &IngestConfig::overwrite_existing)
        .def_readwrite("incremental", &IngestConfig::incremental)
        .def_readwrite("since_date", &IngestConfig::since_date);
    
    py::class_<GoldStandardIngest>(m, "GoldStandardIngest")
        .def(py::init<VectorDatabase&>())
        .def("ingest", [](GoldStandardIngest& self, const IngestConfig& config) {
            auto result = self.ingest(config);
            if (!result) {
                throw std::runtime_error(result.error().message);
            }
            return *result;
        }, py::arg("config"))
        .def("ingest_journal", [](GoldStandardIngest& self, const std::string& path) {
            auto result = self.ingest_journal(path);
            if (!result) {
                throw std::runtime_error(result.error().message);
            }
            return *result;
        }, py::arg("path"))
        .def("ingest_chart", [](GoldStandardIngest& self, const std::string& path) {
            auto result = self.ingest_chart(path);
            if (!result) {
                throw std::runtime_error(result.error().message);
            }
            return *result;
        }, py::arg("path"))
        .def("ingest_report", [](GoldStandardIngest& self, const std::string& path) {
            auto result = self.ingest_report(path);
            if (!result) {
                throw std::runtime_error(result.error().message);
            }
            return *result;
        }, py::arg("path"))
        .def("last_stats", &GoldStandardIngest::last_stats);
    
    // ========================================================================
    // Factory Functions
    // ========================================================================
    
    m.def("create_gold_standard_db", [](const std::string& path) {
        auto result = create_gold_standard_db(path);
        if (!result) {
            throw std::runtime_error(result.error().message);
        }
        return std::move(*result);
    }, py::arg("path"), "Create a new database optimized for Gold Standard");
    
    m.def("open_database", [](const std::string& path) {
        auto result = open_database(path);
        if (!result) {
            throw std::runtime_error(result.error().message);
        }
        return std::move(*result);
    }, py::arg("path"), "Open an existing database");
    
    // ========================================================================
    // Utility Functions
    // ========================================================================
    
    m.def("detect_best_provider", &detect_best_provider,
          "Detect the best available execution provider (CPU/CUDA/DirectML)");
    
    m.def("is_provider_available", &is_provider_available, py::arg("provider"),
          "Check if a specific execution provider is available");
}
