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

#endif /* essentials_hpp */
