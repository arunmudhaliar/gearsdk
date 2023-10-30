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
#include "qtimer.hpp"

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

#undef __LOGTAG__
#define __LOGTAG__ "essentials"

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
                                         std::string& host, std::string& port, fs::path& rootDir);
};

// h3 structs
struct getorpost_reqdata {
    getorpost_reqdata(){
    }
    getorpost_reqdata(const std::string& path) :
        path(path) {
    }
    getorpost_reqdata(const std::string& path, const std::string& payload) :
        path(path), payload(payload) {
    }
    bool is_postrequest() const { return payload.size()>0; }
    void clear_payload() {
        payload.clear();
        reminder_payload.clear();
    }
    std::string path;
    std::string payload;
    std::vector<std::string> reminder_payload;
};

struct getorpost_response_data {
    getorpost_response_data( const std::string& payload) :
        payload(payload) {
    }
    void clear_payload() {
        payload = "{}";
    }
    std::string payload = "{}"; // empty response
};

#endif /* essentials_hpp */
