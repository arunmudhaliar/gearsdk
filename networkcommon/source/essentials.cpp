//
//  essentials.cpp
//  networkcommon
//
//  Created by Arun A on 26/10/23.
//

#include "essentials.hpp"
#include <cstring>

#pragma region QMutex
qmutex::qmutex() {
    inited = false;
}
int qmutex::init(const std::string& name) {
    this->name = name+"_mutex";
    int ret_val = pthread_mutex_init(&mutex, nullptr);
    if (ret_val != 0) {
        DEBUG_PRINT_ERROR(__LOGTAG__, "%s mutex init has failed: %s - %d", name.c_str(), strerror (errno), errno);
    } else {
        inited = true;
        condition.init(name);
        DEBUG_PRINT(LOG_LEVEL_2, __LOGTAG__, "%s mutex init", name.c_str());
    }
    return ret_val;
}

qmutex::~qmutex(){
    if (inited) {
        pthread_mutex_destroy(&mutex);
    }
    DEBUG_PRINT(LOG_LEVEL_2, __LOGTAG__, "%s mutex destroyed", name.c_str());
}

int qmutex::tryInitIfNot() {
    int retVal = 0;
    if (!inited) {
        retVal = init("QMutex");
        if(retVal!=0) {
            DEBUG_PRINT_ERROR(__LOGTAG__, "tryInitIfNot failed (%s), %s", name.c_str());
        }
    }
    return retVal;
}

int qmutex::tryLock(const char* lockedBy, const char* msg) {
    int retVal = 0;
    if (!inited) {
        retVal = tryInitIfNot();
        if(retVal!=0) {
            return retVal;
        }
    }
    retVal = pthread_mutex_trylock(&mutex);
    if(retVal!=0) {
        DEBUG_PRINT_ERROR(__LOGTAG__, "failed to acuire lock(%s). Locked by %s, %s", name.c_str(), this->lockedBy.c_str(), (msg!=nullptr)? msg : "");
    } else {
        this->lockedBy = (lockedBy!=nullptr)? lockedBy : "";
    }
    return retVal;
}

int qmutex::unLock(const char* msg) {
    int retVal = 0;
    if (!inited) {
        retVal = tryInitIfNot();
        if(retVal!=0) {
            return retVal;
        }
    }
    retVal = pthread_mutex_unlock(&mutex);
    if (retVal != 0) {
        DEBUG_PRINT_ERROR(__LOGTAG__, "failed to unlock(%s), %s", name.c_str(), (msg!=nullptr)? msg : "");
    }
    return retVal;
}
void qmutex::conditionalWait(const char* waiting_at) {
    wanted++;
    waitingAT = (waiting_at!=nullptr) ? waiting_at : "";
    while (!allowTask) {
        DEBUG_PRINT_IMPORTANT2(__LOGTAG__, "%s Blocked by %s", name.c_str(), blockedBy.c_str());
        DEBUG_ASSERT(__LOGTAG__, (condition.conditionWait(*this, __FUNCTION__)==0), __FUNCTION__);
    }
//    allowTask = false;
    waitingAT = "";
    wanted--;
}

void qmutex::block(const char* blockedBy_) {
    // block close
    int result = tryLock(blockedBy_);
    if (result !=0 && blockCount==0 && allowTask == false) {
        // safe return;
        return;
    } else {
        if (result!=0 && wanted>1) {
            DEBUG_ASSERT(__LOGTAG__, false, __FUNCTION__);
        }
    }
    blockedBy = (blockedBy_==nullptr)?"???" : blockedBy_;
    allowTask = false;
    blockCount++;
    DEBUG_ASSERT(__LOGTAG__, (unLock()==0), __FUNCTION__);
    //
}

void qmutex::unBlock(const char* unblockedBy_) {
    int result = tryLock(__FUNCTION__);
    if (result !=0 && blockCount==0 && allowTask == false) {
        // safe return;
        allowTask = true;   // Not sure of this. Data race conditions can cause.
        unblockedBy = unblockedBy_!=nullptr ? unblockedBy_ : "";
        return;
    } else {
        if (result!=0 && wanted>1) {
            DEBUG_ASSERT(__LOGTAG__, false, __FUNCTION__);
        }
        allowTask = true;   // Not sure of this. Data race conditions can cause.
    }
//    DEBUG_ASSERT(__LOGTAG__, (tryLock(__FUNCTION__)==0), __FUNCTION__);
    allowTask = true;
    unblockedBy = unblockedBy_!=nullptr ? unblockedBy_ : "";
    blockCount--;
    DEBUG_ASSERT(__LOGTAG__, (condition.signal()==0), __FUNCTION__);
    DEBUG_ASSERT(__LOGTAG__, (unLock()==0), __FUNCTION__);
}
#pragma endregion QMutex

#pragma region QMutexCondition
qmutexcondition::qmutexcondition() {
    inited = false;
}
int qmutexcondition::init(const std::string& name) {
    this->name = name+"_cond";
    int retVal = pthread_cond_init(&cond, nullptr);
    if (retVal != 0) {
        DEBUG_PRINT_ERROR(__LOGTAG__, "%s condition init has failed: %s - %d", name.c_str(), strerror (errno), errno);
    } else {
        inited = true;
        DEBUG_PRINT(LOG_LEVEL_2, __LOGTAG__, "%s condtion init", name.c_str());
    }
    return retVal;
}

int qmutexcondition::tryInitIfNot() {
    int retVal = 0;
    if (!inited) {
        retVal = init("QMutexCondition");
        if(retVal!=0) {
            DEBUG_PRINT_ERROR(__LOGTAG__, "tryInitIfNot failed (%s), %s", name.c_str());
        }
    }
    return retVal;
}

qmutexcondition::~qmutexcondition() {
    if (inited) {
        pthread_cond_destroy(&cond);
    }
    DEBUG_PRINT(LOG_LEVEL_2, __LOGTAG__, "%s condition destroyed", name.c_str());
}

int qmutexcondition::signal(const char* msg) {
    int retVal = 0;
    if (!inited) {
        retVal = tryInitIfNot();
        if(retVal!=0) {
            return retVal;
        }
    }
    int sig_req = pthread_cond_signal(&cond);
    if (sig_req!=0) {
        DEBUG_PRINT_ERROR(__LOGTAG__, "SIGNAL failed %d on %s. %s", sig_req, name.c_str(), (msg!=nullptr)? msg : "");
    }
    return sig_req;
}
int qmutexcondition::broadcast(const char* msg) {
    int retVal = 0;
    if (!inited) {
        retVal = tryInitIfNot();
        if(retVal!=0) {
            return retVal;
        }
    }
    int broadcast_req = pthread_cond_broadcast(&cond);
    if (broadcast_req!=0) {
        DEBUG_PRINT_ERROR(__LOGTAG__, "BROADCAST failed %d on %s. %s", broadcast_req, name.c_str(), (msg!=nullptr)? msg : "");
    }
    return broadcast_req;
}

int qmutexcondition::conditionWait(qmutex& qmutex, const char* msg) {
    int retVal = 0;
    if (!inited) {
        retVal = tryInitIfNot();
        if(retVal!=0) {
            return retVal;
        }
    }
    int wait_req = pthread_cond_wait(&cond, qmutex.getMutexInternal());
    if (wait_req!=0) {
        DEBUG_PRINT_ERROR(__LOGTAG__, "COND_WAIT failed %d for %s, %s", wait_req, name.c_str(), (msg!=nullptr)? msg : "");
    }
    return wait_req;
}
#pragma endregion QMutexCondition


int32_t essentials::resolve_cmd_line_args(const char *tag, int32_t argc, const char * argv[],
                              const std::string& version_string_, unsigned version_code_,
                              std::string& host, std::string& port, fs::path& rootDir) {
    if (argc==2 && strcmp(argv[1], "--version")==0) {
        DEBUG_PRINT(LOG_LEVEL_0, tag, "version %s(%d)", version_string_.c_str(), version_code_);
        return -1;
    }

    DEBUG_PRINT(LOG_LEVEL_0, tag, "version %s(%d)", version_string_.c_str(), version_code_);
    PrintCommonInfo();
    if (argc==3) {
        host = argv[1];
        port = argv[2];
        const struct addrinfo hints = {
            .ai_family = PF_UNSPEC,
            .ai_socktype = SOCK_DGRAM,
            .ai_protocol = IPPROTO_UDP
        };
        struct addrinfo *peer = nullptr;
        if (getaddrinfo(host.c_str(), port.c_str(), &hints, &peer) != 0) {
            DEBUG_PRINT_ERROR(tag, "Failed to resolve host. Exiting !!!");
            DEBUG_PRINT_IMPORTANT2(tag, "Usage : <executable> 'ip address' 'port'");
            return -1;
        }
        if (peer) {
            freeaddrinfo(peer);
            peer = nullptr;
        }
    } else {
        DEBUG_PRINT_IMPORTANT2(tag, "Usage : <executable> 'ip address' 'port'. Ignore for debug builds running locally.");
    }
    DEBUG_PRINT_IMPORTANT(tag, "host:%s, port:%s", host.c_str(), port.c_str());

    fs::path executablePath(argc>0 ? argv[0] : "");
    rootDir = executablePath.parent_path();
    DEBUG_PRINT_IMPORTANT(tag, "Root dir : %s", rootDir.c_str());
    return 0;
}
