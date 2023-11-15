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
#include "../../common/gxcrc32.h"
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
#include <quiche.h>
#include <zlib.h>

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
    static int32_t resolve_cmd_line_args(const char *tag, int32_t argc, const char * argv[],
                                  const std::string& version_string_, unsigned version_code_,
                                         std::string& host, std::string& port, std::string& mongodb_uri, fs::path& rootDir);
};

// h3 structs

struct conn_io_req_res {
    typedef struct header {
        header(const header& header_) {
            set_header(header_.name, header_.name_len, header_.value, header_.value_len);
        }
        header(const uint8_t *name_, uintptr_t name_len_, const uint8_t *value_, uintptr_t value_len_) {
            set_header(name_, name_len_, value_, value_len_);
        }
        void set_header(const uint8_t *name_, uintptr_t name_len_, const uint8_t *value_, uintptr_t value_len_) {
            name_len = name_len_;
            value_len = value_len_;
            GX_DELETE_ARY(name);
            GX_DELETE_ARY(value);
            name = new uint8_t[name_len];
            value = new uint8_t[value_len];
            memcpy(name, name_, name_len);
            memcpy(value, value_, value_len);
        }
        ~header() {
            GX_DELETE_ARY(name);
            GX_DELETE_ARY(value);
        }
      uint8_t *name = nullptr;
      uintptr_t name_len = 0;
      uint8_t *value = nullptr;
      uintptr_t value_len = 0;
    } header;
    
    typedef struct payload {
        payload(const uint8_t* buf_, ssize_t len_) : len(len_) {
            buf = new uint8_t[len];
            memcpy(buf, buf_, len);
        }
        ~payload() {
            GX_DELETE_ARY(buf);
        }
        ssize_t len = 0;
        uint8_t* buf = nullptr;
    } payload;
    
    conn_io_req_res(const conn_io_req_res& data) {
        for(auto h : data.headers) {
            add_header(h.second->name, h.second->name_len, h.second->value, h.second->value_len);
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
        add_header(header.name, header.name_len, header.value, header.value_len);
        add_payload(payload.buf, payload.len);
    }

public:
    static conn_io_req_res* create() {
        return new conn_io_req_res();
    }
    static conn_io_req_res* create(const uint8_t* path, ssize_t path_len, const uint8_t* payload, ssize_t payload_len) {
        return new conn_io_req_res(conn_io_req_res::header((const uint8_t*)":path", strlen(":path"), path, path_len),
                         conn_io_req_res::payload(payload, payload_len));
    }
    payload* get_payload(int index) const {
        if (index>=payload_list.size()) {
            return nullptr;
        }
        return payload_list[index];
    }
    void add_payload(uint8_t* buf_, ssize_t len_ ) {
        payload* data = new payload(buf_, len_);
        payload_list.push_back(data);
    }

    void add_header(const uint8_t *name_, uintptr_t name_len_, const uint8_t *value_, uintptr_t value_len_) {
        unsigned long  crc = crc32(0L, Z_NULL, 0);
        crc = crc32_z(crc, (const unsigned char*)name_, name_len_);
        
        if (headers.find(crc)==headers.end()) {
            header* data = new header(name_, name_len_, value_, value_len_);
            headers[crc] = data;
        }
    }
    
    header* get_header(const uint8_t *name_, uintptr_t name_len_) const {
        unsigned long  crc = crc32(0L, Z_NULL, 0);
        crc = crc32_z(crc, (const unsigned char*)name_, name_len_);
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
