//
//  SDKTypes.cpp
//  Common
//
//  Created by Arun A on 26/09/23.
//

#include "SDKTypes.hpp"

#include <iostream>
#include <time.h>
#include <unistd.h>

#if PLATFORM == PLATFORM_ANDROID
#include <android/log.h>
#endif

#if PLATFORM == PLATFORM_LINUX
#include <linux/limits.h>
#include <stdarg.h>
#include <bits/stdc++.h>
#endif

#define LOGBUFFER_SIZE FILENAME_MAX*2

extern "C"
{

void PrintCommonInfo() {
    DEBUG_PRINT(LOG_LEVEL, __DEFAULT_LOG_TAG__, "Log level [LOG_LEVEL_%d]", LOG_LEVEL);
    char cwd[PATH_MAX];
    if (getcwd(cwd, sizeof(cwd)) != NULL) {
        DEBUG_PRINT(LOG_LEVEL, __DEFAULT_LOG_TAG__, "Current working dir : %s", cwd);
    } else {
        DEBUG_PRINT_WARN(__DEFAULT_LOG_TAG__, "getcwd() error");
    }
#if DEBUG
    DEBUG_PRINT(LOG_LEVEL_0, __DEFAULT_LOG_TAG__, "DEBUG");
#endif
}

void DEBUG_PRINT(int logLevel, const char* tag, const char* format, ...) {
    if (logLevel>LOG_LEVEL) {
        return;
    }
    time_t givemetime = time(NULL);
    char buffer[LOGBUFFER_SIZE];
    va_list v;
    va_start(v,format);
    vsnprintf(buffer, LOGBUFFER_SIZE, format, v);
    va_end(v);
#if PLATFORM == PLATFORM_MAC
    fprintf(stderr, "%s : [%s] - %s\n", strtok(ctime(&givemetime), "\n"), tag, buffer);
#elif PLATFORM == PLATFORM_ANDROID
     __android_log_print(ANDROID_LOG_INFO, tag, buffer);
#else
    fprintf(stderr, "%s : [%s] - %s\n", strtok(ctime(&givemetime), "\n"), tag, buffer);
#endif
}

void DEBUG_PRINT_WARN(const char* tag, const char* format, ...) {
    char buffer[LOGBUFFER_SIZE];
    va_list v;
    va_start(v,format);
    vsnprintf(buffer, LOGBUFFER_SIZE, format, v);
    va_end(v);
    DEBUG_PRINT(LOG_LEVEL, "\x1B[33mWARN !!!", "[%s] : %s\x1b[0m", tag, buffer);
}
void DEBUG_PRINT_ERROR(const char* tag, const char* format, ...){
    char buffer[LOGBUFFER_SIZE];
    va_list v;
    va_start(v,format);
    vsnprintf(buffer, LOGBUFFER_SIZE, format, v);
    va_end(v);
    DEBUG_PRINT(LOG_LEVEL, "\x1B[31mERROR !!!", "[%s] : %s\x1b[0m", tag, buffer);
}
void DEBUG_ASSERT(const char* tag, bool condition, const char* format, ...) {
    if (condition) {
        return;
    }
    char buffer[LOGBUFFER_SIZE];
    va_list v;
    va_start(v,format);
    vsnprintf(buffer, LOGBUFFER_SIZE, format, v);
    va_end(v);
    DEBUG_PRINT(LOG_LEVEL, "\x1B[31mASSERT !!!", "[%s] : %s\x1b[0m", tag, buffer);
    assert(condition);
}
void DEBUG_PRINT_IMPORTANT(const char* tag, const char* format, ...) {
    char buffer[LOGBUFFER_SIZE];
    va_list v;
    va_start(v,format);
    vsnprintf(buffer, LOGBUFFER_SIZE, format, v);
    va_end(v);
    DEBUG_PRINT(LOG_LEVEL, "\x1B[32m****", "[%s] : %s\x1b[0m", tag, buffer);
}
void DEBUG_PRINT_IMPORTANT2(const char* tag, const char* format, ...) {
    char buffer[LOGBUFFER_SIZE];
    va_list v;
    va_start(v,format);
    vsnprintf(buffer, LOGBUFFER_SIZE, format, v);
    va_end(v);
    DEBUG_PRINT(LOG_LEVEL, "\x1B[35m****", "[%s] : %s\x1b[0m", tag, buffer);
}

} //extern "C"


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
