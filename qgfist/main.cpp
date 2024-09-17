#include "../qclient/source/qnetworkclient.hpp"
#include "gclient.hpp"

#include <iostream>
#include <uv.h>
#include <cstring>
#include <sstream>
#include <string>
#include <unordered_map>
#include <chrono>
#include <thread>

// Constants

#undef __LOGTAG__
#define __LOGTAG__ "qgfist-app"

// Color codes for terminal output
#define COLOR_RESET "\033[0m"
#define COLOR_YELLOW "\033[33m"
#define COLOR_ORANGE "\033[38;5;208m"  // 256-color mode code for orange
#define COLOR_RED "\033[31m"
#define COLOR_GREEN "\033[32m"
#define COLOR_CYAN "\033[36m"

#define GAME_SERVER_IP "15.206.79.30:4004"
#define MAX_PARALLEL_REQUESTS 10
#define MIN_TIMEOUT_MS 50	// Minimum timeout in milliseconds
#define MAX_TIMEOUT_MS 300	// Maximum timeout in milliseconds
#define DEFAULT_WEIGHTAGE 0.65f
#define EXIT_AFTER 10		// Exit after this time in seconds

// Structs
struct WorkData {
	WorkData() {
        uv_gettimeofday(&start_time);
        client = DEBUG_NEW gclient();
	}

    ~WorkData() {
        GX_DELETE(client);
    }
	uv_timeval64_t start_time = {0, 0};
    gclient* client = nullptr;
};

struct AppState {
	qstring host = "192.168.0.230";
	qstring port = "4004";
	uv_loop_t* loop;
    uv_timer_t request_timer;
    uv_timer_t exit_timer;
    double request_weightage;
	int max_parallel_requests;
    int min_timeout_ms;
	int max_timeout_ms;
    int exit_after_seconds = 0;
    bool exit_signal = false;			  // Flag to signal exit
    std::unordered_map<WorkData*, std::thread::id> workers_map;
};

AppState app_state;


void request_qconnect(WorkData* data) {
    app_state.workers_map[data] = std::this_thread::get_id();
}

void finish_and_destroy_work(WorkData* data) {
	// Check if the worker exists in the map before erasing
	auto it = app_state.workers_map.find(data);
	if (it != app_state.workers_map.end()) {
		app_state.workers_map.erase(it);
	} else {
		std::cout << COLOR_RED << "Worker not found in map!" << COLOR_RESET << std::endl;
	}
	GX_DELETE(data);
}

// Worker to handle requests
void request_worker(uv_timer_t* handle) {
	int num_requests = 0;
	while (num_requests <= 0) {
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
		num_requests = static_cast<int>((rand() % app_state.max_parallel_requests) * app_state.request_weightage);
	}

	// std::cout << " r:" << num_requests << " " << std::endl;
	for (int i = 0; i < num_requests; i++) {
		WorkData* data = DEBUG_NEW WorkData();
		request_qconnect(data);
        finish_and_destroy_work(data);
	}

	// Calculate the next timeout interval between min_timeout_ms and max_timeout_ms
	int next_timeout_ms = app_state.min_timeout_ms + (rand() % (app_state.max_timeout_ms - app_state.min_timeout_ms + 1));

	// std::cout << "next trigger in " << next_timeout_ms << "ms\n";
	// Restart the timer with the calculated interval
	uv_timer_start(handle, request_worker, next_timeout_ms, 0);	 // Set repeat interval to 0
}

void print_parameters() {
    std::cout << "Parameters: Min Timeout=" << app_state.min_timeout_ms << "ms, Max Timeout=" << app_state.max_timeout_ms << "ms, Weightage=" << app_state.request_weightage << ", Max Parallel=" << app_state.max_parallel_requests
                << ", Exit After=" << app_state.exit_after_seconds << "s, game server=" << app_state.host.c_str() << ":" << app_state.port.c_str() << std::endl;
}

// Function to parse command line arguments
void parse_arguments(int argc, char* argv[]) {
	// server address
	qstring request_ip(GAME_SERVER_IP);
	std::vector<qstring> request_ip_parts;
	request_ip.split(":", request_ip_parts, false);
	if (request_ip_parts.size() == 2) {
		app_state.host = request_ip_parts[0];
		app_state.port = request_ip_parts[1];
	} else {
		debug_print_error(__LOGTAG__, "Invalid request IP: %s", request_ip.c_str());
	}
	//

	for (int i = 1; i < argc; i++) {
		if (strcmp(argv[i], "--gserver") == 0 && i + 1 < argc) {
			qstring request_ip(argv[++i]);
			std::vector<qstring> request_ip_parts;
			request_ip.split(":", request_ip_parts, false);
			if (request_ip_parts.size() == 2) {
				app_state.host = request_ip_parts[0];
				app_state.port = request_ip_parts[1];
			} else {
				debug_print_error(__LOGTAG__, "Invalid request IP: %s. Defaulting to %s", request_ip.c_str(), GAME_SERVER_IP);
			}
		} else if (strcmp(argv[i], "--exit-after") == 0 && i + 1 < argc) {
			app_state.exit_after_seconds = atoi(argv[++i]);
		} else if (strcmp(argv[i], "--weight") == 0 && i + 1 < argc) {
			double val = atof(argv[++i]);
			app_state.request_weightage = std::max(val, 0.2);
		} else if (strcmp(argv[i], "--max-parallel") == 0 && i + 1 < argc) {
			int val = atoi(argv[++i]);
			app_state.max_parallel_requests = std::clamp(val, 1, 100);
		} else if (strcmp(argv[i], "--tmin") == 0 && i + 1 < argc) {
			size_t val = atoi(argv[++i]);
			app_state.min_timeout_ms = std::max(val, (size_t) 10);
		} else if (strcmp(argv[i], "--tmax") == 0 && i + 1 < argc) {
			size_t val = atoi(argv[++i]);
			app_state.max_timeout_ms = std::max(val, (size_t) 20);
		}
	}
}

// Function to initialize AppState with default values
void initialize_app_state(AppState& app_state) {
	app_state.host = "192.168.0.230";
	app_state.port = "4004";
	app_state.loop = nullptr;
    app_state.max_parallel_requests = MAX_PARALLEL_REQUESTS;
    app_state.request_weightage = DEFAULT_WEIGHTAGE;
    app_state.min_timeout_ms = MIN_TIMEOUT_MS;
	app_state.max_timeout_ms = MAX_TIMEOUT_MS;
    app_state.exit_after_seconds = EXIT_AFTER;
    app_state.exit_signal = false;
}

void print_summary() {
}

// Timer callback to handle exit
void exit_after_delay(uv_timer_t* handle) {
	UNUSED(handle);
	// if (app_state.finished_requests < app_state.total_requests) {
	// 	std::cout << "Waiting for pending requests to finish... pending: " << (app_state.total_requests - app_state.finished_requests) << " workers:" << app_state.workers_map.size() << std::endl;
		
	// 	// Print worker data
	// 	for (const auto& worker : app_state.workers_map) {
	// 		WorkData* data = worker.first;
	// 		std::cout << "Worker connection establishment timeout (in sec): " << data->connection_establishment_timeout_in_sec << "s" << std::endl;
	// 	}
	// 	//

	// 	uv_timer_start(&app_state.exit_timer, exit_after_delay, 2 * 1000, 0);

	// 	uv_timer_stop(&app_state.request_timer);
	// 	if (app_state.summary_interval > 0) {
	// 		uv_timer_stop(&app_state.summary_timer);
	// 	}
	// 	return;
	// }

	app_state.exit_signal = true;
	uv_stop(app_state.loop);  // Stop the loop

	print_summary();
	std::cout << "\nFINISHED.\n\n" << std::endl;
}

void print_usage(const char* program_name) {
    std::cout << "Usage: " << program_name << " [options]\n\n";
    std::cout << "Options:\n";
	std::cout << "  --gserver <host:port>      Game server address in the form of host:port (default: " << GAME_SERVER_IP << ")\n";
    std::cout << "  --exit-after <seconds>    [optional] Exit after a certain number of seconds (default: " << EXIT_AFTER << ")\n";
    std::cout << "  --weight <weight>         [optional] Request weightage (default: " << DEFAULT_WEIGHTAGE << ")\n";
    std::cout << "  --tmin <ms>               [optional] Minimum timeout in milliseconds (default: " << MIN_TIMEOUT_MS << ")\n";
    std::cout << "  --tmax <ms>               [optional] Maximum timeout in milliseconds (default: " << MAX_TIMEOUT_MS << ")\n";
    std::cout << "\n";
    std::cout << "Example:\n";
    std::cout << "  " << program_name << " --gserver 127.0.0.1:4004\n";
}

void cleanup() {
    uv_timer_stop(&app_state.request_timer);
    uv_timer_stop(&app_state.exit_timer);
    uv_loop_close(app_state.loop);
}

int main(int argc, char** argv) {
    // Check for --help flag
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--help") == 0) {
            print_usage(argv[0]);
            return 0;
        }
    }

    std::cout << "qgfist: v0.1\nSTARTED...\n";
	srand(static_cast<unsigned int>(time(nullptr)));  // Seed the random number generator

	// Initialize app_state
	initialize_app_state(app_state);

	// Parse command-line arguments to potentially override defaults
	parse_arguments(argc, argv);
    print_parameters();

    app_state.loop = uv_default_loop();

	// Initialize timers
	uv_timer_init(app_state.loop, &app_state.request_timer);
    uv_timer_init(app_state.loop, &app_state.exit_timer);

    // Start the request timer immediately
	uv_timer_start(&app_state.request_timer, request_worker, 0, 0);

	// Start exit timer if specified
	if (app_state.exit_after_seconds > 0) {
		uv_timer_start(&app_state.exit_timer, exit_after_delay, app_state.exit_after_seconds * 1000, 0);
	}

    // Connect to the game server
    // connect_to_server(loop, ip.c_str(), port);

    // Run the loop to handle async operations
    uv_run(app_state.loop, UV_RUN_DEFAULT);

	// Wait for the exit signal
	while (!app_state.exit_signal) {
		uv_sleep(3000);
	}

    // Cleanup
	cleanup();

    return 0;
}
