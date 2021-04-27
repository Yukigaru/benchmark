#pragma once

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <memory>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

#if defined(__APPLE__)
#include <mach/mach.h>
#include <mach/processor_info.h>
#include <sys/sysctl.h>
#endif

namespace benchmark {
namespace detail {

struct CoreFrequency {
    int curFreq;
    int maxFreq;
};

inline int getCPUCoresNum() {
#if defined(__APPLE__)
    int sysctlCores = 0;
    std::size_t size = sizeof(sysctlCores);
    if (sysctlbyname("hw.logicalcpu", &sysctlCores, &size, nullptr, 0) == 0 && sysctlCores > 0)
        return sysctlCores;
#endif

    const unsigned cores = std::thread::hardware_concurrency();
    return cores == 0 ? 1 : static_cast<int>(cores);
}

#if defined(__linux__)
inline std::string getFileText(const std::string &filePath) {
    std::ifstream stream(filePath.c_str());
    std::string text;
    std::getline(stream, text);
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
        if (!governor.empty() && governor != "performance")
            return true;
    }
#endif
    // Scaling state unknown
    return false;
}

#if defined(__APPLE__)
inline std::uint64_t readSysctlUInt64(const char *name) {
    std::uint64_t value = 0;
    std::size_t size = sizeof(value);
    return sysctlbyname(name, &value, &size, nullptr, 0) == 0 ? value : 0;
}
#endif

inline std::vector<CoreFrequency> readCPUFreqs() {
    const int coresNum = getCPUCoresNum();
    std::vector<CoreFrequency> result(static_cast<std::size_t>(coresNum), {0, 0});

#if defined(__linux__)
    for (int i = 0; i < coresNum; ++i) {
        const std::string cpuPath = "/sys/devices/system/cpu/cpu" +
                                    std::to_string(i) + "/cpufreq/";
        result[static_cast<std::size_t>(i)] = {
            std::atoi(getFileText(cpuPath + "scaling_cur_freq").c_str()),
            std::atoi(getFileText(cpuPath + "cpuinfo_max_freq").c_str())
        };
    }
#elif defined(__APPLE__)
    // Frequency may be unavailable on Apple Silicon
    const std::uint64_t currentHz = readSysctlUInt64("hw.cpufrequency");
    std::uint64_t maximumHz = readSysctlUInt64("hw.cpufrequency_max");
    if (maximumHz == 0)
        maximumHz = currentHz;

    const int currentKHz = static_cast<int>(currentHz / 1000);
    const int maximumKHz = static_cast<int>(maximumHz / 1000);
    std::fill(result.begin(), result.end(), CoreFrequency{currentKHz, maximumKHz});
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
        // guest time is already in user/nice
        for (int i = 0; i <= StateSteal; ++i)
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
#elif defined(__APPLE__)
    natural_t processorCount = 0;
    processor_info_array_t info = nullptr;
    mach_msg_type_number_t infoCount = 0;
    const host_t host = mach_host_self();
    const kern_return_t status = host_processor_info(
        host, PROCESSOR_CPU_LOAD_INFO, &processorCount, &info, &infoCount);
    mach_port_deallocate(mach_task_self(), host);
    if (status != KERN_SUCCESS || info == nullptr)
        return result;

    const processor_cpu_load_info_data_t *loads =
        reinterpret_cast<const processor_cpu_load_info_data_t *>(info);
    result->statsByCore.resize(processorCount);
    for (natural_t i = 0; i < processorCount; ++i) {
        CPUCoreStats &coreStats = result->statsByCore[i];
        coreStats.timeSample[StateUser] = loads[i].cpu_ticks[CPU_STATE_USER];
        coreStats.timeSample[StateNice] = loads[i].cpu_ticks[CPU_STATE_NICE];
        coreStats.timeSample[StateSystem] = loads[i].cpu_ticks[CPU_STATE_SYSTEM];
        coreStats.timeSample[StateIdle] = loads[i].cpu_ticks[CPU_STATE_IDLE];
    }

    vm_deallocate(mach_task_self(), reinterpret_cast<vm_address_t>(info),
                  infoCount * sizeof(integer_t));
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
        const std::uint64_t loadDelta = secondLoad >= firstLoad ? secondLoad - firstLoad : 0;
        const std::uint64_t idleDelta = secondIdle >= firstIdle ? secondIdle - firstIdle : 0;
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
