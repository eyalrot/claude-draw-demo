#include <gtest/gtest.h>
#include <gmock/gmock.h>

// Main entry point for tests
// GTest will automatically run all TEST() and TEST_F() tests

// Custom test environment for setup/teardown
class ClaudeDrawTestEnvironment : public ::testing::Environment {
public:
    void SetUp() override {
        // Global test setup
        // e.g., initialize memory pools, set up test data directories
    }
    
    void TearDown() override {
        // Global test cleanup
        // e.g., clean up temporary files, check for memory leaks
    }
};

int main(int argc, char **argv) {
    // Initialize Google Test
    ::testing::InitGoogleTest(&argc, argv);
    
    // Add custom test environment
    ::testing::AddGlobalTestEnvironment(new ClaudeDrawTestEnvironment);
    
    // Run all tests
    return RUN_ALL_TESTS();
}

// Example test to verify test framework is working
TEST(TestFramework, BasicAssertion) {
    EXPECT_TRUE(true);
    EXPECT_FALSE(false);
    EXPECT_EQ(1 + 1, 2);
}

TEST(TestFramework, StringComparison) {
    std::string expected = "hello";
    std::string actual = "hello";
    EXPECT_EQ(expected, actual);
}

TEST(TestFramework, FloatingPointComparison) {
    double a = 1.0 / 3.0;
    double b = 0.333333333;
    EXPECT_NEAR(a, b, 0.000001);
}