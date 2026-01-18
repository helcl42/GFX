#include <gfx_cpp/Gfx.hpp>
#include <gtest/gtest.h>

using namespace gfx;

// Test Result enum type
TEST(GfxCppResultTest, SuccessValue) {
    Result result = Result::Success;
    
    EXPECT_EQ(result, Result::Success);
    EXPECT_EQ(static_cast<int>(result), 0);
}

TEST(GfxCppResultTest, ErrorValue) {
    Result result = Result::ErrorInvalidArgument;
    
    EXPECT_EQ(result, Result::ErrorInvalidArgument);
    EXPECT_LT(static_cast<int>(result), 0);
}

TEST(GfxCppResultTest, EnumComparison) {
    Result success = Result::Success;
    Result error = Result::ErrorUnknown;
    
    EXPECT_NE(success, error);
    EXPECT_EQ(success, Result::Success);
    EXPECT_EQ(error, Result::ErrorUnknown);
}

TEST(GfxCppResultTest, OptionalPattern) {
    // Demonstrate the optional pattern used by the C++ wrapper
    std::optional<int> value = 42;
    Result result = Result::Success;
    
    if (value.has_value() && result == Result::Success) {
        EXPECT_EQ(value.value(), 42);
    } else {
        FAIL() << "Should have a value";
    }
}

TEST(GfxCppResultTest, ErrorCodePattern) {
    // Demonstrate error handling pattern
    std::optional<int> value = std::nullopt;
    Result result = Result::ErrorInvalidArgument;
    
    if (!value.has_value()) {
        EXPECT_EQ(result, Result::ErrorInvalidArgument);
        SUCCEED();
    } else {
        FAIL() << "Should not have a value on error";
    }
}
