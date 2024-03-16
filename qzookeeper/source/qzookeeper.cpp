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
        qzk->connection_state = state;
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
                qzk->retry_count = 0;
                qzk->connection_in_progress = false;
            }
        } else if (state == ZOO_AUTH_FAILED_STATE) {
            DEBUG_PRINT_ERROR(__LOGTAG__, "Authentication failure. Shutting down...");
            qzk->close_zk(state);
        } else if (state == ZOO_EXPIRED_SESSION_STATE) {
            DEBUG_PRINT_ERROR(__LOGTAG__, "Session expired. Shutting down...");
            qzk->close_zk(state);
        }
    }
}

qzookeeper::qzookeeper() :
    qtimer_sceduler() {
        connection_in_progress = false;
        running = false;
        op_in_progress = false;
        op_result = 0;
}

qzookeeper::~qzookeeper() {
    if (zh) {
        zookeeper_close(zh);
        zh = nullptr;
    }
    if (mainloop) {
        ev_loop_destroy(mainloop);
        mainloop = nullptr;
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
    return connection_state==ZOO_CONNECTED_STATE ? 0 : -1;
}

int qzookeeper::retry_connection() {
    if (zh) {
        DEBUG_PRINT_ERROR(__LOGTAG__, "qzookeeper retry_connection - already inited (thiz->zh != null)");
        return -1;
    }
    
    retry_count++;
    DEBUG_PRINT_IMPORTANT2(__LOGTAG__, "qzookeeper - retry connection !!!");
    int flags = ZOO_READONLY;
    bool use_fresh = connection_state==ZOO_EXPIRED_SESSION_STATE || connection_state==ZOO_AUTH_FAILED_STATE || connection_state==ZOO_NOTCONNECTED_STATE;
    zh = zookeeper_init(connection_url.c_str(), watcher, 30000, use_fresh? nullptr : &myid, this, flags);
    return 0;
}

void* qzookeeper::connect_internal(void* data) {
    qzookeeper* thiz = (qzookeeper*)data;
    thiz->running = true;
    thiz->connection_in_progress = true;
    
    if (thiz->zh) {
        DEBUG_PRINT_ERROR(__LOGTAG__, "qzookeeper - already inited (thiz->zh != null)");
        thiz->connection_in_progress = false;
        pthread_exit(0);
    }
    zoo_deterministic_conn_order(1); // enable deterministic order
    
    zoo_set_debug_level(ZOO_LOG_LEVEL_WARN);
    
    int flags = ZOO_READONLY;
    thiz->zh = zookeeper_init(thiz->connection_url.c_str(), watcher, 30000, &thiz->myid, thiz, flags);
    if (!thiz->zh) {
        DEBUG_PRINT_ERROR(__LOGTAG__, "qzookeeper - zookeeper_init failed : %s - %d", strerror(errno), errno);
        pthread_exit(0);
    }
    PTHREAD_NAME("qzookeeper");
    
    thiz->mainloop = ev_loop_new(0);
    thiz->set_ev_lopp(thiz->mainloop);
    
    thiz->connection_check_timer = thiz->schedule_repeat_timer([thiz](qtimer& timer) {
        UNUSED(timer);
        if (!(thiz->connection_state==ZOO_CONNECTING_STATE || thiz->connection_state==ZOO_ASSOCIATING_STATE || thiz->connection_state==ZOO_CONNECTED_STATE) /*&& thiz->connection_in_progress*/) {
            thiz->close_zk(-1);
            DEBUG_PRINT_ERROR(__LOGTAG__, "qzookeeper - zk server not responding !!! connection failed. re-try:%d, STATE:%d",
                              thiz->retry_count, thiz->connection_state);
            if (thiz->retry_count<2) {
                thiz->retry_connection();
            } else {
                ev_break(thiz->mainloop, EVBREAK_ONE);
            }
        }
    }, 3);
    
    ev_run(thiz->mainloop, 0);
    
    if (thiz->zh) {
        zookeeper_close(thiz->zh);
        thiz->zh = nullptr;
    }
    thiz->connection_in_progress = false;
    thiz->running = false;
    thiz->shutdown();
    ev_loop_destroy(thiz->mainloop);
    thiz->mainloop = nullptr;
    pthread_exit(0);
}

void qzookeeper::shutdown() {
    if (connection_check_timer) {
        if(cancel_and_destroy_timer(connection_check_timer)) {
            connection_check_timer = nullptr;
        }
    }
}

void qzookeeper::my_data_completion(int rc, const char *value, int value_len,
        const struct Stat *stat, const void *data) {
    qzookeeper* thiz = (qzookeeper*)data;
    thiz->op_in_progress = true;
    if (value) {
        DEBUG_RAW(LOG_LEVEL_0, "\tvalue = %.*s", value_len, value);
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

    memset(tctimes, 0, sizeof(tctimes));
    memset(tmtimes, 0, sizeof(tmtimes));
    tctimes[sizeof(tctimes)-1] = '\0';
    tmtimes[sizeof(tmtimes)-1] = '\0';
    ctime_r(&tmtime, tmtimes);
    ctime_r(&tctime, tctimes);

    DEBUG_RAW(LOG_LEVEL_0, "\tctime = %s\t\tczxid=%llx\n"
    "\t\tmtime=%s\t\tmzxid=%llx\n"
    "\t\tversion=%x\taversion=%x\n"
    "\t\tephemeralOwner = %llx",
     tctimes, _LL_CAST_ stat->czxid, tmtimes,
    _LL_CAST_ stat->mzxid,
    (unsigned int)stat->version, (unsigned int)stat->aversion,
    _LL_CAST_ stat->ephemeralOwner);
    fflush(stderr);
}

void qzookeeper::my_stat_completion(int rc, const struct Stat *stat, const void *data) {
    qzookeeper* thiz = (qzookeeper*)data;
    thiz->op_in_progress = true;
    DEBUG_RAW(LOG_LEVEL_0, "\trc = %d Stat:", rc);
    dumpStat(stat);
    thiz->op_result = rc;
    thiz->op_in_progress = false;
}

void qzookeeper::my_void_completion(int rc, const void *data) {
    qzookeeper* thiz = (qzookeeper*)data;
    thiz->op_in_progress = true;
    DEBUG_RAW(LOG_LEVEL_0, "\trc = %d delete:", rc);
    thiz->op_result = rc;
    thiz->op_in_progress = false;
}

int qzookeeper::set_data(const qstring& zk_path, const qstring& data) {
    if (!zh) {
        return -1;
    }
    if (connection_state != ZOO_CONNECTED_STATE) {
        DEBUG_PRINT_ERROR(__LOGTAG__, "set_data - zk state not in ZOO_CONNECTED_STATE %s, connection_state:%d", zk_path.c_str(), connection_state);
        return -2;
    }
    if (op_in_progress) {
        return -5;
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
    DEBUG_PRINT(LOG_LEVEL_0, __LOGTAG__, "set : %.*s", data.length(), data.c_str());
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
    if (connection_state != ZOO_CONNECTED_STATE) {
        DEBUG_PRINT_ERROR(__LOGTAG__, "get_data - zk state not in ZOO_CONNECTED_STATE %s, connection_state:%d", zk_path.c_str(), connection_state);
        return -2;
    }
    if (op_in_progress) {
        return -5;
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
    if (connection_state != ZOO_CONNECTED_STATE) {
        DEBUG_PRINT_ERROR(__LOGTAG__, "delete_path - zk state not in ZOO_CONNECTED_STATE %s, connection_state:%d", zk_path.c_str(), connection_state);
        return -2;
    }
    if (op_in_progress) {
        return -5;
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

void qzookeeper::close_zk(const int state) {
    if (zh) {
        zookeeper_close(zh);
        zh = nullptr;
    }
}
