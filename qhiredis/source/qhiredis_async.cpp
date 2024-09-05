//
//  Copyright 2024 homenet25
//  qhiredis_async.cpp
//  qhiredis
//
//  Created by Arun A on 03/03/24.
//

#include "qhiredis_async.hpp"

qhiredis_async::qhiredis_async(const qstring& redis_ip, uint16_t redis_port, interface_qhiredis_async* interface) : redis_ip(redis_ip), redis_port(redis_port), interface(interface) {
	async_context = nullptr;
}

qhiredis_async::~qhiredis_async() {
	disconnect_async_redis();
}

int qhiredis_async::connect_async_redis(struct ev_loop* loop) {
	if (async_context) {
		debug_warn(LOG_LEVEL_0, __LOGTAG__, "Already connected !!!");
		return 2;
	}
	async_context = redisAsyncConnect(redis_ip.c_str(), redis_port);
	if (async_context->err) {
		debug_print_error(__LOGTAG__, "Connection failed : %s", async_context->errstr);
		return 1;
	} else {
		debug_print(LOG_LEVEL_0, __LOGTAG__, "Trying async connection to %s:%d", redis_ip.c_str(), redis_port);
	}

	// redisLibeventAttach(async_context, base);
	redisLibevAttach(loop, async_context);
	async_context->data = this;
	redisAsyncSetConnectCallback(async_context, on_connect_cb);
	redisAsyncSetDisconnectCallback(async_context, on_disconnect_cb);

	// Subscribe to the keyspace events for all keys
	redisAsyncCommand(async_context, on_message_cb, this, "PSUBSCRIBE __key*__:*");

	return 0;
}

void qhiredis_async::disconnect_async_redis() {
	if (async_context == nullptr) {
		debug_warn(LOG_LEVEL_0, __LOGTAG__, "qhiredis_async::connect_async_redis async_context == null");
		return;
	}

	if (!connected) {
		debug_warn(LOG_LEVEL_0, __LOGTAG__, "qhiredis_async not connected. Returning!!!");
		return;
	}
	redisAsyncContext* temp_async_context = async_context;
	async_context = nullptr;
	redisAsyncDisconnect(temp_async_context);
	redisAsyncFree(temp_async_context);
}

void qhiredis_async::on_connect_cb(const redisAsyncContext* c, int status) {
	if (status != REDIS_OK) {
		debug_print_error(__LOGTAG__, "Message: %s", c->errstr);
	} else {
		qhiredis_async* thiz = reinterpret_cast<qhiredis_async*>(const_cast<redisAsyncContext*>(c));
		thiz->connected = true;
		debug_print(LOG_LEVEL_0, __LOGTAG__, "Connected hiredis async...");
	}
}

void qhiredis_async::on_disconnect_cb(const redisAsyncContext* c, int status) {
	if (status != REDIS_OK) {
		debug_print_error(__LOGTAG__, "Message: %s", c->errstr);
	} else {
		qhiredis_async* thiz = reinterpret_cast<qhiredis_async*>(const_cast<redisAsyncContext*>(c));
		thiz->connected = false;
		debug_print(LOG_LEVEL_0, __LOGTAG__, "Disconnected hiredis async...");
	}
}

void qhiredis_async::on_message_cb(struct redisAsyncContext* c, void* reply, void* priv) {
	UNUSED(c);
	redisReply* r = (redisReply*) reply;
	qhiredis_async* thiz = (qhiredis_async*) priv;

	if (reply == nullptr || thiz == nullptr)
		return;

	if (r->type == REDIS_REPLY_ARRAY && r->elements > 2) {
		if (r->elements == 4 && strncmp(r->element[2]->str, "__keyevent@0__:expired", strlen("__keyevent@0__:expired")) == 0) {
			debug_print(LOG_LEVEL_4, __LOGTAG__, "Received expiry hiredis async: %s %s", r->element[2]->str, r->element[3]->str);
			if (thiz->interface) {
				thiz->interface->on_qhiredis_async_key_expired(qstring(r->element[3]->str));
			}
		} else {
			debug_print(LOG_LEVEL_4, __LOGTAG__, "Received message hiredis async: %s %s", r->element[1]->str, r->element[2]->str);
		}
	}
}
