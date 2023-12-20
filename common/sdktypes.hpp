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

#ifndef DEBUG_NEW
#include "./nvwa/debug_new.pch"
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

#include <sys/utsname.h>

#define LOG_LEVEL_0 0
#define LOG_LEVEL_1 1
#define LOG_LEVEL_2 2
#define LOG_LEVEL_3 3
#define LOG_LEVEL_4 4

#ifndef LOG_LEVEL
#define LOG_LEVEL LOG_LEVEL_2
#endif
extern "C" DECLSPEC int init_gsdk();
extern "C" DECLSPEC void print_common_info();
extern "C" DECLSPEC int number_of_digits(unsigned int num);
extern "C" DECLSPEC void DEBUG_PRINT(int logLevel, const char* tag, const char* format, ...);
extern "C" DECLSPEC void DEBUG_PRINT_WARN(const char* tag, const char* format, ...);
extern "C" DECLSPEC void DEBUG_PRINT_ERROR(const char* tag, const char* format, ...);
extern "C" DECLSPEC void DEBUG_ASSERT(const char* tag, bool condition, const char* format, ...);
extern "C" DECLSPEC void DEBUG_PRINT_IMPORTANT(const char* tag, const char* format, ...);
extern "C" DECLSPEC void DEBUG_PRINT_IMPORTANT2(const char* tag, const char* format, ...);

namespace gsdk {
    class device {
    public:
        static struct utsname device_details;
    };

    //https://stackoverflow.com/questions/7021725/how-to-convert-a-string-to-integer-in-c
    typedef enum {
        STR2INT_SUCCESS,
        STR2INT_OVERFLOW,
        STR2INT_UNDERFLOW,
        STR2INT_INCONVERTIBLE
    } str2int_errno;

    /* Convert string s to int out.
     *
     * @param[out] out The converted int. Cannot be NULL.
     *
     * @param[in] s Input string to be converted.
     *
     *     The format is the same as strtol,
     *     except that the following are inconvertible:
     *
     *     - empty string
     *     - leading whitespace
     *     - any trailing characters that are not part of the number
     *
     *     Cannot be NULL.
     *
     * @param[in] base Base to interpret string in. Same range as strtol (2 to 36).
     *
     * @return Indicates if the operation succeeded, or why it failed.
     */
extern "C" DECLSPEC str2int_errno str2int(int *out, const char *s, int base);
};
#endif /* sdktypes_hpp */
