//
//  essentials.cpp
//  networkcommon
//
//  Created by Arun A on 26/10/23.
//

#include "essentials.hpp"
#include <cstring>

using namespace gsdk;

#pragma region QMutex
qmutex::qmutex() {
    inited = false;
}
int qmutex::init(const std::string& name) {
    this->name = name + "_mutex";
    int ret_val = pthread_mutex_init(&mutex, nullptr);
    if (ret_val != 0) {
        DEBUG_PRINT_ERROR(__LOGTAG__, "%s mutex init has failed: %s - %d", name.c_str(), strerror(errno), errno);
    }
    else {
        inited = true;
        condition.init(name);
        DEBUG_PRINT(LOG_LEVEL_2, __LOGTAG__, "%s mutex init", name.c_str());
    }
    return ret_val;
}

qmutex::~qmutex() {
    if (inited) {
        pthread_mutex_destroy(&mutex);
    }
    DEBUG_PRINT(LOG_LEVEL_2, __LOGTAG__, "%s mutex destroyed", name.c_str());
}

int qmutex::tryInitIfNot() {
    int retVal = 0;
    if (!inited) {
        retVal = init("QMutex");
        if (retVal != 0) {
            DEBUG_PRINT_ERROR(__LOGTAG__, "tryInitIfNot failed (%s), %s", name.c_str());
        }
    }
    return retVal;
}

int qmutex::tryLock(const char* lockedBy, const char* msg) {
    int retVal = 0;
    if (!inited) {
        retVal = tryInitIfNot();
        if (retVal != 0) {
            return retVal;
        }
    }
    retVal = pthread_mutex_trylock(&mutex);
    if (retVal != 0) {
        DEBUG_PRINT_ERROR(__LOGTAG__, "failed to acuire lock(%s). Locked by %s, %s", name.c_str(), this->lockedBy.c_str(), (msg != nullptr) ? msg : "");
    }
    else {
        this->lockedBy = (lockedBy != nullptr) ? lockedBy : "";
    }
    return retVal;
}

int qmutex::unLock(const char* msg) {
    int retVal = 0;
    if (!inited) {
        retVal = tryInitIfNot();
        if (retVal != 0) {
            return retVal;
        }
    }
    retVal = pthread_mutex_unlock(&mutex);
    if (retVal != 0) {
        DEBUG_PRINT_ERROR(__LOGTAG__, "failed to unlock(%s), %s", name.c_str(), (msg != nullptr) ? msg : "");
    }
    return retVal;
}
void qmutex::conditionalWait(const char* waiting_at) {
    wanted++;
    waitingAT = (waiting_at != nullptr) ? waiting_at : "";
    while (!allowTask) {
        DEBUG_PRINT_IMPORTANT2(__LOGTAG__, "%s Blocked by %s", name.c_str(), blockedBy.c_str());
        DEBUG_ASSERT(__LOGTAG__, (condition.conditionWait(*this, __FUNCTION__) == 0), __FUNCTION__);
    }
    //    allowTask = false;
    waitingAT = "";
    wanted--;
}

void qmutex::block(const char* blockedBy_) {
    // block close
    int result = tryLock(blockedBy_);
    if (result != 0 && blockCount == 0 && allowTask == false) {
        // safe return;
        return;
    }
    else {
        if (result != 0 && wanted > 1) {
            DEBUG_ASSERT(__LOGTAG__, false, __FUNCTION__);
        }
    }
    blockedBy = (blockedBy_ == nullptr) ? "???" : blockedBy_;
    allowTask = false;
    blockCount++;
    DEBUG_ASSERT(__LOGTAG__, (unLock() == 0), __FUNCTION__);
    //
}

void qmutex::unBlock(const char* unblockedBy_) {
    int result = tryLock(__FUNCTION__);
    if (result != 0 && blockCount == 0 && allowTask == false) {
        // safe return;
        allowTask = true;   // Not sure of this. Data race conditions can cause.
        unblockedBy = unblockedBy_ != nullptr ? unblockedBy_ : "";
        return;
    }
    else {
        if (result != 0 && wanted > 1) {
            DEBUG_ASSERT(__LOGTAG__, false, __FUNCTION__);
        }
        allowTask = true;   // Not sure of this. Data race conditions can cause.
    }
    //    DEBUG_ASSERT(__LOGTAG__, (tryLock(__FUNCTION__)==0), __FUNCTION__);
    allowTask = true;
    unblockedBy = unblockedBy_ != nullptr ? unblockedBy_ : "";
    blockCount--;
    DEBUG_ASSERT(__LOGTAG__, (condition.signal() == 0), __FUNCTION__);
    DEBUG_ASSERT(__LOGTAG__, (unLock() == 0), __FUNCTION__);
}
#pragma endregion QMutex

#pragma region QMutexCondition
qmutexcondition::qmutexcondition() {
    inited = false;
}
int qmutexcondition::init(const std::string& name) {
    this->name = name + "_cond";
    int retVal = pthread_cond_init(&cond, nullptr);
    if (retVal != 0) {
        DEBUG_PRINT_ERROR(__LOGTAG__, "%s condition init has failed: %s - %d", name.c_str(), strerror(errno), errno);
    }
    else {
        inited = true;
        DEBUG_PRINT(LOG_LEVEL_2, __LOGTAG__, "%s condtion init", name.c_str());
    }
    return retVal;
}

int qmutexcondition::tryInitIfNot() {
    int retVal = 0;
    if (!inited) {
        retVal = init("QMutexCondition");
        if (retVal != 0) {
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
        if (retVal != 0) {
            return retVal;
        }
    }
    int sig_req = pthread_cond_signal(&cond);
    if (sig_req != 0) {
        DEBUG_PRINT_ERROR(__LOGTAG__, "SIGNAL failed %d on %s. %s", sig_req, name.c_str(), (msg != nullptr) ? msg : "");
    }
    return sig_req;
}
int qmutexcondition::broadcast(const char* msg) {
    int retVal = 0;
    if (!inited) {
        retVal = tryInitIfNot();
        if (retVal != 0) {
            return retVal;
        }
    }
    int broadcast_req = pthread_cond_broadcast(&cond);
    if (broadcast_req != 0) {
        DEBUG_PRINT_ERROR(__LOGTAG__, "BROADCAST failed %d on %s. %s", broadcast_req, name.c_str(), (msg != nullptr) ? msg : "");
    }
    return broadcast_req;
}

int qmutexcondition::conditionWait(qmutex& qmutex, const char* msg) {
    int retVal = 0;
    if (!inited) {
        retVal = tryInitIfNot();
        if (retVal != 0) {
            return retVal;
        }
    }
    int wait_req = pthread_cond_wait(&cond, qmutex.getMutexInternal());
    if (wait_req != 0) {
        DEBUG_PRINT_ERROR(__LOGTAG__, "COND_WAIT failed %d for %s, %s", wait_req, name.c_str(), (msg != nullptr) ? msg : "");
    }
    return wait_req;
}
#pragma endregion QMutexCondition


int32_t essentials::resolve_cmd_line_args(const char* tag, int32_t argc, const char* argv[],
    const std::string& version_string_, unsigned version_code_,
    std::string& host, std::string& port, std::string& mongodb_uri, fs::path& rootDir,
    std::string& redis_ip, int& redis_port) {
    if (argc == 2 && strcmp(argv[1], "--version") == 0) {
        DEBUG_PRINT(LOG_LEVEL_0, tag, "version %s(%d)", version_string_.c_str(), version_code_);
        DEBUG_PRINT_IMPORTANT2(tag, "Usage : <executable> '--h <ip address>' '--p <port>' '--db <mongodb uri_string>' '--certdir <certpath>' '--rh <redis ip>' '--rp <redis port>'");
        return -1;
    }

    DEBUG_PRINT(LOG_LEVEL_0, tag, "version %s(%d)", version_string_.c_str(), version_code_);
    print_common_info();

    if (argc % 2 == 0) {
        DEBUG_PRINT_ERROR(tag, "Failed to resolve arguments. Exiting !!!");
        DEBUG_PRINT_IMPORTANT2(tag, "Usage : <executable> '--h <ip address>' '--p <port>' '--db <mongodb uri_string>' '--certdir <certpath>' '--rh <redis ip>' '--rp <redis port>'");
        return -1;
    }

    // default to root
    rootDir = "";
    if (argc > 0) {
        fs::path executablePath(argv[0]);
        rootDir = executablePath.parent_path();
    }
    //

    int pairs = (argc - 1) / 2;
    for (int x = 0;x < pairs;x++) {
        const char* lf = argv[1 + x * 2 + 0];
        const char* rg = argv[1 + x * 2 + 1];

        if (strcmp(lf, "--h") == 0) {
            host = rg;
        }
        else if (strcmp(lf, "--p") == 0) {
            port = rg;
        }
        else if (strcmp(lf, "--certdir") == 0) {
            rootDir = fs::path(rg);
        }
        else if (strcmp(lf, "--db") == 0) {
            mongodb_uri = rg;
        }
        else if (strcmp(lf, "--rh") == 0) {
            redis_ip = rg;
        }
        else if (strcmp(lf, "--rp") == 0) {
            if (gsdk::str2int(&redis_port, rg, 10) != gsdk::STR2INT_SUCCESS) {
                DEBUG_PRINT_ERROR(tag, "Unable to parse redis port, defaulting to %d !!!", redis_port);
            }
        }
    }

    // check host and port
    const struct addrinfo hints = {
        .ai_family = PF_UNSPEC,
        .ai_socktype = SOCK_DGRAM,
        .ai_protocol = IPPROTO_UDP
    };
    struct addrinfo* peer = nullptr;
    if (getaddrinfo(host.c_str(), port.c_str(), &hints, &peer) != 0) {
        DEBUG_PRINT_ERROR(tag, "Failed to resolve host. Exiting !!!");
        DEBUG_PRINT_IMPORTANT2(tag, "Usage : <executable> '--h <ip address>' '--p <port>' '--db <mongodb uri_string>' '--certdir <certpath>' '--rh <redis ip>' '--rp <redis port>'");
        return -1;
    }
    if (peer) {
        freeaddrinfo(peer);
        peer = nullptr;
    }
    DEBUG_PRINT_IMPORTANT(tag, "server %s:%s, mongodb_uri %s, redis %s:%d", host.c_str(), port.c_str(), mongodb_uri.c_str(), redis_ip.c_str(), redis_port);
    //

    DEBUG_PRINT_IMPORTANT(tag, "Root dir : %s", rootDir.c_str());
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
    time_t local_time = tm->tm_sec + tm->tm_min * 60 + tm->tm_hour * 3600 + tm->tm_yday * 86400 +
        (tm->tm_year - 70) * 31536000 + ((tm->tm_year - 69) / 4) * 86400 -
        ((tm->tm_year - 1) / 100) * 86400 + ((tm->tm_year + 299) / 400) * 86400;
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
    time_t utc_time = tm->tm_sec + tm->tm_min * 60 + tm->tm_hour * 3600 + tm->tm_yday * 86400 +
        (tm->tm_year - 70) * 31536000 + ((tm->tm_year - 69) / 4) * 86400 -
        ((tm->tm_year - 1) / 100) * 86400 + ((tm->tm_year + 299) / 400) * 86400;
    return utc_time;
}

qstring essentials::get_time_utc_tostring() {
    time_t utc_time = get_time_utc();
    qstring tmp(ctime(&utc_time));
    return tmp;
}

qstring essentials::get_time_utc_postgresql_format() {
    time_t now;
    time(&now);
    struct tm* tm = gmtime(&now);
    char timestampStr[32];
    snprintf(timestampStr, sizeof(timestampStr), "%04d-%02d-%02d %02d:%02d:%02d",
        tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday,
        tm->tm_hour, tm->tm_min, tm->tm_sec);
    return qstring(timestampStr);
}

bool conn_io_req_res::validate() {
    header* crc_header = get_header("crc");
    if (crc_header == nullptr) {
        return  false;
    }

    const payload& payload = get_payload();
    if (payload.buffer.length() == 0) {
        // check if its get method or not
        header* method_header = get_header(":method");
        if (method_header == nullptr) {
            return  false;
        }
        else if (crc_header->value == "0") { //for get methods there wont be any payload and the crc will be zero.
            return true;
        }
        return false;
    }
    unsigned long  crc_ = payload.get_crc_value();
    unsigned long crc_from_req = 0;
    sscanf((const char*)crc_header->value.c_str(), "%8lx", &crc_from_req);

    if (crc_from_req != crc_) {
        DEBUG_PRINT_ERROR(__LOGTAG__, "CRC validation Error %lu != %lu, payload sz %lu, crc_as_string %s",
            crc_, crc_from_req, payload.buffer.length(), crc_header->value.c_str());
        assert(crc_from_req == crc_);
    }
    //     DEBUG_PRINT_IMPORTANT(__LOGTAG__, "CRC validation %lu == %lu, payload sz %lu, crc_as_string %s",
    //        crc_, crc_from_req, payload->len, crc_header->value);
    return crc_from_req == crc_;
}
