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

            if (sample < _minimum)
                _minimum = sample;
            if (sample > _maximum)
                _maximum = sample;

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

        // median
        std::sort(_samples.begin(), _samples.end());
        if (_samples.size() % 2 == 1) {
            _median = _samples[_samples.size() / 2];
        } else {
            auto j = _samples.size() / 2;
            _median = _samples[j-1] / 2 + _samples[j] / 2;
        }
    }

    bool removeOutliers() {
        if (_samples.size() < 3)
            return false;

        bool removed = false;
        auto outlierThreshold = _average + _stdDev * 2.0f;
        for (size_t i = 0; i < _samples.size();) {
            auto sample = _samples[i];
            if (sample > outlierThreshold) {
                _samples[i] = _samples.back();
                _samples.pop_back();
                removed = true;
            } else {
                i++;
            }
        }
        return removed;
    }

public:
    TimeStatistics():
        _totalSum(0)
        , _average(0)
        , _median(0)
        , _minimum(0)
        , _maximum(0)
        , _stdDev(0) {
        _samples.reserve(256);
    }

    void addSample(benchmark::duration_t sample) {
        _samples.push_back(sample);
    }

    void clear() {
        _samples.clear();
    }

    bool calculate() {
        if (_samples.empty())
            return false;

        calculateStats();
        if (removeOutliers()) {
            calculateStats();
        }
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
        size_t idx = (size_t)(_samples.size() * ((float)nth / 100.0f)) - 1;
        if (idx < 0)
            idx = 0;
        return _samples[idx];
    }

    benchmark::duration_t standardDeviation() const {
        return _stdDev;
    }

    bool highDeviation() const {
        return _stdDev > (_average / 4);
    }

    double standardDeviationLevel() const {
        return (double)_stdDev.count() / (double)_average.count();
    }
};

} // namespace benchmark
