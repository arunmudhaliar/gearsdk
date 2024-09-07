#include "../qh3client/qh3client/qh3client_helper.hpp"

#include <algorithm>  // For std::sort
#include <atomic>	  // For std::atomic
#include <cstddef>	  // For std::size_t
#include <ctime>	  // For time (to seed rand)
#include <fstream>	  // For file handling (CSV)
#include <iomanip>	  // For std::setfill and std::setw
#include <iostream>
#include <rapidjson/document.h>
#include <rapidjson/filereadstream.h>
#include <sstream>	// For std::ostringstream
#include <string>	// For string handling
#include <uv.h>
#include <vector>  // For std::vector>
#include <chrono>
#include <thread>

// Constants

#undef __LOGTAG__
#define __LOGTAG__ "qfist-app"

// Color codes for terminal output
#define COLOR_RESET "\033[0m"
#define COLOR_YELLOW "\033[33m"
#define COLOR_ORANGE "\033[38;5;208m"  // 256-color mode code for orange
#define COLOR_RED "\033[31m"

#define JSON_FILE "users.json"
// #define REQUEST_URL "https://google.com"
#define REQUEST_IP "15.206.79.30:4004"
#define MAX_PARALLEL_REQUESTS 10
#define MIN_TIMEOUT_MS 50	// Minimum timeout in milliseconds
#define MAX_TIMEOUT_MS 300	// Maximum timeout in milliseconds
#define WINDOW_SIZE 1000	// Define the size of the moving window
#define EXIT_AFTER 60		// Exit after this time in seconds
#define DEFAULT_WEIGHTAGE 0.65f
#define SUMMARY_INTERVAL 5	// Default interval to print summary in seconds

using namespace client;

// Structs
struct WorkData {
	WorkData() : result(0) {
		start_time.tv_sec = 0;
		start_time.tv_usec = 0;
	}
	uv_timeval64_t start_time;
	short result;
	qstring api;
	qstring payload;
};

struct AppState {
	qstring host = "192.168.0.230";
	qstring port = "4004";
	uv_loop_t* loop;
	uv_timer_t exit_timer;
	uv_timer_t request_timer;
	uv_timer_t summary_timer;
	std::atomic<int>  total_requests {0};
	std::atomic<int> finished_requests {0};
	std::atomic<double> cumulative_response_time {0.0};
	std::atomic<std::size_t> total_data_transferred;
	// std::atomic<std::size_t> chunk_counter; // Atomic counter to avoid race conditions
	double response_times[WINDOW_SIZE];			   // Circular buffer for response times
	std::atomic<size_t> response_times_count {0};  // Number of entries in the circular buffer
	size_t window_start_index;					   // Index to track the start of the window
	size_t window_end_index;					   // Index to track the end of the window
	int exit_after_seconds;
	double request_weightage;
	int max_parallel_requests;
	int min_timeout_ms;
	int max_timeout_ms;
	int summary_interval;
	uv_timeval64_t start_time;
	uv_timeval64_t summary_start_time;
	bool exit_signal;			  // Flag to signal exit
	bool single_request = false;  // Flag to enable single request mode

	// New variables for summary tracking
	std::atomic<int> summary_requests {0};
	std::atomic<int> summary_successful_requests {0};
	double summary_cumulative_response_time;
	std::atomic<int> connection_establishment_timeout {(int)CONNECTION_ESTABLISHMENT_TIMEOUT};

	// Total run metrics
	std::atomic<int> total_successful_requests {0};
	std::atomic<double> total_cumulative_response_time  {0.0};

	// Requests
	std::vector<std::pair<qstring, qstring>> requests;

	qstring export_csv_filename = "summary.csv";

	std::mutex response_time_mutex;
};

// Global App State
AppState app_state;

// Helper function to calculate percentiles
double calculate_percentile(std::vector<double>& sorted_times, double percentile) {
	if (sorted_times.empty())
		return 0.0;
	double index = percentile * (sorted_times.size() - 1);
	int lower = static_cast<int>(index);
	int upper = lower + 1;
	if (upper >= (int) sorted_times.size()) {
		return sorted_times[lower];
	} else {
		return sorted_times[lower] + (sorted_times[upper] - sorted_times[lower]) * (index - lower);
	}
}

// Function to print percentiles
void print_percentiles() {
	std::vector<double> response_times_sorted(app_state.response_times, app_state.response_times + app_state.response_times_count);
	std::sort(response_times_sorted.begin(), response_times_sorted.end());

	double p20 = calculate_percentile(response_times_sorted, 0.20);
	double p50 = calculate_percentile(response_times_sorted, 0.50);
	double p75 = calculate_percentile(response_times_sorted, 0.75);

	std::cout << "Response Time Percentiles:\n";
	std::cout << "\t20th Percentile: " << p20 << " ms\n";
	std::cout << "\t50th Percentile: " << p50 << " ms\n";
	std::cout << "\t75th Percentile: " << p75 << " ms\n" << std::endl;
}

// Helper function to format elapsed time
std::string format_elapsed_time(double elapsed_seconds) {
	int hours = static_cast<int>(elapsed_seconds / 3600);
	int minutes = static_cast<int>((elapsed_seconds - (hours * 3600)) / 60);
	int seconds = static_cast<int>(elapsed_seconds) % 60;

	std::ostringstream oss;
	oss << std::setfill('0') << std::setw(2) << hours << ":" << std::setfill('0') << std::setw(2) << minutes << ":" << std::setfill('0') << std::setw(2) << seconds;

	return oss.str();
}

double elapsed_seconds_since_app_launch() {
	uv_timeval64_t now;
	uv_gettimeofday(&now);
	double elapsed_seconds = (now.tv_sec - app_state.start_time.tv_sec) + (now.tv_usec - app_state.start_time.tv_usec) / 1000000.0;
	return elapsed_seconds;
}

double elapsed_seconds_since_summary() {
	uv_timeval64_t now;
	uv_gettimeofday(&now);
	double elapsed_seconds = (now.tv_sec - app_state.summary_start_time.tv_sec) + (now.tv_usec - app_state.summary_start_time.tv_usec) / 1000000.0;
	return elapsed_seconds;
}

void print_parameters() {
	if (!app_state.single_request) {
		std::cout << "Parameters: Min Timeout=" << app_state.min_timeout_ms << "ms, Max Timeout=" << app_state.max_timeout_ms << "ms, Weightage=" << app_state.request_weightage << ", Max Parallel=" << app_state.max_parallel_requests
				  << ", Exit After=" << app_state.exit_after_seconds << "s, Summary Interval=" << app_state.summary_interval << "s, server=" << app_state.host.c_str() << ":" << app_state.port.c_str() << std::endl;
	} else {
		std::cout << "SINGLE-SHOT-MODE, server=" << app_state.host.c_str() << ":" << app_state.port.c_str() << std::endl;
	}
}

// Function to write summary data to a CSV file
void write_summary_to_csv(const qstring& filename) {
	std::ofstream csv_file(filename.c_str(), std::ios::app);  // Open in append mode
	if (!csv_file.is_open()) {
		std::cerr << "Failed to open CSV file." << std::endl;
		return;
	}

	double total_success_percentage = (app_state.total_requests > 0) ? (app_state.total_successful_requests * 100.0 / app_state.total_requests) : 0.0;
	double total_average_response_time = (app_state.total_requests > 0) ? (app_state.total_cumulative_response_time / app_state.total_requests) : 0.0;

	double elapsed_seconds = elapsed_seconds_since_app_launch();
	std::size_t total_data_bytes = app_state.total_data_transferred.load();
	double data_transfer_rate_kb = (elapsed_seconds > 0) ? (total_data_bytes / 1024.0 / elapsed_seconds) : 0.0;	 // KB/s

	// Calculate requests per second
	double elapsed_since_summary = elapsed_seconds_since_summary();
	double rps = (elapsed_since_summary > 0) ? (app_state.summary_requests / elapsed_since_summary) : 0.0;

	// Write headers if file is empty (optional)
	if (csv_file.tellp() == 0) {
		csv_file << "Elapsed Time,Total Requests,Success Percentage,Avg Response Time (ms),Data Transfer Rate (KB/s),Rq/s\n";
	}

	// Write the summary data
	csv_file << format_elapsed_time(elapsed_seconds) << "," << app_state.total_requests << "," << total_success_percentage << "," << total_average_response_time << "," << data_transfer_rate_kb << "," << rps << "\n";

	csv_file.close();
}

void print_open_fds_count() {
    char cmd[256];
    snprintf(cmd, sizeof(cmd), "lsof -p %d | wc -l", getpid());
    system(cmd);
}

// Calculate and print statistics
void print_summary() {
	if (app_state.summary_requests == 0) {
		return;
	}
	double success_percentage, average_response_time;
	double elapsed_seconds = elapsed_seconds_since_app_launch();

	// Summary period metrics
	success_percentage = (app_state.summary_requests > 0) ? (app_state.summary_successful_requests * 100.0 / app_state.summary_requests) : 0.0;
	average_response_time = (app_state.summary_requests > 0) ? (app_state.summary_cumulative_response_time / app_state.summary_requests) : 0.0;

	// Total run metrics
	double total_success_percentage = (app_state.total_requests > 0) ? (app_state.total_successful_requests * 100.0 / app_state.total_requests) : 0.0;
	double total_average_response_time = (app_state.total_requests > 0) ? (app_state.total_cumulative_response_time / app_state.total_requests) : 0.0;

	std::size_t total_data_bytes = app_state.total_data_transferred.load();
	double data_transfer_rate_kb = (elapsed_seconds > 0) ? (total_data_bytes / 1024.0 / elapsed_seconds) : 0.0;	 // KB/s
	double total_data_mb = total_data_bytes / (1024.0 * 1024.0);												 // MB
	double total_data_kb = total_data_bytes / 1024.0;															 // KB

	std::string elapsed_time_str = format_elapsed_time(elapsed_seconds);

	// Determine color for success percentage
	std::string color;
	if (success_percentage < 70) {
		color = COLOR_RED;
	} else if (success_percentage < 80) {
		color = COLOR_ORANGE;
	} else if (success_percentage < 100) {
		color = COLOR_YELLOW;
	} else {
		color = COLOR_RESET;
	}

	// Calculate requests per second
	double elapsed_since_summary = elapsed_seconds_since_summary();
	double rps = (elapsed_since_summary > 0) ? (app_state.summary_requests / elapsed_since_summary) : 0.0;

	std::cout << "\n";
	std::cout << "SUMMARY " << elapsed_time_str << ":\n";
	print_parameters();
	std::cout << "Total Requests: " << app_state.total_requests << "\tSuccess: " << app_state.total_successful_requests << " (" << total_success_percentage << "%) \tFinished: " << app_state.finished_requests << "\n";
	std::cout << "Total Avg Response Time: " << total_average_response_time << " ms\n";
	std::cout << "Data Downloaded: " << total_data_mb << " MB (" << total_data_kb << " KB)" << " Bytes (" << total_data_bytes << ")\n";
	std::cout << "Data Transfer Rate: " << data_transfer_rate_kb << " KB/s\n";

	std::cout << "\tSummary Period Requests: " << app_state.summary_requests << '\n';
	std::cout << "\tSummary Period Success: " << color << app_state.summary_successful_requests << " (" << success_percentage << "%)" << COLOR_RESET << "\n";
	std::cout << "\tSummary Period Avg Response Time: " << average_response_time << " ms\n";
	std::cout << "\tRequests Per Second: " << rps << "\n";
	print_percentiles();
	// std::cout << std::endl;

	write_summary_to_csv(app_state.export_csv_filename);

	// print_open_fds_count();

	// Reset summary counters
	app_state.summary_requests = 0;
	app_state.summary_successful_requests = 0;
	app_state.summary_cumulative_response_time = 0.0;
	uv_gettimeofday(&app_state.summary_start_time);
}

void atomic_add(std::atomic<double>& atomic_var, double value) {
    double current = atomic_var.load(std::memory_order_relaxed);
    double desired;
    do {
        desired = current + value;
    } while (!atomic_var.compare_exchange_weak(current, desired, std::memory_order_relaxed));
}

void finish_and_destroy_work(WorkData* data) {
	uv_timeval64_t end_time;
	uv_gettimeofday(&end_time);

	double response_time = (end_time.tv_sec - data->start_time.tv_sec) * 1000.0 + (end_time.tv_usec - data->start_time.tv_usec) / 1000.0;  // in milliseconds
	std::lock_guard<std::mutex> lock(app_state.response_time_mutex);
	atomic_add(app_state.total_cumulative_response_time, response_time);

	if (data->result > 0) {
		app_state.finished_requests++;
		if (data->result > 1) {
			app_state.total_successful_requests++;
		}
	}

	// Add response time to the circular buffer
	app_state.response_times[app_state.window_end_index] = response_time;
	app_state.window_end_index = (app_state.window_end_index + 1) % WINDOW_SIZE;
	if (app_state.response_times_count < WINDOW_SIZE) {
		app_state.response_times_count++;
	}

	// Update summary statistics
	app_state.summary_cumulative_response_time += response_time;
	if (data->result > 1) {
		app_state.summary_successful_requests++;
	}

	// double current_average_response_time = (app_state.summary_requests > 0) ? (app_state.summary_cumulative_response_time / app_state.summary_requests) : 0.0;
	// double connection_establishment_timeout = static_cast<double>(app_state.connection_establishment_timeout);
	// double summary_cumulative_response_time_in_sec = current_average_response_time * 0.001;
	// double half_way_mark_response_time = summary_cumulative_response_time_in_sec * 0.5;
	// double half_way_mark = connection_establishment_timeout * 0.5;
	// if (half_way_mark_response_time > half_way_mark) {
	// 	double next_connection_establishment_timeout_in_sec = connection_establishment_timeout * 1.25;
	// 	std::cout << "+Updated connection_establishment_timeout to " << next_connection_establishment_timeout_in_sec  << "s from " << app_state.connection_establishment_timeout << "s [" << summary_cumulative_response_time_in_sec << "] " <<
	// 	"half_way_mark_response_time: " << half_way_mark_response_time << " half_way_mark: " << half_way_mark
	// 	<< std::endl;
	// 	app_state.connection_establishment_timeout = next_connection_establishment_timeout_in_sec;
	// } else if (half_way_mark_response_time < half_way_mark && half_way_mark>CONNECTION_ESTABLISHMENT_TIMEOUT*0.6) {
	// 	double next_connection_establishment_timeout_in_sec = connection_establishment_timeout * 0.95;
	// 	std::cout << "-Updated connection_establishment_timeout to " << next_connection_establishment_timeout_in_sec  << "s from " << app_state.connection_establishment_timeout << "s [" << summary_cumulative_response_time_in_sec << "] " <<
	// 	"half_way_mark_response_time: " << half_way_mark_response_time << " half_way_mark: " << half_way_mark
	// 	<< std::endl;
	// 	app_state.connection_establishment_timeout = next_connection_establishment_timeout_in_sec;
	// }

	const double INCREASE_SCALE = 1.25; // Scale factor to increase timeout
	const double DECREASE_SCALE = 0.95; // Scale factor to decrease timeout
	const double MIN_TIMEOUT = 1.0; // Minimum allowed timeout
	const double THRESHOLD_RATIO = 0.6; // Ratio for threshold comparison

	// Calculate average response time in seconds
	double current_average_response_time = (app_state.summary_requests > 0) 
		? (app_state.summary_cumulative_response_time / app_state.summary_requests * 0.001) 
		: 0.0;

	double connection_establishment_timeout = static_cast<double>(app_state.connection_establishment_timeout);

	// Calculate halfway marks
	double half_way_mark_response_time = current_average_response_time * 0.5;
	double half_way_mark = connection_establishment_timeout * 0.5;

	// Adjust timeout based on response times
	if (half_way_mark_response_time > half_way_mark) {
		double next_connection_establishment_timeout_in_sec = connection_establishment_timeout * INCREASE_SCALE;
		std::cout << "\r" << "+connection_establishment_timeout to " << std::fixed << std::setprecision(2) << next_connection_establishment_timeout_in_sec
				<< "s from " << app_state.connection_establishment_timeout << "s, avg response time: "
				<< current_average_response_time
				<< " half_way_mark_response_time: " << half_way_mark_response_time << " half_way_mark: " << half_way_mark
				<< "      "  // Adding extra spaces at the end to clear any remaining old output
				<< std::flush;
		app_state.connection_establishment_timeout = std::max(next_connection_establishment_timeout_in_sec, MIN_TIMEOUT);
	} else if (half_way_mark_response_time < half_way_mark && half_way_mark > (CONNECTION_ESTABLISHMENT_TIMEOUT * THRESHOLD_RATIO)) {
		double next_connection_establishment_timeout_in_sec = connection_establishment_timeout * DECREASE_SCALE;
		std::cout << "\r" << "-connection_establishment_timeout to " << std::fixed << std::setprecision(2) << next_connection_establishment_timeout_in_sec
				<< "s from " << app_state.connection_establishment_timeout << "s, avg response time: "
				<< current_average_response_time
				<< " half_way_mark_response_time: " << half_way_mark_response_time << " half_way_mark: " << half_way_mark
				<< "      "  // Adding extra spaces at the end to clear any remaining old output
				<< std::flush;
		app_state.connection_establishment_timeout = std::max(next_connection_establishment_timeout_in_sec, MIN_TIMEOUT);
	}


	// // Print percentiles when buffer is full (amudaliar) - disabled this due to bug. it keeps on printing once the count is reached.
	// if (app_state.response_times_count == WINDOW_SIZE) {
	//     print_percentiles();
	// }

	GX_DELETE(data);
}

std::vector<std::pair<qstring, qstring>> load_requests_from_json(const char* filename) {
	std::vector<std::pair<qstring, qstring>> requests;

	FILE* fp = fopen(filename, "r");
	if (!fp) {
		std::cerr << "Failed to open file " << filename << std::endl;
		return requests;
	}

	char readBuffer[65536];
	rapidjson::FileReadStream is(fp, readBuffer, sizeof(readBuffer));
	rapidjson::Document document;
	document.ParseStream(is);
	fclose(fp);

	if (!document.IsArray()) {
		std::cerr << "JSON is not an array" << std::endl;
		return requests;
	}

	for (auto& value : document.GetArray()) {
		if (value.IsObject() && value.HasMember("api") && value.HasMember("payload")) {
			const rapidjson::Value& api = value["api"];
			const rapidjson::Value& payload = value["payload"];

			if (api.IsString() && payload.IsString()) {
				requests.emplace_back(api.GetString(), payload.GetString());
			}
		}
	}

	return requests;
}

void request_qh3(WorkData* data) {
	uv_gettimeofday(&data->start_time);
	int result = qh3client_helper::send_async_request<client::qh3client>(
		app_state.host, app_state.port, conn_io_req_res::create(data->api, data->payload), data,
		[&](conn_io_req_res* request, conn_io_req_res* response, void* client_specific_data, void* arg, bool success) {
			UNUSED(client_specific_data);
			UNUSED(request);
			bool validate = response->validate();
			if (!validate) {
				// debug_print_error(__LOGTAG__, "crc fail !!!");
			}
			WorkData* data = static_cast<WorkData*>(arg);
			data->result = 1 + (success && validate);
			const conn_io_req_res::payload& payload = response->data;

			std::size_t total_size = payload.get_size();
			app_state.total_data_transferred.fetch_add(total_size);
			// std::size_t current_chunk = app_state.chunk_counter.fetch_add(1);
			// std::cout << "<";
			// debug_print(LOG_LEVEL_0, __LOGTAG__, "async returned [%d, %d] %s !!!", success, validate, payload.buffer.c_str());
			finish_and_destroy_work(data);
		},
		0, app_state.connection_establishment_timeout);

	if (result == 0) {
		app_state.total_requests++;
		app_state.summary_requests++;
	}
}

void prepareRequest(WorkData* data, int request_index) {
	const auto& request = app_state.requests[request_index];
	data->api = request.first;
	data->payload = request.second;
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

		// Pick a random request
		size_t index = rand() % app_state.requests.size();
		prepareRequest(data, index);
		request_qh3(data);
	}

	// Calculate the next timeout interval between min_timeout_ms and max_timeout_ms
	int next_timeout_ms = app_state.min_timeout_ms + (rand() % (app_state.max_timeout_ms - app_state.min_timeout_ms + 1));

	// std::cout << "next trigger in " << next_timeout_ms << "ms\n";
	// Restart the timer with the calculated interval
	uv_timer_start(handle, request_worker, next_timeout_ms, 0);	 // Set repeat interval to 0
}

// Timer callback to handle exit
void exit_after_delay(uv_timer_t* handle) {
	UNUSED(handle);
	if (app_state.finished_requests < app_state.total_requests) {
		std::cout << "Waiting for pending requests to finish... pending :" << (app_state.total_requests - app_state.finished_requests) << "\n";
		uv_timer_start(&app_state.exit_timer, exit_after_delay, 2 * 1000, 0);

		uv_timer_stop(&app_state.request_timer);
		if (app_state.summary_interval > 0) {
			uv_timer_stop(&app_state.summary_timer);
		}
		return;
	}

	app_state.exit_signal = true;
	uv_stop(app_state.loop);  // Stop the loop

	print_summary();
	std::cout << "\nFINISHED.\n\n";
}

// Timer callback to print summary periodically
void summary_timer_cb(uv_timer_t* handle) {
	UNUSED(handle);
	print_summary();
}

std::string get_timestamp() {
	std::time_t t = std::time(nullptr);
	std::tm tm = *std::localtime(&t);

	std::ostringstream oss;
	oss << std::put_time(&tm, "%Y-%m-%d_%H-%M-%S");

	return oss.str();
}

// Function to parse command line arguments
void parse_arguments(int argc, char* argv[]) {
	app_state.min_timeout_ms = MIN_TIMEOUT_MS;
	app_state.max_timeout_ms = MAX_TIMEOUT_MS;
	app_state.request_weightage = DEFAULT_WEIGHTAGE;
	app_state.max_parallel_requests = MAX_PARALLEL_REQUESTS;
	app_state.exit_after_seconds = EXIT_AFTER;
	app_state.summary_interval = SUMMARY_INTERVAL;
	app_state.single_request = false;

	// server address
	qstring request_ip(REQUEST_IP);
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
		if (strcmp(argv[i], "--tmin") == 0 && i + 1 < argc) {
			size_t val = atoi(argv[++i]);
			app_state.min_timeout_ms = std::max(val, (size_t) 10);
		} else if (strcmp(argv[i], "--tmax") == 0 && i + 1 < argc) {
			size_t val = atoi(argv[++i]);
			app_state.max_timeout_ms = std::max(val, (size_t) 20);
		} else if (strcmp(argv[i], "--weight") == 0 && i + 1 < argc) {
			double val = atof(argv[++i]);
			app_state.request_weightage = std::max(val, 0.2);
		} else if (strcmp(argv[i], "--max-parallel") == 0 && i + 1 < argc) {
			int val = atoi(argv[++i]);
			app_state.max_parallel_requests = std::clamp(val, 1, 100);
		} else if (strcmp(argv[i], "--exit-after") == 0 && i + 1 < argc) {
			app_state.exit_after_seconds = atoi(argv[++i]);
		} else if (strcmp(argv[i], "--summary-interval") == 0 && i + 1 < argc) {
			app_state.summary_interval = atoi(argv[++i]);
		} else if (strcmp(argv[i], "--server") == 0 && i + 1 < argc) {
			qstring request_ip(argv[++i]);
			std::vector<qstring> request_ip_parts;
			request_ip.split(":", request_ip_parts, false);
			if (request_ip_parts.size() == 2) {
				app_state.host = request_ip_parts[0];
				app_state.port = request_ip_parts[1];
			} else {
				debug_print_error(__LOGTAG__, "Invalid request IP: %s. Defaulting to %s", request_ip.c_str(), REQUEST_IP);
			}
		} else if (strcmp(argv[i], "--single-mode") == 0) {
			app_state.single_request = true;
		}
	}
}

// Function to initialize AppState with default values
void initialize_app_state(AppState& app_state) {
	app_state.host = "192.168.0.230";
	app_state.port = "4004";
	app_state.loop = nullptr;
	app_state.total_requests = 0;
	app_state.finished_requests = 0;
	app_state.cumulative_response_time = 0.0;
	app_state.total_data_transferred = 0;
	// app_state.chunk_counter = 0;
	app_state.response_times_count = 0;
	app_state.window_start_index = 0;
	app_state.window_end_index = 0;
	app_state.exit_after_seconds = EXIT_AFTER;
	app_state.request_weightage = DEFAULT_WEIGHTAGE;
	app_state.max_parallel_requests = MAX_PARALLEL_REQUESTS;
	app_state.min_timeout_ms = MIN_TIMEOUT_MS;
	app_state.max_timeout_ms = MAX_TIMEOUT_MS;
	app_state.summary_interval = SUMMARY_INTERVAL;
	app_state.exit_signal = false;

	// Initialize summary counters
	app_state.summary_requests = 0;
	app_state.summary_successful_requests = 0;
	app_state.summary_cumulative_response_time = 0.0;

	// Initialize total run counters
	app_state.total_successful_requests = 0;
	app_state.total_cumulative_response_time = 0.0;

	// Get the current time
	uv_gettimeofday(&app_state.start_time);
	uv_gettimeofday(&app_state.summary_start_time);

	// Generate timestamp and update export CSV filename
	std::string timestamp = get_timestamp();
	app_state.export_csv_filename = qstring(("summary-" + timestamp + ".csv").c_str());

	// Load requests from the JSON file
	app_state.requests = load_requests_from_json("requests.json");
}

void cleanup() {
	if (app_state.single_request) {
		uv_loop_close(app_state.loop);
	} else {
		if (app_state.loop) {
			uv_timer_stop(&app_state.exit_timer);
			uv_timer_stop(&app_state.request_timer);
			uv_timer_stop(&app_state.summary_timer);
			// uv_run(app_state.loop, UV_RUN_DEFAULT);
			uv_loop_close(app_state.loop);
			// delete app_state.loop;   // Only needed for custom loops
		}
	}
}

void single_mode() {
	WorkData* data = DEBUG_NEW WorkData();
	prepareRequest(data, 0);
	int result = qh3client_helper::send_async_request<client::qh3client>(
		app_state.host, app_state.port, conn_io_req_res::create(data->api, data->payload), data,
		[&](conn_io_req_res* request, conn_io_req_res* response, void* client_specific_data, void* arg, bool success) {
			UNUSED(client_specific_data);
			UNUSED(request);
			bool validate = response->validate();
			if (!validate) {
				debug_print_error(__LOGTAG__, "crc fail !!!");
			}
			WorkData* data = static_cast<WorkData*>(arg);
			data->result = 1 + (success && validate);
			const conn_io_req_res::payload& payload = response->data;

			std::size_t total_size = payload.get_size();
			app_state.total_data_transferred.fetch_add(total_size);
			// std::size_t current_chunk = app_state.chunk_counter.fetch_add(1);
			// std::cout << "<";
			// debug_print(LOG_LEVEL_0, __LOGTAG__, "async returned [%d, %d] %s !!!", success, validate, payload.buffer.c_str());
			std::cout << "Total Requests: " << app_state.total_requests << "\tSuccess: " << app_state.total_successful_requests << " Finished: " << app_state.finished_requests << "\n";
			std::cout << "Total Avg Response Time: " << app_state.total_cumulative_response_time << " ms\n";
			finish_and_destroy_work(data);
		},
		0,
		app_state.connection_establishment_timeout,
		[](void* arg) {
			UNUSED(arg);
			app_state.exit_signal = true;
			uv_stop(app_state.loop);  // Stop the loop
			print_summary();
			std::cout << "\nFINISHED.\n\n";
		});

	if (result == 0) {
		app_state.total_requests++;
		app_state.summary_requests++;
	} else {
		std::cerr << "Failed to send request" << std::endl;
		app_state.exit_signal = true;
		uv_stop(app_state.loop);  // Stop the loop
		print_summary();
		std::cout << "\nFINISHED.\n\n";
	}
}

void multi_mode() {
	// Initialize timers
	uv_timer_init(app_state.loop, &app_state.request_timer);
	uv_timer_init(app_state.loop, &app_state.exit_timer);
	uv_timer_init(app_state.loop, &app_state.summary_timer);

	// Start the request timer immediately
	uv_timer_start(&app_state.request_timer, request_worker, 0, 0);

	// Start summary timer if specified
	if (app_state.summary_interval > 0) {
		uv_timer_start(&app_state.summary_timer, summary_timer_cb, app_state.summary_interval * 1000, app_state.summary_interval * 1000);
	}

	// Start exit timer if specified
	if (app_state.exit_after_seconds > 0) {
		uv_timer_start(&app_state.exit_timer, exit_after_delay, app_state.exit_after_seconds * 1000, 0);
	}
}

int main(int argc, char* argv[]) {
	std::cout << "qfist: v0.1\nSTARTED...\n";
	struct rlimit limit;
    if (getrlimit(RLIMIT_NOFILE, &limit) == 0) {
        std::cout << "fd Soft limit: " << limit.rlim_cur << std::endl;
        std::cout << "fd Hard limit: " << limit.rlim_max << std::endl;
    } else {
        std::cerr << "Error getting file descriptor limits" << std::endl;
    }
	long open_max = sysconf(_SC_OPEN_MAX);
    if (open_max != -1) {
        std::cout << "Maximum number of open file descriptors: " << open_max << std::endl;
    } else {
        std::cerr << "Error getting maximum file descriptors" << std::endl;
    }
	std::cout << "FD_SETSIZE: " << FD_SETSIZE << std::endl;

	// struct rlimit limit;
    // limit.rlim_cur = 4096; // Soft limit
    // limit.rlim_max = 4096; // Hard limit
    // setrlimit(RLIMIT_NOFILE, &limit);

	srand(static_cast<unsigned int>(time(nullptr)));  // Seed the random number generator

	// Initialize app_state
	initialize_app_state(app_state);
	if (app_state.requests.empty()) {
		std::cerr << "Failed to load requests from JSON file" << std::endl;
		return 0;
	}

	// Parse command-line arguments to potentially override defaults
	parse_arguments(argc, argv);
	print_parameters();

	app_state.loop = uv_default_loop();

	if (app_state.single_request) {
		single_mode();
	} else {
		multi_mode();
	}

	uv_run(app_state.loop, UV_RUN_DEFAULT);

	// Wait for the exit signal
	while (!app_state.exit_signal) {
		uv_sleep(3000);
	}

	// Cleanup
	cleanup();

	return 0;
}
