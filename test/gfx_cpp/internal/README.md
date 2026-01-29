# Internal Tests for C++ Wrapper

This directory contains tests for internal implementation details of the C++ wrapper.

## Structure

- **GfxCppWrapperTest.cpp**: Tests for RAII wrapper classes
  - Default construction validity
  - Move semantics and ownership transfer
  - Backend management functions
  - Version queries
  - Instance creation with real backends

## Test Philosophy

### Internal vs. API Tests

- **API Tests** (`test/gfx_cpp/api/`): Black-box tests that verify the public C++ API contract using real backends
- **Internal Tests** (`test/gfx_cpp/internal/`): White-box tests that verify RAII semantics and wrapper implementation

### What to test here:

- **RAII semantics**: Resource lifetime management, smart pointer behavior
- **Type conversions**: C++ ↔ C API type mappings
- **Error propagation**: std::optional/Result handling in wrapper layer
- **Move semantics**: Proper ownership transfer, no copies
- **Template implementations**: Generic code paths in wrapper
- **Internal helpers**: Utility functions not exposed in public API

### Example tests:

- Proper cleanup of resources in destructors
- Move semantics and copy prevention
- Conversion between C++ wrapper types and C API handles
- Exception safety guarantees
- Handle validity checks

These tests are not part of the public API contract and may change with implementation details.

## Running Tests

```bash
# Run all internal tests
./build/test/gfx_cpp/gfx_cpp_test --gtest_filter="GfxCppWrapperTest.*"

# Run with LeakSanitizer
LD_PRELOAD=$(gcc -print-file-name=libasan.so) ./build/test/gfx_cpp/gfx_cpp_test --gtest_filter="GfxCppWrapperTest.*"
```

## Test Coverage

Current internal test coverage (46 tests):
- ✅ Default construction validity (21 tests)
- ✅ Move semantics (17 tests)
- ✅ Backend management (4 tests)
- ✅ Version queries (1 test)
- ✅ Instance creation with real backends (2 tests)
- ✅ Error propagation (1 test)

**Coverage: All major RAII wrapper classes** (21 resource types tested for validity and move semantics)

## Adding New Internal Tests

1. Add test functions to `GfxCppWrapperTest.cpp`
2. Focus on C++ wrapper semantics, not full API functionality
3. Use Google Test and std::optional for return values
4. Test RAII patterns and resource management
