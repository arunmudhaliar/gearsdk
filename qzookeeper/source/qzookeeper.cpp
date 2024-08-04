//
//  Copyright 2024 homenet25
//  qzookeeper.cpp
//  qzookeeper
//
//  Created by Arun A on 05/01/24.
//

#include "qzookeeper.hpp"
#include <chrono>
#include <thread>

#define _LL_CAST_ (long long)

#ifdef THREADED
void qzookeeper::millisleep(int ms) {
	struct timespec ts;
	ts.tv_sec = ms / 1000;
	ts.tv_nsec = (ms % 1000) * 1000000;	 // to nanoseconds
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

void qzookeeper::watcher(zhandle_t* zzh, int type, int state, const char* path, void* context) {
	/* Be careful using zh here rather than zzh - as this may be mt code
	 * the client lib may call the watcher before zookeeper_init returns */
	qzookeeper* qzk = (qzookeeper*) context;
    PTHREAD_NAME(qzk->wname.c_str());
    const char* logtag = qzk->name.c_str(); // its safe to read since no one going to write on this variable except constructor;
    
	DEBUG_PRINT(LOG_LEVEL_0, logtag, "Watcher %s state = %s", type2String(type), state2String(state));
	if (path && strlen(path) > 0) {
		DEBUG_PRINT(LOG_LEVEL_0, logtag, "for path %s", path);
	}
    
    std::lock_guard<std::mutex> lock(qzk->watcher_gaurd_mutex); // Lock the mutex for thread-safe access

    qzk->connection_state = state;
    
	if (type == ZOO_SESSION_EVENT) {
		if (state == ZOO_CONNECTED_STATE) {
			const clientid_t* id = zoo_client_id(zzh);
			if (qzk->myid.client_id == 0 || qzk->myid.client_id != id->client_id) {
				qzk->myid = *id;
				DEBUG_PRINT(LOG_LEVEL_0, logtag, "Got a new session id: 0x%llx", _LL_CAST_ qzk->myid.client_id);
				if (qzk->clientIdFile) {
					DEBUG_PRINT(LOG_LEVEL_0, logtag, "clientIdFile %s", qzk->clientIdFile);
					FILE* fh = fopen(qzk->clientIdFile, "w");
					if (!fh) {
						DEBUG_PRINT_ERROR(logtag, "fopen failed %s", qzk->clientIdFile);
					} else {
						long rc = fwrite(&qzk->myid, sizeof(qzk->myid), 1, fh);
						if (rc != sizeof(qzk->myid)) {
							DEBUG_PRINT_ERROR(logtag, "rc != sizeof(qzk->myid) !!! writing client id");
						}
						fclose(fh);
					}
				}
                
                // retry pending config change if any
                for(auto p : qzk->pending_config_updates) {
                    char buffer[4*1024];
                    int buffer_len = sizeof(buffer);
                    auto operation = [&]() {
                        return zoo_wget(zzh, p.c_str(), watcher, context, buffer, &buffer_len, nullptr);
                    };
                    int rc = retry_with_backoff(operation, QZK_MAX_RETRIES_FOR_API, QZK_RETRY_BASE_DELAY_MS);
//                    int rc = zoo_wget(zzh, p.c_str(), watcher, context, buffer, &buffer_len, NULL);
                    if (rc == ZOK) {
                        DEBUG_PRINT(LOG_LEVEL_2, logtag, "updated value for %s --> [%d bytes]\n%.*s", p.c_str(), buffer_len, buffer_len, buffer);
                        qzk->broadcast_value_change_to_all(p.c_str(), qstring(buffer, buffer_len));
                    } else {
                        DEBUG_PRINT_ERROR(logtag, "zk config update failed with error %s on zoo_wget for %s", zerror(rc), path);
                        qzk->close_zk(state);
                        return;
                    }
                }
                if (qzk->pending_config_updates.size()) {
                    DEBUG_PRINT_IMPORTANT2(logtag, "qzookeeper - pending updates applied !!!");
                    qzk->pending_config_updates.clear();
                }
                // ~retry
                
                qzk->retry_count = 0;
                qzk->connection_in_progress = false;
			}
		} else if (state == ZOO_AUTH_FAILED_STATE) {
			DEBUG_PRINT_ERROR(logtag, "Authentication failure. Shutting down...");
			qzk->close_zk(state);
		} else if (state == ZOO_EXPIRED_SESSION_STATE) {
			DEBUG_PRINT_ERROR(logtag, "Session expired. Shutting down...");
			qzk->close_zk(state);
//            qzk->retry_connection();
        }
	} else if (type == ZOO_CHANGED_EVENT) {
        if (state == ZOO_CONNECTED_STATE) {
            char buffer[4*1024];
            int buffer_len = sizeof(buffer);
            auto operation = [&]() {
                return zoo_wget(zzh, path, watcher, context, buffer, &buffer_len, nullptr);
            };
            int rc = retry_with_backoff(operation, QZK_MAX_RETRIES_FOR_API, QZK_RETRY_BASE_DELAY_MS);
//            int rc = zoo_wget(zzh, path, watcher, context, buffer, &buffer_len, NULL);
            if (rc == ZOK) {
                DEBUG_PRINT(LOG_LEVEL_2, logtag, "updated value for %s --> [%d bytes]\n%.*s", path, buffer_len, buffer_len, buffer);
                qzk->broadcast_value_change_to_all(path, qstring(buffer, buffer_len));
            } else {
                qzk->pending_config_updates.push_back(path);
                DEBUG_PRINT_ERROR(logtag, "zk config update failed with error %s on zoo_wget for %s", zerror(rc), path);
//                qzk->close_zk(state);
//                qzk->retry_connection();
            }
        }
    }
}

int qzookeeper::retry_with_backoff(std::function<int()> operation, int max_retries, int base_delay_ms) {
    int attempt = 0;
    int delay = base_delay_ms;

    while (attempt < max_retries) {
        int rc = operation();
        if (rc == ZOK) {
            return ZOK;
        } else if (rc == ZOPERATIONTIMEOUT) {
            std::this_thread::sleep_for(std::chrono::milliseconds(delay));
            delay *= 2; // Exponential backoff
            attempt++;
        } else {
            return rc; // If it's not a timeout, return the error code
        }
    }
    return ZOPERATIONTIMEOUT; // If all retries fail, return timeout
}

qzookeeper::qzookeeper(const qstring& name) :
 qtimer_sceduler(), name(name), wname(qstring::format_string("%s-watcher", name.c_str())) {
    retry_in_progress = false;
	connection_in_progress = false;
	running = false;
	op_in_progress = false;
	op_result = 0;
    DEBUG_ASSERT(name.c_str(), (close_mutex.init("qzk_close") == 0), "qzookeeper Constructor - CHECK !!!");
    DEBUG_ASSERT(name.c_str(), (reconnect_mutex.init("qzk_reconnect") == 0), "qzookeeper Constructor - CHECK !!!");
}

qzookeeper::~qzookeeper() {
    const char* logtag = name.c_str(); // its safe to read since no one going to write on this variable except constructor;
	if (zh) {
		zookeeper_close(zh);
		zh = nullptr;
        DEBUG_PRINT(LOG_LEVEL_4, logtag, "zk closed");
    } else {
        DEBUG_PRINT(LOG_LEVEL_4, logtag, "no zk handle. no need to close.");
    }
	if (mainloop) {
		ev_loop_destroy(mainloop);
		mainloop = nullptr;
        DEBUG_PRINT(LOG_LEVEL_4, logtag, "zk mainloop destroyed");
	} else {
        DEBUG_PRINT(LOG_LEVEL_4, logtag, "no zk mainloop. no need to destroy mainloop.");
    }
    DEBUG_PRINT(LOG_LEVEL_0, logtag, "zk destroyed");
}

int qzookeeper::connect(const qstring& url) {
    const char* logtag = name.c_str(); // its safe to read since no one going to write on this variable except constructor;
	connection_url = url;
	connection_in_progress = true;
	if (pthread_create(&zk_thread_id, nullptr, qzookeeper::connect_internal, (void*) this) < 0) {
		DEBUG_PRINT_ERROR(logtag, "qzookeeper - could not create thread: %s - %d", strerror(errno), errno);
		connection_in_progress = false;
		return -1;
	}

	while (connection_in_progress) {
		millisleep(50);
	}
    if (connection_state != ZOO_CONNECTED_STATE) {
        DEBUG_PRINT_ERROR(logtag, "qzookeeper not in ZOO_CONNECTED_STATE state while returning from connect - %s", state2String(connection_state));
    }
	return connection_state == ZOO_CONNECTED_STATE ? 0 : -1;
}

int qzookeeper::retry_connection() {
    const char* logtag = name.c_str(); // its safe to read since no one going to write on this variable except constructor;
	if (zh) {
		DEBUG_PRINT_ERROR(logtag, "qzookeeper retry_connection - already inited (thiz->zh != null)");
		return -1;
	}

    if (reconnect_mutex.tryLock(__FUNCTION__) != 0) {
        DEBUG_PRINT_ERROR(logtag, "qzookeeper - retry_connection acquire lock failed. returning !!!");
        return -2;
    }
    
    retry_in_progress = true;
	retry_count++;
	DEBUG_PRINT_IMPORTANT2(logtag, "qzookeeper - retry connection !!!");
	int flags = ZOO_READONLY;
	bool use_fresh = connection_state == ZOO_EXPIRED_SESSION_STATE || connection_state == ZOO_AUTH_FAILED_STATE || connection_state == ZOO_NOTCONNECTED_STATE;
	zh = zookeeper_init(connection_url.c_str(), watcher, 30000, use_fresh ? nullptr : &myid, this, flags);
    retry_in_progress = false;
    DEBUG_ASSERT(logtag, (reconnect_mutex.unLock() == 0), "retry_connection unlock CHECK !!!");
	return 0;
}

void* qzookeeper::connect_internal(void* data) {
	qzookeeper* thiz = (qzookeeper*) data;
    const char* logtag = thiz->name.c_str(); // its safe to read since no one going to write on this variable except constructor;
    PTHREAD_NAME(logtag);
	thiz->running = true;
	thiz->connection_in_progress = true;

	if (thiz->zh) {
		DEBUG_PRINT_ERROR(logtag, "qzookeeper - already inited (thiz->zh != null)");
		thiz->connection_in_progress = false;
		pthread_exit(0);
	}
	zoo_deterministic_conn_order(1);  // enable deterministic order

	zoo_set_debug_level(ZOO_LOG_LEVEL_ERROR);

    if (thiz->reconnect_mutex.tryLock(__FUNCTION__) != 0) {
        DEBUG_PRINT_ERROR(logtag, "qzookeeper - retry_connection (connect_internal) acquire lock failed. returning !!!");
        pthread_exit(0);
    }
    
	int flags = ZOO_READONLY;
	thiz->zh = zookeeper_init(thiz->connection_url.c_str(), watcher, 30000, &thiz->myid, thiz, flags);
    
    DEBUG_ASSERT(logtag, (thiz->reconnect_mutex.unLock() == 0), "retry_connection (connect_internal) unlock CHECK !!!");
    
	if (!thiz->zh) {
		DEBUG_PRINT_ERROR(logtag, "qzookeeper - zookeeper_init failed : %s - %d", strerror(errno), errno);
		pthread_exit(0);
	}
	
	thiz->mainloop = ev_loop_new(0);
	thiz->set_ev_lopp(thiz->mainloop);

	thiz->connection_check_timer = thiz->schedule_repeat_timer(
		[thiz, logtag](qtimer& timer) {
			UNUSED(timer);
			if (!(thiz->connection_state == ZOO_CONNECTING_STATE || thiz->connection_state == ZOO_ASSOCIATING_STATE || thiz->connection_state == ZOO_CONNECTED_STATE) /*&& !thiz->connection_in_progress*/) {
				thiz->close_zk(-1);
				if (thiz->retry_count < 5) {
                    DEBUG_PRINT_ERROR(logtag,
                                      "qzookeeper - zk server not in CONNECTED state !!! "
                                      "retrying... retry-count:%d, STATE:%s [%d]",
                                      thiz->retry_count, qzookeeper::state2String(thiz->connection_state), thiz->connection_state);
					thiz->retry_connection();
				} else {
                    DEBUG_PRINT_IMPORTANT2(logtag, "mainloop break %s", qzookeeper::state2String(thiz->connection_state));
					ev_break(thiz->mainloop, EVBREAK_ONE);
				}
			}
		},
		3);

	ev_run(thiz->mainloop, 0);

    DEBUG_PRINT_IMPORTANT2(logtag, "mainloop exited with connection state %s", qzookeeper::state2String(thiz->connection_state));
    
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
    const char* logtag = name.c_str(); // its safe to read since no one going to write on this variable except constructor;
	if (connection_check_timer) {
		if (cancel_and_destroy_timer(connection_check_timer)) {
			connection_check_timer = nullptr;
            DEBUG_PRINT(LOG_LEVEL_0, logtag, "qzookeeper - connection_check_timer detroyed !!!");
		}
	}
    
    if (mainloop != nullptr) {
        ev_break(mainloop, EVBREAK_ONE);
        DEBUG_PRINT(LOG_LEVEL_0, logtag, "mainloop break");
    }
}

void qzookeeper::my_data_completion(int rc, const char* value, int value_len, const struct Stat* stat, const void* data) {
	UNUSED(stat);
	qzookeeper* thiz = (qzookeeper*) data;
	thiz->op_in_progress = true;
	if (value) {
//		DEBUG_RAW(LOG_LEVEL_0, "\t[zk] : value = %.*s", value_len, value);
		thiz->get_result.clear();
		thiz->get_result.run_printf(value, value_len);
	}
	thiz->op_result = rc;
	thiz->op_in_progress = false;
}

void qzookeeper::dumpStat(const struct Stat* stat) {
	char tctimes[40];
	char tmtimes[40];
	time_t tctime;
	time_t tmtime;

	if (!stat) {
		DEBUG_PRINT_ERROR(__LOGTAG__, "[zk] : dumpStat --> null");
		return;
	}
	tctime = stat->ctime / 1000;
	tmtime = stat->mtime / 1000;

	memset(tctimes, 0, sizeof(tctimes));
	memset(tmtimes, 0, sizeof(tmtimes));
	tctimes[sizeof(tctimes) - 1] = '\0';
	tmtimes[sizeof(tmtimes) - 1] = '\0';
	ctime_r(&tmtime, tmtimes);
	ctime_r(&tctime, tctimes);

	DEBUG_RAW(LOG_LEVEL_0,
			  "\t[zk] : ctime = %s\t\tczxid=%llx\n"
			  "\t\tmtime=%s\t\tmzxid=%llx\n"
			  "\t\tversion=%x\taversion=%x\n"
			  "\t\tephemeralOwner = %llx",
			  tctimes, _LL_CAST_ stat->czxid, tmtimes, _LL_CAST_ stat->mzxid, (unsigned int) stat->version, (unsigned int) stat->aversion, _LL_CAST_ stat->ephemeralOwner);
	fflush(stderr);
}

void qzookeeper::my_stat_completion(int rc, const struct Stat* stat, const void* data) {
	qzookeeper* thiz = (qzookeeper*) data;
	thiz->op_in_progress = true;
	DEBUG_RAW(LOG_LEVEL_0, "\t[zk] : rc = %s Stat:", zerror(rc));
	dumpStat(stat);
	thiz->op_result = rc;
	thiz->op_in_progress = false;
}

void qzookeeper::my_void_completion(int rc, const void* data) {
	qzookeeper* thiz = (qzookeeper*) data;
	thiz->op_in_progress = true;
	DEBUG_RAW(LOG_LEVEL_0, "\t[zk] : rc = %s delete:", zerror(rc));
	thiz->op_result = rc;
	thiz->op_in_progress = false;
}

int qzookeeper::set_data(const qstring& zk_path, const qstring& data) {
    while (retry_in_progress) {
        millisleep(50);
    }
    
	if (!zh) {
        DEBUG_PRINT_ERROR(__LOGTAG__, "qzookeeper zh == null - %s", state2String(connection_state));
		return -1;
	}
	if (connection_state != ZOO_CONNECTED_STATE) {
		DEBUG_PRINT_ERROR(__LOGTAG__,
						  "set_data - zk state not in ZOO_CONNECTED_STATE %s, "
						  "connection_state:%d [%s]",
						  zk_path.c_str(), connection_state, state2String(connection_state));
		return -2;
	}
	if (op_in_progress) {
		return -5;
	}
	op_in_progress = true;
	op_result = -1;
	int rc = zoo_aset(zh, zk_path.c_str(), data.c_str(), (int) data.length(), -1, my_stat_completion, this);
	if (rc) {
		DEBUG_PRINT_ERROR(__LOGTAG__, "Error (%s) setting %s", zerror(rc), data.length(), zk_path.c_str());
		op_in_progress = false;
		return -1;
	}
	DEBUG_PRINT(LOG_LEVEL_0, __LOGTAG__, "set : %.*s", data.length(), data.c_str());
	while (op_in_progress) {
		millisleep(50);
	}
	return op_result.load();
}

int qzookeeper::get_data(const qstring& zk_path, qstring& result, const qstring& default_value) {
	result = default_value;
    
    while (retry_in_progress) {
        millisleep(50);
    }
    
	if (!zh) {
        DEBUG_PRINT_ERROR(__LOGTAG__, "qzookeeper zh == null - %s", state2String(connection_state));
		return -1;
	}
	if (connection_state != ZOO_CONNECTED_STATE) {
		DEBUG_PRINT_ERROR(__LOGTAG__,
						  "get_data - zk state not in ZOO_CONNECTED_STATE %s, "
						  "connection_state:%d [%s]",
						  zk_path.c_str(), connection_state, state2String(connection_state));
		return -2;
	}
	if (op_in_progress) {
		return -5;
	}
//	DEBUG_PRINT(LOG_LEVEL_0, __LOGTAG__, "get : %.*s", zk_path.length(), zk_path.c_str());
	op_in_progress = true;
	op_result = -1;
	int rc = zoo_aget(zh, zk_path.c_str(), 1, my_data_completion, this);
	if (rc) {
		DEBUG_PRINT_ERROR(__LOGTAG__, "Error (%s) fetching %s", zerror(rc), zk_path.c_str());
		op_in_progress = false;
		return -1;
	}

	while (op_in_progress) {
		millisleep(50);
	}
	result.copy(get_result);
    int return_val = op_result.load();
    if (return_val==0) {
        DEBUG_PRINT(LOG_LEVEL_0, __LOGTAG__, "get : %.*s", zk_path.length(), zk_path.c_str());
        DEBUG_RAW(LOG_LEVEL_0, "\t[zk] : value = %.*s", result.length(), result.c_str());
    }
	return return_val;
}

int qzookeeper::delete_path(const qstring& zk_path) {
    while (retry_in_progress) {
        millisleep(50);
    }
	if (!zh) {
        DEBUG_PRINT_ERROR(__LOGTAG__, "qzookeeper zh == null - %s", state2String(connection_state));
		return -1;
	}
	if (connection_state != ZOO_CONNECTED_STATE) {
		DEBUG_PRINT_ERROR(__LOGTAG__,
						  "delete_path - zk state not in ZOO_CONNECTED_STATE %s, "
						  "connection_state:%d [%s]",
						  zk_path.c_str(), connection_state, state2String(connection_state));
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
		DEBUG_PRINT_ERROR(__LOGTAG__, "Error (%s) delete %s", zerror(rc), zk_path.c_str());
		op_in_progress = false;
		return -1;
	}

	while (op_in_progress) {
		millisleep(50);
	}
	return op_result;
}

void qzookeeper::close_zk(const int state) {
    const char* logtag = name.c_str(); // its safe to read since no one going to write on this variable except constructor;
	UNUSED(state);
    if (close_mutex.tryLock(__FUNCTION__) != 0) {
        DEBUG_PRINT_ERROR(logtag, "qzookeeper - close_zk acquire lock failed. returning !!!");
        return;
    }
	if (zh != nullptr) {
		zookeeper_close(zh);
		zh = nullptr;
        DEBUG_PRINT_IMPORTANT2(logtag, "qzookeeper - close_zk called !!!");
	}
    DEBUG_ASSERT(logtag, (close_mutex.unLock() == 0), "close_zk unlock CHECK !!!");
}

void qzookeeper::register_value_change_callback(type_qzk_value_changed callback, void* context) {
    std::map<type_qzk_value_changed, void*>::iterator it = value_change_callbacks.find(callback);
    if (it!=value_change_callbacks.end()) {
        return;
    }
    value_change_callbacks[callback] = context;
}

void qzookeeper::unregister_value_change_callback(type_qzk_value_changed callback, void* context) {
    std::map<type_qzk_value_changed, void*>::iterator it = value_change_callbacks.find(callback);
    if (it!=value_change_callbacks.end()) {
        value_change_callbacks.erase(it);
    }
}

void qzookeeper::broadcast_value_change_to_all(const qstring& path, const qstring& data) {
    for (std::map<type_qzk_value_changed, void*>::iterator it = value_change_callbacks.begin(); it != value_change_callbacks.end(); it++) {
        it->first(path, data, it->second);
    }
}
