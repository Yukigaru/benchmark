#pragma once
#include <string>
#include <vector>

class ProgramArguments {
    std::vector<std::string> args;
    std::string modulePath;

public:
    ProgramArguments(int argc, const char **argv)
    {
        if (argc < 1)
            return;

        modulePath = argv[0];

        args.reserve((size_t)argc - 1);
        for (int i = 1; i < argc; i++) {
            std::string arg = argv[i];
            if (arg.empty())
                continue;

            if (arg[0] == '-') {
                if (arg.size() > 1 && arg[1] == '-') {
                    arg = arg.substr(2);
                } else {
                    arg = arg.substr(1);
                }
            }

            args.push_back(std::move(arg));
        }
    }

    bool contains(const char *argName, const char *argAltName = nullptr) const
    {
        for (auto i = std::begin(args), ie = std::end(args); i != ie; ++i) {
            if (*i == argName || (argAltName && *i == argAltName))
                return true;
        }
        return false;
    }

    // returns argument that goes after the given, or an empty string
    std::string after(const char *argName, const char *argAltName = nullptr) const
    {
        std::string result;
        for (auto i = std::begin(args), ie = std::end(args); i != ie; ++i) {
            if (*i == argName || (argAltName && *i == argAltName)) {
                if (i != ie - 1) // if it's not the last argument
                    result = *(i + 1);
                break;
            }
        }
        return result;
    }

    const std::string &operator[](size_t ind) const
    {
        return args[ind];
    }

    size_t count() const
    {
        return args.size();
    }

    bool hasAny() const
    {
        return !args.empty();
    }

    const std::string &getModulePath() const
    { // UTF-8
        return modulePath;
    }
};