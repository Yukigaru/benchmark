#include <benchmark/benchmark.h>
#include <chrono>
#include <gtest/gtest.h>
#include <iostream>
#include <thread>

static BenchmarkSetup bs;

TEST(Benchmark, Durations)
{
    for (int timeMs = 10; timeMs <= 1000; timeMs *= 10) {
        Benchmark b(bs);
        b.run([=](benchmark::detail::RunState &) { std::this_thread::sleep_for(std::chrono::milliseconds(timeMs)); });
        ASSERT_NEAR((double)std::chrono::milliseconds(timeMs).count(),
                    (double)std::chrono::duration_cast<std::chrono::milliseconds>(b.statistics().averageTime()).count(),
                    10.0);
        ASSERT_NEAR((double)std::chrono::milliseconds(timeMs).count(),
                    (double)std::chrono::duration_cast<std::chrono::milliseconds>(b.statistics().medianTime()).count(),
                    10.0);
        ASSERT_GT(b.statistics().averageTime(), std::chrono::steady_clock::duration(0));
        ASSERT_GT(b.statistics().minimalTime(), std::chrono::steady_clock::duration(0));
        ASSERT_GT(b.statistics().medianTime(), std::chrono::steady_clock::duration(0));
        ASSERT_LE(b.statistics().standardDeviation(), b.statistics().averageTime());
        ASSERT_GE(b.statistics().averageTime(), b.statistics().minimalTime());
        ASSERT_GE(b.statistics().medianTime(), b.statistics().minimalTime());
        ASSERT_LE(b.statistics().averageTime(), b.statistics().maximalTime());
        ASSERT_LE(b.statistics().medianTime(), b.statistics().maximalTime());
        ASSERT_LE(b.statistics().minimalTime(), b.statistics().maximalTime());
        ASSERT_GT(b.totalIterations(), 0u);
    }
}

TEST(Main, CustomSamples)
{
    Benchmark b(bs);

    b.debugAddSample(std::chrono::milliseconds(1));
    b.debugAddSample(std::chrono::milliseconds(2));
    b.debugAddSample(std::chrono::milliseconds(3));
    b.debugAddSample(std::chrono::milliseconds(4));
    b.calculateTimings();

    ASSERT_EQ(b.statistics().minimalTime(), std::chrono::milliseconds(1));
    ASSERT_EQ(b.statistics().maximalTime(), std::chrono::milliseconds(4));
    ASSERT_EQ(b.statistics().averageTime(), std::chrono::microseconds(2500));
    ASSERT_EQ(b.statistics().medianTime(), std::chrono::microseconds(2500));
    ASSERT_EQ(b.totalIterations(), 4);
    ASSERT_EQ(b.statistics().totalTimeRun(), std::chrono::milliseconds(10));
}

TEST(Main, StdDeviation)
{
    Benchmark b(bs);
    b.run([](benchmark::detail::RunState &) { std::this_thread::sleep_for(std::chrono::milliseconds(100)); });
    ASSERT_LT(b.statistics().standardDeviation(), std::chrono::microseconds(100));
}

TEST(Main, DoNothing)
{
    Benchmark b(bs);

    b.run([](benchmark::detail::RunState &) {
        // do nothing
    });

    std::cout << "Do nothing took " << b.statistics().maximalTime() << " max" << std::endl;
    ASSERT_LE(b.statistics().averageTime(), std::chrono::microseconds(2));
    ASSERT_LE(b.statistics().medianTime(), std::chrono::microseconds(2));
    ASSERT_LE(b.statistics().minimalTime(), std::chrono::microseconds(2));
    ASSERT_LE(b.statistics().maximalTime(), std::chrono::microseconds(2));
    ASSERT_GT(b.totalIterations(), 1);
}

int main(int argc, char **argv)
{
    bs.outputStyle = BenchmarkSetup::Nothing;
    bs.verbose = true;

    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
