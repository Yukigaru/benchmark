#pragma once

namespace benchmark {
    namespace detail {
        using ColorTag = const char *;

#if !defined(_WIN32)
        static ColorTag ColorLightGreen = "\x1B[92m";
        static ColorTag ColorLightYellow = "\x1B[33m";
        static ColorTag ColorLightRed = "\x1B[91m";
        static ColorTag ColorRed = "\x1B[31m";
        static ColorTag ColorReset = "\x1B[0m";
#else
        static ColorTag ColorLightGreen = "";
        static ColorTag ColorLightYellow = "";
        static ColorTag ColorLightRed = "";
        static ColorTag ColorRed = "";
        static ColorTag ColorReset = "";
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
