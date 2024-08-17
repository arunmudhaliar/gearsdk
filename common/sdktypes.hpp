/**
 * @file sdktypes.hpp
 * @brief Contains platform-specific definitions, macros, and utility functions for the SDK.
 *
 * This header file defines platform macros, utility macros, and functions for logging,
 * error handling, and platform-specific functionality.
 *
 * @author Arun A
 * @copyright (c) [2023], [amudaliar]
 * All rights reserved.
 */

#ifndef sdktypes_hpp
#define sdktypes_hpp

/**
 * @file sdktypes.hpp
 * @brief Contains platform-specific definitions, macros, and utility functions for the SDK.
 *
 * This header file defines platform macros, utility macros, and functions for logging,
 * error handling, and platform-specific functionality.
 */

/** Platform definitions */
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

#include <filesystem>
#include <assert.h>             // assert

#if PLATFORM == PLATFORM_MAC
namespace fs = std::__fs::filesystem;
#elif PLATFORM == PLATFORM_LINUX
namespace fs = std::filesystem;
#else
namespace fs = std::__fs::filesystem;
#endif

#ifndef DEBUG_NEW
#include "./nvwa/debug_new.pch"
#endif
#include "endian_check.h"

/** Macro definitions */
#define DECLSPEC

#define GSDK_UDP_DEFAULT_PORT 5000  ///< Default UDP port for the SDK

/** Macro to safely delete a pointer */
#define GX_DELETE(x) \
    if (x)           \
    {                \
        delete x;    \
        x = nullptr;    \
    }

/** Macro to safely delete an array of pointers */
#define GX_DELETE_ARY(x) \
    if (x)               \
    {                    \
        delete[] x;      \
        x = nullptr;        \
    }

/** Macro to calculate absolute value */
#define GX_ABS(v) std::abs(v)

/** Macro to mark unused variables */
#define UNUSED(x) (void)x

/** Default log tag */
#define __DEFAULT_LOG_TAG__ "gsdk_log"

#if PLATFORM == PLATFORM_LINUX
#include <linux/limits.h>
#include <stdarg.h>
    #if defined(__GNUC__)
        #include <bits/stdc++.h>
    #endif
#elif PLATFORM == PLATFORM_ANDROID
#include <jni.h>
#endif

#include <sys/utsname.h>

/** Logging levels */
#define LOG_LEVEL_0 0
#define LOG_LEVEL_1 1
#define LOG_LEVEL_2 2
#define LOG_LEVEL_3 3
#define LOG_LEVEL_4 4
#define LOG_LEVEL_5 5

#ifndef LOG_LEVEL
#define LOG_LEVEL LOG_LEVEL_3   ///< Default log level
#endif

#define LOGBUFFER_SIZE 256 * 2  ///< Size of the log buffer

/**
 * @brief Asserts an expression and logs an error if the expression is false.
 *
 * @param tag Log tag.
 * @param expr Expression to be checked.
 * @param ... Additional formatting arguments.
 */
#define DEBUG_ASSERT(tag, expr, ...) \
    do { \
        if (!(expr)) { \
            DEBUG_ASSERT_INTERNAL(tag, #expr, __FILE__, __FUNCTION__, __LINE__, __VA_ARGS__); \
            assert(expr); \
        } \
    } while (0)

#define __FILENAME__ (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)

/**
 * @brief Prints debug information with line number and function name.
 *
 * @param logLevel Log level.
 * @param tag Log tag.
 * @param ... Format string and arguments.
 */
#define DEBUG_PRINT2(logLevel, tag, ...) \
    do { \
        DEBUG_PRINT2_INTERNAL(logLevel, tag, __FILENAME__, __FUNCTION__, __LINE__, __VA_ARGS__); \
    } while (0)

#if PLATFORM == PLATFORM_LINUX
    #define PTHREAD_NAME(name) pthread_setname_np(pthread_self(), name) ///< Set thread name for Linux
#elif PLATFORM == PLATFORM_MAC
    #define PTHREAD_NAME(name) pthread_setname_np(name) ///< Set thread name for MacOS
#else
    #define PTHREAD_NAME(name) pthread_setname_np(name) ///< Set thread name
#endif

#if PLATFORM == PLATFORM_ANDROID
extern "C" DECLSPEC int init_gsdk(JavaVM* JavaVM); ///< Initialize SDK for Android
#else
extern "C" DECLSPEC int init_gsdk(); ///< Initialize SDK for other platforms
#endif
extern "C" DECLSPEC void print_common_info(); ///< Print common information
extern "C" DECLSPEC int number_of_digits(unsigned int num); ///< Get the number of digits in a number
extern "C" DECLSPEC void DEBUG_RAW(int logLevel, const char* format, ...); ///< Raw debug print
extern "C" DECLSPEC void DEBUG_PRINT(int logLevel, const char* tag, const char* format, ...); ///< Debug print with tag
extern "C" DECLSPEC void DEBUG_PRINT2_INTERNAL(int logLevel, const char* tag, const char* file, const char* function, int line, const char* format, ...);
extern "C" DECLSPEC void DEBUG_WARN(int logLevel, const char* tag, const char* format, ...); ///< Warn with debug info
extern "C" DECLSPEC void DEBUG_WARN_COND(const char* tag, bool condition, const char* format, ...); ///< Conditional warning
extern "C" DECLSPEC void DEBUG_PRINT_WARN(const char* tag, const char* format, ...);
extern "C" DECLSPEC void DEBUG_PRINT_ERROR(const char* tag, const char* format, ...);
extern "C" DECLSPEC void DEBUG_ASSERT_INTERNAL(const char* tag, const char* condition, const char* file, const char* function, int line, const char* format, ...);
extern "C" DECLSPEC void DEBUG_PRINT_IMPORTANT(const char* tag, const char* format, ...); ///< Print important debug info
extern "C" DECLSPEC void DEBUG_PRINT_IMPORTANT2(const char* tag, const char* format, ...); ///< Print important debug info
extern "C" DECLSPEC void DEBUG_PRINT_scid(int logLevel, const uint8_t *scid, size_t scid_len); ///< Print SCID information

namespace gsdk {

    /**
     * @brief Provides device-related information.
     */
    class device {
    public:
        static struct utsname device_details; ///< Details about the device
#if PLATFORM == PLATFORM_ANDROID
        static JavaVM* g_JavaVM; ///< Java VM instance for Android
#endif
    };

    /**
     * @brief Provides server-related information.
     */
    class server {
    public:
        static char machine_public_ip[16]; ///< Public IP address of the machine
    };

    
    /**
     * @brief Error codes for string-to-int conversion.
     */
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

typedef void (*type_debug_warn_or_err_cb)(const char*);
/**
 * @brief Set a callback function for warnings.
 *
 * @param cb Callback function.
 */
extern "C" DECLSPEC void set_warn_callback(type_debug_warn_or_err_cb cb);

/**
 * @brief Set a callback function for errors.
 *
 * @param cb Callback function.
 */

extern "C" DECLSPEC void set_error_callback(type_debug_warn_or_err_cb cb);

/**
 * @brief Set a callback function for assertions.
 *
 * @param cb Callback function.
 */
extern "C" DECLSPEC void set_assert_callback(type_debug_warn_or_err_cb cb);

};
#endif /* sdktypes_hpp */
