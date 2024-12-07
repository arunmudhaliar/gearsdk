//
//  Copyright 2024 homenet25
//  qhiredis_async.cpp
//  qhiredis
//
//  Created by Arun A on 03/03/24.
//

#include "qhiredis_async.hpp"

qhiredis_async::qhiredis_async(const qstring& redis_ip, uint16_t redis_port, const qstring& username, const qstring& password, interface_qhiredis_async* interface, const qstring& key_event_config)
	: redis_ip(redis_ip), redis_port(redis_port), redis_user_name(username), redis_user_password(password), interface(interface), key_event_config(key_event_config) {
	async_context = nullptr;
}

qhiredis_async::~qhiredis_async() {
	disconnect_async_redis();
}

void qhiredis_async::set_notify_keyspace_events_cb(redisAsyncContext* context, void* reply, void* privdata) {
	if (reply == nullptr) {
		debug_print_error(__LOGTAG__, "f:set_notify_keyspace_events_cb - err: %s", context->errstr);
		return;
	}
	redisReply* r = (redisReply*) reply;
	debug_print(LOG_LEVEL_0, __LOGTAG__, "f:set_notify_keyspace_events_cb - Reply:%s", r->str);
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

	// Authenticate with username and password
	if (!redis_user_name.isempty() && !redis_user_password.isempty()) {
		qstring auth_command = qstring::format_string("AUTH %s %s", redis_user_name.c_str(), redis_user_password.c_str());
		redisAsyncCommand(async_context, nullptr, nullptr, auth_command.c_str());
		debug_print(LOG_LEVEL_0, __LOGTAG__, "Sent AUTH command for username: %s", redis_user_name.c_str());
	} else if (!redis_user_password.isempty()) {
		// Fallback to AUTH <password> for Redis without ACLs (older versions or single password setups)
		redisAsyncCommand(async_context, nullptr, nullptr, "AUTH %s", redis_user_password.c_str());
		debug_print(LOG_LEVEL_0, __LOGTAG__, "Sent AUTH command with password.");
	} else {
		debug_warn(LOG_LEVEL_0, __LOGTAG__, "No username or password provided for authentication.");
	}

	if (key_event_config.length()) {
		// Make sure we have set the config for key events
		// cmd: config set notify-keyspace-events "KEA"
		// Set the notify-keyspace-events configuration asynchronously
		redisAsyncCommand(async_context, set_notify_keyspace_events_cb, this, key_event_config.c_str());

		// Subscribe to the keyspace events for all keys
		redisAsyncCommand(async_context, on_redis_event_cb, this, "PSUBSCRIBE __keyevent*:*");
	}
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
		qhiredis_async* thiz = reinterpret_cast<qhiredis_async*>(c->data);
		thiz->connected = true;
		debug_print(LOG_LEVEL_0, __LOGTAG__, "Connected hiredis async...");
		if (thiz->interface) {
			thiz->interface->on_qhiredis_connect();
		}
	}
}

void qhiredis_async::on_disconnect_cb(const redisAsyncContext* c, int status) {
	if (status != REDIS_OK) {
		debug_print_error(__LOGTAG__, "Message: %s", c->errstr);
	} else {
		qhiredis_async* thiz = reinterpret_cast<qhiredis_async*>(c->data);
		thiz->connected = false;
		debug_print(LOG_LEVEL_0, __LOGTAG__, "Disconnected hiredis async...");
		if (thiz->interface) {
			thiz->interface->on_qhiredis_disconnect();
		}
	}
}

// Helper function to wrap lambda in a static callback
void qhiredis_async::get_value_async_lambda_callback(redisAsyncContext* context, void* reply, void* privdata) {
	std::function<void(redisAsyncContext*, redisReply*)>* callback = static_cast<std::function<void(redisAsyncContext*, redisReply*)>*>(privdata);
	redisReply* r = static_cast<redisReply*>(reply);
	if (callback) {
		(*callback)(context, r);
	}
	GX_DELETE(callback);
}

int qhiredis_async::get_value_async(const qstring& key, std::function<void(const qstring&)> on_success, std::function<void(const qstring&)> on_error) {
	if (!async_context) {
		return 2;
	}

	// Create the lambda to handle the reply
	auto lambda = [on_success, on_error](redisAsyncContext* context, redisReply* reply) {
		if (reply == nullptr) {
			if (context->err) {
				on_error(context->errstr);
			}
			return;
		}
		if (reply->type == REDIS_REPLY_STRING) {
			qstring value = qstring::format_string("%.*s", reply->len, reply->str);
			on_success(value);
		} else {
			on_error("Unexpected reply type");
		}
	};

	auto callback_wrapper = DEBUG_NEW std::function<void(redisAsyncContext*, redisReply*)>(lambda);
	int status = redisAsyncCommand(async_context, get_value_async_lambda_callback, callback_wrapper, "GET %b", key.c_str(), key.length());
	if (status != REDIS_OK) {
		on_error("Failed to issue Redis GET command");
		GX_DELETE(callback_wrapper);
		return 1;
	}

	return 0;
}

void qhiredis_async::on_redis_event_cb(struct redisAsyncContext* c, void* reply, void* priv) {
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
		} else if (r->elements == 4 && strncmp(r->element[2]->str, "__keyevent@0__:expire", strlen("__keyevent@0__:expire")) == 0) {
			debug_print(LOG_LEVEL_4, __LOGTAG__, "Received expire hiredis async: %s %s", r->element[2]->str, r->element[3]->str);
		} else if (r->elements == 4 && strncmp(r->element[2]->str, "__keyevent@0__:set", strlen("__keyevent@0__:set")) == 0) {
			debug_print(LOG_LEVEL_4, __LOGTAG__, "Received set hiredis async: %s %s", r->element[2]->str, r->element[3]->str);
			if (thiz->interface) {
				thiz->interface->on_qhiredis_async_key_changed(qstring(r->element[3]->str), "set");
			}
		} else if (r->elements == 4 && strncmp(r->element[2]->str, "__keyevent@0__:hset", strlen("__keyevent@0__:hset")) == 0) {
			debug_print(LOG_LEVEL_4, __LOGTAG__, "Received hset hiredis async: %s %s", r->element[2]->str, r->element[3]->str);
			if (thiz->interface) {
				thiz->interface->on_qhiredis_async_key_changed(qstring(r->element[3]->str), "hset");
			}
		} else if (r->elements == 4 && strncmp(r->element[2]->str, "__keyevent@0__:del", strlen("__keyevent@0__:del")) == 0) {
			debug_print(LOG_LEVEL_4, __LOGTAG__, "Received del hiredis async: %s %s", r->element[2]->str, r->element[3]->str);
			if (thiz->interface) {
				thiz->interface->on_qhiredis_async_key_changed(qstring(r->element[3]->str), "del");
			}
		} else if (r->elements == 4 && strncmp(r->element[2]->str, "__keyevent@0__:hdel", strlen("__keyevent@0__:hdel")) == 0) {
			debug_print(LOG_LEVEL_4, __LOGTAG__, "Received hdel hiredis async: %s %s", r->element[2]->str, r->element[3]->str);
			if (thiz->interface) {
				thiz->interface->on_qhiredis_async_key_changed(qstring(r->element[3]->str), "hdel");
			}
		} else {
			debug_print(LOG_LEVEL_4, __LOGTAG__, "Unhandled: Received hiredis event async: %s %s", r->element[1]->str, r->element[2]->str);
		}
	}
}
