#pragma once
#include <string>
#include <cstdio>
#include <cstring>

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