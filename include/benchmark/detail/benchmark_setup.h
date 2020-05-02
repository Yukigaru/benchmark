#pragma once
#include <iostream>
#include "config.h"
#include "program_arguments.h"

struct BenchmarkSetup {
    enum OutputStyle {
        Table,
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
        ProgramArguments args(argc, argv);

        std::string outputStyle_ = args.after("output");
        if (outputStyle_ == "full") {
            outputStyle = OutputStyle::Full;
        } else if (outputStyle_ == "oneline") {
            outputStyle = OutputStyle::OneLine;
        } else if (outputStyle_ == "table") {
            outputStyle = OutputStyle::Table;
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