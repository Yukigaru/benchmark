#pragma once
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

        std::string outputStyle = args.after("output");
        if (outputStyle == "full") {
            outputStyle = OutputStyle::Full;
        } else if (outputStyle == "oneline") {
            outputStyle = OutputStyle::OneLine;
        } else if (outputStyle == "table") {
            outputStyle = OutputStyle::Table;
        } else if (outputStyle == "nothing") {
            outputStyle = OutputStyle::Nothing;
        } else {
            fprintf(stderr, "Unexpected value of 'output' argument: %s\n", outputStyle.c_str());
        }

        verbose = args.contains("verbose");
        skipWarmup = args.contains("skipWarmup");
    }

    OutputStyle outputStyle;
    bool verbose;
    bool skipWarmup;
};