#pragma once
#include <vector>

class TimeStatistics {
public:
    using duration_t = std::chrono::steady_clock::duration;

private:
    std::vector<duration_t> _samples;
    bool _significant;

    duration_t _totalTimeRun;
    duration_t _averageTime;
    duration_t _medianTime;
    duration_t _minimalTime;
    duration_t _maximalTime;
    duration_t _standardDeviation;
    unsigned _repeats;

public:
    TimeStatistics():
        _totalTimeRun(0)
        , _averageTime(0)
        , _medianTime(0)
        , _minimalTime(0)
        , _maximalTime(0)
        , _standardDeviation(0)
        , _repeats(1) {
        _samples.reserve(512);
    }

    void setRepeats(unsigned repeats) {
        _repeats = repeats;
    }

    void addSample(duration_t sample) {
        _samples.push_back(sample);
    }

    void clear() {
	_samples.clear();
    }

    bool calculate() {
        if (_samples.empty())
            return false;

	_totalTimeRun = std::chrono::steady_clock::duration(0);
        _averageTime = std::chrono::steady_clock::duration(0);
        _maximalTime = _minimalTime = _samples[0];

        // minimum and average
        for (size_t i = 0; i < _samples.size(); i++) {
            auto sample = _samples[i];

            _averageTime += sample; // TODO: can it overflow?
            _totalTimeRun += sample;

            if (sample < _minimalTime)
                _minimalTime = sample;
            if (sample > _maximalTime)
                _maximalTime = sample;
        }

        _minimalTime /= _repeats;
        _maximalTime /= _repeats;
        _averageTime /= (_samples.size() * _repeats);

        // standard deviation
        auto averageNs = std::chrono::duration_cast<std::chrono::nanoseconds>(_averageTime).count();
        unsigned long long sumOfSquares = 0;

        for (size_t i = 0; i < _samples.size(); i++) {
            auto sampleNs = _samples[i].count() / _repeats;

            auto d = (sampleNs > averageNs) ? (sampleNs - averageNs) : (averageNs - sampleNs);
            sumOfSquares += d * d;
        }
        sumOfSquares /= _samples.size();
        long long stdDevNs = llround(sqrtl((double)sumOfSquares));
        _standardDeviation = std::chrono::nanoseconds(stdDevNs);

        // median
        std::sort(_samples.begin(), _samples.end());

        _medianTime = _samples[_samples.size() / 2] / _repeats;

        return true;
    }
    
    bool significant() const {
        return _significant;
    }
    
    size_t size() const {
        return _samples.size();
    }

    unsigned repeats() const {
        return _repeats;
    }
    
    duration_t totalTimeRun() const {
        return _totalTimeRun;
    }

    duration_t averageTime() const {
        return _averageTime;
    }

    duration_t medianTime() const {
        return _medianTime;
    }

    duration_t minimalTime() const {
        return _minimalTime;
    }

    duration_t maximalTime() const {
        return _maximalTime;
    }

    duration_t standardDeviation() const {
        return _standardDeviation;
    }
};
