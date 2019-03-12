#pragma once
#include <chrono>
#include <vector>
#include "config.h"

namespace benchmark {
    using duration_t = std::chrono::steady_clock::duration;
    using time_point_t = std::chrono::steady_clock::time_point;

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
        };

        bool isPowerOf2(int number) {
            if (number < 1)
                return false;
            while (number > 1) {
                if (number & 0x1 && number != 1)
                    return false;
                number >>= 1;
            }
            return true;
        }

        GrowthType findGrowthType(int from, int to) {
            if (isPowerOf2(from) && isPowerOf2(to)) {
                return GrowthType::Exponential2;
            } else if (from % 10 == 0 && to % 10 == 0) {
                return GrowthType::Exponential10;
            }
            return GrowthType::Linear;
        }

        class BenchmarkState {
            std::vector<VariableArgument> _variableArguments;
            bool _needRestart;

        public:
            BenchmarkState() :_needRestart(false) {
            }

            bool addArgument(int from, int to) {
                auto i = std::find_if(_variableArguments.begin(), _variableArguments.end(), [=](const VariableArgument& arg){ return arg.range.first == from && arg.range.second == to; });
                if (i == _variableArguments.end()) {
                    GrowthType growthType = findGrowthType(from, to);
                    bool growing = to > from;
                    _variableArguments.emplace_back(VariableArgument{{from, to}, growthType, growing});
                    _needRestart = true;
                    return true;
                }
                _needRestart = false;
                return false;
            }

            bool needRestart() const {
                return _needRestart;
            }
        };

        class RunState {
            time_point_t _start{};
            time_point_t _end{duration_t::max()};
            bool _ended{false};
            unsigned _repeats;
            BenchmarkState &_bstate;

        public:
            RunState(BenchmarkState &bstate, unsigned repeats): _repeats(repeats), _bstate(bstate)
            {
            }

            // returns if the argument has been activated and the benchmark should restart
            bool addArgument(int from, int to) {
                return _bstate.addArgument(from, to);
            }

            BENCHMARK_ALWAYS_INLINE void start() {
                _start = std::chrono::steady_clock::now();
            }

            BENCHMARK_ALWAYS_INLINE void stop() {
                if (!_ended) {
                    _ended = true;
                    _end = std::chrono::steady_clock::now();
                }
            }

            std::chrono::steady_clock::duration getSample() const {
                auto sample = (_end - _start);
                if (sample < std::chrono::nanoseconds(1))
                    sample = std::chrono::nanoseconds(1);
                return sample;
            }

            BENCHMARK_ALWAYS_INLINE unsigned repeats() const {
                return _repeats;
            }

            BENCHMARK_ALWAYS_INLINE int arg1() const {
                return 0; // TODO: Implement
            }
        };
    }
}