# Internal Tests for C API

This directory contains tests for internal implementation details of the C API layer:

## What to test here:
- **Dispatcher**: Backend routing logic, function dispatch correctness
- **Converters**: Gfx ↔ Vulkan/WebGPU type conversions
- **Backend implementations**: Internal state management, edge cases
- **Manager**: Backend lifecycle management

## Example tests:
- Dispatcher correctly routes to Vulkan/WebGPU backends
- Converter functions preserve semantic meaning across type conversions
- Backend-specific memory management
- Error handling in internal code paths

These tests are not part of the public API contract and may change with implementation details.
