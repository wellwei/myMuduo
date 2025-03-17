#include <gtest/gtest.h>
#include "TimeStamp.h"

using namespace muduo;

TEST(TimeStampTest, DefaultConstructor) {
    TimeStamp ts;
    EXPECT_EQ(ts.toString(), "1970-01-01 08:00:00");
}

TEST(TimeStampTest, ParameterizedConstructor) {
    TimeStamp ts(1000000000);
    EXPECT_EQ(ts.toString(), "1970-01-01 08:16:40");
}