//
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
#include <proto.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <sys/time.h>
#include <unistd.h>
#include <sys/select.h>
#include <getopt.h>

#include <time.h>
#include <errno.h>
#include <assert.h>

#include "../../common/qstring.h"
#include "../../common/sdktypes.hpp"
#include "../../networkcommon/source/essentials.hpp"
#include "../../networkcommon/source/qtimer.hpp"

#undef __LOGTAG__
#define __LOGTAG__ "qzookeeper"

class qzookeeper : public qtimer_sceduler{
public:
    qzookeeper();
    ~qzookeeper();

    int connect(const qstring& url);
    void shutdown();
    bool is_running() { return running; }
    int get_data(const qstring& zk_path, qstring& result, const qstring& default_value="{}");
    int set_data(const qstring& zk_path, const qstring& data);
    int delete_path(const qstring& zk_path);
    
private:
    int retry_connection();
    void close_zk(const int state);
    
    static const char* state2String(int state);
    static const char* type2String(int state);
    static void watcher(zhandle_t *zzh, int type, int state, const char *path, void* context);
    static void my_stat_completion(int rc, const struct Stat *stat, const void *data);
    static void my_data_completion(int rc, const char *value, int value_len, const struct Stat *stat, const void *data);
    static void my_void_completion(int rc, const void *data);
    static void dumpStat(const struct Stat *stat);
    static void millisleep(int ms);
    
    static void* connect_internal(void* data);
    
    zhandle_t* zh = nullptr;
    clientid_t myid;
    const char* clientIdFile = nullptr;
    
    pthread_t zk_thread_id;
    qstring connection_url;
    std::atomic<bool> connection_in_progress;
    std::atomic<bool> running;
    struct ev_loop* mainloop = nullptr;
    std::atomic<bool> op_in_progress;
    std::atomic<int> op_result;
    qstring get_result;
    int connection_state = -1;
    int retry_count = 0;
    qtimer* connection_check_timer = nullptr;
};
#endif /* qzookeeper_hpp */
