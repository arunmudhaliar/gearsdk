//
//  essentials.hpp
//  networkcommon
//
//  Created by Arun A on 26/10/23.
//

#ifndef essentials_hpp
#define essentials_hpp

#include <string>
#include "../../common/sdktypes.hpp"
#include "../../common/qstring.h"
#include "qtimer.hpp"
#include "../../common/timer.h"
#include <map>

#if PLATFORM == PLATFORM_MAC
namespace fs = std::__fs::filesystem;
#elif PLATFORM == PLATFORM_LINUX
#include <linux/limits.h>
namespace fs = std::filesystem;
#else
namespace fs = std::__fs::filesystem;
#endif

#include <netdb.h>
#include <filesystem>
#include <vector>
//#include <quiche.h>
#include <zlib.h>
#include <time.h>

#undef __LOGTAG__
#define __LOGTAG__ "essentials"

#define EV_START_RECORD(timestamp_)  unsigned long timestamp_ = timer::getCurrentTimeInMilliSec()
#define EV_STOP_RECORD(timestamp_, tag, formatted_msg, warn_after_ms) \
unsigned long elapsed_since_##timestamp_ = timer::getCurrentTimeInMilliSec() - timestamp_; \
if (elapsed_since_##timestamp_ > warn_after_ms) \
{ \
    DEBUG_PRINT_WARN(tag, formatted_msg, elapsed_since_##timestamp_); \
}

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
    pthread_cond_t  cond;
    std::string name;
};

class qmutex {
public:
    qmutex();
    ~qmutex();
    int init(const std::string& name);
    int tryLock(const char* lockedBy, const char* msg = nullptr);
    int unLock(const char* msg = nullptr);
    inline pthread_mutex_t* getMutexInternal() {
        return &mutex;
    }
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
};

class essentials {
public:
    int init_essentials();
    static int32_t resolve_cmd_line_args(const char *tag, int32_t argc, const char * argv[],
                                  const qstring& version_string_, unsigned version_code_,
                                         qstring& host, qstring& port, qstring& mongodb_uri, fs::path& rootDir,
                                         qstring& redis_ip, int& redis_port);
    
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
};

// h3 structs

struct conn_io_req_res {
    typedef struct header {
    private:
        header(const header& header_) :
            name(header_.name),
            value(header_.value) {
        }
        header(const qstring& name_, const qstring& value_) :
            name(name_),
            value(value_) {
        }
    public:
        static header* create(const qstring& name, const qstring& value) {
            header* new_header = DEBUG_NEW header(name, value);
            return new_header;
        }
        ~header() {
        }
        qstring name;
        qstring value;
    } header;
    
    typedef struct payload {
        payload(){}
        payload(const qstring& buffer_) : buffer(buffer_) {
        }
        ~payload() {
        }
        qstring get_crc_string() const {
            unsigned long  crc = get_crc_value();
            qstring crc_buffer = qstring::format_string("%lx", crc);
            return crc_buffer;
        }
        
        unsigned long get_crc_value() const {
            unsigned long  crc_ = crc32(0L, Z_NULL, 0);
            crc_ = crc32_z(crc_, (const unsigned char*)buffer.c_str(), buffer.length());
            return crc_;
        }
        qstring buffer;
    } payload;

    ~conn_io_req_res() {
        for(auto h : headers) {
            GX_DELETE(h.second);
        }
    }
    
private:
    conn_io_req_res() {
    }
    conn_io_req_res(const conn_io_req_res& data) {
        for(auto h : data.headers) {
            add_or_get_header(h.second->name, h.second->value);
        }
        set_payload(data.data.buffer);
    }
    
    conn_io_req_res(const header& header) {
        add_or_get_header(header.name, header.value);
    }
    conn_io_req_res(const header& header, const payload& payload) {
        add_or_get_header(header.name, header.value);
        set_payload(payload.buffer);
    }
    
    conn_io_req_res(const header& header, const qstring& payload) {
        add_or_get_header(header.name, header.value);
        set_payload(payload);
    }

public:
    static conn_io_req_res* create() {
        return DEBUG_NEW conn_io_req_res();
    }
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
    const payload& get_payload() const {
        return data;
    }
    const payload& set_payload(const qstring& payload_) {
        data.buffer = payload_;
        return data;
    }
    const payload& append_to_payload(const qstring& payload_) {
        data.buffer += payload_;
        return data;
    }

    header* add_or_get_header(const qstring& name_, const qstring& value_) {
        unsigned long  crc = crc32(0L, Z_NULL, 0);
        crc = crc32_z(crc, (const unsigned char*)name_.c_str(), name_.length());
        
        std::map<unsigned long, header*>::iterator it = headers.find(crc);
        if (it==headers.end()) {
            header* data = header::create(name_, value_);
            headers[crc] = data;
            return data;
        }
        return it->second;
    }
    
    header* get_header(const qstring& name_) const {
        unsigned long  crc = crc32(0L, Z_NULL, 0);
        crc = crc32_z(crc, (const unsigned char*)name_.c_str(), name_.length());
        std::map<unsigned long, header*>::const_iterator it = headers.find(crc);
        return it!=headers.end() ? it->second : nullptr;
    }
    
    bool is_postrequest() const { return data.buffer.length()>0; }
    bool validate();
    bool has_crc_header();
    payload data;
    std::map<unsigned long, header*> headers;
};

#endif /* essentials_hpp */
