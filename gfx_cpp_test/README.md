# GfxWrapper C++ Wrapper Tests

Simple tests for the C++ wrapper using Google Test.

## Running Tests

```bash
cd build
./gfx_cpp_test/gfx_cpp_test
```

## Test Coverage

- **basic_test.cpp**: Basic sanity tests (enum values, linking)
- **instance_test.cpp**: Instance creation/destruction tests using shared_ptr semantics
- **result_test.cpp**: Result enum and error handling pattern tests

## Adding Tests

Add new test files to `CMakeLists.txt` and follow the existing pattern using Google Test macros.
