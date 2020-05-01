#pragma once
#include <string>
#include <cstdio>
#include <cstring>
#include <memory>
#include <fstream>
#include <sstream>

#ifndef WIN32
#include <sched.h>
#endif

namespace benchmark {
namespace detail {

struct CoreFrequency {
    int curFreq;
    int maxFreq;
};

static int getCPUCoresNum() {
    int cpuCores = 0;

#ifdef WIN32
    return 1;
#else
    // open '/proc/cpuinfo' and count 'processor' word
    FILE *fh = std::fopen("/proc/cpuinfo", "rb");
    if (!fh)
        return 1;

    static const int BufSize = 1000;
    char *buf = (char *)std::malloc(BufSize);

    while (true) {
        if (!std::fgets(buf, BufSize, fh))
            break;

        if (std::strstr(buf, "processor")) {
            cpuCores++;
        }
    }
    std::fclose(fh);
    std::free(buf);

    if (cpuCores == 0)
        return 1;

    return cpuCores;
#endif
}

static std::string getFileText(const std::string &filePath) {
    static const int BufSize = 256;
    char buf[BufSize];

    FILE *fh = std::fopen(filePath.c_str(), "r");
    if (!fh) {
        std::cerr << "Couldn't open '" << filePath << "'\n";
        return "";
    }

    char *fresult = std::fgets(buf, BufSize, fh);
    if (!fresult) {
        std::fclose(fh);
        std::cerr << "Couldn't read from '" << filePath << "'\n";
        return "";
    }
    std::fclose(fh);

    return std::string(buf);
}

static bool isCPUScalingEnabled() {
#ifdef WIN32
    return false;
#else
    int coresNum = getCPUCoresNum();

    for (int i = 0; i < coresNum; i++) {
        std::string governorPath = "/sys/devices/system/cpu/cpu";
        governorPath += std::to_string(i);
        governorPath += "/cpufreq/scaling_governor";

        std::string governorText = getFileText(governorPath);
        if (governorText != "performance\n" &&
            governorText != "performance") {
            return true;
        }
    }

    return false;
#endif
}

static std::vector<CoreFrequency> readCPUFreqs() {
#ifdef WIN32
    return std::vector<CoreFrequency>();
#else
    int coresNum = getCPUCoresNum();
    std::vector<CoreFrequency> result;

    for (int i = 0; i < coresNum; i++) {
        std::string cpuPath = "/sys/devices/system/cpu/cpu";
        cpuPath += std::to_string(i);
        cpuPath += "/cpufreq/";

        std::string curFreqText = getFileText(cpuPath + "scaling_cur_freq");
        std::string maxFreqText = getFileText(cpuPath + "cpuinfo_max_freq");

        result.push_back({std::atoi(curFreqText.c_str()), std::atoi(maxFreqText.c_str())});
    }

    return result;
#endif
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
    size_t timeSample[NumStates];

    size_t idleTime() const {
        return timeSample[StateIdle] + timeSample[StateIOWait];
    }

    size_t loadTime() const {
        size_t result = 0;
        for (int i = 0; i < NumStates; i++) {
            result += timeSample[i];
        }
        return result - idleTime();
    }
};

struct CPUStats {
    std::vector<CPUCoreStats> statsByCore;
};

static std::unique_ptr<CPUStats> readCPUStats()
{
    std::ifstream ifs("/proc/stat");

    std::unique_ptr<CPUStats> result{ new CPUStats{} };

    std::string line;
    while (std::getline(ifs, line)) {
        if (!line.compare(0, 3, "cpu")) {
            std::istringstream ss(line);
            std::string cpuLabel;
            ss >> cpuLabel;

            if (cpuLabel == "cpu") // total stats, skip
                continue;

            result->statsByCore.push_back({});
            CPUCoreStats &coreStats = result->statsByCore.back();

            for (size_t i = 0; i < NumStates; ++i)
                ss >> coreStats.timeSample[i];
        }
    }

    return result;
}

struct CPULoadResult {
    int numCores;
    std::vector<float> loadByCore;
    std::vector<CoreFrequency> freqByCore;
};

static std::unique_ptr<CPULoadResult> getCPULoad() {
    int cores = getCPUCoresNum();

    std::unique_ptr<CPUStats> s1 = readCPUStats();
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    std::unique_ptr<CPUStats> s2 = readCPUStats();

    std::vector<CoreFrequency> freqs = readCPUFreqs();

    std::unique_ptr<CPULoadResult> result{ new CPULoadResult{} };
    result->numCores = cores;

    for (int i = 0; i < cores; i++) {
        size_t loadTimeD = s2->statsByCore[i].loadTime() - s1->statsByCore[i].loadTime();
        size_t idleTimeD = s2->statsByCore[i].idleTime() - s1->statsByCore[i].idleTime();
        size_t totalTimeD = loadTimeD + idleTimeD;
        result->loadByCore.push_back((float)loadTimeD / (float)totalTimeD);

        int curFreq = freqs.empty() ? 0 : freqs[i].curFreq;
        int maxFreq = freqs.empty() ? 0 : freqs[i].maxFreq;
        result->freqByCore.push_back({curFreq, maxFreq});
    }

    return result;
}

}} //namespaces