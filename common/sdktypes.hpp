//
//  sdktypes.hpp
//  common
//
//  Created by Arun A on 26/09/23.
//
#ifndef sdktypes_hpp
#define sdktypes_hpp

#define PLATFORM_MAC 2
#define PLATFORM_UNIX 3
#define PLATFORM_ANDROID 4
#define PLATFORM_LINUX 5

#if defined(_WIN32)
#error "Not supported"
#elif defined(__APPLE__)
#define PLATFORM PLATFORM_MAC
#elif defined(ANDROID)
#define PLATFORM PLATFORM_ANDROID
#elif defined(__linux__)
#define PLATFORM PLATFORM_LINUX
#else
#define PLATFORM PLATFORM_UNIX
#endif

#include "endian_check.h"

#define DECLSPEC

#define GSDK_UDP_DEFAULT_PORT 5000

#define GX_DELETE(x) \
    if (x)           \
    {                \
        delete x;    \
        x = NULL;    \
    }
#define GX_DELETE_ARY(x) \
    if (x)               \
    {                    \
        delete[] x;      \
        x = NULL;        \
    }

#define GX_ABS(v) std::abs(v)

#define UNUSED(x) (void)x

#define __DEFAULT_LOG_TAG__ "gsdk_log"

#if PLATFORM == PLATFORM_LINUX
#include <linux/limits.h>
#include <stdarg.h>
#include <bits/stdc++.h>
#endif

#include<sys/utsname.h>

#define LOG_LEVEL_0 0
#define LOG_LEVEL_1 1
#define LOG_LEVEL_2 2
#define LOG_LEVEL_3 3

#ifndef LOG_LEVEL
#define LOG_LEVEL LOG_LEVEL_2
#endif
extern "C" DECLSPEC int init_gsdk();
extern "C" DECLSPEC void print_common_info();
extern "C" DECLSPEC int number_of_digits(unsigned int num);
extern "C" DECLSPEC void DEBUG_PRINT(int logLevel, const char *tag, const char *format, ...);
extern "C" DECLSPEC void DEBUG_PRINT_WARN(const char *tag, const char *format, ...);
extern "C" DECLSPEC void DEBUG_PRINT_ERROR(const char *tag, const char *format, ...);
extern "C" DECLSPEC void DEBUG_ASSERT(const char *tag, bool condition, const char *format, ...);
extern "C" DECLSPEC void DEBUG_PRINT_IMPORTANT(const char *tag, const char *format, ...);
extern "C" DECLSPEC void DEBUG_PRINT_IMPORTANT2(const char *tag, const char *format, ...);

namespace gsdk {
class device {
public:
    static struct utsname device_details;
};
};
#endif /* sdktypes_hpp */
