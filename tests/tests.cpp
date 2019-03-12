#include <gtest/gtest.h>
#include <benchmark.h>
#include <thread>
#include <chrono>
#include <iostream>

static BenchmarkSetup bs;

TEST(Benchmark, Durations)
{
    for (int timeMs = 10; timeMs <= 1000; timeMs *= 10)
    {
        Benchmark b(bs);
        b.run([=](benchmark::detail::RunState &) { std::this_thread::sleep_for(std::chrono::milliseconds(timeMs)); });
        ASSERT_NEAR((double)std::chrono::milliseconds(timeMs).count(),
                    (double)std::chrono::duration_cast<std::chrono::milliseconds>(b.averageTime()).count(), 10.0);
        ASSERT_NEAR((double)std::chrono::milliseconds(timeMs).count(),
                    (double)std::chrono::duration_cast<std::chrono::milliseconds>(b.medianTime()).count(), 10.0);
        ASSERT_GT(b.averageTime(), std::chrono::steady_clock::duration(0));
        ASSERT_GT(b.minimalTime(), std::chrono::steady_clock::duration(0));
        ASSERT_GT(b.medianTime(), std::chrono::steady_clock::duration(0));
        ASSERT_LE(b.standardDeviation(), b.averageTime());
        ASSERT_GE(b.averageTime(), b.minimalTime());
        ASSERT_GE(b.medianTime(), b.minimalTime());
        ASSERT_LE(b.averageTime(), b.maximalTime());
        ASSERT_LE(b.medianTime(), b.maximalTime());
        ASSERT_LE(b.minimalTime(), b.maximalTime());
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

    ASSERT_EQ(b.minimalTime(), std::chrono::milliseconds(1));
    ASSERT_EQ(b.maximalTime(), std::chrono::milliseconds(4));
    ASSERT_EQ(b.averageTime(), std::chrono::microseconds(2500));
    ASSERT_EQ(b.medianTime(), std::chrono::milliseconds(3));
    ASSERT_EQ(b.totalIterations(), 4);
    ASSERT_EQ(b.totalTimeRun(), std::chrono::milliseconds(10));
}

TEST(Main, StdDeviation)
{
    Benchmark b(bs);
    
    b.run([](benchmark::detail::RunState &){
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    });
    
    std::cout << "Test stddev: ";
    b.printTime(b.standardDeviation());
    std::cout << std::endl;
}

TEST(Main, DoNothing)
{
    Benchmark b(bs);

    b.run([](benchmark::detail::RunState &){
        // do nothing
    });

    ASSERT_LE(b.averageTime(), std::chrono::nanoseconds(1));
    ASSERT_LE(b.medianTime(), std::chrono::nanoseconds(1));
    ASSERT_LE(b.minimalTime(), std::chrono::nanoseconds(1));
    ASSERT_LE(b.maximalTime(), std::chrono::nanoseconds(1));
    ASSERT_GT(b.totalIterations(), 1);
    ASSERT_GT(b.repeats(), 1);
}

int main(int argc, char **argv)
{
    bs.outputStyle = BenchmarkSetup::Nothing;
    bs.verbose = true;

    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
