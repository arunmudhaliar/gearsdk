//
//  qhiredis.hpp
//  qhiredis
//
//  Created by Arun A on 01/11/23.
//

#ifndef qhiredis_hpp
#define qhiredis_hpp

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <hiredis.h>

#include "../../common/qstring.h"
#include "../../common/sdktypes.hpp"

#undef __LOGTAG__
#define __LOGTAG__ "qhiredis"

//https://www.tutorialspoint.com/redis/redis_server.htm
//https://redis.io/docs/management/persistence/

class qhiredis {
public:
    qhiredis(const qstring& redis_ip, uint16_t redis_port);
    ~qhiredis();

#if 0
    void example_argv_command(redisContext* c, int n);
    int hiredis_main(int argc, char** argv);
#endif

    int connect_redis(bool unix_socket = false);
    void disconnect_redis();

    int set_value(const qstring& key, const qstring& value);
    int set_value(const qstring& key, const qstring& value, int expiry_in_sec);
    int get_value(const qstring& key, qstring& value);
    
private:
    int connect_redis_internal(const qstring& hostname = "127.0.0.1", uint16_t port = 6379, bool unix_socket = false);
    redisContext* context = nullptr;
    qstring redis_ip = "127.0.0.1";
    uint16_t redis_port;
};

#endif /* qhiredis_hpp */
