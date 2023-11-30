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
#define EV_STOP_RECORD(timestamp_, log_level, tag, formatted_msg) DEBUG_PRINT(log_level, tag, formatted_msg, timer::getCurrentTimeInMilliSec() - timestamp_)

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
                                  const std::string& version_string_, unsigned version_code_,
                                         std::string& host, std::string& port, std::string& mongodb_uri, fs::path& rootDir);
    
    static time_t get_time_local();
    static qstring get_time_local_tostring();
    static time_t get_time_utc();
    static qstring get_time_utc_tostring();
    static qstring get_time_utc_postgresql_format();
    
    static qstring get_sysname();
    static qstring get_device_name();
    static qstring get_device_arch();
    static qstring get_device_release_str();
};

// h3 structs

struct conn_io_req_res {
    typedef struct header {
        header(const header& header_) :
            name(header_.name),
            value(header_.value) {
        }
        header(const qstring& name_, const qstring& value_) :
            name(name_),
            value(value_) {
        }
        ~header() {
        }
        qstring name;
        qstring value;
    } header;
    
    typedef struct payload {
        payload(const uint8_t* buf_, ssize_t len_) : len(len_) {
            buf = new uint8_t[len+1];
            memcpy(buf, buf_, len);
            buf[len]=0;
        }
        ~payload() {
            GX_DELETE_ARY(buf);
        }
        qstring get_crc() {
            unsigned long  crc = crc32(0L, Z_NULL, 0);
            crc = crc32_z(crc, (const unsigned char*)buf, len);
            qstring crc_buffer = qstring::format_string("%lx", crc);
            return crc_buffer;
        }
        ssize_t len = 0;
        uint8_t* buf = nullptr;
    } payload;
    
    conn_io_req_res(const conn_io_req_res& data) {
        for(auto h : data.headers) {
            add_header(h.second->name, h.second->value);
        }
        for(std::vector<payload*>::const_iterator it=data.payload_list.begin(); it!= data.payload_list.end(); it++) {
            payload* payload = *it;
            add_payload(payload->buf, payload->len);
        }
    }

    ~conn_io_req_res() {
        for(std::vector<payload*>::iterator it=payload_list.begin(); it!= payload_list.end(); it++) {
            GX_DELETE(*it);
        }
        for(auto h : headers) {
            GX_DELETE(h.second);
        }
    }
    
private:
    conn_io_req_res() {
    }
    conn_io_req_res(const header& header, const payload& payload) {
        add_header(header.name, header.value);
        add_payload(payload.buf, payload.len);
    }

public:
    static conn_io_req_res* create() {
        return new conn_io_req_res();
    }
    static conn_io_req_res* create(const qstring& path, const uint8_t* payload, ssize_t payload_len) {
        return new conn_io_req_res(conn_io_req_res::header(":path", path),
                         conn_io_req_res::payload(payload, payload_len));
    }
    payload* get_payload(int index) const {
        if (index>=(int)payload_list.size()) {
            return nullptr;
        }
        return payload_list[index];
    }
    payload* add_payload(uint8_t* buf_, ssize_t len_ ) {
        payload* data = new payload(buf_, len_);
        payload_list.push_back(data);
        return data;
    }

    header* add_header(const qstring& name_, const qstring& value_) {
        unsigned long  crc = crc32(0L, Z_NULL, 0);
        crc = crc32_z(crc, (const unsigned char*)name_.c_str(), name_.length());
        
        std::map<unsigned long, header*>::iterator it = headers.find(crc);
        if (it==headers.end()) {
            header* data = new header(name_, value_);
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
    
    bool is_postrequest() const { return payload_list.size()>0; }
    bool validate();
    std::vector<payload*> payload_list;
    std::map<unsigned long, header*> headers;
};

struct getorpost_reqdata {
    getorpost_reqdata(){
    }
    getorpost_reqdata(const std::string& path) :
        path(path) {
    }
    getorpost_reqdata(const std::string& path, const std::string& payload) :
        path(path), payload(payload) {
    }
    getorpost_reqdata(const getorpost_reqdata& data) {
        path = data.path;
        payload = data.payload;
    }
    ~getorpost_reqdata() {
    }
    bool is_postrequest() const { return payload.size()>0; }
    void clear_payload() {
        payload.clear();
        reminder_payload.clear();
    }
    
    bool validate();
    std::string crc;
    int crc_length=0;
    std::string path;
    std::string payload;
    std::vector<std::string> reminder_payload;
};

/*
struct getorpost_response_data {
    getorpost_response_data() {
    }
    getorpost_response_data( const std::string& payload) :
        payload(payload) {
    }
    void clear_payload() {
        payload = "{}";
    }
    std::string payload = "{}"; // empty response
    std::vector<Header> additional_headers;
};
 */

#endif /* essentials_hpp */
