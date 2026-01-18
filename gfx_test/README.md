# GfxWrapper C API Tests

Simple tests for the C API using Google Test.

## Running Tests

```bash
cd build
./gfx_test/gfx_test
```

## Test Coverage

- **basic_test.cpp**: Basic sanity tests (enum values, linking)
- **instance_test.cpp**: Instance creation/destruction tests for both backends

## Adding Tests

Add new test files to `CMakeLists.txt` and follow the existing pattern using Google Test macros.
