#pragma once
#include <chrono>
#include <iosfwd>
#include "colorization.h"
#include "cpu_info.h"

namespace benchmark {
using duration_t = std::chrono::steady_clock::duration;

namespace io {

struct ColoredDuration {
    benchmark::duration_t duration;
    benchmark::detail::ColorTag colorTag = benchmark::detail::ColorLightGreen;

    ColoredDuration(benchmark::duration_t duration_, benchmark::detail::ColorTag colorTag_):
        duration(duration_),
        colorTag(colorTag_) {
    }
};

struct Iterations {
    unsigned iterations;
};
}
}

inline std::ostream& operator <<(std::ostream &os, benchmark::duration_t duration) {
    auto durationNs = std::chrono::duration_cast<std::chrono::nanoseconds>(duration).count();
    auto durationMcs = std::chrono::duration_cast<std::chrono::microseconds>(duration).count();
    auto durationMs = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
    auto durationSec = std::chrono::duration_cast<std::chrono::seconds>(duration).count();
    auto durationMin = std::chrono::duration_cast<std::chrono::minutes>(duration).count();

    auto oldPrecision = os.precision();

    if (durationNs < 1000) {
        os << durationNs << " ns";
    } else if (durationMcs < 10) {
        os << std::setprecision(2) << (float) durationNs / 1000.0f << " μs";
    } else if (durationMcs < 1000) {
        os << durationMcs << " μs";
    } else if (durationMs < 10) {
        os << std::setprecision(2) << (float) durationMcs / 1000.0f << " ms";
    } else if (durationMs < 1000) {
        os << durationMs << " ms";
    } else if (durationSec < 10) {
        os << std::setprecision(2) << (float) durationMs / 1000.0f << " sec";
    } else if (durationMin < 1) {
        os << durationSec << " sec";
    } else {
        os << std::setprecision(2) << (float) durationSec / 60.0f << " min";
    }
    os << benchmark::detail::ColorReset << std::setprecision(oldPrecision);
    return os;
}

inline std::ostream& operator <<(std::ostream &os, benchmark::io::ColoredDuration v) {
    os << v.colorTag << v.duration << benchmark::detail::ColorReset;
    return os;
}

inline std::ostream& operator <<(std::ostream &os, benchmark::io::Iterations v) {
    auto oldPrecision = os.precision();

    if (v.iterations < 1000) {
        os << v.iterations;
    } else if (v.iterations < 1000000) { // thousands
        if (v.iterations % 1000 < 100) {
            os << v.iterations / 1000 << "k";
        } else {
            os << std::setprecision(1) << (float) v.iterations / 1000.0f << "k";
        }
    } else { // millions
        os << std::setprecision(1) << (float) v.iterations / 1000000.0f << "m";
    }
    os << std::setprecision(oldPrecision);
    return os;
}

inline std::ostream& operator <<(std::ostream &os, std::unique_ptr<benchmark::detail::CPULoadResult> &cpuLoad) {
    for (int i = 0; i < cpuLoad->numCores; i++) {
        float loadRel = cpuLoad->loadByCore[i];

        benchmark::detail::ColorTag color = benchmark::detail::selectColorForCPULoad(loadRel);
        os << "[Core " << i << ": " << color << (int) (loadRel * 100.0f) << "%"
                  << benchmark::detail::ColorReset << "] ";

        if (i % 4 == 0 && i > 0) // split by a column
            os << "\n";
    }
    os << "\n";
    for (int i = 0; i < cpuLoad->numCores; i++) {
        int curFreq = cpuLoad->freqByCore[i].curFreq;
        int maxFreq = cpuLoad->freqByCore[i].maxFreq;

        float freqRel = 0.0f;
        if (curFreq > 0 && maxFreq > 0)
            freqRel = (float) curFreq / (float) maxFreq;

        benchmark::detail::ColorTag color = benchmark::detail::selectColorForCPUFreq(freqRel);
        os << "[Freq " << i << ": " << color << (int) (freqRel * 100.0f) << "%"
                  << benchmark::detail::ColorReset << "] ";

        if (i % 4 == 0 && i > 0) // split by a column
            os << "\n";
    }
    return os;
}