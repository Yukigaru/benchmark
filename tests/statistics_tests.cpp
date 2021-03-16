#include <benchmark/detail/state.h>
#include <benchmark/detail/statistics.h>
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

TEST(TimeStatistics, CalculatesLargeStandardDeviationWithoutOverflow)
{
    benchmark::TimeStatistics statistics;
    statistics.addSample(benchmark::duration_t(0));
    statistics.addSample(benchmark::duration_t::max());
    ASSERT_TRUE(statistics.calculate());

    const auto expected = benchmark::duration_t::max().count() / 2;
    const auto actual = statistics.standardDeviation().count();
    const auto difference = actual > expected ? actual - expected : expected - actual;
    EXPECT_LE(difference, 1);
}

TEST(TimeStatistics, SaturatesOverflowingTotals)
{
    benchmark::TimeStatistics statistics;
    statistics.addSample(benchmark::duration_t::max());
    statistics.addSample(benchmark::duration_t::max());
    ASSERT_TRUE(statistics.calculate());

    EXPECT_EQ(benchmark::duration_t::max(), statistics.totalTimeRun());
    EXPECT_EQ(benchmark::duration_t::max(), statistics.averageTime());
    EXPECT_EQ(benchmark::duration_t(0), statistics.standardDeviation());

    benchmark::TimeStatistics mixed;
    mixed.addSample(benchmark::duration_t::max());
    mixed.addSample(benchmark::duration_t::max());
    mixed.addSample(-benchmark::duration_t::max());
    ASSERT_TRUE(mixed.calculate());
    EXPECT_EQ(benchmark::duration_t::max(), mixed.totalTimeRun());
}

TEST(TimeStatistics, CalculatesMedianWithoutLosingOddTicks)
{
    benchmark::TimeStatistics statistics;
    statistics.addSample(benchmark::duration_t(1));
    statistics.addSample(benchmark::duration_t(3));
    ASSERT_TRUE(statistics.calculate());
    EXPECT_EQ(benchmark::duration_t(2), statistics.medianTime());
}



TEST(TimeStatistics, PercentileHandlesBoundsAndSingleSample)
{
    benchmark::TimeStatistics empty;
    EXPECT_EQ(benchmark::duration_t(0), empty.percentile(90));

    benchmark::TimeStatistics single;
    single.addSample(benchmark::duration_t(7));
    ASSERT_TRUE(single.calculate());
    EXPECT_EQ(benchmark::duration_t(7), single.percentile(90));

    benchmark::TimeStatistics values;
    values.addSample(benchmark::duration_t(1));
    values.addSample(benchmark::duration_t(3));
    ASSERT_TRUE(values.calculate());
    EXPECT_EQ(benchmark::duration_t(1), values.percentile(-1));
    EXPECT_EQ(benchmark::duration_t(3), values.percentile(101));
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
