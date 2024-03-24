//
//  qhiredis_async.hpp
//  qhiredis
//
//  Created by Arun A on 03/03/24.
//

#ifndef qhiredis_async_hpp
#define qhiredis_async_hpp

#include "../../common/qstring.h"
#include "../../common/sdktypes.hpp"

#include <adapters/libev.h>
#include <async.h>
#include <event.h>
#include <hiredis.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#undef __LOGTAG__
#define __LOGTAG__ "qhiredis_async"

class interface_qhiredis_async {
   public:
	virtual void on_qhiredis_async_key_expired(const qstring& expired_key) = 0;
};

class qhiredis_async {
   public:
	qhiredis_async(const qstring& redis_ip, uint16_t redis_port, interface_qhiredis_async* interface);
	~qhiredis_async();

	int connect_async_redis(struct ev_loop* loop);
	void disconnect_async_redis();

   private:
	static void on_connect_cb(const redisAsyncContext* c, int status);
	static void on_disconnect_cb(const redisAsyncContext* c, int status);
	static void on_message_cb(struct redisAsyncContext* c, void* reply, void* priv);

	redisAsyncContext* async_context = nullptr;
	qstring redis_ip = "127.0.0.1";
	uint16_t redis_port;
	interface_qhiredis_async* interface = nullptr;
};

#endif /* qhiredis_async_hpp */
