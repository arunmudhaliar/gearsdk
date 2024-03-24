//
//  qhiredis.hpp
//  qhiredis
//
//  Created by Arun A on 01/11/23.
//

#ifndef qhiredis_hpp
#define qhiredis_hpp

#include "../../common/qstring.h"
#include "../../common/sdktypes.hpp"

#include <hiredis.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#undef __LOGTAG__
#define __LOGTAG__ "qhiredis"

// https://www.tutorialspoint.com/redis/redis_server.htm
// https://redis.io/docs/management/persistence/

// typedef void (*type_redis_hash_iterator_key_value_cb)(const char* field, const char* value, void* arg);
typedef std::function<void(const char* field, const char* value, void* arg)> type_redis_hash_iterator_key_value_cb;

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

	int set_hash_value(const qstring& hashkey, const qstring& field, const qstring& value);
	int get_hash_value(const qstring& hashkey, const qstring& field, qstring& value);

	int incr(const qstring& key, long long& value);
	int decr(const qstring& key, long long& value);

	int incr_by(const qstring& key, const int delta, long long& value);
	int decr_by(const qstring& key, const int delta, long long& value);

	int expire_key(const qstring& key, int expiry_in_sec);
	void iterate_hash(const qstring& key, void* arg, type_redis_hash_iterator_key_value_cb callback);
	int delete_key(const qstring& key);

   private:
	int connect_redis_internal(const qstring& hostname = "127.0.0.1", uint16_t port = 6379, bool unix_socket = false);
	static void iterate_hash(redisContext* context, const char* hash_key, void* arg, type_redis_hash_iterator_key_value_cb callback);
	redisContext* context = nullptr;
	qstring redis_ip = "127.0.0.1";
	uint16_t redis_port;
};

#endif /* qhiredis_hpp */
