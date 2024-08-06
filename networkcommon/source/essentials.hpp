//
//  Copyright 2024 homenet25
//  essentials.hpp
//  networkcommon
//
//  Created by Arun A on 26/10/23.
//

#ifndef essentials_hpp
#define essentials_hpp

#include "../../common/qstring.h"
#include "../../common/sdktypes.hpp"
#include "../../common/timer.h"
#include "qtimer.hpp"

#include <map>
#include <netdb.h>
#include <netinet/in.h>
#include <string>
#include <vector>
// #include <quiche.h>
#include <rapidjson/document.h>
#include <rapidjson/rapidjson.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>
#include <time.h>
#include <zlib.h>

#undef __LOGTAG__
#define __LOGTAG__ "essentials"

#define EV_START_RECORD(timestamp_) unsigned long timestamp_ = timer::getCurrentTimeInMilliSec()
#define EV_PRINT_ELAPSED(timestamp_, tag, formatted_msg)                                            \
        DEBUG_PRINT_IMPORTANT(tag, formatted_msg, timer::getCurrentTimeInMilliSec() - timestamp_);
#define EV_PRINT_ELAPSED_AND_CLEAR(timestamp_, tag, formatted_msg)                              \
        DEBUG_PRINT_IMPORTANT(tag, formatted_msg, timer::getCurrentTimeInMilliSec() - timestamp_);  \
        timestamp_ = timer::getCurrentTimeInMilliSec()
#define EV_PRINT_IF_ELAPSED(timestamp_, tag, formatted_msg, warn_after_ms)                          \
    do {                                                                                            \
        unsigned long elapsed_since_##timestamp_ = timer::getCurrentTimeInMilliSec() - timestamp_;  \
        if (elapsed_since_##timestamp_ > warn_after_ms) {                                           \
            DEBUG_PRINT_IMPORTANT(tag, formatted_msg, elapsed_since_##timestamp_);                  \
        }                                                                                           \
    } while(false)
#define EV_PRINT_IF_ELAPSED_AND_CLEAR(timestamp_, tag, formatted_msg, warn_after_ms)                \
    EV_PRINT_IF_ELAPSED(timestamp_, tag, formatted_msg, warn_after_ms);                             \
    timestamp_ = timer::getCurrentTimeInMilliSec()

class qmutex;
class qmutexcondition {
   public:
	qmutexcondition();
	~qmutexcondition();
	int init(const std::string& name);
	int signal(const char* msg = nullptr);
	int broadcast(const char* msg = nullptr);
	int conditionWait(qmutex& qmutex, const char* msg);

   private:
	int tryInitIfNot();
	bool inited = false;
	pthread_cond_t cond;
	std::string name;
};

class qmutex {
   public:
	qmutex();
	~qmutex();
	int init(const std::string& name);
	int tryLock(const char* lockedBy, const char* msg = nullptr);
	int unLock(const char* msg = nullptr);
	inline pthread_mutex_t* getMutexInternal() { return &mutex; }
	void conditionalWait(const char* waiting_at);
	void block(const char* blockedBy = nullptr);
	void unBlock(const char* unblockedBy);

   private:
	int tryInitIfNot();
	bool inited = false;
	bool allowTask = true;
	pthread_mutex_t mutex;
	std::string name;
	std::string blockedBy = "none";
	std::string lockedBy = "none";
	std::string waitingAT = "none";
	std::string unblockedBy = "none";
	qmutexcondition condition;
	long blockCount = 0;
	int wanted = 0;
	static int log_flag;
};

class essentials {
   public:
	int init_essentials();
	static int32_t resolve_cmd_line_args(const char* tag, int32_t argc, const char* argv[], const qstring& version_string_, unsigned version_code_, qstring& host, qstring& port, qstring& mongodb_uri, fs::path& rootDir, qstring& redis_ip,
										 uint16_t& redis_port, qstring& zk_uri);

	static time_t get_time_local();
	static qstring get_time_local_tostring();
	static time_t get_time_utc();
	static qstring get_time_utc_string();
	static qstring get_time_utc_readable();
	static qstring get_time_utc_postgresql_format();

	static qstring get_sysname();
	static qstring get_device_name();
	static qstring get_device_arch();
	static qstring get_device_release_str();

	static int get_memory_info(int& currRealMem, int& peakRealMem, int& currVirtMem, int& peakVirtMem);
	static long long get_total_ram();
	static long long get_used_mem();
	static int get_process_used_mem();

	// fs
	static int get_all_child_folders(const fs::path& folder_path, std::vector<fs::path>& names);
    static int get_all_files(const fs::path& folder_path, std::vector<fs::path>& names, const qstring& extension = ".log");
    
	static unsigned long get_crc(const uint8_t* buffer, ssize_t len);

	static int get_addr_storage(struct sockaddr_storage& storage, const char* ip, const int port);
	static int update_port(struct sockaddr* sa, uint16_t newPort);
	static uLong mod_crc32_z(uLong adler, const Bytef* buf, z_size_t len);

	static bool get_json_string(rapidjson::Document& obj, qstring& output);
};

// h3 structs

struct conn_io_req_res {
	typedef struct header {
	   private:
		header(const header& header_) : name(header_.name), value(header_.value) {}
		header(const qstring& name_, const qstring& value_) : name(name_), value(value_) {}

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
		payload(const qstring& buffer_) : buffer(buffer_) {}
		~payload() {}
		qstring get_crc_string() const {
			unsigned long crc = get_crc_value();
			qstring crc_buffer = qstring::format_string("%lx", crc);
			return crc_buffer;
		}

		unsigned long get_crc_value() const {
			unsigned long crc_ = crc32(0L, Z_NULL, 0);
			crc_ = essentials::mod_crc32_z(crc_, (const unsigned char*) buffer.c_str(), buffer.length());
			return crc_;
		}
        unsigned long get_size() const {
            return buffer.length();
        }
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
	static conn_io_req_res* create(const qstring& path, const qstring& payload_) {
		conn_io_req_res::header* new_header = conn_io_req_res::header::create(":path", path);
		conn_io_req_res* new_rq_rs = DEBUG_NEW conn_io_req_res(*new_header, payload_);
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
	const payload& set_payload(const qstring& payload_) {
		data.buffer = payload_;
		return data;
	}
	const payload& append_to_payload(const uint8_t* str, int len) {
		data.buffer.run_printf((const char*) str, len);
		return data;
	}

	void clear_payload() { data.buffer.clear(); }

	header* add_or_get_header(const qstring& name_, const qstring& value_) {
		unsigned long crc = crc32(0L, Z_NULL, 0);
		crc = essentials::mod_crc32_z(crc, (const unsigned char*) name_.c_str(), name_.length());

		std::map<unsigned long, header*>::iterator it = headers.find(crc);
		if (it == headers.end()) {
			header* data = header::create(name_, value_);
			headers[crc] = data;
			return data;
		}
		return it->second;
	}

	header* get_header(const qstring& name_) const {
		unsigned long crc = crc32(0L, Z_NULL, 0);
		crc = essentials::mod_crc32_z(crc, (const unsigned char*) name_.c_str(), name_.length());
		std::map<unsigned long, header*>::const_iterator it = headers.find(crc);
		return it != headers.end() ? it->second : nullptr;
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
	qaddress(const qstring& ip_, uint16_t port_) : port(port_), ip(ip_) {}
	qaddress(const qstring& ip_, const qstring& port_) { set(ip_, port_); }
	qaddress(struct sockaddr& addr) { set(addr); }
	qaddress(struct sockaddr* addr) { set(*addr); }
	int set(struct sockaddr& addr) {
		char name[INET6_ADDRSTRLEN];
		char port[10];
		if (getnameinfo(&addr, sizeof(sockaddr), name, sizeof(name), port, sizeof(port), NI_NUMERICHOST | NI_NUMERICSERV) != 0) {
			DEBUG_PRINT_ERROR("qaddress", "Unable to parse sockaddr !!!");
			return -1;
		}
		return set(name, port);
	}
	int set(const qstring& ip_, const qstring& port_) {
		ip = ip_;
		int tmp = 0;
		if (gsdk::str2int(&tmp, port_.c_str(), 10) != gsdk::STR2INT_SUCCESS) {
			DEBUG_PRINT_ERROR("qaddress", "Unable to parse port !!! - %s", port_.c_str());
			return -1;
		}
		port = (uint16_t) tmp;
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
			if (gsdk::str2int(&v, n.c_str(), 10) != gsdk::STR2INT_SUCCESS) {
				DEBUG_PRINT_ERROR("qaddress", "Unable to serialise value !!! - %s", n.c_str());
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
			DEBUG_PRINT_ERROR("qaddress", "Unable to de-serialise buffer !!! - length = %d", len);
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
