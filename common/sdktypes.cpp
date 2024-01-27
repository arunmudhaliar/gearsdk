//
//  sdktypes.cpp
//  common
//
//  Created by Arun A on 26/09/23.
//

#include "sdktypes.hpp"
#include <iostream>
#include <time.h>
#include <unistd.h>

#if PLATFORM == PLATFORM_ANDROID
#include <android/log.h>
#endif


#define LOGBUFFER_SIZE 256 * 2
using namespace gsdk;

struct utsname device::device_details;

extern "C"
{
    int init_gsdk() {
        errno = 0;
        if (uname(&device::device_details) != 0) {
            perror("uname doesn't return 0, so there is an error");
            return -1;
        }
        return 0;
    }

    void print_common_info() {
#if GSDK_ENDIAN == GSDK_LITTLEENDIAN
        const char* endian_str = "Little endian machine";
#else
        const char* endian_str = "Big endian machine";
#endif

#ifdef __aarch64__
        const char* arch_str = "ARM64 arch";
#elif __x86_64__
    #define ARCH "Intel 64-bit"
        const char* arch_str = "Intel x86_64 arch";
#else
        const char* arch_str = "Unknown architecture";
#endif
        DEBUG_PRINT(LOG_LEVEL, __DEFAULT_LOG_TAG__, "%s [%s], sz(int):%d", endian_str, arch_str, sizeof(int));
        
        DEBUG_PRINT(LOG_LEVEL, __DEFAULT_LOG_TAG__, "Log level [LOG_LEVEL_%d]", LOG_LEVEL);
        DEBUG_PRINT(LOG_LEVEL_0, __DEFAULT_LOG_TAG__, "Lvl0");
        DEBUG_PRINT(LOG_LEVEL_1, __DEFAULT_LOG_TAG__, "Lvl1");
        DEBUG_PRINT(LOG_LEVEL_2, __DEFAULT_LOG_TAG__, "Lvl2");
        DEBUG_PRINT(LOG_LEVEL_3, __DEFAULT_LOG_TAG__, "Lvl3");
        DEBUG_PRINT(LOG_LEVEL_4, __DEFAULT_LOG_TAG__, "Lvl4");
        DEBUG_PRINT(LOG_LEVEL_5, __DEFAULT_LOG_TAG__, "Lvl5");
        

        char cwd[PATH_MAX];
        if (getcwd(cwd, sizeof(cwd)) != NULL) {
            DEBUG_PRINT(LOG_LEVEL, __DEFAULT_LOG_TAG__, "Current working dir : %s", cwd);
        }
        else {
            DEBUG_PRINT_WARN(__DEFAULT_LOG_TAG__, "getcwd() error");
        }
#if DEBUG
        DEBUG_PRINT(LOG_LEVEL_0, __DEFAULT_LOG_TAG__, "DEBUG");
#endif
    }

    int number_of_digits(unsigned int num) {
        if (num == 0) {
            return 1;
        }
        int len = 0;
        unsigned int n = num;
        while (n != 0) {
            len++;
            n /= 10;
        }
        return len;
    }

    void DEBUG_RAW(int logLevel, const char* format, ...) {
        if (logLevel > LOG_LEVEL) {
            return;
        }
        char buffer[LOGBUFFER_SIZE + 1];
        va_list v;
        va_start(v, format);
        vsnprintf(buffer, LOGBUFFER_SIZE, format, v);
#if PLATFORM == PLATFORM_MAC
        fprintf(stderr, "[%d] %s\n", getpid(), buffer);
#elif PLATFORM == PLATFORM_ANDROID
        __android_log_print(ANDROID_LOG_INFO, "", "%s", buffer);
#else
        fprintf(stderr, "[%d] %s\n", getpid(), buffer);
#endif
        va_end(v);
    }

    void DEBUG_PRINT(int logLevel, const char* tag, const char* format, ...) {
        if (logLevel > LOG_LEVEL) {
            return;
        }
        time_t givemetime = time(NULL);
        char buffer[LOGBUFFER_SIZE + 1];
        va_list v;
        va_start(v, format);
        vsnprintf(buffer, LOGBUFFER_SIZE, format, v);
#if PLATFORM == PLATFORM_MAC
        fprintf(stderr, "%s : [%d] [%s] - %s\n", strtok(ctime(&givemetime), "\n"), getpid(), tag, buffer);
#elif PLATFORM == PLATFORM_ANDROID
        __android_log_print(ANDROID_LOG_INFO, tag, "%s", buffer);
#else
        fprintf(stderr, "%s : [%d] [%s] - %s\n", strtok(ctime(&givemetime), "\n"), getpid(), tag, buffer);
#endif
        va_end(v);
    }

    void DEBUG_WARN(int logLevel, const char* tag, const char* format, ...) {
        if (logLevel > LOG_LEVEL) {
            return;
        }
        char buffer[LOGBUFFER_SIZE + 1];
        va_list v;
        va_start(v, format);
        vsnprintf(buffer, LOGBUFFER_SIZE, format, v);
        va_end(v);
        DEBUG_PRINT(logLevel, "\x1b[93mWARN !!!", "[%s] : %s\x1b[0m", tag, buffer);
    }

    void DEBUG_WARN_COND(const char* tag, bool condition, const char* format, ...) {
        if (condition == false) {
            return;
        }
        char buffer[LOGBUFFER_SIZE + 1];
        va_list v;
        va_start(v, format);
        vsnprintf(buffer, LOGBUFFER_SIZE, format, v);
        va_end(v);
        DEBUG_PRINT(LOG_LEVEL, "\x1b[93mWARN !!!", "[%s] : %s\x1b[0m", tag, buffer);
    }
    void DEBUG_PRINT_WARN(const char* tag, const char* format, ...) {
        char buffer[LOGBUFFER_SIZE + 1];
        va_list v;
        va_start(v, format);
        vsnprintf(buffer, LOGBUFFER_SIZE, format, v);
        va_end(v);
        DEBUG_PRINT(LOG_LEVEL, "\x1b[93mWARN !!!", "[%s] : %s\x1b[0m", tag, buffer);
    }
    void DEBUG_PRINT_ERROR(const char* tag, const char* format, ...) {
        char buffer[LOGBUFFER_SIZE + 1];
        va_list v;
        va_start(v, format);
        vsnprintf(buffer, LOGBUFFER_SIZE, format, v);
        va_end(v);
        DEBUG_PRINT(LOG_LEVEL, "\033[41mERROR !!!", "[%s] : %s\x1b[0m", tag, buffer);
    }
    void DEBUG_ASSERT(const char* tag, bool condition, const char* format, ...) {
        if (condition) {
            return;
        }
        char buffer[LOGBUFFER_SIZE + 1];
        va_list v;
        va_start(v, format);
        vsnprintf(buffer, LOGBUFFER_SIZE, format, v);
        va_end(v);
        DEBUG_PRINT(LOG_LEVEL, "\x1B[31mASSERT !!!", "[%s] : %s\x1b[0m", tag, buffer);
        assert(condition);
    }
    void DEBUG_PRINT_IMPORTANT(const char* tag, const char* format, ...) {
        char buffer[LOGBUFFER_SIZE + 1];
        va_list v;
        va_start(v, format);
        vsnprintf(buffer, LOGBUFFER_SIZE, format, v);
        va_end(v);
        DEBUG_PRINT(LOG_LEVEL, "\x1b[36m****", "[%s] : %s\x1b[0m", tag, buffer);
    }
    void DEBUG_PRINT_IMPORTANT2(const char* tag, const char* format, ...) {
        char buffer[LOGBUFFER_SIZE + 1];
        va_list v;
        va_start(v, format);
        vsnprintf(buffer, LOGBUFFER_SIZE, format, v);
        va_end(v);
        DEBUG_PRINT(LOG_LEVEL, "\x1b[96m****", "[%s] : %s\x1b[0m", tag, buffer);
    }

    void DEBUG_PRINT_scid(int logLevel, const uint8_t *scid, size_t scid_len) {
        if (logLevel > LOG_LEVEL) {
            return;
        }
        fprintf(stderr, "[%d] SCID: ", getpid());
        for (size_t i = 0; i < scid_len; ++i) {
            fprintf(stderr, "%02x", scid[i]);
        }
        fprintf(stderr, "\n");
    }

namespace gsdk {
    str2int_errno str2int(int *out, const char *s, int base) {
        if (out == nullptr || s == nullptr) {
            return STR2INT_INCONVERTIBLE;
        }
        char *end = nullptr;
        if (s[0] == '\0' || isspace(s[0]))
            return STR2INT_INCONVERTIBLE;
        errno = 0;
        long l = strtol(s, &end, base);
        /* Both checks are needed because INT_MAX == LONG_MAX is possible. */
        if (l > INT_MAX || (errno == ERANGE && l == LONG_MAX))
            return STR2INT_OVERFLOW;
        if (l < INT_MIN || (errno == ERANGE && l == LONG_MIN))
            return STR2INT_UNDERFLOW;
        if (*end != '\0')
            return STR2INT_INCONVERTIBLE;
        *out = (int)l;
        return STR2INT_SUCCESS;
    }
}
} // extern "C"

/*
 // Console colour
 printf("\x1B[31mTexting\033[0m\t\t");
 printf("\x1B[32mTexting\033[0m\t\t");
 printf("\x1B[33mTexting\033[0m\t\t");
 printf("\x1B[34mTexting\033[0m\t\t");
 printf("\x1B[35mTexting\033[0m\n");

 printf("\x1B[36mTexting\033[0m\t\t");
 printf("\x1B[36mTexting\033[0m\t\t");
 printf("\x1B[36mTexting\033[0m\t\t");
 printf("\x1B[37mTexting\033[0m\t\t");
 printf("\x1B[93mTexting\033[0m\n");

 printf("\033[3;42;30mTexting\033[0m\t\t");
 printf("\033[3;43;30mTexting\033[0m\t\t");
 printf("\033[3;44;30mTexting\033[0m\t\t");
 printf("\033[3;104;30mTexting\033[0m\t\t");
 printf("\033[3;100;30mTexting\033[0m\n");

 printf("\033[3;47;35mTexting\033[0m\t\t");
 printf("\033[2;47;35mTexting\033[0m\t\t");
 printf("\033[1;47;35mTexting\033[0m\t\t");
 */
