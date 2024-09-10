//
//  Copyright 2024 homenet25
//  essentials.cpp
//  networkcommon
//
//  Created by Arun A on 26/10/23.
//

#include "essentials.hpp"

#include <arpa/inet.h>
#include <cstring>
#if PLATFORM == PLATFORM_LINUX
#include <sys/sysinfo.h>
#include <sys/types.h>
#endif

using namespace gsdk;

// MARK: - QMutex
int qmutex::log_flag = 0;

qmutex::qmutex() {
	inited = false;
}
int qmutex::init(const std::string& name) {
	this->name = name + "_mutex";
	int ret_val = pthread_mutex_init(&mutex, nullptr);
	if (ret_val != 0) {
		debug_print_error(__LOGTAG__, "%s mutex init has failed: %s - %d", name.c_str(), strerror(errno), errno);
	} else {
		inited = true;
		condition.init(name);
		debug_print(LOG_LEVEL_5, __LOGTAG__, "%s mutex init", name.c_str());
	}
	return ret_val;
}

qmutex::~qmutex() {
	if (inited) {
		pthread_mutex_destroy(&mutex);
	}
	debug_print(LOG_LEVEL_5, __LOGTAG__, "%s mutex destroyed", name.c_str());
}

int qmutex::try_init_if_not() {
	int ret_val = 0;
	if (!inited) {
		ret_val = init("QMutex");
		if (ret_val != 0) {
			debug_print_error(__LOGTAG__, "try_init_if_not failed (%s), %s", name.c_str());
		}
	}
	return ret_val;
}

int qmutex::try_lock(const char* locked_by, const char* msg) {
	int ret_val = 0;
	if (!inited) {
		ret_val = try_init_if_not();
		if (ret_val != 0) {
			return ret_val;
		}
	}
	ret_val = pthread_mutex_trylock(&mutex);
	if (ret_val != 0 && qmutex::log_flag) {
		debug_print_error(__LOGTAG__, "failed to acuire lock(%s). Locked by %s, %s", name.c_str(), this->locked_by.c_str(), (msg != nullptr) ? msg : "");
	} else {
		this->locked_by = (locked_by != nullptr) ? locked_by : "";
	}
	return ret_val;
}

int qmutex::unlock(const char* msg) {
	int ret_val = 0;
	if (!inited) {
		ret_val = try_init_if_not();
		if (ret_val != 0) {
			return ret_val;
		}
	}
	ret_val = pthread_mutex_unlock(&mutex);
	if (ret_val != 0) {
		debug_print_error(__LOGTAG__, "failed to unlock(%s), %s", name.c_str(), (msg != nullptr) ? msg : "");
	}
	return ret_val;
}
void qmutex::conditional_wait(const char* waiting_at) {
	wanted++;
	this->waiting_at = (waiting_at != nullptr) ? waiting_at : "";
	while (!allow_task) {
		debug_print_important2(__LOGTAG__, "%s Blocked by %s", name.c_str(), blocked_by.c_str());
		DEBUG_ASSERT(__LOGTAG__, (condition.condition_wait(*this, __FUNCTION__) == 0), __FUNCTION__);
	}
	//    allowTask = false;
	this->waiting_at = "";
	wanted--;
}

void qmutex::block(const char* blocked_by) {
	// block close
	int result = try_lock(blocked_by);
	if (result != 0 && block_count == 0 && allow_task == false) {
		// safe return;
		return;
	} else {
		if (result != 0 && wanted > 1) {
			DEBUG_ASSERT(__LOGTAG__, false, __FUNCTION__);
		}
	}
	this->blocked_by = (blocked_by == nullptr) ? "???" : blocked_by;
	allow_task = false;
	block_count++;
	DEBUG_ASSERT(__LOGTAG__, (unlock() == 0), __FUNCTION__);
	//
}

void qmutex::unblock(const char* unblocked_by) {
	int result = try_lock(__FUNCTION__);
	if (result != 0 && block_count == 0 && allow_task == false) {
		// safe return;
		allow_task = true;	// Not sure of this. Data race conditions can cause.
		this->unblocked_by = unblocked_by != nullptr ? unblocked_by : "";
		return;
	} else {
		if (result != 0 && wanted > 1) {
			DEBUG_ASSERT(__LOGTAG__, false, __FUNCTION__);
		}
		allow_task = true;	// Not sure of this. Data race conditions can cause.
	}
	//    DEBUG_ASSERT(__LOGTAG__, (try_lock(__FUNCTION__)==0), __FUNCTION__);
	allow_task = true;
	this->unblocked_by = unblocked_by != nullptr ? unblocked_by : "";
	block_count--;
	DEBUG_ASSERT(__LOGTAG__, (condition.signal() == 0), __FUNCTION__);
	DEBUG_ASSERT(__LOGTAG__, (unlock() == 0), __FUNCTION__);
}

// MARK: - QMutexCondition
qmutexcondition::qmutexcondition() {
	inited = false;
}
int qmutexcondition::init(const std::string& name) {
	this->name = name + "_cond";
	int ret_val = pthread_cond_init(&cond, nullptr);
	if (ret_val != 0) {
		debug_print_error(__LOGTAG__, "%s condition init has failed: %s - %d", name.c_str(), strerror(errno), errno);
	} else {
		inited = true;
		debug_print(LOG_LEVEL_5, __LOGTAG__, "%s condtion init", name.c_str());
	}
	return ret_val;
}

int qmutexcondition::try_init_if_not() {
	int ret_val = 0;
	if (!inited) {
		ret_val = init("QMutexCondition");
		if (ret_val != 0) {
			debug_print_error(__LOGTAG__, "try_init_if_not failed (%s), %s", name.c_str());
		}
	}
	return ret_val;
}

qmutexcondition::~qmutexcondition() {
	if (inited) {
		pthread_cond_destroy(&cond);
	}
	debug_print(LOG_LEVEL_5, __LOGTAG__, "%s condition destroyed", name.c_str());
}

int qmutexcondition::signal(const char* msg) {
	int ret_val = 0;
	if (!inited) {
		ret_val = try_init_if_not();
		if (ret_val != 0) {
			return ret_val;
		}
	}
	int sig_req = pthread_cond_signal(&cond);
	if (sig_req != 0) {
		debug_print_error(__LOGTAG__, "SIGNAL failed %d on %s. %s", sig_req, name.c_str(), (msg != nullptr) ? msg : "");
	}
	return sig_req;
}
int qmutexcondition::broadcast(const char* msg) {
	int ret_val = 0;
	if (!inited) {
		ret_val = try_init_if_not();
		if (ret_val != 0) {
			return ret_val;
		}
	}
	int broadcast_req = pthread_cond_broadcast(&cond);
	if (broadcast_req != 0) {
		debug_print_error(__LOGTAG__, "BROADCAST failed %d on %s. %s", broadcast_req, name.c_str(), (msg != nullptr) ? msg : "");
	}
	return broadcast_req;
}

int qmutexcondition::condition_wait(qmutex& qmutex, const char* msg) {
	int ret_val = 0;
	if (!inited) {
		ret_val = try_init_if_not();
		if (ret_val != 0) {
			return ret_val;
		}
	}
	int wait_req = pthread_cond_wait(&cond, qmutex.get_mutex_internal());
	if (wait_req != 0) {
		debug_print_error(__LOGTAG__, "COND_WAIT failed %d for %s, %s", wait_req, name.c_str(), (msg != nullptr) ? msg : "");
	}
	return wait_req;
}
// END MARK: -

int32_t essentials::resolve_cmd_line_args(const char* tag, int32_t argc, const char* argv[], const qstring& version_string, unsigned version_code, qstring& host, qstring& port, qstring& mongodb_uri, fs::path& root_dir, qstring& redis_ip,
										  uint16_t& redis_port, qstring& zk_uri) {
	if (argc == 2 && strcmp(argv[1], "--version") == 0) {
		debug_print(LOG_LEVEL_0, tag, "version %s(%d)", version_string.c_str(), version_code);
		debug_print_important2(tag, "Usage : <executable> '--h <ip address>' '--p <port>' '--db <mongodb uri_string>' '--certdir <certpath>' '--rh <redis ip>' '--rp <redis port>'");
		return -1;
	}

	debug_print(LOG_LEVEL_0, tag, "version %s(%d)", version_string.c_str(), version_code);

	if (argc % 2 == 0) {
		debug_print_error(tag, "Failed to resolve arguments. Exiting !!!");
		debug_print_important2(tag, "Usage : <executable> '--h <ip address>' '--p <port>' '--db <mongodb uri_string>' '--certdir <certpath>' '--rh <redis ip>' '--rp <redis port>' '--zk <zk uri_string>'");
		return -1;
	}

	// default to root
	root_dir = "";
	if (argc > 0) {
		fs::path executable_path(argv[0]);
		root_dir = executable_path.parent_path();
	}
	//

	int pairs = (argc - 1) / 2;
	for (int x = 0; x < pairs; x++) {
		const char* lf = argv[1 + x * 2 + 0];
		const char* rg = argv[1 + x * 2 + 1];

		if (strcmp(lf, "--h") == 0) {
			host = rg;
		} else if (strcmp(lf, "--p") == 0) {
			port = rg;
		} else if (strcmp(lf, "--certdir") == 0) {
			root_dir = fs::path(rg);
		} else if (strcmp(lf, "--db") == 0) {
			mongodb_uri = rg;
		} else if (strcmp(lf, "--rh") == 0) {
			redis_ip = rg;
		} else if (strcmp(lf, "--rp") == 0) {
			int tmp = 0;
			if (gsdk::str2int(&tmp, rg, strlen(rg), 10) != gsdk::STR2INT_SUCCESS) {
				debug_print_error(tag, "Unable to parse redis port, defaulting to %d !!!", tmp);
			}
			redis_port = tmp;
		} else if (strcmp(lf, "--zk") == 0) {
			zk_uri = rg;
		}
	}

	// check host and port
	const struct addrinfo HINTS = {.ai_family = PF_UNSPEC, .ai_socktype = SOCK_DGRAM, .ai_protocol = IPPROTO_UDP};
	struct addrinfo* peer = nullptr;
	if (getaddrinfo(host.c_str(), port.c_str(), &HINTS, &peer) != 0) {
		debug_print_error(tag, "Failed to resolve host. Exiting !!!");
		debug_print_important2(tag, "Usage : <executable> '--h <ip address>' '--p <port>' '--db <mongodb uri_string>' '--certdir <certpath>' '--rh <redis ip>' '--rp <redis port>' '--zk <zk uri_string>'");
		return -1;
	}
	if (peer) {
		freeaddrinfo(peer);
		peer = nullptr;
	}
	debug_print_important(tag, "server %s:%s, mongodb_uri %s, redis %s:%d, zk_uri %s", host.c_str(), port.c_str(), mongodb_uri.c_str(), redis_ip.c_str(), redis_port, zk_uri.c_str());
	//

	debug_print_important(tag, "Root dir : %s", root_dir.c_str());
	return 0;
}

qstring essentials::get_sysname() {
	return qstring(device::device_details.sysname);
}

qstring essentials::get_device_name() {
	return qstring(device::device_details.nodename);
}
qstring essentials::get_device_arch() {
	return qstring(device::device_details.machine);
}
qstring essentials::get_device_release_str() {
	return qstring(device::device_details.release);
}

time_t essentials::get_time_local() {
	time_t now = time(NULL);
	struct tm* tm = localtime(&now);
	time_t local_time = tm->tm_sec + tm->tm_min * 60 + tm->tm_hour * 3600 + tm->tm_yday * 86400 + (tm->tm_year - 70) * 31536000 + ((tm->tm_year - 69) / 4) * 86400 - ((tm->tm_year - 1) / 100) * 86400 + ((tm->tm_year + 299) / 400) * 86400;
	return local_time;
}

qstring essentials::get_time_local_tostring() {
	time_t local_time = get_time_local();
	qstring tmp(ctime(&local_time));
	return tmp;
}

time_t essentials::get_time_utc() {
	time_t now;
	time(&now);
	struct tm* tm = gmtime(&now);
	time_t utc_time = tm->tm_sec + tm->tm_min * 60 + tm->tm_hour * 3600 + tm->tm_yday * 86400 + (tm->tm_year - 70) * 31536000 + ((tm->tm_year - 69) / 4) * 86400 - ((tm->tm_year - 1) / 100) * 86400 + ((tm->tm_year + 299) / 400) * 86400;
	return utc_time;
}

qstring essentials::get_time_utc_string() {
	time_t utc_time = get_time_utc();
	qstring tmp = qstring::format_string("%ld", utc_time);
	return tmp;
}

qstring essentials::get_time_utc_readable() {
	time_t utc_time = get_time_utc();
	qstring tmp(ctime(&utc_time));
	return tmp;
}

qstring essentials::get_time_utc_postgresql_format() {
	time_t now;
	time(&now);
	struct tm* tm = gmtime(&now);
	char timestamp_str[32];
	snprintf(timestamp_str, sizeof(timestamp_str), "%04d-%02d-%02d %02d:%02d:%02d", tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec);
	return qstring(timestamp_str);
}

/*
 * Measures the current (and peak) resident and virtual memories
 * usage of your linux C process, in kB
 */
int essentials::get_memory_info(int& curr_real_mem, int& peak_real_mem, int& curr_virt_mem, int& peak_virt_mem) {
	// stores each word in status file
	char buffer[1024] = "";
	// linux file contains this-process info
	FILE* file = fopen("/proc/self/status", "r");
	if (file == nullptr) {
		curr_real_mem = peak_real_mem = curr_virt_mem = peak_virt_mem = 0;
		return -1;
	}
	// read the entire file
	while (fscanf(file, " %1023s", buffer) == 1) {
		if (strcmp(buffer, "VmRSS:") == 0) {
			fscanf(file, " %d", &curr_real_mem);
		}
		if (strcmp(buffer, "VmHWM:") == 0) {
			fscanf(file, " %d", &peak_real_mem);
		}
		if (strcmp(buffer, "VmSize:") == 0) {
			fscanf(file, " %d", &curr_virt_mem);
		}
		if (strcmp(buffer, "VmPeak:") == 0) {
			fscanf(file, " %d", &peak_virt_mem);
		}
	}
	fclose(file);
	return 0;
}

int essentials::get_process_used_mem() {
	int curr_real_mem = 0;
#if PLATFORM == PLATFORM_LINUX
	// https://itecnote.com/tecnote/macos-memory-used-by-a-process-under-mac-os-x/
	//  stores each word in status file
	char buffer[1024] = "";
	// linux file contains this-process info
	FILE* file = fopen("/proc/self/status", "r");
	if (file == nullptr) {
		return curr_real_mem;
	}
	// read the entire file
	while (fscanf(file, " %1023s", buffer) == 1) {
		if (strcmp(buffer, "VmRSS:") == 0) {
			fscanf(file, " %d", &curr_real_mem);
			break;
		}
	}
	fclose(file);
#endif
	return curr_real_mem;
}

long long essentials::get_total_ram() {
#if PLATFORM == PLATFORM_LINUX
	struct sysinfo mem_info;
	if (sysinfo(&mem_info) == -1)
		return 0;
	//    uint64_t total_ram = ((uint64_t)mem_info.totalram * mem_info.mem_unit)/1024;
	long long total_phys_mem = mem_info.totalram;
	// Multiply in next statement to avoid int overflow on right hand side...
	total_phys_mem *= mem_info.mem_unit;
	return total_phys_mem / (1024 * 1024);
#else
	return 0;
#endif
}

long long essentials::get_used_mem() {
#if PLATFORM == PLATFORM_LINUX
	struct sysinfo mem_info;
	if (sysinfo(&mem_info) == -1)
		return 0;
	long long phys_mem_used = mem_info.totalram - mem_info.freeram;
	// Multiply in next statement to avoid int overflow on right hand side...
	phys_mem_used *= mem_info.mem_unit;
	return phys_mem_used / (1024 * 1024);
#else
	return 0;
#endif
}

int essentials::get_all_child_folders(const fs::path& folder_path, std::vector<fs::path>& names) {
	try {
		for (const auto& entry : fs::directory_iterator(folder_path)) {
			if (fs::is_directory(entry.path())) {
				names.push_back(entry.path());
			}
		}
	} catch (const fs::filesystem_error& e) {
		debug_print_error(__LOGTAG__, "Error accessing the folder: %s", e.what());
		return 1;
	}
	return 0;
}

int essentials::get_all_files(const fs::path& folder_path, std::vector<fs::path>& names, const qstring& extension) {
	try {
		for (const auto& entry : fs::directory_iterator(folder_path)) {
			if (fs::is_regular_file(entry.path()) && entry.path().extension() == extension.c_str()) {
				names.push_back(entry.path());
			}
		}
	} catch (const fs::filesystem_error& e) {
		debug_print_error(__LOGTAG__, "Error accessing the folder: %s", e.what());
		return 1;
	}
	return 0;
}

int essentials::get_addr_storage(struct sockaddr_storage& storage, const char* ip, const int PORT) {
	struct sockaddr_in addr;
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(PORT);
	if (inet_pton(AF_INET, ip, &(addr.sin_addr)) <= 0) {
		perror("Invalid IP address");
		return -1;
	}
	memset(&storage, 0, sizeof(storage));
	memcpy(&storage, &addr, sizeof(addr));
	return 0;
}

int essentials::update_port(struct sockaddr* sa, uint16_t new_port) {
	if (sa->sa_family == AF_INET) {
		// IPv4
		struct sockaddr_in* sa_in = (struct sockaddr_in*) sa;
		sa_in->sin_port = htons(new_port);
		return 0;
	} else if (sa->sa_family == AF_INET6) {
		// IPv6
		struct sockaddr_in6* sa_in6 = (struct sockaddr_in6*) sa;
		sa_in6->sin6_port = htons(new_port);
		return 0;
	} else {
		// Unknown address family or unsupported type
		fprintf(stderr, "Unsupported address family\n");
	}
	return -1;
}

uLong essentials::mod_crc32_z(uLong adler, const Bytef* buf, z_size_t len) {
#if PLATFORM == PLATFORM_ANDROID
	return crc32(adler, buf, (unsigned int) len);
#else
	return crc32_z(adler, buf, len);
#endif
}

unsigned long essentials::get_crc(const uint8_t* buffer, ssize_t len) {
	unsigned long crc = crc32(0L, Z_NULL, 0);
	crc = mod_crc32_z(crc, (const unsigned char*) buffer, len);
	return crc;
}

bool essentials::get_json_string(rapidjson::Document& obj, qstring& output) {
	// Convert JSON document to string
	rapidjson::StringBuffer buffer;
	rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
	obj.Accept(writer);
	output = buffer.GetString();
	return true;
}

void essentials::close_uv_handle_callback(uv_handle_t* handle, void* arg) {
	if (!uv_is_closing(handle)) {
		uv_close(handle, nullptr);
	}
}

bool essentials::cleanup_and_destroy_uv_loop(uv_loop_t* loop) {
	if (loop == nullptr) {
		debug_print_important(__LOGTAG__, "loop pointer is null, Nothing to delete. returning !!!");
		return true;
	}
	//	uv_print_all_handles(loop, stderr);
	uv_walk(loop, close_uv_handle_callback, nullptr);
	uv_run(loop, UV_RUN_NOWAIT);  // Run pending callbacks
	if (uv_loop_alive(loop)) {
		debug_print_error(__LOGTAG__, "The loop still has active handles!");
		return false;
	}
	//	int result = uv_loop_close(loop);
	//	if (result == UV_EBUSY) {
	//		debug_print_warn(__LOGTAG__, "uv_loop_close returned UV_EBUSY !!!");
	//	}
	// Note: No need to call uv_loop_close, since uv_loop_delete call uv_loop_close internally.
	uv_loop_delete(loop);
	return true;
}

bool conn_io_req_res::has_crc_header() {
	header* crc_header = get_header("crc");
	return crc_header != nullptr;
}

bool conn_io_req_res::validate() {
	header* crc_header = get_header("crc");
	if (crc_header == nullptr) {
		return false;
	}

	const payload& payload = get_payload();
	if (payload.buffer.length() == 0) {
		// check if its get method or not
		header* method_header = get_header(":method");
		if (method_header == nullptr) {
			return false;
		} else if (crc_header->value == "0") {	// for get methods there wont be any payload and the crc will be zero.
			return true;
		}
		return false;
	}
	unsigned long crc = payload.get_crc_value();
	unsigned long crc_from_req = 0;
	sscanf((const char*) crc_header->value.c_str(), "%8lx", &crc_from_req);

	if (crc_from_req != crc) {
		debug_print_error(__LOGTAG__, "CRC validation Error %lu != %lu, payload sz %lu, crc_as_string %s", crc, crc_from_req, payload.buffer.length(), crc_header->value.c_str());
		//        assert(crc_from_req == crc_);
	}
	//     debug_print_important(__LOGTAG__, "CRC validation %lu == %lu, payload sz %lu, crc_as_string %s",
	//        crc_, crc_from_req, payload->len, crc_header->value);
	return crc_from_req == crc;
}
