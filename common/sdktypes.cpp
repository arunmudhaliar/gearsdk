//
//  Copyright 2024 homenet25
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

using namespace gsdk;

struct utsname device::device_details;
char server::machine_public_ip[16] = "0.0.0.0";

extern "C" {
#if PLATFORM == PLATFORM_ANDROID
JavaVM* device::g_JavaVM = nullptr;
int init_gsdk(JavaVM* JavaVM) {
	device::g_JavaVM = JavaVM;
#else
int init_gsdk() {
#endif
	errno = 0;
	if (uname(&device::device_details) != 0) {
		perror("uname doesn't return 0, so there is an error");
		return -1;
	}

	print_common_info();
	return 0;
}

void print_common_info() {
#if DEV_BUILD
	debug_print(LOG_LEVEL, __DEFAULT_LOG_TAG__, "DEVELOPMENT BUILD");
#elif PROD_BUILD
	debug_print(LOG_LEVEL, __DEFAULT_LOG_TAG__, "PRODUCTION BUILD");
#else
	debug_print(LOG_LEVEL, __DEFAULT_LOG_TAG__, "UNRECOGNISED BUILD CONFIGURATION !!!. Please set 'DEV_BUILD' or 'PROD_BUILD' in make file.\nThis server can lead to unstable behaviour !!!");
#endif
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
	debug_print(LOG_LEVEL, __DEFAULT_LOG_TAG__, "%s [%s], sz(int):%d", endian_str, arch_str, sizeof(int));

	debug_print(LOG_LEVEL, __DEFAULT_LOG_TAG__, "Log level [LOG_LEVEL_%d]", LOG_LEVEL);
	debug_print(LOG_LEVEL_0, __DEFAULT_LOG_TAG__, "Lvl0");
	debug_print(LOG_LEVEL_1, __DEFAULT_LOG_TAG__, "Lvl1");
	debug_print(LOG_LEVEL_2, __DEFAULT_LOG_TAG__, "Lvl2");
	debug_print(LOG_LEVEL_3, __DEFAULT_LOG_TAG__, "Lvl3");
	debug_print(LOG_LEVEL_4, __DEFAULT_LOG_TAG__, "Lvl4");
	debug_print(LOG_LEVEL_5, __DEFAULT_LOG_TAG__, "Lvl5");

	char cwd[PATH_MAX];
	if (getcwd(cwd, sizeof(cwd)) != NULL) {
		debug_print(LOG_LEVEL, __DEFAULT_LOG_TAG__, "Current working dir : %s", cwd);
	} else {
		debug_print_warn(__DEFAULT_LOG_TAG__, "getcwd() error");
	}
#if DEBUG
	debug_print(LOG_LEVEL_0, __DEFAULT_LOG_TAG__, "DEBUG");
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

void debug_raw(int log_level, const char* format, ...) {
	if (log_level > LOG_LEVEL) {
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

void debug_print(int log_level, const char* tag, const char* format, ...) {
	if (log_level > LOG_LEVEL) {
		return;
	}
	time_t givemetime = time(NULL);
	char buffer[LOGBUFFER_SIZE + 1];
	va_list v;
	va_start(v, format);
	vsnprintf(buffer, LOGBUFFER_SIZE, format, v);
#if PLATFORM == PLATFORM_MAC
	fprintf(stderr, "%s : [%d] [%s] %s\n", strtok(ctime(&givemetime), "\n"), getpid(), tag, buffer);
#elif PLATFORM == PLATFORM_ANDROID
	__android_log_print(ANDROID_LOG_INFO, tag, "%s", buffer);
#else
	fprintf(stderr, "%s : [%d] [%s] %s\n", strtok(ctime(&givemetime), "\n"), getpid(), tag, buffer);
#endif
	va_end(v);
}

void debug_print2_internal(int log_level, const char* tag, const char* file, const char* function, int line, const char* format, ...) {
	if (log_level > LOG_LEVEL) {
		return;
	}
	time_t givemetime = time(NULL);
	char buffer[LOGBUFFER_SIZE + 1];
	va_list v;
	va_start(v, format);
	vsnprintf(buffer, LOGBUFFER_SIZE, format, v);
#if PLATFORM == PLATFORM_MAC
	fprintf(stderr, "%s : [%d] [%s] %s\t\t(%s : %s:%d)\n", strtok(ctime(&givemetime), "\n"), getpid(), tag, buffer, file, function, line);
#elif PLATFORM == PLATFORM_ANDROID
	__android_log_print(ANDROID_LOG_INFO, tag, "%s\t\t(%s : %s:%d)", buffer, file, function, line);
#else
	fprintf(stderr, "%s : [%d] [%s] %s\t\t(%s : %s:%d)\n", strtok(ctime(&givemetime), "\n"), getpid(), tag, buffer, file, function, line);
#endif
	va_end(v);
}

type_debug_warn_or_err_cb global_warn_cb = nullptr;
type_debug_warn_or_err_cb global_err_cb = nullptr;
type_debug_warn_or_err_cb global_assert_cb = nullptr;

void set_warn_callback(type_debug_warn_or_err_cb cb) {
	global_warn_cb = cb;
}
void set_error_callback(type_debug_warn_or_err_cb cb) {
	global_err_cb = cb;
}
void set_assert_callback(type_debug_warn_or_err_cb cb) {
	global_assert_cb = cb;
}

void debug_warn(int log_level, const char* tag, const char* format, ...) {
	if (log_level > LOG_LEVEL) {
		return;
	}
	char buffer[LOGBUFFER_SIZE + 1];
	va_list v;
	va_start(v, format);
	vsnprintf(buffer, LOGBUFFER_SIZE, format, v);
	va_end(v);
	debug_print(log_level, "\x1b[93mWARN !!!", "[%s] : %s\x1b[0m", tag, buffer);
	if (global_warn_cb) {
		global_warn_cb(buffer);
	}
}

void debug_warn_cond(const char* tag, bool condition, const char* format, ...) {
	if (condition == false) {
		return;
	}
	char buffer[LOGBUFFER_SIZE + 1];
	va_list v;
	va_start(v, format);
	vsnprintf(buffer, LOGBUFFER_SIZE, format, v);
	va_end(v);
	debug_print(LOG_LEVEL, "\x1b[93mWARN !!!", "[%s] : %s\x1b[0m", tag, buffer);
	if (global_warn_cb) {
		global_warn_cb(buffer);
	}
}
void debug_print_warn(const char* tag, const char* format, ...) {
	char buffer[LOGBUFFER_SIZE + 1];
	va_list v;
	va_start(v, format);
	vsnprintf(buffer, LOGBUFFER_SIZE, format, v);
	va_end(v);
	debug_print(LOG_LEVEL, "\x1b[93mWARN !!!", "[%s] : %s\x1b[0m", tag, buffer);
	if (global_warn_cb) {
		global_warn_cb(buffer);
	}
}
void debug_print_error(const char* tag, const char* format, ...) {
	char buffer[LOGBUFFER_SIZE + 1];
	va_list v;
	va_start(v, format);
	vsnprintf(buffer, LOGBUFFER_SIZE, format, v);
	va_end(v);
	debug_print(LOG_LEVEL, "\033[41mERROR !!!", "[%s] : %s\x1b[0m", tag, buffer);
	if (global_err_cb) {
		global_err_cb(buffer);
	}
}
void debug_assert_internal(const char* tag, const char* condition, const char* file, const char* function, int line, const char* format, ...) {
	char buffer[LOGBUFFER_SIZE + 1];
	va_list v;
	va_start(v, format);
	vsnprintf(buffer, LOGBUFFER_SIZE, format, v);
	va_end(v);
	// fprintf(stderr, "Assertion '%s' failed: %s (%s: %s: %d)\n", condition, buffer, file, function, line);
	debug_print(LOG_LEVEL, "\x1B[31mASSERT !!!", "[%s] : '%s' failed\n%s\n(%s: %s: %d)\x1b[0m", tag, condition, buffer, file, function, line);
	if (global_assert_cb) {
		global_assert_cb(buffer);
	}
}
void debug_print_important(const char* tag, const char* format, ...) {
	char buffer[LOGBUFFER_SIZE + 1];
	va_list v;
	va_start(v, format);
	vsnprintf(buffer, LOGBUFFER_SIZE, format, v);
	va_end(v);
	debug_print(LOG_LEVEL, tag, "\x1b[36m%s\x1b[0m", buffer);
}
void debug_print_important2(const char* tag, const char* format, ...) {
	char buffer[LOGBUFFER_SIZE + 1];
	va_list v;
	va_start(v, format);
	vsnprintf(buffer, LOGBUFFER_SIZE, format, v);
	va_end(v);
	debug_print(LOG_LEVEL, tag, "\x1b[96m%s\x1b[0m", buffer);
}

void debug_print_scid(int log_level, const uint8_t* scid, size_t scid_len) {
	if (log_level > LOG_LEVEL) {
		return;
	}
	fprintf(stderr, "[%d] SCID: ", getpid());
	for (size_t i = 0; i < scid_len; ++i) {
		fprintf(stderr, "%02x", scid[i]);
	}
	fprintf(stderr, "\n");
}

namespace gsdk {
str2int_errno str2int(int* out, const char* s, int base) {
	if (out == nullptr || s == nullptr) {
		return STR2INT_INCONVERTIBLE;
	}
	char* end = nullptr;
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
	*out = (int) l;
	return STR2INT_SUCCESS;
}
}  // namespace gsdk
}  // extern "C"

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
