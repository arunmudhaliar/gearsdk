//
//  Copyright 2024 homenet25
//  qzookeeper.hpp
//  qzookeeper
//
//  Created by Arun A on 05/01/24.
//

#ifndef qzookeeper_hpp
#define qzookeeper_hpp

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdocumentation"
#endif
#include <zookeeper.h>
#ifdef __clang__
#pragma clang diagnostic pop
#endif
#include "../../common/qstring.h"
#include "../../common/sdktypes.hpp"
#include "../../networkcommon/source/essentials.hpp"
#include "../../networkcommon/source/qtimer.hpp"

#include <assert.h>
#include <errno.h>
#include <getopt.h>
#include <proto.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>
#include <map>
#include <atomic>
#include <mutex>

#undef __LOGTAG__
#define __LOGTAG__ "qzookeeper"

#define QZK_MAX_RETRIES_FOR_API 5
#define QZK_RETRY_BASE_DELAY_MS 100

typedef void(*type_qzk_value_changed)(const qstring& path, const qstring& data, void* context);

class interface_qzookeeper {
public:
    virtual void register_value_change_callback(type_qzk_value_changed callback, void* context) = 0;
    virtual void unregister_value_change_callback(type_qzk_value_changed callback, void* context) = 0;
};

class qzookeeper : public qtimer_sceduler, public interface_qzookeeper {
   public:
	qzookeeper(const qstring& name);
    ~qzookeeper();

	int connect(const qstring& url);
	void shutdown();
	bool is_running() { return running; }
    bool is_zk_active() { return zh!=nullptr; }
    int get_connection_state() { return connection_state; }
    
	int get_data(const qstring& zk_path, qstring& result, const qstring& default_value = "{}");
	int set_data(const qstring& zk_path, const qstring& data);
	int delete_path(const qstring& zk_path);
    
    void register_value_change_callback(type_qzk_value_changed callback, void* context) final;
    void unregister_value_change_callback(type_qzk_value_changed callback, void* context) final;
    void broadcast_value_change_to_all(const qstring& path, const qstring& data);
    
    static const char* state2String(int state);
    static const char* type2String(int state);
    
   private:
	int retry_connection();
	void close_zk(const int state);

    const char* get_name() const;
    const char* get_wname() const;
    
	static void watcher(zhandle_t* zzh, int type, int state, const char* path, void* context);
	static void my_stat_completion(int rc, const struct Stat* stat, const void* data);
	static void my_data_completion(int rc, const char* value, int value_len, const struct Stat* stat, const void* data);
	static void my_void_completion(int rc, const void* data);
	static void dumpStat(const struct Stat* stat);
	static void millisleep(int ms);

	static void* connect_internal(void* data);
    
    static int retry_with_backoff(std::function<int()> operation, int max_retries, int base_delay_ms);
    
	zhandle_t* zh = nullptr;
	clientid_t myid;
	const char* clientIdFile = nullptr;

	pthread_t zk_thread_id;
	qstring connection_url;
    const qstring name;
    const qstring wname;
	std::atomic<bool> connection_in_progress;
	std::atomic<bool> running;
	struct ev_loop* mainloop = nullptr;
	std::atomic<bool> op_in_progress;
	std::atomic<int> op_result;
	qstring get_result;
	int connection_state = -1;
	int retry_count = 0;
	qtimer* connection_check_timer = nullptr;
    std::map<type_qzk_value_changed, void*> value_change_callbacks;
    std::vector<qstring> pending_config_updates;
    qmutex close_mutex;
    qmutex reconnect_mutex;
    std::mutex watcher_gaurd_mutex;
    std::atomic<bool> retry_in_progress;
    mutable std::timed_mutex nameMutex;
    mutable std::timed_mutex wnameMutex;
};
#endif /* qzookeeper_hpp */
