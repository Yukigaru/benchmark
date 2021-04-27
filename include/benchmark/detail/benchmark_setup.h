#pragma once
#include <chrono>
#include <exception>
#include <iostream>
#include <limits>
#include <string>
#include "program_arguments.h"


namespace benchmark {
struct BenchmarkSetup {
    enum class OutputStyle {
        OneLine,
        Full,
        Nothing
    };

    BenchmarkSetup():
        outputStyle(OutputStyle::OneLine),
        verbose(false),
        skipWarmup(false),
        adjustPriority(false),
        showCpuInfo(true),
        iterations(200),
        timeLimit(std::chrono::seconds(2)),
        warmupTime(std::chrono::seconds(1))
    {
    }

    BenchmarkSetup(int argc, const char *const *argv):
        BenchmarkSetup()
    {
        detail::ProgramArguments args(argc, argv);

        if (args.contains("output")) {
            const std::string value = args.after("output");
            if (value == "full") {
                outputStyle = OutputStyle::Full;
            } else if (value == "oneline") {
                outputStyle = OutputStyle::OneLine;
            } else if (value == "nothing") {
                outputStyle = OutputStyle::Nothing;
            } else {
                std::cerr << "Unexpected value of 'output' argument: " << value << std::endl;
            }
        }

        verbose = args.contains("verbose");
        skipWarmup = args.contains("skip-warmup", "skipWarmup");
        adjustPriority = args.contains("high-priority");
        showCpuInfo = !args.contains("no-cpu-info");

        iterations = readUnsigned(args, "iterations", iterations);
        timeLimit = std::chrono::milliseconds(
            readUnsigned(args, "time-limit-ms", static_cast<unsigned>(timeLimit.count())));
        warmupTime = std::chrono::milliseconds(
            readUnsigned(args, "warmup-ms", static_cast<unsigned>(warmupTime.count())));
    }

    OutputStyle outputStyle;
    bool verbose;
    bool skipWarmup;
    bool adjustPriority;
    bool showCpuInfo;
    unsigned iterations;
    std::chrono::milliseconds timeLimit;
    std::chrono::milliseconds warmupTime;

private:
    static unsigned readUnsigned(const detail::ProgramArguments &args, const char *name, unsigned fallback)
    {
        if (!args.contains(name))
            return fallback;

        const std::string value = args.after(name);
        try {
            std::size_t parsed = 0;
            const unsigned long number = std::stoul(value, &parsed);
            if (!value.empty() && value[0] != '-' && parsed == value.size() &&
                number <= std::numeric_limits<unsigned>::max())
                return static_cast<unsigned>(number);
        } catch (const std::exception &) {
        }

        std::cerr << "Unexpected value of '" << name << "' argument: " << value << std::endl;
        return fallback;
    }
};

} // namespace benchmark
