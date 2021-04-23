#pragma once

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

namespace benchmark {
namespace detail {

struct CoreFrequency {
    int curFreq;
    int maxFreq;
};

inline int getCPUCoresNum() {
    const unsigned cores = std::thread::hardware_concurrency();
    return cores == 0 ? 1 : static_cast<int>(cores);
}

#if defined(__linux__)
inline std::string getFileText(const std::string &filePath) {
    std::ifstream stream(filePath.c_str());
    if (!stream) {
        std::cerr << "Couldn't open '" << filePath << "'\n";
        return "";
    }
    std::string text;
    std::getline(stream, text);
    if (!stream && text.empty())
        std::cerr << "Couldn't read from '" << filePath << "'\n";
    return text;
}
#endif

inline bool isCPUScalingEnabled() {
#if defined(__linux__)
    const int coresNum = getCPUCoresNum();
    for (int i = 0; i < coresNum; ++i) {
        const std::string governorPath = "/sys/devices/system/cpu/cpu" +
                                         std::to_string(i) +
                                         "/cpufreq/scaling_governor";
        const std::string governor = getFileText(governorPath);
        if (governor != "performance")
            return true;
    }
#endif
    return false;
}

inline std::vector<CoreFrequency> readCPUFreqs() {
    std::vector<CoreFrequency> result;

#if defined(__linux__)
    const int coresNum = getCPUCoresNum();
    result.reserve(static_cast<std::size_t>(coresNum));
    for (int i = 0; i < coresNum; ++i) {
        const std::string cpuPath = "/sys/devices/system/cpu/cpu" +
                                    std::to_string(i) + "/cpufreq/";
        result.push_back({
            std::atoi(getFileText(cpuPath + "scaling_cur_freq").c_str()),
            std::atoi(getFileText(cpuPath + "cpuinfo_max_freq").c_str())
        });
    }
#endif

    return result;
}

enum CPUStates
{
    StateUser,
    StateNice,
    StateSystem,
    StateIdle,
    StateIOWait,
    StateIRQ,
    StateSoftIRQ,
    StateSteal,
    StateGuest,
    StateGuestNice,
    NumStates
};

struct CPUCoreStats {
    std::uint64_t timeSample[NumStates];

    std::uint64_t idleTime() const {
        return timeSample[StateIdle] + timeSample[StateIOWait];
    }

    std::uint64_t loadTime() const {
        std::uint64_t result = 0;
        for (int i = 0; i < NumStates; ++i)
            result += timeSample[i];
        return result - idleTime();
    }
};

struct CPUStats {
    std::vector<CPUCoreStats> statsByCore;
};

inline std::unique_ptr<CPUStats> readCPUStats()
{
    std::unique_ptr<CPUStats> result(new CPUStats{});

#if defined(__linux__)
    std::ifstream stream("/proc/stat");
    std::string line;
    while (std::getline(stream, line)) {
        if (line.compare(0, 3, "cpu") != 0)
            continue;

        std::istringstream values(line);
        std::string cpuLabel;
        values >> cpuLabel;
        if (cpuLabel == "cpu")
            continue;
        if (cpuLabel.size() == 3 || cpuLabel.find_first_not_of("0123456789", 3) != std::string::npos)
            continue;

        result->statsByCore.push_back({});
        CPUCoreStats &coreStats = result->statsByCore.back();
        for (std::size_t i = 0; i < NumStates && values; ++i)
            values >> coreStats.timeSample[i];
    }
#endif

    return result;
}

struct CPULoadResult {
    int numCores;
    std::vector<float> loadByCore;
    std::vector<CoreFrequency> freqByCore;
};

inline std::unique_ptr<CPULoadResult> getCPULoad() {
    std::unique_ptr<CPUStats> first = readCPUStats();
    if (!first->statsByCore.empty())
        std::this_thread::sleep_for(std::chrono::milliseconds(300));
    std::unique_ptr<CPUStats> second = readCPUStats();

    const std::size_t cores = std::min(first->statsByCore.size(), second->statsByCore.size());
    std::vector<CoreFrequency> frequencies = readCPUFreqs();
    std::unique_ptr<CPULoadResult> result(new CPULoadResult{});
    result->numCores = static_cast<int>(cores);

    for (std::size_t i = 0; i < cores; ++i) {
        const std::uint64_t firstLoad = first->statsByCore[i].loadTime();
        const std::uint64_t secondLoad = second->statsByCore[i].loadTime();
        const std::uint64_t firstIdle = first->statsByCore[i].idleTime();
        const std::uint64_t secondIdle = second->statsByCore[i].idleTime();
        const std::uint64_t loadDelta = secondLoad - firstLoad;
        const std::uint64_t idleDelta = secondIdle - firstIdle;
        const std::uint64_t totalDelta = loadDelta + idleDelta;

        result->loadByCore.push_back(totalDelta == 0
            ? -1.0f
            : static_cast<float>(loadDelta) / static_cast<float>(totalDelta));
        result->freqByCore.push_back(i < frequencies.size()
            ? frequencies[i]
            : CoreFrequency{0, 0});
    }

    return result;
}

}} // namespaces
