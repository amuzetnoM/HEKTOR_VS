# Hektor CLI - Comprehensive Test Suite

## Overview

This document describes the comprehensive testing strategy for the Hektor CLI system, including automated regression testing for the entire Hektor ecosystem.

## Test Categories

### 1. Unit Tests (200+ tests)

Located in `tests/cli/unit/`

#### Command Tests
- `test_db_commands.cpp` - Database management commands
- `test_data_commands.cpp` - Data operations
- `test_search_commands.cpp` - Search functionality
- `test_hybrid_commands.cpp` - Hybrid search
- `test_ingest_commands.cpp` - Data ingestion
- `test_index_commands.cpp` - Index management
- `test_collection_commands.cpp` - Collections
- `test_metadata_commands.cpp` - Metadata and tags
- `test_export_commands.cpp` - Export operations
- `test_nlp_commands.cpp` - NLP engine
- `test_model_commands.cpp` - Model management
- `test_monitor_commands.cpp` - Monitoring
- `test_report_commands.cpp` - Reports

#### Framework Tests
- `test_cli_framework.cpp` - CLI core functionality
- `test_argument_parser.cpp` - Argument parsing
- `test_output_formatter.cpp` - Output formatting
- `test_interactive_shell.cpp` - REPL functionality
- `test_progress.cpp` - Progress indicators
- `test_colors.cpp` - Color output

### 2. Integration Tests (50+ tests)

Located in `tests/cli/integration/`

#### Workflow Tests
- `test_basic_workflow.cpp` - Init, add, search workflow
- `test_nlp_workflow.cpp` - NLP-powered search
- `test_ingestion_workflow.cpp` - Multi-source ingestion
- `test_hybrid_search_workflow.cpp` - Hybrid search pipeline
- `test_model_workflow.cpp` - Model download and quantization
- `test_monitoring_workflow.cpp` - Monitoring setup
- `test_export_workflow.cpp` - ML training export

#### End-to-End Tests
- `test_e2e_production.cpp` - Production deployment scenario
- `test_e2e_batch_processing.cpp` - Batch processing workflow
- `test_e2e_interactive.cpp` - Interactive REPL session

### 3. Regression Tests (Automated, Non-blocking)

Located in `tests/regression/`

#### System-Wide Regression Tests
- `test_regression_database.cpp` - Database operations
- `test_regression_search.cpp` - Search functionality
- `test_regression_nlp.cpp` - NLP engine
- `test_regression_ingestion.cpp` - Data ingestion
- `test_regression_performance.cpp` - Performance benchmarks

#### Regression Test Framework
```cpp
// tests/regression/regression_framework.hpp
class RegressionTest {
public:
    virtual void setup() = 0;
    virtual void run() = 0;
    virtual void teardown() = 0;
    virtual bool validate() = 0;
    
    // Non-blocking execution
    void runAsync();
    
    // Report generation
    void generateReport(const std::string& output_file);
};
```

#### Continuous Regression Testing
- Runs automatically on commit
- Non-blocking (doesn't stop development)
- Generates detailed reports
- Tracks performance over time
- Alerts on regressions

### 4. Performance Tests

Located in `tests/cli/performance/`

- `bench_search.cpp` - Search performance
- `bench_nlp.cpp` - NLP operations
- `bench_ingestion.cpp` - Ingestion throughput
- `bench_hybrid.cpp` - Hybrid search
- `bench_export.cpp` - Export operations

### 5. Security Tests

Located in `tests/cli/security/`

- `test_password_security.cpp` - Password handling
- `test_injection_protection.cpp` - SQL injection prevention
- `test_encryption.cpp` - Data encryption
- `test_authentication.cpp` - Auth system

## Running Tests

### All Tests
```bash
cd build
ctest --output-on-failure

# Verbose output
ctest -V
```

### Specific Test Suites
```bash
# Unit tests only
ctest -R "unit_"

# Integration tests
ctest -R "integration_"

# Regression tests (non-blocking)
ctest -R "regression_" &

# Performance tests
ctest -R "bench_"
```

### With Coverage
```bash
mkdir build-coverage
cd build-coverage
cmake .. -DCMAKE_BUILD_TYPE=Debug -DENABLE_COVERAGE=ON
cmake --build .
ctest
lcov --capture --directory . --output-file coverage.info
genhtml coverage.info --output-directory coverage-report
```

## Automated Regression Testing

### Configuration

Create `.hektor-regression.yml`:

```yaml
regression:
  enabled: true
  non_blocking: true
  schedule: "on_commit"
  
tests:
  - name: "Database Operations"
    suite: "test_regression_database"
    timeout: 300
    
  - name: "Search Performance"
    suite: "test_regression_search"
    timeout: 180
    
  - name: "NLP Engine"
    suite: "test_regression_nlp"
    timeout: 240
    
reporting:
  format: ["html", "json"]
  output_dir: "regression-reports"
  alert_on_failure: true
  performance_threshold: 0.1  # 10% regression threshold
```

### Running Regression Tests

```bash
# Manual trigger
./scripts/run_regression_tests.sh

# With specific suites
./scripts/run_regression_tests.sh --suite database,search

# Generate report only
./scripts/run_regression_tests.sh --report-only

# Continuous mode
./scripts/run_regression_tests.sh --continuous &
```

### Regression Test Report

Generated in `regression-reports/`:

- `summary.html` - Visual summary with charts
- `details.json` - Machine-readable results
- `performance.csv` - Performance metrics
- `failures.log` - Failure details

## Test Data

Located in `tests/data/`:

```
tests/data/
├── documents/
│   ├── sample.txt
│   ├── sample.pdf
│   ├── sample.csv
│   └── sample.json
├── models/
│   └── test-model/
├── databases/
│   └── test-db/
└── expected-results/
    ├── search-results.json
    ├── nlp-results.json
    └── hybrid-results.json
```

## Continuous Integration

### GitHub Actions Workflow

`.github/workflows/test.yml`:

```yaml
name: Hektor CLI Tests

on: [push, pull_request]

jobs:
  test:
    runs-on: ${{ matrix.os }}
    strategy:
      matrix:
        os: [ubuntu-latest, macos-latest, windows-latest]
        
    steps:
      - uses: actions/checkout@v3
      
      - name: Install Dependencies
        run: ./scripts/install_deps.sh
        
      - name: Build
        run: |
          mkdir build && cd build
          cmake .. -DCMAKE_BUILD_TYPE=Release
          cmake --build .
          
      - name: Run Unit Tests
        run: cd build && ctest -R "unit_" --output-on-failure
        
      - name: Run Integration Tests
        run: cd build && ctest -R "integration_" --output-on-failure
        
      - name: Run Regression Tests (Non-blocking)
        run: cd build && ctest -R "regression_" --output-on-failure || true
        
      - name: Upload Test Results
        if: always()
        uses: actions/upload-artifact@v3
        with:
          name: test-results
          path: build/Testing/
```

## Test Fixtures

### Database Fixture
```cpp
class DatabaseFixture {
public:
    DatabaseFixture() {
        temp_dir = create_temp_directory();
        db_path = temp_dir / "test_db";
        // Initialize test database
    }
    
    ~DatabaseFixture() {
        // Cleanup
        std::filesystem::remove_all(temp_dir);
    }
    
    std::filesystem::path db_path;
};
```

### CLI Fixture
```cpp
class CLIFixture {
public:
    CLIFixture() {
        cli = std::make_unique<CLI>();
        // Setup CLI environment
    }
    
    std::string execute(const std::string& command) {
        // Execute command and capture output
    }
    
    std::unique_ptr<CLI> cli;
};
```

## Mocking

### Mock Database
```cpp
class MockDatabase : public IDatabase {
public:
    MOCK_METHOD(void, init, (const std::string&), (override));
    MOCK_METHOD(void, add, (const Document&), (override));
    MOCK_METHOD(std::vector<Result>, search, (const std::string&, size_t), (override));
};
```

### Mock NLP Engine
```cpp
class MockNLPEngine : public INLPEngine {
public:
    MOCK_METHOD(std::vector<float>, embed, (const std::string&), (override));
    MOCK_METHOD(std::vector<Entity>, ner, (const std::string&), (override));
};
```

## Test Coverage Goals

- **Unit Tests**: 90% code coverage
- **Integration Tests**: All major workflows
- **Regression Tests**: All critical paths
- **Performance Tests**: Baseline + regression tracking
- **Security Tests**: All security-critical code

## Reporting

### Test Reports

Generated after each test run:

- `test-report.html` - Visual report with charts
- `test-results.json` - Machine-readable results
- `coverage-report/` - Code coverage
- `regression-report/` - Regression analysis

### Accessing Reports

```bash
# Open test report
open build/test-report.html

# View coverage
open build/coverage-report/index.html

# Check regression
open regression-reports/summary.html
```

## Best Practices

1. **Test Naming**: Use descriptive names (e.g., `test_search_with_filters_returns_correct_results`)
2. **Test Independence**: Each test should be independent and idempotent
3. **Fast Tests**: Unit tests should run in <1s, integration tests in <10s
4. **Clear Assertions**: Use descriptive assertion messages
5. **Mock External Dependencies**: Don't rely on network, external services
6. **Cleanup**: Always clean up test artifacts
7. **Regression Protection**: Add regression test for every bug fix

## Troubleshooting

### Test Failures

```bash
# Run specific test with verbose output
ctest -R "test_name" -V

# Run test directly
./build/tests/cli/unit/test_db_commands --gtest_filter=TestName

# Debug test
gdb --args ./build/tests/cli/unit/test_db_commands
```

### Common Issues

1. **Timeout**: Increase timeout in CTest configuration
2. **Race Conditions**: Use proper synchronization in parallel tests
3. **Resource Cleanup**: Ensure proper teardown in fixtures
4. **Flaky Tests**: Identify and fix non-deterministic behavior

## Future Improvements

- [ ] Property-based testing with rapidcheck
- [ ] Fuzzing with libFuzzer/AFL
- [ ] Stress testing framework
- [ ] Visual regression testing for CLI output
- [ ] Automated performance regression detection
- [ ] Test result dashboards
- [ ] Historical test data analysis

---

For questions or issues with tests, see [CONTRIBUTING.md](../../CONTRIBUTING.md) or open an issue.
