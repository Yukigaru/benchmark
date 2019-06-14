#pragma once

using ColorTag = const char *;

#ifndef WIN32
static ColorTag ColorLightGreen = "\x1B[92m";
static ColorTag ColorLightGray = "\x1B[37m";
static ColorTag ColorRed = "\x1B[31m";
static ColorTag ColorReset = "\x1B[0m";
#else
static ColorTag ColorLightGreen = "";
static ColorTag ColorLightGray = "";
static ColorTag ColorRed = "";
static ColorTag ColorReset = "";
#endif

