#pragma once
#include <cstddef>
#include <string>
#include <utility>
#include <vector>

namespace benchmark {
namespace detail {

class ProgramArguments {
    std::vector<std::string> args;
    std::string modulePath;

public:
    ProgramArguments(int argc, const char *const *argv)
    {
        if (argc < 1)
            return;

        modulePath = argv[0];

        args.reserve(static_cast<std::size_t>(argc - 1));
        for (int i = 1; i < argc; i++) {
            std::string arg = argv[i];
            if (arg.empty())
                continue;

            args.push_back(std::move(arg));
        }
    }

    bool contains(const char *argName, const char *argAltName = nullptr) const
    {
        for (auto i = args.begin(), ie = args.end(); i != ie; ++i) {
            const std::string option = withoutPrefix(*i);
            const std::string name = option.substr(0, option.find('='));
            if (name == argName || (argAltName && name == argAltName))
                return true;
        }
        return false;
    }

    // returns argument that goes after the given, or an empty string
    std::string after(const char *argName, const char *argAltName = nullptr) const
    {
        std::string result;
        for (auto i = args.begin(), ie = args.end(); i != ie; ++i) {
            const std::string option = withoutPrefix(*i);
            const auto separator = option.find('=');
            const std::string name = option.substr(0, separator);
            if (name == argName || (argAltName && name == argAltName)) {
                if (separator != std::string::npos)
                    return option.substr(separator + 1);
                if (i != ie - 1) // if it's not the last argument
                    result = *(i + 1);
                break;
            }
        }
        return result;
    }

    const std::string &operator[](std::size_t ind) const
    {
        return args[ind];
    }

    std::size_t count() const
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

private:
    static std::string withoutPrefix(const std::string &arg)
    {
        if (arg.size() > 1 && arg[0] == '-' && arg[1] == '-')
            return arg.substr(2);
        if (!arg.empty() && arg[0] == '-')
            return arg.substr(1);
        return arg;
    }
};

} // namespace detail
} // namespace benchmark
