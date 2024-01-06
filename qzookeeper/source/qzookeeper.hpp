//
//  qzookeeper.hpp
//  qzookeeper
//
//  Created by Arun A on 05/01/24.
//

#ifndef qzookeeper_hpp
#define qzookeeper_hpp

#include <zookeeper.h>
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

class qzookeeper {
public:
    int init(int argc, char **argv);
    
    int init_test(const qstring& url);
    
private:
    void processline(const char *line);
    int handleBatchMode(const char* arg, const char** buf);
    static void watcher(zhandle_t *zzh, int type, int state, const char *path,
                 void* context);
    
    static void my_string_completion(int rc, const char *name, const void *data);
    static void my_string_completion_free_data(int rc, const char *name, const void *data);
    static void my_string_stat_completion(int rc, const char *name, const struct Stat *stat,
            const void *data);
    static void my_string_stat_completion_free_data(int rc, const char *name,
            const struct Stat *stat, const void *data);
    static void my_data_completion(int rc, const char *value, int value_len,
            const struct Stat *stat, const void *data);
    static void my_silent_data_completion(int rc, const char *value, int value_len,
            const struct Stat *stat, const void *data);
    static void my_strings_completion(int rc, const struct String_vector *strings,
            const void *data);
    static void my_strings_stat_completion(int rc, const struct String_vector *strings,
            const struct Stat *stat, const void *data);
    static void my_void_completion(int rc, const void *data);
    static void my_stat_completion(int rc, const struct Stat *stat, const void *data);
    static void my_silent_stat_completion(int rc, const struct Stat *stat,
            const void *data);
    static void sendRequest(const char* data);
    static void od_completion(int rc, const struct Stat *stat, const void *data);
    
    static int startsWith(const char *line, const char *prefix);
    int verbose = 0;
    char *hostPort = nullptr;
    static zhandle_t *zh;
    clientid_t myid;
    const char *clientIdFile = nullptr;
    static timeval startTime;
    
    const char *cmd = nullptr;
    const char *cert = nullptr;
    static int batchMode;
    
    static int to_send;
    static int sent;
    static int recvd;
    
    static int shutdownThisThing;
};
#endif /* qzookeeper_hpp */
