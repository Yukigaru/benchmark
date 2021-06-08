#pragma once
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <limits>
#include <vector>
#include "config.h"

namespace benchmark {

class TimeStatistics {
private:
    std::vector<benchmark::duration_t> _samples;
    benchmark::duration_t _totalSum;
    benchmark::duration_t _average;
    benchmark::duration_t _median;
    benchmark::duration_t _minimum;
    benchmark::duration_t _maximum;
    benchmark::duration_t _stdDev;
    size_t _outlierCount;

private:
    static benchmark::duration_t midpoint(benchmark::duration_t lower,
                                          benchmark::duration_t upper) {
        using rep = benchmark::duration_t::rep;

        const rep lowerTicks = lower.count();
        const rep upperTicks = upper.count();
        if ((lowerTicks < 0) == (upperTicks < 0))
            return benchmark::duration_t(lowerTicks + (upperTicks - lowerTicks) / 2);

        const rep sum = lowerTicks + upperTicks;
        const rep roundedDown = sum < 0 && sum % 2 != 0 ? sum / 2 - 1 : sum / 2;
        return benchmark::duration_t(roundedDown);
    }

private:
    void calculateStats() {
        using rep = benchmark::duration_t::rep;

        _totalSum = benchmark::duration_t(0);
        _maximum = _minimum = _samples[0];

        long double total = 0.0L;
        long double mean = 0.0L;
        long double sumOfSquares = 0.0L;
        size_t count = 0;
        for (auto sample : _samples) {
            const rep valueTicks = sample.count();
            total += static_cast<long double>(valueTicks);

            _minimum = std::min(_minimum, sample);
            _maximum = std::max(_maximum, sample);

            const long double value = static_cast<long double>(valueTicks);
            const long double delta = value - mean;
            mean += delta / static_cast<long double>(++count);
            sumOfSquares += delta * (value - mean);
        }

        const long double minDuration = static_cast<long double>(std::numeric_limits<rep>::min());
        const long double maxDuration = static_cast<long double>(std::numeric_limits<rep>::max());
        const rep totalTicks = total <= minDuration
            ? std::numeric_limits<rep>::min()
            : total >= maxDuration
                ? std::numeric_limits<rep>::max()
                : static_cast<rep>(total);
        _totalSum = benchmark::duration_t(totalTicks);

        const rep averageTicks = mean <= minDuration
            ? std::numeric_limits<rep>::min()
            : mean >= maxDuration
                ? std::numeric_limits<rep>::max()
                : static_cast<rep>(mean);
        _average = benchmark::duration_t(averageTicks);

        const long double stdDev = std::sqrt(sumOfSquares / static_cast<long double>(count));
        const auto stdDevTicks = stdDev >= maxDuration
            ? std::numeric_limits<rep>::max()
            : static_cast<rep>(stdDev + 0.5L);
        _stdDev = benchmark::duration_t(stdDevTicks);

        _outlierCount = 0;
        if (_samples.size() >= 3 && stdDev > 0.0L) {
            const long double outlierDistance = stdDev * 2.0L;
            for (auto sample : _samples) {
                const long double distanceAboveMean =
                    static_cast<long double>(sample.count()) - mean;
                if (distanceAboveMean > outlierDistance)
                    ++_outlierCount;
            }
        }

        // median
        std::sort(_samples.begin(), _samples.end());
        if (_samples.size() % 2 == 1) {
            _median = _samples[_samples.size() / 2];
        } else {
            auto j = _samples.size() / 2;
            _median = midpoint(_samples[j - 1], _samples[j]);
        }
    }

public:
    TimeStatistics():
        _totalSum(0)
        , _average(0)
        , _median(0)
        , _minimum(0)
        , _maximum(0)
        , _stdDev(0)
        , _outlierCount(0) {
        _samples.reserve(256);
    }

    void addSample(benchmark::duration_t sample) {
        _samples.push_back(sample);
    }

    void clear() {
        _samples.clear();
        _totalSum = benchmark::duration_t(0);
        _average = benchmark::duration_t(0);
        _median = benchmark::duration_t(0);
        _minimum = benchmark::duration_t(0);
        _maximum = benchmark::duration_t(0);
        _stdDev = benchmark::duration_t(0);
        _outlierCount = 0;
    }

    bool calculate() {
        if (_samples.empty())
            return false;

        calculateStats();
        return true;
    }
    
    size_t size() const {
        return _samples.size();
    }

    bool empty() const {
        return _samples.empty();
    }

    benchmark::duration_t totalTimeRun() const {
        return _totalSum;
    }

    benchmark::duration_t averageTime() const {
        return _average;
    }

    benchmark::duration_t medianTime() const {
        return _median;
    }

    benchmark::duration_t minimalTime() const {
        return _minimum;
    }

    benchmark::duration_t maximalTime() const {
        return _maximum;
    }

    benchmark::duration_t percentile(int nth) const {
        if (_samples.empty())
            return benchmark::duration_t(0);

        const size_t percentile = static_cast<size_t>(std::clamp(nth, 0, 100));
        if (percentile == 0)
            return _samples.front();

        const size_t rank = (_samples.size() / 100) * percentile
            + ((_samples.size() % 100) * percentile + 99) / 100;
        return _samples[rank - 1];
    }

    benchmark::duration_t standardDeviation() const {
        return _stdDev;
    }

    size_t outlierCount() const {
        return _outlierCount;
    }

    bool highDeviation() const {
        return _stdDev > (_average / 4);
    }

    double standardDeviationLevel() const {
        if (_average == benchmark::duration_t(0))
            return 0.0;
        return (double)_stdDev.count() / (double)_average.count();
    }
};

} // namespace benchmark
