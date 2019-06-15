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

static bool isCPUScalingEnabled() {
#ifdef WIN32
    return false;
#else
    int coresNum = getCPUCoresNum();

    for (int i = 0; i < coresNum; i++) {
        std::string governorPath = "/sys/devices/system/cpu/cpu";
        governorPath += std::to_string(i);
        governorPath += "/cpufreq/scaling_governor";

        static const int BufSize = 256;
        char buf[BufSize];

        FILE *fh = std::fopen(governorPath.c_str(), "r");
        if (!fh) {
            fprintf(stderr, "Couldn't open '%s'\n", governorPath.c_str());
            return false;
        }
        char *fresult = std::fgets(buf, BufSize, fh);
        if (!fresult) {
            std::fclose(fh);
            fprintf(stderr, "Couldn't read from '%s'\n", governorPath.c_str());
            return false;
        }
        std::fclose(fh);

        if (std::strcmp(buf, "performance\n") != 0 &&
            std::strcmp(buf, "performance") != 0) {
            return true;
        }
    }

    return false;
#endif
}

static int currentCPUCore() {
#ifndef WIN32
    return sched_getcpu();
#else
    return -1;
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

            for (int i = 0; i < NumStates; ++i)
                ss >> coreStats.timeSample[i];
        }
    }

    return result;
}

struct CPULoadResult {
    std::vector<float> loadByCore;
};

static std::unique_ptr<CPULoadResult> getCPULoad() {
    int cores = getCPUCoresNum();

    std::unique_ptr<CPUStats> s1 = readCPUStats();
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    std::unique_ptr<CPUStats> s2 = readCPUStats();

    std::unique_ptr<CPULoadResult> result{ new CPULoadResult{} };

    for (int i = 0; i < cores; i++) {
        size_t loadTimeD = s2->statsByCore[i].loadTime() - s1->statsByCore[i].loadTime();
        size_t idleTimeD = s2->statsByCore[i].idleTime() - s1->statsByCore[i].idleTime();
        size_t totalTimeD = loadTimeD + idleTimeD;

        result->loadByCore.push_back((float)loadTimeD / (float)totalTimeD);
    }

    return result;
}

}} //namespaces