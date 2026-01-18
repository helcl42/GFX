# Internal Tests for C++ Wrapper

This directory contains tests for internal implementation details of the C++ wrapper:

## What to test here:
- **RAII semantics**: Resource lifetime management, smart pointer behavior
- **Type conversions**: C++ ↔ C API type mappings
- **Error propagation**: Exception/Result enum handling in wrapper layer
- **Template implementations**: Generic code paths in wrapper
- **Internal helpers**: Utility functions not exposed in public API

## Example tests:
- Proper cleanup of resources in destructors
- Move semantics and copy prevention
- Conversion between C++ wrapper types and C API handles
- Exception safety guarantees

These tests are not part of the public API contract and may change with implementation details.
