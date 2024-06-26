#pragma once
#include <chrono>
#include <vector>
#include "config.h"

namespace benchmark {
    namespace detail {
        enum GrowthType {
            Linear,
            Exponential2,
            Exponential10
        };

        using ArgumentRange = std::pair<int, int>;

        struct VariableArgument {
            ArgumentRange range;
            GrowthType growth;
            bool growing;
            int value;
        };

        inline bool isPowerOf2(int number) {
            if (number < 1)
                return false;
            while (number > 1) {
                if (number & 0x1 && number != 1)
                    return false;
                number >>= 1;
            }
            return true;
        }

        inline GrowthType findGrowthType(int from, int to) {
            if (isPowerOf2(from) && isPowerOf2(to)) {
                return GrowthType::Exponential2;
            } else if (from % 10 == 0 && to % 10 == 0) {
                return GrowthType::Exponential10;
            }
            return GrowthType::Linear;
        }

        class BenchmarkState {
            bool _firstTime;

            std::vector<VariableArgument> _variableArgs;
            int _currentArg1;
            bool _variablesDone;

            bool _needRestart;

        public:
            BenchmarkState() :_firstTime(true), _needRestart(false), _currentArg1(0), _variablesDone(true) {
            }

            bool addArgument(int from, int to) {
                auto i = std::find_if(_variableArgs.begin(), _variableArgs.end(),
                    [=](const VariableArgument& arg) {
                        return arg.range.first == from && arg.range.second == to;
                    });

                if (i == _variableArgs.end()) {
                    GrowthType growthType = findGrowthType(from, to);
                    bool growing = to > from;
                    _variableArgs.emplace_back(VariableArgument{{from, to}, growthType, growing, from});
                    _variablesDone = false;
                    _needRestart = true;
                    return true;
                }
                _needRestart = false;
                return false;
            }

            void pickNextArgument() {
                if (_variableArgs.empty()) {
                    return;
                }

                VariableArgument &varg = _variableArgs[0];

                _currentArg1 = varg.value;

                switch (varg.growth) {
                    case Linear: {
                        varg.value += varg.growing ? 1 : -1;
                        break;
                    }
                    case Exponential2: {
                        if (varg.growing) {
                            varg.value *= 2;
                        } else {
                            varg.value /= 2;
                        }
                        break;
                    }
                    case Exponential10: {
                        if (varg.growing) {
                            varg.value *= 10;
                        } else {
                            varg.value /= 10;
                        }
                        break;
                    }
                }

                if ((varg.growing && varg.value > varg.range.second) ||
                    (!varg.growing && varg.value < varg.range.second)) {
                    _variablesDone = true;
                }
            }

            bool running() {
                if (_variableArgs.empty()) {
                    // without variable arguments we just run only once
                    bool result = _firstTime;
                    _firstTime = false;
                    return result;
                }

                return !_variablesDone;
            }

            bool variableArgsMode() const {
                return !_variableArgs.empty();
            }

            BENCHMARK_ALWAYS_INLINE int getArg() {
                return _currentArg1;
            }

            bool needRestart() const {
                return _needRestart;
            }
        };

        class RunState {
            time_point_t _start{};
            time_point_t _end{duration_t::max()};
            duration_t _duration{0};

            duration_t _noopTime;

            bool _ended{false};
            BenchmarkState &_bstate;

        public:
            RunState(BenchmarkState &bstate, duration_t noopTime):
                _bstate(bstate),
                _noopTime(noopTime)
            {
            }

            // returns if the argument has been activated and the benchmark should restart
            bool addArgument(int from, int to) {
                return _bstate.addArgument(from, to);
            }

            BENCHMARK_ALWAYS_INLINE void start() {
                _ended = false;
                _start = clock_t::now();
            }

            BENCHMARK_ALWAYS_INLINE void stop() {
                if (!_ended) {
                    _end = clock_t::now();

                    _duration += (_end - _start);
                    _ended = true;
                }
            }

            duration_t getSample() const {
                auto sample = _duration;
                if (_noopTime > std::chrono::nanoseconds(0)) {
                    if (_noopTime < sample) {
                        sample -= _noopTime;
                    } else {
                        sample = std::chrono::nanoseconds(0);
                    }
                }
                return sample;
            }

            BENCHMARK_ALWAYS_INLINE int arg1() const {
                return _bstate.getArg();
            }
        };
    }
}
