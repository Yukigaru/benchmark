#include <benchmark/detail/state.h>
#include <gtest/gtest.h>

#include <climits>
#include <cstddef>
#include <vector>

namespace {

std::vector<int> arguments(int from, int to)
{
    benchmark::detail::BenchmarkState state;
    EXPECT_TRUE(state.addArgument(from, to));

    std::vector<int> result;
    while (state.running()) {
        state.pickNextArgument();
        result.push_back(state.getArg());
    }
    return result;
}

TEST(BenchmarkState, GeneratesCompleteLinearRanges)
{
    const auto zeroToTen = arguments(0, 10);
    ASSERT_EQ(11u, zeroToTen.size());
    for (size_t i = 0; i < zeroToTen.size(); ++i)
        EXPECT_EQ(static_cast<int>(i), zeroToTen[i]);

    const auto twentyToHundred = arguments(20, 100);
    ASSERT_EQ(81u, twentyToHundred.size());
    for (size_t i = 0; i < twentyToHundred.size(); ++i)
        EXPECT_EQ(20 + static_cast<int>(i), twentyToHundred[i]);
}

TEST(BenchmarkState, StopsAtIntegerLimits)
{
    EXPECT_EQ((std::vector<int>{INT_MAX - 1, INT_MAX}), arguments(INT_MAX - 1, INT_MAX));
    EXPECT_EQ((std::vector<int>{INT_MIN + 1, INT_MIN}), arguments(INT_MIN + 1, INT_MIN));
}

TEST(BenchmarkState, PreservesPowerRanges)
{
    EXPECT_EQ((std::vector<int>{1, 2, 4, 8}), arguments(1, 8));
    EXPECT_EQ((std::vector<int>{1000, 100, 10}), arguments(1000, 10));
}

} // namespace
