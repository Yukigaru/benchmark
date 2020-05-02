#pragma once
#include <vector>
#include "chrono_utils.h"

class TimeStatistics {
private:
    std::vector<benchmark::duration_t> _samples;
    bool _significant;

    benchmark::duration_t _totalTimeRun;
    benchmark::duration_t _averageTime;
    benchmark::duration_t _medianTime;
    benchmark::duration_t _minimalTime;
    benchmark::duration_t _maximalTime;
    benchmark::duration_t _standardDeviation;

private:
    void calculateStats() {
        _totalTimeRun = std::chrono::steady_clock::duration(0);
        _averageTime = std::chrono::steady_clock::duration(0);
        _maximalTime = _minimalTime = _samples[0];

        // minimum and average
        for (auto sample : _samples) {
            _averageTime += sample;
            _totalTimeRun += sample;

            if (sample < _minimalTime)
                _minimalTime = sample;
            if (sample > _maximalTime)
                _maximalTime = sample;
        }

        _averageTime /= _samples.size();

        // standard deviation
        auto averageNs = std::chrono::duration_cast<std::chrono::nanoseconds>(_averageTime).count();
        unsigned long long sumOfSquares = 0;

        for (auto sample : _samples) {
            auto sampleNs = sample.count();
            auto d = (sampleNs > averageNs) ? (sampleNs - averageNs) : (averageNs - sampleNs);
            sumOfSquares += d * d;
        }
        sumOfSquares /= _samples.size();
        long long stdDevNs = llround(sqrt((double)sumOfSquares));
        _standardDeviation = std::chrono::nanoseconds(stdDevNs);

        // median
        std::sort(_samples.begin(), _samples.end());
        if (_samples.size() % 2 == 1) {
            _medianTime = _samples[_samples.size() / 2];
        } else {
            auto j = _samples.size() / 2;
            auto ns = std::chrono::nanoseconds::rep(
                    (double)std::chrono::duration_cast<std::chrono::nanoseconds>(_samples[j-1]).count() / 2.0 +
                    (double)std::chrono::duration_cast<std::chrono::nanoseconds>(_samples[j]).count() / 2.0);
            _medianTime = std::chrono::nanoseconds(ns);
        }
    }

    bool removeOutliers() {
        if (_samples.size() < 3)
            return false;

        bool removed = false;
        auto outlierThreshold = _averageTime + _standardDeviation * 2.0f;
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
        _totalTimeRun(0)
        , _averageTime(0)
        , _medianTime(0)
        , _minimalTime(0)
        , _maximalTime(0)
        , _standardDeviation(0) {
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
    
    bool significant() const {
        return _significant;
    }
    
    size_t size() const {
        return _samples.size();
    }

    bool empty() const {
        return _samples.empty();
    }

    benchmark::duration_t totalTimeRun() const {
        return _totalTimeRun;
    }

    benchmark::duration_t averageTime() const {
        return _averageTime;
    }

    benchmark::duration_t medianTime() const {
        return _medianTime;
    }

    benchmark::duration_t minimalTime() const {
        return _minimalTime;
    }

    benchmark::duration_t maximalTime() const {
        return _maximalTime;
    }

    benchmark::duration_t percentile(int nth) const {
        size_t idx = (size_t)(_samples.size() * ((float)nth / 100.0f)) - 1;
        if (idx < 0)
            idx = 0;
        return _samples[idx];
    }

    benchmark::duration_t standardDeviation() const {
        return _standardDeviation;
    }

    bool highDeviation() const {
        return _standardDeviation > (_averageTime / 4);
    }

    double standardDeviationLevel() const {
        return (double)_standardDeviation.count() / (double)_averageTime.count();
    }
};
