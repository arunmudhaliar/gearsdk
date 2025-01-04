//
//  Copyright 2024 homenet25
//  essentials.hpp
//  networkcommon
//
//  Created by Arun A on 26/10/23.
//

#ifndef essentials_hpp
#define essentials_hpp

#include "../../common/qstring.hpp"
#include "../../common/sdktypes.hpp"
#include "../../common/timer.hpp"
#include "qtimer.hpp"
#if USE_LIBUV
#include "qtimer_uv.hpp"
#endif
#include <map>
#if PLATFORM != PLATFORM_WINDOWS
#include <netdb.h>
#include <netinet/in.h>
#include <unistd.h>
#else
// Prevent multiple inclusion of winsock2.h
#define _WINSOCKAPI_				// Prevent inclusion of winsock.h before winsock2.h
#include <winsock2.h>				// Include winsock2.h first
#include <ws2tcpip.h>				// For getnameinfo
#pragma comment(lib, "ws2_32.lib")	// Link ws2_32.lib
#include <windows.h>
#endif

#include <pthread.h>
#include <rapidjson/document.h>
#include <rapidjson/rapidjson.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>
#include <string>
#include <time.h>
#include <vector>
#include <zlib.h>

#undef __LOGTAG__
#define __LOGTAG__ "essentials"

#define DISABLE_PERF_READS (DEV_BUILD == 1)

#if DISABLE_PERF_READS
#define EV_START_RECORD(timestamp_)
#define EV_PRINT_IF_ELAPSED(timestamp_, tag, formatted_msg, warn_after_ms)
#define EV_PRINT_IF_ELAPSED_AND_CLEAR(timestamp_, tag, formatted_msg, warn_after_ms)
#else
#define EV_START_RECORD(timestamp_) unsigned long timestamp_ = timer::get_current_time_in_milli_sec()
#define EV_PRINT_IF_ELAPSED(timestamp_, tag, formatted_msg, warn_after_ms)                              \
	do {                                                                                                \
		unsigned long elapsed_since_##timestamp_ = timer::get_current_time_in_milli_sec() - timestamp_; \
		if (elapsed_since_##timestamp_ > warn_after_ms) {                                               \
			debug_print_important(tag, formatted_msg, elapsed_since_##timestamp_);                      \
		}                                                                                               \
	} while (false)
#define EV_PRINT_IF_ELAPSED_AND_CLEAR(timestamp_, tag, formatted_msg, warn_after_ms) \
	EV_PRINT_IF_ELAPSED(timestamp_, tag, formatted_msg, warn_after_ms);              \
	timestamp_ = timer::get_current_time_in_milli_sec()
#endif

class qmutex;
class qmutexcondition {
   public:
	qmutexcondition();
	~qmutexcondition();
	int init(const std::string& name);
	int signal(const char* msg = nullptr);
	int broadcast(const char* msg = nullptr);
	int condition_wait(qmutex& qmutex, const char* msg);

   private:
	int try_init_if_not();
	bool inited = false;
	pthread_cond_t cond;
	std::string name;
};

class qmutex {
   public:
	qmutex();
	~qmutex();
	int init(const std::string& name);
	int try_lock(const char* locked_by, const char* msg = nullptr);
	int unlock(const char* msg = nullptr);
	inline pthread_mutex_t* get_mutex_internal() { return &mutex; }
	void conditional_wait(const char* waiting_at);
	void block(const char* blocked_by = nullptr);
	void unblock(const char* unblocked_by);

   private:
	int try_init_if_not();
	bool inited = false;
	bool allow_task = true;
	pthread_mutex_t mutex;
	std::string name;
	std::string blocked_by = "none";
	std::string locked_by = "none";
	std::string waiting_at = "none";
	std::string unblocked_by = "none";
	qmutexcondition condition;
	long block_count = 0;
	int wanted = 0;
	static int log_flag;
};

class essentials {
   public:
	int init_essentials();
	static int32_t resolve_cmd_line_args(const char* tag, int32_t argc, const char* argv[], const qstring& version_string, unsigned version_code, qstring& host, qstring& port, qstring& mongodb_uri, fs::path& root_dir, qstring& redis_ip,
										 uint16_t& redis_port, qstring& zk_uri);

	static time_t get_time_local();
	static qstring get_time_local_tostring(time_t& local_time);
	static time_t get_time_utc();
	static qstring get_time_utc_string(time_t& utc_time);
	static qstring get_time_utc_readable(time_t& utc_time);
	static qstring get_time_utc_postgresql_format();

	static qstring get_sysname();
	static qstring get_device_name();
	static qstring get_device_arch();
	static qstring get_device_release_str();

	static int get_memory_info(int& curr_real_mem, int& peak_real_mem, int& curr_virt_mem, int& peak_virt_mem);
	static long long get_total_ram();
	static long long get_used_mem();
	static int get_process_used_mem();

	// fs
	static int get_all_child_folders(const fs::path& folder_path, std::vector<fs::path>& names);
	static int get_all_files(const fs::path& folder_path, std::vector<fs::path>& names, const qstring& extension = ".log");

	static unsigned long get_crc(const uint8_t* buffer, ssize_t len);

	static int get_addr_storage(struct sockaddr_storage& storage, const char* ip, const int PORT);
	static int update_port(struct sockaddr* sa, uint16_t new_port);
	static uLong mod_crc32_z(uLong adler, const Bytef* buf, z_size_t len);

	static bool get_json_string(rapidjson::Document& obj, qstring& output);
	static void sleep_for(int milliseconds) {
#if PLATFORM == PLATFORM_WINDOWS
		// Convert microseconds to milliseconds and call Sleep
		Sleep(milliseconds);
#else
		// Use usleep on Unix-based systems
		usleep(milliseconds * 1000);
#endif
	}

	static int set_non_blocking(int socket);
	static int generate_random_data(uint8_t* buffer, size_t length);

#if USE_LIBUV
	// uv cleanups
	static bool cleanup_and_destroy_uv_loop(uv_loop_t* loop);

   private:
	static void close_uv_handle_callback(uv_handle_t* handle, void* arg);
#endif
};

// h3 structs

struct conn_io_req_res {
	typedef struct header {
	   private:
		header(const header& header) : name(header.name), value(header.value) {}
		header(const qstring& name, const qstring& value) : name(name), value(value) {}

	   public:
		static header* create(const qstring& name, const qstring& value) {
			header* new_header = DEBUG_NEW header(name, value);
			return new_header;
		}
		~header() {}
		qstring name;
		qstring value;
	} header;

	typedef struct payload {
		payload() {}
		payload(const qstring& buffer) : buffer(buffer) {}
		~payload() {}
		qstring get_crc_string() const {
			unsigned long crc = get_crc_value();
			qstring crc_buffer = qstring::format_string("%lx", crc);
			return crc_buffer;
		}

		unsigned long get_crc_value() const {
			unsigned long crc = crc32(0L, Z_NULL, 0);
			crc = essentials::mod_crc32_z(crc, (const unsigned char*) buffer.c_str(), buffer.length());
			return crc;
		}
		unsigned long get_size() const { return buffer.length(); }
		qstring buffer;
	} payload;

	~conn_io_req_res() {
		for (auto h : headers) {
			GX_DELETE(h.second);
		}
	}

   private:
	conn_io_req_res() {}
	conn_io_req_res(const conn_io_req_res& data) {
		for (auto h : data.headers) {
			add_or_get_header(h.second->name, h.second->value);
		}
		set_payload(data.data.buffer);
	}

	conn_io_req_res(const header& header) { add_or_get_header(header.name, header.value); }
	conn_io_req_res(const header& header, const payload& payload) {
		add_or_get_header(header.name, header.value);
		set_payload(payload.buffer);
	}

	conn_io_req_res(const header& header, const qstring& payload) {
		add_or_get_header(header.name, header.value);
		set_payload(payload);
	}

   public:
	static conn_io_req_res* create() { return DEBUG_NEW conn_io_req_res(); }
	static conn_io_req_res* create(const qstring& path, const qstring& payload) {
		conn_io_req_res::header* new_header = conn_io_req_res::header::create(":path", path);
		conn_io_req_res* new_rq_rs = DEBUG_NEW conn_io_req_res(*new_header, payload);
		GX_DELETE(new_header);
		return new_rq_rs;
	}
	static conn_io_req_res* create(const qstring& path) {
		conn_io_req_res::header* new_header = conn_io_req_res::header::create(":path", path);
		conn_io_req_res* new_rq_rs = DEBUG_NEW conn_io_req_res(*new_header);
		GX_DELETE(new_header);
		return new_rq_rs;
	}
	const payload& get_payload() const { return data; }
	const payload& set_payload(const qstring& payload) {
		data.buffer = payload;
		return data;
	}
	const payload& append_to_payload(const uint8_t* str, int len) {
		data.buffer.run_printf((const char*) str, len);
		return data;
	}

	void clear_payload() { data.buffer.clear(); }

	header* add_or_get_header(const qstring& name, const qstring& value) {
		unsigned long crc = crc32(0L, Z_NULL, 0);
		crc = essentials::mod_crc32_z(crc, (const unsigned char*) name.c_str(), name.length());

		std::map<unsigned long, header*>::iterator it = headers.find(crc);
		if (it == headers.end()) {
			header* data = header::create(name, value);
			headers[crc] = data;
			return data;
		}
		return it->second;
	}

	header* get_header(const qstring& name) const {
		unsigned long crc = crc32(0L, Z_NULL, 0);
		crc = essentials::mod_crc32_z(crc, (const unsigned char*) name.c_str(), name.length());
		std::map<unsigned long, header*>::const_iterator it = headers.find(crc);
		return it != headers.end() ? it->second : nullptr;
	}

    void headers_to_json(qstring& json_string) {
        rapidjson::Document document;
        document.SetObject();

        // Get the allocator for the JSON object
        rapidjson::Document::AllocatorType& allocator = document.GetAllocator();

        for (const auto& [crc, header_ptr] : headers) {
            if (header_ptr) {
                // Create a JSON object for each header
                rapidjson::Value header_obj(rapidjson::kObjectType);
                header_obj.AddMember("name", rapidjson::Value(header_ptr->name.c_str(), allocator), allocator);
                header_obj.AddMember("value", rapidjson::Value(header_ptr->value.c_str(), allocator), allocator);

                // Add the header object to the main JSON object with the CRC as the key
                document.AddMember(
                    rapidjson::Value(std::to_string(crc).c_str(), allocator),
                    header_obj,
                    allocator
                );
            }
        }

        // Convert the JSON document to a string
        rapidjson::StringBuffer buffer;
        rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
        document.Accept(writer);
        json_string.copy(buffer.GetString());
    }
    
	bool is_postrequest() const { return data.buffer.length() > 0; }
	bool validate();
	bool has_crc_header();
	payload data;
	std::map<unsigned long, header*> headers;
};

struct qaddress {
	qaddress() {
		port = 0;
		ip.clear();
	}
	qaddress(const qstring& ip, uint16_t port) : port(port), ip(ip) {}
	qaddress(const qstring& ip, const qstring& port) { set(ip, port); }
	qaddress(struct sockaddr& addr) { set(addr); }
	qaddress(struct sockaddr* addr) { set(*addr); }
	qaddress(const qstring& address) {
		std::vector<qstring> parts;
		address.split(":", parts, false);
		if (parts.size() == 2) {
			ip = parts[0];
			int tmp = 0;
			if (gsdk::str2int(&tmp, parts[1].c_str(), parts[1].length(), 10) == gsdk::STR2INT_SUCCESS) {
				port = (uint16_t) tmp;
			} else {
				debug_print_error("qaddress", "Unable to parse port !!! - %s", parts[1].c_str());
			}
		} else {
			debug_print_error(__LOGTAG__, "Invalid request IP: %s", address.c_str());
		}
	}

	int set(struct sockaddr& addr) {
		char name[INET6_ADDRSTRLEN];
		char port[10];
		if (getnameinfo(&addr, sizeof(sockaddr), name, sizeof(name), port, sizeof(port), NI_NUMERICHOST | NI_NUMERICSERV) != 0) {
			debug_print_error("qaddress", "Unable to parse sockaddr !!!");
			return -1;
		}
		return set(name, port);
	}
	int set(const qstring& ip, const qstring& port) {
		this->ip = ip;
		int tmp = 0;
		if (gsdk::str2int(&tmp, port.c_str(), port.length(), 10) != gsdk::STR2INT_SUCCESS) {
			debug_print_error("qaddress", "Unable to parse port !!! - %s", port.c_str());
			return -1;
		}
		this->port = (uint16_t) tmp;
		return 0;
	}

	int serialise(qstring& buf) {
		uint8_t tmp[16];
		uint32_t index = 0;
		*((uint16_t*) (tmp + index)) = htons(port);
		index += sizeof(uint16_t);
		std::vector<qstring> array;
		ip.split(".", array);
		for (auto n : array) {
			int v = 0;
			if (gsdk::str2int(&v, n.c_str(), n.length(), 10) != gsdk::STR2INT_SUCCESS) {
				debug_print_error("qaddress", "Unable to serialise value !!! - %s", n.c_str());
				return -1;
			}
			*((uint8_t*) (tmp + index)) = (uint8_t) v;
			index += sizeof(uint8_t);
		}
		buf.bin_copy(tmp, index);
		return 0;
	}

	int deserialise(const uint8_t* buf, ssize_t len) {
		const uint8_t* tmp = buf;
		if (len < 6) {
			debug_print_error("qaddress", "Unable to de-serialise buffer !!! - length = %d", len);
			return -1;
		}
		uint32_t index = 0;
		port = ntohs(*((uint16_t*) (tmp + index)));
		index += sizeof(uint16_t);
		ip.clear();
		uint8_t v[4];
		for (int x = 0; x < 4; x++) {
			v[x] = *((uint8_t*) (tmp + index));
			index += sizeof(uint8_t);
		}
		ip.format("%d.%d.%d.%d", v[0], v[1], v[2], v[3]);
		return 0;
	}

	uint16_t port;
	qstring ip;
};
#endif /* essentials_hpp */
