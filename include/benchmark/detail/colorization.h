#pragma once

namespace benchmark {
    namespace detail {
        using ColorTag = const char *;

#if !defined(_WIN32)
        inline constexpr ColorTag ColorLightGreen = "\x1B[92m";
        inline constexpr ColorTag ColorLightYellow = "\x1B[33m";
        inline constexpr ColorTag ColorLightRed = "\x1B[91m";
        inline constexpr ColorTag ColorRed = "\x1B[31m";
        inline constexpr ColorTag ColorReset = "\x1B[0m";
#else
        inline constexpr ColorTag ColorLightGreen = "";
        inline constexpr ColorTag ColorLightYellow = "";
        inline constexpr ColorTag ColorLightRed = "";
        inline constexpr ColorTag ColorRed = "";
        inline constexpr ColorTag ColorReset = "";
#endif

        inline ColorTag selectColorForCPULoad(float relValue) { // [0.0, 1.0]
            if (relValue > 0.6f) {
                return ColorLightRed;
            } else if (relValue > 0.2f) {
                return ColorLightYellow;
            }
            return ColorLightGreen;
        }

        inline ColorTag selectColorForCPUFreq(float relValue) { // [0.0, 1.0]
            if (relValue < 0.6f) {
                return ColorLightRed;
            } else if (relValue < 0.8f) {
                return ColorLightYellow;
            }
            return ColorLightGreen;
        }
    }
}
