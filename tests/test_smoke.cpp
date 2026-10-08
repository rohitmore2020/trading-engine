#include <gtest/gtest.h>

#include <string_view>

#include "te/placeholder.hpp"

TEST(SmokeTest, EngineVersionIsNotEmpty) {
    const char* ver = te::version();
    ASSERT_NE(ver, nullptr);
    EXPECT_EQ(std::string_view(ver), "0.1.0");
}

TEST(SmokeTest, BasicMathInvariants) {
    EXPECT_EQ(1 + 1, 2);
}
