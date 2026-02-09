# Gfx C API Tests

Tests for the C API using Google Test, organized into separate test targets.

## Test Targets

### `gfx_api_test` - Public API Tests (Always Built)

Black-box tests that verify the public C API contract. These tests:
- Use only public header: `#include <gfx/gfx.h>`
- Test API behavior from a user's perspective
- Are backend-agnostic (work with any backend)
- Run on all platforms

**Location**: `test/gfx/api/`

### `gfx_internal_test` - Internal Common Tests (Always Built)

White-box tests for internal implementation that's not backend-specific:
- Dispatcher logic (`GfxImpl.cpp`)
- Common utilities (Logger, etc.)
- Handle management

**Location**: `test/gfx/internal/`

### `gfx_internal_vulkan_test` - Vulkan Backend Tests (Conditional)

Internal tests specific to the Vulkan backend:
- Only built when `BUILD_VULKAN_BACKEND=ON` and not on web
- Tests Vulkan-specific converters, resource management, etc.
- White-box tests that access internal Vulkan implementation

**Location**: `test/gfx/internal/backend/vulkan/`

### `gfx_internal_webgpu_test` - WebGPU Backend Tests (Conditional)

Internal tests specific to the WebGPU backend:
- Only built when `BUILD_WEBGPU_BACKEND=ON`
- Tests WebGPU-specific converters, resource management, etc.
- White-box tests that access internal WebGPU implementation

**Location**: `test/gfx/internal/backend/webgpu/`

## Running Tests

```bash
cd build

# Run all API tests
./test/gfx/gfx_api_test

# Run internal common tests
./test/gfx/gfx_internal_test

# Run Vulkan backend tests (if built)
./test/gfx/gfx_internal_vulkan_test

# Run WebGPU backend tests (if built)
./test/gfx/gfx_internal_webgpu_test

# Run all tests with CTest
ctest

# Run specific test suite using filters
./test/gfx/gfx_api_test --gtest_filter="InstanceTest.*"
```

## Adding Tests

1. **Public API test**: Add to `test/gfx/api/` and update the `gfx_api_test` target in `CMakeLists.txt`
2. **Internal common test**: Add to `test/gfx/internal/` and update `gfx_internal_test` target
3. **Vulkan-specific test**: Add to `test/gfx/internal/backend/vulkan/` and update `gfx_internal_vulkan_test` target
4. **WebGPU-specific test**: Add to `test/gfx/internal/backend/webgpu/` and update `gfx_internal_webgpu_test` target
