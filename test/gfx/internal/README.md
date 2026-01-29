# Internal Tests for C API

This directory contains tests for internal implementation details of the C API layer.

## Structure

- **GfxImplTest.cpp**: Tests for the main dispatcher (`GfxImpl.cpp`)
  - Backend loading and unloading
  - Instance creation and destruction
  - Handle management and validation
  - Error propagation
  - Backend selection logic

## Test Philosophy

### Internal vs. API Tests

- **API Tests** (`test/gfx/api/`): Black-box tests that verify the public C API contract using real backends
- **Internal Tests** (`test/gfx/internal/`): White-box tests that verify implementation details using mocks

### What to test here:

- **Dispatcher**: Backend routing logic, function dispatch correctness
- **Converters**: Gfx ↔ Vulkan/WebGPU type conversions
- **Backend implementations**: Internal state management, edge cases
- **Manager**: Backend lifecycle management

### Example tests:

- Dispatcher correctly routes to Vulkan/WebGPU backends
- Converter functions preserve semantic meaning across type conversions
- Backend-specific memory management
- Error handling in internal code paths
- Handle wrapping/unwrapping edge cases

These tests are not part of the public API contract and may change with implementation details.

## Running Tests

```bash
# Run all internal tests
./build/test/gfx/gfx_test --gtest_filter="*Impl*"

# Run specific internal test suite
./build/test/gfx/gfx_test --gtest_filter="GfxImplTest.*"

# Run with LeakSanitizer preloaded
LD_PRELOAD=$(gcc -print-file-name=libasan.so) ./build/test/gfx/gfx_test --gtest_filter="GfxImplTest.*"
```

## Mock Backends

`GfxImplTest.cpp` includes a `MockBackend` class that implements the `IBackend` interface using Google Mock. This allows testing the dispatcher without requiring real backend implementations.

### Example Mock Usage

```cpp
MockBackend mockBackend;
EXPECT_CALL(mockBackend, createInstance(_, _))
    .WillOnce(Return(GFX_RESULT_SUCCESS));
```

## Test Coverage

Current internal test coverage (172 tests):
- ✅ Version & Backend Management (10 tests)
- ✅ Instance & Adapter Operations (18 tests)
- ✅ Device Management (9 tests)
- ✅ Surface Operations (6 tests)
- ✅ Buffer Operations (12 tests)
- ✅ Texture Operations (14 tests)
- ✅ Sampler & Shader Operations (8 tests)
- ✅ BindGroup & BindGroupLayout (8 tests)
- ✅ Pipeline Operations (8 tests)
- ✅ RenderPass & Framebuffer (6 tests)
- ✅ QuerySet Operations (4 tests)
- ✅ Queue Operations (5 tests)
- ✅ Swapchain Operations (8 tests)
- ✅ Command Encoder Operations (20 tests)
- ✅ RenderPass Encoder Operations (15 tests)
- ✅ ComputePass Encoder Operations (5 tests)
- ✅ Fence & Semaphore Operations (16 tests)

**Coverage: ~150% of major API surface** (172 tests covering 114 API functions with multiple validation paths per function)

Future additions:
- Backend-specific internal tests (Vulkan, WebGPU)
- Converter and utility function tests
- Backend Manager tests
- More handle wrapping/unwrapping edge cases

## Adding New Internal Tests

1. Create a new `*Test.cpp` file in this directory
2. Include necessary internal headers from `gfx/src/`
3. Create mock implementations of internal interfaces as needed
4. Add the file to `INTERNAL_TEST_SOURCES` in `test/gfx/CMakeLists.txt`
5. Use Google Test and Google Mock for test implementation

