#pragma once
#include <chrono>
#include "config.h"

namespace benchmark {
    using duration_t = std::chrono::steady_clock::duration;
    using time_point_t = std::chrono::steady_clock::time_point;

    namespace detail {
        class SampleTimer {
            time_point_t _start{0};
            time_point_t _end{duration_t::max()};
            bool _ended{false};
            unsigned _repeats;

        public:
            SampleTimer(unsigned repeats): _repeats(repeats)
            {
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
        };
    }
}