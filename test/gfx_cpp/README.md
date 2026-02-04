# GfxWrapper C++ API Tests

Tests for the C++ API using Google Test, organized into separate test targets.

## Test Targets

### `gfx_cpp_api_test` - Public API Tests (Always Built)

Black-box tests that verify the public C++ API contract. These tests:
- Use only public header: `#include <gfx_cpp/gfx.hpp>`
- Test API behavior from a user's perspective
- Are backend-agnostic (work with any backend)
- Run on all platforms

**Location**: `test/gfx_cpp/api/`

### `gfx_cpp_internal_test` - Internal Implementation Tests (Always Built)

White-box tests for internal C++ wrapper implementation:
- Converter logic (C++ to C descriptor conversion)
- Shared pointer management
- Internal utilities

**Location**: `test/gfx_cpp/internal/`

## Running Tests

```bash
cd build

# Run all C++ API tests
./test/gfx_cpp/gfx_cpp_api_test

# Run internal C++ wrapper tests
./test/gfx_cpp/gfx_cpp_internal_test

# Run all tests with CTest
ctest

# Run specific test suite using filters
./test/gfx_cpp/gfx_cpp_api_test --gtest_filter="InstanceTest.*"
```

## Adding Tests

1. **Public API test**: Add to `test/gfx_cpp/api/` and update the `gfx_cpp_api_test` target in `CMakeLists.txt`
2. **Internal wrapper test**: Add to `test/gfx_cpp/internal/` and update `gfx_cpp_internal_test` target
