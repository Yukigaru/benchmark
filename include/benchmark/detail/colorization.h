#pragma once

namespace benchmark {
    namespace detail {
        using ColorTag = const char *;

#ifndef WIN32
        static ColorTag ColorLightGreen = "\x1B[92m";
        static ColorTag ColorLightYellow = "\x1B[33m";
        static ColorTag ColorLightRed = "\x1B[91m";
        static ColorTag ColorRed = "\x1B[31m";
        static ColorTag ColorReset = "\x1B[0m";
#else
        static ColorTag ColorLightGreen = "";
        static ColorTag ColorLightYellow = "";
        static ColorTag ColorLightGray = "";
        static ColorTag ColorRed = "";
        static ColorTag ColorReset = "";
#endif

        static ColorTag selectColorForCPULoad(float relValue) { // value is in [0.0, 1.0] range
            if (relValue > 0.6f) {
                return ColorLightRed;
            } else if (relValue > 0.2f) {
                return ColorLightYellow;
            }
            return ColorLightGreen;
        }
    }
}
