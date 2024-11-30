#include "signal_handler.hpp"

#include <dlfcn.h>
#include <libgen.h>
#include <libunwind.h>
#include <sys/stat.h>

#ifdef __linux__
#include <linux/ptrace.h>
#include <sys/ptrace.h>
#include <sys/wait.h>
#include <sys/user.h>
#endif
namespace signal_handler {

// Demangle C++ symbols
std::string demangle(const char* mangled_name) {
	int status = 0;
	std::unique_ptr<char, void (*)(void*)> demangled_name(abi::__cxa_demangle(mangled_name, nullptr, nullptr, &status), std::free);
	return (status == 0) ? std::string(demangled_name.get()) : std::string(mangled_name);
}

// Generate timestamp for filenames
void get_timestamp(char* buffer, size_t size) {
	time_t now = time(nullptr);
	struct tm* tm_info = localtime(&now);
	strftime(buffer, size, "%Y-%m-%d_%H-%M-%S", tm_info);
}

// Print a single thread's stack trace
void print_stack_trace(int fd) {
	void* buffer[100];
	int size = backtrace(buffer, 100);
	char** symbols = backtrace_symbols(buffer, size);

	if (symbols) {
		for (int i = 0; i < size; ++i) {
			std::string demangled = demangle(symbols[i]);
			dprintf(fd, "%s\n", demangled.c_str());
		}
		free(symbols);
	} else {
		dprintf(fd, "Failed to capture stack trace.\n");
	}
}

// MacOS-specific implementation of unwind_stack
#ifdef __APPLE__
void unwind_stack_macos(int fd, thread_t thread) {
	uintptr_t thread_id = thread;
	dprintf(fd, "\nThread ID: %lu\n", thread_id);

	if (thread_id == pthread_mach_thread_np(pthread_self())) {
		dprintf(fd, "This is the current thread.\n");
		print_stack_trace(fd);
		return;
	}

	unw_cursor_t cursor;
	unw_context_t context;

#if defined(__x86_64__)
	x86_thread_state64_t state;
	mach_msg_type_number_t count = x86_THREAD_STATE64_COUNT;
	if (thread_get_state(thread, x86_THREAD_STATE64, (thread_state_t) &state, &count) != KERN_SUCCESS) {
		dprintf(fd, "Failed to get thread state for x86_64.\n");
		return;
	}

	memset(&context, 0, sizeof(context));
	memcpy(&context.data[UNW_X86_64_RIP], &state.__rip, sizeof(state.__rip));  // Instruction pointer
	memcpy(&context.data[UNW_X86_64_RSP], &state.__rsp, sizeof(state.__rsp));  // Stack pointer
	memcpy(&context.data[UNW_X86_64_RBP], &state.__rbp, sizeof(state.__rbp));  // Base pointer

#elif defined(__arm__)
	arm_thread_state_t state;
	mach_msg_type_number_t count = ARM_THREAD_STATE_COUNT;
	if (thread_get_state(thread, ARM_THREAD_STATE, (thread_state_t) &state, &count) != KERN_SUCCESS) {
		dprintf(fd, "Failed to get thread state for ARM.\n");
		return;
	}

	memset(&context, 0, sizeof(context));
	context.data[UNW_ARM_R7] = state.__r[7];  // Frame pointer
	context.data[UNW_ARM_SP] = state.__sp;	  // Stack pointer
	context.data[UNW_ARM_LR] = state.__lr;	  // Link register
	context.data[UNW_ARM_PC] = state.__pc;	  // Program counter

#elif defined(__aarch64__)
	arm_thread_state64_t state;
	mach_msg_type_number_t count = ARM_THREAD_STATE64_COUNT;
	if (thread_get_state(thread, ARM_THREAD_STATE64, (thread_state_t) &state, &count) != KERN_SUCCESS) {
		dprintf(fd, "Failed to get thread state for ARM64.\n");
		return;
	}

	memset(&context, 0, sizeof(context));
	context.data[UNW_AARCH64_X30] = state.__lr;	 // Link register
	context.data[UNW_AARCH64_SP] = state.__sp;	 // Stack pointer
	context.data[UNW_AARCH64_PC] = state.__pc;	 // Program counter

#else
	dprintf(fd, "Unsupported architecture.\n");
	return;
#endif

	if (unw_init_local(&cursor, &context) < 0) {
		dprintf(fd, "Failed to initialize unwinding.\n");
		return;
	}

	// Unwind and log stack frames
	int depth = 0;
	while (unw_step(&cursor) > 0) {
		unw_word_t ip, sp;
		unw_get_reg(&cursor, UNW_REG_IP, &ip);	// Instruction pointer
		unw_get_reg(&cursor, UNW_REG_SP, &sp);	// Stack pointer

		Dl_info info;
		if (dladdr((const void*) ip, &info) && info.dli_sname) {
			std::string symbol = info.dli_sname;
			std::string demangled = demangle(symbol.c_str());
			const char* library_name = basename((char*) info.dli_fname);
			dprintf(fd, "%-3d %-35s 0x%016llx %s\n", depth++, library_name, (unsigned long long) ip, demangled.c_str());
		} else {
			dprintf(fd, "%-3d %-35s 0x%016llx [unknown symbol]\n", depth++, "[unknown library]", (unsigned long long) ip);
		}
	}
}
#endif	// __APPLE__

// Linux-specific implementation of unwind_stack
#ifdef __linux__
void print_stack_trace(int fd, unw_cursor_t* cursor) {
    int depth = 0;

    // Unwind the stack and print the stack frames
    while (unw_step(cursor) > 0) {
        unw_word_t ip, sp;
        unw_get_reg(cursor, UNW_REG_IP, &ip);  // Instruction pointer
        unw_get_reg(cursor, UNW_REG_SP, &sp);  // Stack pointer

        Dl_info info;
        if (dladdr((const void*)ip, &info) && info.dli_sname) {
            std::string symbol = info.dli_sname;
            const char* library_name = basename((char*)info.dli_fname);
            dprintf(fd, "%-3d %-35s 0x%016lx %s\n", depth++, library_name, (unsigned long)ip, symbol.c_str());
        } else {
            dprintf(fd, "%-3d %-35s 0x%016lx [unknown symbol]\n", depth++, "[unknown library]", (unsigned long)ip);
        }
    }
}

void unwind_stack_linux(int fd, pid_t tid) {
    uintptr_t thread_id = tid;
    dprintf(fd, "\nThread ID: %lu\n", thread_id);

    // Check if we're in the current thread
    if (thread_id == static_cast<uintptr_t>(syscall(SYS_gettid))) {
        dprintf(fd, "This is the current thread.\n");
        unw_context_t context;
        unw_cursor_t cursor;

        // Capture the current execution context
        if (unw_getcontext(&context) < 0) {
            dprintf(fd, "Failed to get the current context.\n");
            return;
        }

        // Initialize the unwinding cursor with the captured context
        if (unw_init_local(&cursor, &context) < 0) {
            dprintf(fd, "Failed to initialize unwinding.\n");
            return;
        }

        print_stack_trace(fd, &cursor);
        return;
    }

    // Handle other threads using ptrace
    if (ptrace((__ptrace_request)PTRACE_ATTACH, tid, NULL, NULL) == -1) {
        dprintf(fd, "Failed to attach to thread %d: %s\n", tid, strerror(errno));
        return;
    }

    // Wait for the thread to stop
    if (waitpid(tid, NULL, 0) == -1) {
        dprintf(fd, "Failed to wait for thread %d: %s\n", tid, strerror(errno));
        ptrace((__ptrace_request)PTRACE_DETACH, tid, NULL, NULL);
        return;
    }

    // Read the thread's registers
    struct user_regs_struct regs;
    if (ptrace((__ptrace_request)PTRACE_GETREGS, tid, NULL, &regs) == -1) {
        dprintf(fd, "Failed to get registers for thread %d: %s\n", tid, strerror(errno));
        ptrace((__ptrace_request)PTRACE_DETACH, tid, NULL, NULL);
        return;
    }

    // Initialize the unwinding context with the captured registers
    unw_context_t context;
    unw_cursor_t cursor;

    memset(&context, 0, sizeof(context));
#if defined(__x86_64__)
	context.uc_mcontext.gregs[REG_RIP] = regs.rip;  // Instruction pointer
    context.uc_mcontext.gregs[REG_RSP] = regs.rsp;  // Stack pointer
    context.uc_mcontext.gregs[REG_RBP] = regs.rbp;  // Base pointer
#elif defined(__aarch64__)
    context.data[UNW_AARCH64_PC] = regs.pc;  // Program counter
    context.data[UNW_AARCH64_SP] = regs.sp;  // Stack pointer
    context.data[UNW_AARCH64_X29] = regs.regs[29];  // Frame pointer
#else
    dprintf(fd, "Unsupported architecture for ptrace.\n");
    ptrace((__ptrace_request)PTRACE_DETACH, tid, NULL, NULL);
    return;
#endif

    if (unw_init_local(&cursor, &context) < 0) {
        dprintf(fd, "Failed to initialize unwinding for thread %d.\n", tid);
        ptrace((__ptrace_request)PTRACE_DETACH, tid, NULL, NULL);
        return;
    }

    // Print the stack trace
    print_stack_trace(fd, &cursor);

    // Detach from the thread
    if (ptrace((__ptrace_request)PTRACE_DETACH, tid, NULL, NULL) == -1) {
        dprintf(fd, "Failed to detach from thread %d: %s\n", tid, strerror(errno));
    }
}
#endif  // __linux__

// Signal handler logic (capture stack trace for all threads)
void print_all_threads_stack_trace(int signal) {
	char timestamp[32];
	get_timestamp(timestamp, sizeof(timestamp));
	char filename[64];
	snprintf(filename, sizeof(filename), "all_threads_stack_trace_%d_%s.log", signal, timestamp);

	FILE* file = fopen(filename, "w");
	if (!file) {
		fprintf(stderr, "Failed to open file for stack traces.\n");
		return;
	}
	int fd = fileno(file);

	// Get the thread ID that received the signal
#ifdef __linux__
	pid_t thread_id = syscall(SYS_gettid);	// Linux-specific system call to get thread ID
#elif __APPLE__
	uint64_t thread_id = pthread_mach_thread_np(pthread_self());  // macOS-specific call to get thread ID
#else
	uint64_t thread_id = 0;	 // Fallback for unsupported platforms
	dprintf(fd, "unsupported platform\n");
#endif
	const char* signal_readable_format = strsignal(signal);
	dprintf(fd, "Caught signal %d (%s) in process %d\n", signal, signal_readable_format, getpid());
	dprintf(fd, "Signal received by thread ID: %lu\n", (unsigned long) thread_id);

#ifdef __APPLE__
	thread_act_array_t thread_list;
	mach_msg_type_number_t thread_count;
	if (task_threads(mach_task_self(), &thread_list, &thread_count) == KERN_SUCCESS) {
		for (mach_msg_type_number_t i = 0; i < thread_count; ++i) {
			unwind_stack_macos(fd, thread_list[i]);
		}
		vm_deallocate(mach_task_self(), (vm_address_t) thread_list, thread_count * sizeof(thread_act_t));
	} else {
		dprintf(fd, "Failed to enumerate threads.\n");
	}
#elif __linux__
	DIR* dir = opendir("/proc/self/task");
	if (!dir) {
		dprintf(fd, "Failed to open /proc/self/task.\n");
		fclose(file);
		return;
	}

	struct dirent* entry;
	while ((entry = readdir(dir)) != NULL) {
		if (entry->d_name[0] == '.') {	// Skip "." and ".."
			continue;
		}

		// Check if the entry is a directory using stat (for better portability)
		char path[PATH_MAX];
		snprintf(path, sizeof(path), "/proc/self/task/%s", entry->d_name);

		struct stat statbuf;
		if (stat(path, &statbuf) == 0 && S_ISDIR(statbuf.st_mode)) {
			pid_t tid = atoi(entry->d_name);
			if (tid > 0) {
				unwind_stack_linux(fd, tid);
			}
		}
	}
	closedir(dir);
#endif

	fclose(file);
}

// Setup signal handlers
void setup_signal_handler() {
	struct sigaction sa;
	sa.sa_handler = [](int trap_signal) { 
		print_all_threads_stack_trace(trap_signal);
		// exit or re-raise the signal
		// exit(EXIT_FAILURE);
		signal(trap_signal, SIG_DFL);
		raise(trap_signal);
	};
	sa.sa_flags = SA_RESTART;
	sigemptyset(&sa.sa_mask);

	sigaction(SIGSEGV, &sa, NULL);
	sigaction(SIGABRT, &sa, NULL);
	sigaction(SIGFPE, &sa, NULL);
	sigaction(SIGILL, &sa, NULL);
	sigaction(SIGTRAP, &sa, NULL);
}

// Introducing a segmentation fault by dereferencing a null pointer
void test_segmentation_fault() {
    int* ptr = nullptr;
    *ptr = 42;  // This will cause a segmentation fault
}

// Triggering an abort to simulate a crash
void test_abort() {
    abort();  // This will cause the program to abort immediately
}

}  // namespace signal_handler