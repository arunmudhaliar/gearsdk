//
//  qzookeeper.cpp
//  qzookeeper
//
//  Created by Arun A on 05/01/24.
//

#include "qzookeeper.hpp"

#define _LL_CAST_ (long long)

#ifdef THREADED
void qzookeeper::millisleep(int ms) {
    struct timespec ts;
    ts.tv_sec = ms / 1000;
    ts.tv_nsec = (ms % 1000) * 1000000; // to nanoseconds
    nanosleep(&ts, NULL);
}
#endif /* THREADED */

const char* qzookeeper::state2String(int state) {
  if (state == 0)
    return "CLOSED_STATE";
  if (state == ZOO_CONNECTING_STATE)
    return "CONNECTING_STATE";
  if (state == ZOO_ASSOCIATING_STATE)
    return "ASSOCIATING_STATE";
  if (state == ZOO_CONNECTED_STATE)
    return "CONNECTED_STATE";
  if (state == ZOO_READONLY_STATE)
    return "READONLY_STATE";
  if (state == ZOO_EXPIRED_SESSION_STATE)
    return "EXPIRED_SESSION_STATE";
  if (state == ZOO_AUTH_FAILED_STATE)
    return "AUTH_FAILED_STATE";

  return "INVALID_STATE";
}

const char* qzookeeper::type2String(int state) {
  if (state == ZOO_CREATED_EVENT)
    return "CREATED_EVENT";
  if (state == ZOO_DELETED_EVENT)
    return "DELETED_EVENT";
  if (state == ZOO_CHANGED_EVENT)
    return "CHANGED_EVENT";
  if (state == ZOO_CHILD_EVENT)
    return "CHILD_EVENT";
  if (state == ZOO_SESSION_EVENT)
    return "SESSION_EVENT";
  if (state == ZOO_NOTWATCHING_EVENT)
    return "NOTWATCHING_EVENT";

  return "UNKNOWN_EVENT_TYPE";
}

void qzookeeper::watcher(zhandle_t *zzh, int type, int state, const char *path, void* context) {
    /* Be careful using zh here rather than zzh - as this may be mt code
     * the client lib may call the watcher before zookeeper_init returns */
    qzookeeper* qzk = (qzookeeper*)context;
    
    DEBUG_PRINT(LOG_LEVEL_0, __LOGTAG__, "Watcher %s state = %s", type2String(type), state2String(state));
    if (path && strlen(path) > 0) {
        DEBUG_PRINT(LOG_LEVEL_0, __LOGTAG__, "for path %s", path);
    }
    if (type == ZOO_SESSION_EVENT) {
        if (state == ZOO_CONNECTED_STATE) {
            const clientid_t* id = zoo_client_id(zzh);
            if (qzk->myid.client_id == 0 || qzk->myid.client_id != id->client_id) {
                qzk->myid = *id;
                DEBUG_PRINT(LOG_LEVEL_0, __LOGTAG__, "Got a new session id: 0x%llx", _LL_CAST_ qzk->myid.client_id);
                if (qzk->clientIdFile) {
                    DEBUG_PRINT(LOG_LEVEL_0, __LOGTAG__, "clientIdFile %s", qzk->clientIdFile);
                    FILE *fh = fopen(qzk->clientIdFile, "w");
                    if (!fh) {
                        DEBUG_PRINT_ERROR(__LOGTAG__, "fopen failed %s", qzk->clientIdFile);
                    } else {
                        long rc = fwrite(&qzk->myid, sizeof(qzk->myid), 1, fh);
                        if (rc != sizeof(qzk->myid)) {
                            DEBUG_PRINT_ERROR(__LOGTAG__, "rc != sizeof(qzk->myid) !!! writing client id");
                        }
                        fclose(fh);
                    }
                }
            }
        } else if (state == ZOO_AUTH_FAILED_STATE) {
            DEBUG_PRINT_ERROR(__LOGTAG__, "Authentication failure. Shutting down...");
            qzk->shutdown(state);
        } else if (state == ZOO_EXPIRED_SESSION_STATE) {
            DEBUG_PRINT_ERROR(__LOGTAG__, "Session expired. Shutting down...");
            qzk->shutdown(state);
        }
    }
}

qzookeeper::~qzookeeper() {
    if (zh) {
        zookeeper_close(zh);
        zh = nullptr;
    }
}

int qzookeeper::connect(const qstring& url) {
    connection_url = url;
    connection_in_progress = true;
    if (pthread_create(&zk_thread_id, nullptr, qzookeeper::connect_internal, (void*)this) < 0) {
        DEBUG_PRINT_ERROR(__LOGTAG__, "qzookeeper - could not create thread: %s - %d", strerror(errno), errno);
        connection_in_progress = false;
        return -1;
    }
    
    while (connection_in_progress) {
        millisleep(50);
    }
    return 0;
}

void* qzookeeper::connect_internal(void* data) {
    qzookeeper* thiz = (qzookeeper*)data;
    thiz->connection_in_progress = true;
    
    if (thiz->mainloop) {
        DEBUG_PRINT_ERROR(__LOGTAG__, "qzookeeper - already inited (thiz->mainloop != null)");
        thiz->connection_in_progress = false;
        pthread_exit(0);
    }
    zoo_deterministic_conn_order(1); // enable deterministic order
    
    int flags = ZOO_READONLY;
    thiz->zh = zookeeper_init(thiz->connection_url.c_str(), watcher, 30000, &thiz->myid, thiz, flags);
    thiz->connection_in_progress = false;
    if (!thiz->zh) {
        DEBUG_PRINT_ERROR(__LOGTAG__, "qzookeeper - zookeeper_init failed : %s - %d", strerror(errno), errno);
        pthread_exit(0);
    }
    PTHREAD_NAME("qzookeeper");
    
    qtimer_sceduler scheduler;
    thiz->mainloop = ev_default_loop(0);
    scheduler.set_ev_lopp(thiz->mainloop);
    
    qtimer* keep_alive_loop = scheduler.schedule_repeat_timer([thiz](qtimer& timer) {
        UNUSED(timer);
    }, 60);
    UNUSED(keep_alive_loop);
    ev_run(thiz->mainloop, 0);
    
    ev_loop_destroy(thiz->mainloop);
    thiz->mainloop = nullptr;
    
    if (thiz->zh) {
        zookeeper_close(thiz->zh);
    }
    
    pthread_exit(0);
}


void qzookeeper::my_data_completion(int rc, const char *value, int value_len,
        const struct Stat *stat, const void *data) {
    qzookeeper* thiz = (qzookeeper*)data;
    thiz->op_in_progress = true;
    if (value) {
        DEBUG_PRINT(LOG_LEVEL_0, __LOGTAG__, "\nvalue = %.*s", value_len, value);
        thiz->get_result.clear();
        thiz->get_result.run_printf(value, value_len);
    }
    thiz->op_result = rc;
    thiz->op_in_progress = false;
}

void qzookeeper::dumpStat(const struct Stat *stat) {
    char tctimes[40];
    char tmtimes[40];
    time_t tctime;
    time_t tmtime;

    if (!stat) {
        DEBUG_PRINT_ERROR(__LOGTAG__, "null");
        return;
    }
    tctime = stat->ctime/1000;
    tmtime = stat->mtime/1000;

    ctime_r(&tmtime, tmtimes);
    ctime_r(&tctime, tctimes);

    DEBUG_PRINT(LOG_LEVEL_0, __LOGTAG__, "\tctime = %s\tczxid=%llx\n"
    "\tmtime=%s\tmzxid=%llx\n"
    "\tversion=%x\taversion=%x\n"
    "\tephemeralOwner = %llx",
     tctimes, _LL_CAST_ stat->czxid, tmtimes,
    _LL_CAST_ stat->mzxid,
    (unsigned int)stat->version, (unsigned int)stat->aversion,
    _LL_CAST_ stat->ephemeralOwner);
}

void qzookeeper::my_stat_completion(int rc, const struct Stat *stat, const void *data) {
    qzookeeper* thiz = (qzookeeper*)data;
    thiz->op_in_progress = true;
    DEBUG_PRINT(LOG_LEVEL_0, __LOGTAG__, "\nrc = %d Stat:", rc);
    dumpStat(stat);
    thiz->op_result = rc;
    thiz->op_in_progress = false;
}

void qzookeeper::my_void_completion(int rc, const void *data) {
    qzookeeper* thiz = (qzookeeper*)data;
    thiz->op_in_progress = true;
    DEBUG_PRINT(LOG_LEVEL_0, __LOGTAG__, "\nrc = %d delete:", rc);
    thiz->op_result = rc;
    thiz->op_in_progress = false;
}

int qzookeeper::set_data(const qstring& zk_path, const qstring& data) {
    if (!zh) {
        return -1;
    }
    if (op_in_progress) {
        return -2;
    }
    op_in_progress = true;
    op_result = -1;
    int rc = zoo_aset(zh, zk_path.c_str(), data.c_str(), (int)data.length(), -1, my_stat_completion,
            this);
    if (rc) {
        DEBUG_PRINT_ERROR(__LOGTAG__, "Error (%d) setting %s", rc, data.length(), zk_path.c_str());
        op_in_progress = false;
        return -1;
    }
    DEBUG_PRINT(LOG_LEVEL_0, __LOGTAG__, "\nset : %.*s", data.length(), data.c_str());
    while (op_in_progress) {
        millisleep(50);
    }
    return op_result;
}

int qzookeeper::get_data(const qstring& zk_path, qstring& result, const qstring& default_value) {
    result = default_value;
    if (!zh) {
        return -1;
    }
    if (op_in_progress) {
        return -2;
    }
    DEBUG_PRINT(LOG_LEVEL_0, __LOGTAG__, "get : %.*s", zk_path.length(), zk_path.c_str());
    op_in_progress = true;
    op_result = -1;
    int rc = zoo_aget(zh, zk_path.c_str(), 1, my_data_completion, this);
    if (rc) {
        DEBUG_PRINT_ERROR(__LOGTAG__, "Error (%d) fetching %s", rc, zk_path.c_str());
        op_in_progress = false;
        return -1;
    }
    
    while (op_in_progress) {
        millisleep(50);
    }
    result.copy(get_result);
    return op_result;
}

int qzookeeper::delete_path(const qstring& zk_path) {
    if (!zh) {
        return -1;
    }
    if (op_in_progress) {
        return -2;
    }
    DEBUG_PRINT(LOG_LEVEL_0, __LOGTAG__, "delete : %.*s", zk_path.length(), zk_path.c_str());
    op_in_progress = true;
    op_result = -1;
    int rc = zoo_adelete(zh, zk_path.c_str(), -1, my_void_completion, this);
    if (rc) {
        DEBUG_PRINT_ERROR(__LOGTAG__, "Error (%d) delete %s", rc, zk_path.c_str());
        op_in_progress = false;
        return -1;
    }
    
    while (op_in_progress) {
        millisleep(50);
    }
    return op_result;
}

void qzookeeper::shutdown(const int state) {
    if (zh) {
        zookeeper_close(zh);
    }
    zh = nullptr;
    if (!mainloop) {
        ev_break(mainloop, EVBREAK_ONE);
    }
}
