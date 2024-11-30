#ifndef SIGNAL_HANDLER_H
#define SIGNAL_HANDLER_H

#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <cxxabi.h>
#include <execinfo.h>
#include <memory>
#include <pthread.h>
#include <string>
#include <unistd.h>

#define UNW_LOCAL_ONLY
#include <libunwind.h>

// Conditional includes for platform-specific headers
#ifdef __linux__
#include <dirent.h>
#include <sys/syscall.h>
#include <sys/ptrace.h>  // Ensure ptrace is available on Linux
#include <sys/types.h>
#elif __APPLE__
#include <mach/mach.h>
#include <mach/thread_act.h>
#endif

namespace signal_handler {

// Demangle C++ symbols
std::string demangle(const char* mangled_name);

// Generate timestamp for filenames
void get_timestamp(char* buffer, size_t size);

// Print a single thread's stack trace
void print_stack_trace(int fd);

// Print stack trace for a specific thread (platform-specific)
void print_thread_stack_trace(int fd, uintptr_t thread_id, const char* platform_info);

#ifdef __APPLE__
void unwind_stack_macos(int fd, thread_t thread);
#endif

#ifdef __linux__
void print_stack_trace(int fd, unw_cursor_t* cursor);
void unwind_stack_linux(int fd, pid_t tid);
#endif

// Capture and print stack trace for all threads
void print_all_threads_stack_trace(int signal);

// Signal handler for capturing specific signals like segfaults
void signal_handler(int trap_signal);

// Setup signal handlers
void setup_signal_handler();

}  // namespace signal_handler

#endif  // SIGNAL_HANDLER_H
