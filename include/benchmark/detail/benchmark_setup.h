#pragma once
#include <iostream>
#include "config.h"
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
        skipWarmup(false)
    {
    }

    BenchmarkSetup(int argc, const char **argv):
        BenchmarkSetup()
    {
        benchmark::detail::ProgramArguments args(argc, argv);

        std::string outputStyle_ = args.after("output");
        if (outputStyle_ == "full") {
            outputStyle = OutputStyle::Full;
        } else if (outputStyle_ == "oneline") {
            outputStyle = OutputStyle::OneLine;
        } else if (outputStyle_ == "nothing") {
            outputStyle = OutputStyle::Nothing;
        } else {
            std::cerr << "Unexpected value of 'output' argument: " << outputStyle_ << std::endl;
        }

        verbose = args.contains("verbose");
        skipWarmup = args.contains("skipWarmup");
    }

    OutputStyle outputStyle;
    bool verbose;
    bool skipWarmup;
};
} // namespace benchmark
