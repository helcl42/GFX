#include <common/Logger.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <string>
#include <vector>

// Test Logger functionality
// Tests the singleton Logger class that manages logging callbacks

// ============================================================================
// Test Fixture with Captured Logs
// ============================================================================

struct LogEntry {
    GfxLogLevel level;
    std::string message;
};

class LoggerTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        // Clear any previous callback
        gfx::common::Logger::instance().setCallback(nullptr, nullptr);
        capturedLogs.clear();
    }

    void TearDown() override
    {
        // Clean up after each test
        gfx::common::Logger::instance().setCallback(nullptr, nullptr);
        capturedLogs.clear();
    }

    // Static callback that captures log messages
    static void captureCallback(GfxLogLevel level, const char* message, void* userData)
    {
        auto* logs = static_cast<std::vector<LogEntry>*>(userData);
        logs->push_back({ level, std::string(message) });
    }

    std::vector<LogEntry> capturedLogs;
};

// ============================================================================
// Singleton Tests
// ============================================================================

TEST_F(LoggerTest, GetInstance_ReturnsSameInstance)
{
    gfx::common::Logger& instance1 = gfx::common::Logger::instance();
    gfx::common::Logger& instance2 = gfx::common::Logger::instance();

    EXPECT_EQ(&instance1, &instance2);
}

// ============================================================================
// Callback Management Tests
// ============================================================================

TEST_F(LoggerTest, SetCallback_NullCallback_DoesNotCrash)
{
    EXPECT_NO_THROW({
        gfx::common::Logger::instance().setCallback(nullptr, nullptr);
    });
}

TEST_F(LoggerTest, SetCallback_ValidCallback_DoesNotCrash)
{
    EXPECT_NO_THROW({
        gfx::common::Logger::instance().setCallback(captureCallback, &capturedLogs);
    });
}

TEST_F(LoggerTest, SetCallback_CanBeCleared)
{
    // Set a callback
    gfx::common::Logger::instance().setCallback(captureCallback, &capturedLogs);
    gfx::common::Logger::instance().logInfo("Test message");
    EXPECT_EQ(capturedLogs.size(), 1u);

    // Clear callback
    gfx::common::Logger::instance().setCallback(nullptr, nullptr);
    gfx::common::Logger::instance().logInfo("This should not be captured");

    // Should still only have 1 message
    EXPECT_EQ(capturedLogs.size(), 1u);
}

TEST_F(LoggerTest, SetCallback_CanBeReplaced)
{
    std::vector<LogEntry> firstCapture;
    std::vector<LogEntry> secondCapture;

    // Set first callback
    gfx::common::Logger::instance().setCallback(captureCallback, &firstCapture);
    gfx::common::Logger::instance().logInfo("First message");

    // Replace with second callback
    gfx::common::Logger::instance().setCallback(captureCallback, &secondCapture);
    gfx::common::Logger::instance().logInfo("Second message");

    EXPECT_EQ(firstCapture.size(), 1u);
    EXPECT_EQ(secondCapture.size(), 1u);
    EXPECT_EQ(firstCapture[0].message, "First message");
    EXPECT_EQ(secondCapture[0].message, "Second message");
}

// ============================================================================
// Log Level Tests
// ============================================================================

TEST_F(LoggerTest, LogError_CallsCallbackWithErrorLevel)
{
    gfx::common::Logger::instance().setCallback(captureCallback, &capturedLogs);

    gfx::common::Logger::instance().logError("Error message");

    ASSERT_EQ(capturedLogs.size(), 1u);
    EXPECT_EQ(capturedLogs[0].level, GFX_LOG_LEVEL_ERROR);
    EXPECT_EQ(capturedLogs[0].message, "Error message");
}

TEST_F(LoggerTest, LogWarning_CallsCallbackWithWarningLevel)
{
    gfx::common::Logger::instance().setCallback(captureCallback, &capturedLogs);

    gfx::common::Logger::instance().logWarning("Warning message");

    ASSERT_EQ(capturedLogs.size(), 1u);
    EXPECT_EQ(capturedLogs[0].level, GFX_LOG_LEVEL_WARNING);
    EXPECT_EQ(capturedLogs[0].message, "Warning message");
}

TEST_F(LoggerTest, LogInfo_CallsCallbackWithInfoLevel)
{
    gfx::common::Logger::instance().setCallback(captureCallback, &capturedLogs);

    gfx::common::Logger::instance().logInfo("Info message");

    ASSERT_EQ(capturedLogs.size(), 1u);
    EXPECT_EQ(capturedLogs[0].level, GFX_LOG_LEVEL_INFO);
    EXPECT_EQ(capturedLogs[0].message, "Info message");
}

TEST_F(LoggerTest, LogDebug_CallsCallbackWithDebugLevel)
{
    gfx::common::Logger::instance().setCallback(captureCallback, &capturedLogs);

    gfx::common::Logger::instance().logDebug("Debug message");

    ASSERT_EQ(capturedLogs.size(), 1u);
    EXPECT_EQ(capturedLogs[0].level, GFX_LOG_LEVEL_DEBUG);
    EXPECT_EQ(capturedLogs[0].message, "Debug message");
}

// ============================================================================
// No Callback Tests (should not crash)
// ============================================================================

TEST_F(LoggerTest, LogError_NoCallback_DoesNotCrash)
{
    EXPECT_NO_THROW({
        gfx::common::Logger::instance().logError("Error without callback");
    });
}

TEST_F(LoggerTest, LogWarning_NoCallback_DoesNotCrash)
{
    EXPECT_NO_THROW({
        gfx::common::Logger::instance().logWarning("Warning without callback");
    });
}

TEST_F(LoggerTest, LogInfo_NoCallback_DoesNotCrash)
{
    EXPECT_NO_THROW({
        gfx::common::Logger::instance().logInfo("Info without callback");
    });
}

TEST_F(LoggerTest, LogDebug_NoCallback_DoesNotCrash)
{
    EXPECT_NO_THROW({
        gfx::common::Logger::instance().logDebug("Debug without callback");
    });
}

// ============================================================================
// Format String Tests
// ============================================================================

TEST_F(LoggerTest, LogError_WithFormatArgs_FormatsCorrectly)
{
    gfx::common::Logger::instance().setCallback(captureCallback, &capturedLogs);

    int value = 42;
    gfx::common::Logger::instance().logError("Error code: {}", value);

    ASSERT_EQ(capturedLogs.size(), 1u);
    EXPECT_EQ(capturedLogs[0].message, "Error code: 42");
}

TEST_F(LoggerTest, LogWarning_WithFormatArgs_FormatsCorrectly)
{
    gfx::common::Logger::instance().setCallback(captureCallback, &capturedLogs);

    std::string name = "resource";
    gfx::common::Logger::instance().logWarning("Resource {} not found", name);

    ASSERT_EQ(capturedLogs.size(), 1u);
    EXPECT_EQ(capturedLogs[0].message, "Resource resource not found");
}

TEST_F(LoggerTest, LogInfo_WithMultipleFormatArgs_FormatsCorrectly)
{
    gfx::common::Logger::instance().setCallback(captureCallback, &capturedLogs);

    gfx::common::Logger::instance().logInfo("Created {} with size {}x{}", "texture", 1024, 768);

    ASSERT_EQ(capturedLogs.size(), 1u);
    EXPECT_EQ(capturedLogs[0].message, "Created texture with size 1024x768");
}

TEST_F(LoggerTest, LogDebug_WithHexFormat_FormatsCorrectly)
{
    gfx::common::Logger::instance().setCallback(captureCallback, &capturedLogs);

    uint32_t address = 0xDEADBEEF;
    gfx::common::Logger::instance().logDebug("Memory address: {:#x}", address);

    ASSERT_EQ(capturedLogs.size(), 1u);
    EXPECT_EQ(capturedLogs[0].message, "Memory address: 0xdeadbeef");
}

TEST_F(LoggerTest, LogError_EmptyString_DoesNotCrash)
{
    gfx::common::Logger::instance().setCallback(captureCallback, &capturedLogs);

    EXPECT_NO_THROW({
        gfx::common::Logger::instance().logError("");
    });

    ASSERT_EQ(capturedLogs.size(), 1u);
    EXPECT_EQ(capturedLogs[0].message, "");
}

// ============================================================================
// Multiple Messages Tests
// ============================================================================

TEST_F(LoggerTest, MultipleMessages_AllCaptured)
{
    gfx::common::Logger::instance().setCallback(captureCallback, &capturedLogs);

    gfx::common::Logger::instance().logError("Error 1");
    gfx::common::Logger::instance().logWarning("Warning 1");
    gfx::common::Logger::instance().logInfo("Info 1");
    gfx::common::Logger::instance().logDebug("Debug 1");

    ASSERT_EQ(capturedLogs.size(), 4u);
    EXPECT_EQ(capturedLogs[0].level, GFX_LOG_LEVEL_ERROR);
    EXPECT_EQ(capturedLogs[1].level, GFX_LOG_LEVEL_WARNING);
    EXPECT_EQ(capturedLogs[2].level, GFX_LOG_LEVEL_INFO);
    EXPECT_EQ(capturedLogs[3].level, GFX_LOG_LEVEL_DEBUG);
}

TEST_F(LoggerTest, MultipleMessages_CorrectOrder)
{
    gfx::common::Logger::instance().setCallback(captureCallback, &capturedLogs);

    gfx::common::Logger::instance().logInfo("First");
    gfx::common::Logger::instance().logInfo("Second");
    gfx::common::Logger::instance().logInfo("Third");

    ASSERT_EQ(capturedLogs.size(), 3u);
    EXPECT_EQ(capturedLogs[0].message, "First");
    EXPECT_EQ(capturedLogs[1].message, "Second");
    EXPECT_EQ(capturedLogs[2].message, "Third");
}

// ============================================================================
// User Data Tests
// ============================================================================

TEST_F(LoggerTest, UserData_PassedToCallback)
{
    int testValue = 12345;
    bool callbackInvoked = false;

    auto callback = [](GfxLogLevel level, const char* message, void* userData) {
        (void)level;
        (void)message;
        int* value = static_cast<int*>(userData);
        EXPECT_EQ(*value, 12345);
    };

    gfx::common::Logger::instance().setCallback(callback, &testValue);
    gfx::common::Logger::instance().logInfo("Test");
}

TEST_F(LoggerTest, UserData_CanBeNull)
{
    auto callback = [](GfxLogLevel level, const char* message, void* userData) {
        (void)level;
        (void)message;
        EXPECT_EQ(userData, nullptr);
    };

    gfx::common::Logger::instance().setCallback(callback, nullptr);
    gfx::common::Logger::instance().logInfo("Test with null userData");
}

// ============================================================================
// Special Character Tests
// ============================================================================

TEST_F(LoggerTest, LogMessage_WithNewlines_CapturesCorrectly)
{
    gfx::common::Logger::instance().setCallback(captureCallback, &capturedLogs);

    gfx::common::Logger::instance().logInfo("Line 1\nLine 2\nLine 3");

    ASSERT_EQ(capturedLogs.size(), 1u);
    EXPECT_EQ(capturedLogs[0].message, "Line 1\nLine 2\nLine 3");
}

TEST_F(LoggerTest, LogMessage_WithUnicode_CapturesCorrectly)
{
    gfx::common::Logger::instance().setCallback(captureCallback, &capturedLogs);

    gfx::common::Logger::instance().logInfo("Unicode: ✓ ✗ ♠ ♥");

    ASSERT_EQ(capturedLogs.size(), 1u);
    EXPECT_EQ(capturedLogs[0].message, "Unicode: ✓ ✗ ♠ ♥");
}

TEST_F(LoggerTest, LogMessage_WithSpecialChars_CapturesCorrectly)
{
    gfx::common::Logger::instance().setCallback(captureCallback, &capturedLogs);

    gfx::common::Logger::instance().logInfo("Special: \t \r {{}}");

    ASSERT_EQ(capturedLogs.size(), 1u);
    EXPECT_EQ(capturedLogs[0].message, "Special: \t \r {}");
}

// ============================================================================
// Long Message Tests
// ============================================================================

TEST_F(LoggerTest, LogMessage_VeryLongMessage_HandlesCorrectly)
{
    gfx::common::Logger::instance().setCallback(captureCallback, &capturedLogs);

    std::string longMessage(10000, 'A');
    gfx::common::Logger::instance().logInfo("{}", longMessage);

    ASSERT_EQ(capturedLogs.size(), 1u);
    EXPECT_EQ(capturedLogs[0].message.size(), 10000u);
    EXPECT_EQ(capturedLogs[0].message, longMessage);
}
