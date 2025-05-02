/**
 * @file qhiredis_async.hpp
 * @brief This file contains the declaration of the qhiredis_async class, which provides an asynchronous interface for interacting with a Redis server.
 *
 * The class allows connecting to a Redis server, sending commands, and receiving responses asynchronously.
 * It also provides a callback mechanism for handling expired keys in Redis.
 *
 * @author Arun A
 * @date 03/03/24
 *
 * @copyright Copyright (c) 2024, amudaliar
 *
 */

#ifndef qhiredis_async_hpp
#define qhiredis_async_hpp

#include "../../common/qstring.hpp"
#include "../../common/sdktypes.hpp"

#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-function"
#elif defined(__GNUC__) || defined(__GNUG__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
#endif
#include <adapters/libev.h>
#if defined(__clang__)
#pragma clang diagnostic pop
#elif defined(__GNUC__) || defined(__GNUG__)
#pragma GCC diagnostic pop
#endif

#include <async.h>
#include <event.h>
#include <hiredis.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <functional>
#undef __LOGTAG__
#define __LOGTAG__ "qhiredis_async"

/**
 * @class interface_qhiredis_async
 * @brief Interface class for handling expired keys in Redis.
 */
class interface_qhiredis_async {
   public:
	/**
	 * @brief Callback function for handling expired keys in Redis.
	 *
	 * This function is called when a key in Redis expires.
	 *
	 * @param expired_key The expired key in Redis.
	 */
	virtual void on_qhiredis_async_key_expired(const qstring& expired_key) = 0;

	/**
	 * @brief Callback function for handling modified keys in Redis.
	 *
	 * This function is called when a key in Redis expires.
	 *
	 * @param modified_key The modified key in Redis.
	 *  @param event redis key event
	 */
	virtual void on_qhiredis_async_key_changed(const qstring& modified_key, const qstring& event) = 0;

	virtual void on_qhiredis_connect() = 0;
	virtual void on_qhiredis_disconnect() = 0;
};

/**
 * @class qhiredis_async
 * @brief Provides an asynchronous interface for interacting with a Redis server.
 */
class qhiredis_async {
   public:
	/**
	 * @brief Constructor for qhiredis_async class.
	 *
	 * @param redis_ip The IP address of the Redis server.
	 * @param redis_port The port number of the Redis server.
	 * @param interface A pointer to the interface_qhiredis_async object for handling callbacks.
	 * @param key_event_config if valid config, the context can only be used for SUBSCRIBE, UNSUBSCRIBE, PING, QUIT & RESET (get_value_async cant be used)
	 */
	qhiredis_async(const qstring& redis_ip, uint16_t redis_port, const qstring& username, const qstring& password, interface_qhiredis_async* interface, const qstring& key_event_config);

	/**
	 * @brief Destructor for qhiredis_async class.
	 */
	~qhiredis_async();

	/**
	 * @brief Connects to the Redis server asynchronously.
	 *
	 * @param loop The event loop to use for asynchronous operations.
	 * @return 0 if the connection is successful, -1 otherwise.
	 */
	int connect_async_redis(struct ev_loop* loop);

	/**
	 * @brief Disconnects from the Redis server.
	 */
	void disconnect_async_redis();

	/**
	 * @brief Gets the value of a key from Redis asynchronously.
	 * @param key The key name.
	 * @param on_success success callback.
	 * @param on_error error callback.
	 * @return 0 if the operation is successful, -1 otherwise.
	 */
	int get_value_async(const qstring& key, std::function<void(const qstring&)> on_success, std::function<void(const qstring&)> on_error);

   private:
	/**
	 * @brief Callback function for handling the connection status of the Redis server.
	 *
	 * @param c The redisAsyncContext object.
	 * @param status The connection status.
	 */
	static void on_connect_cb(const redisAsyncContext* c, int status);

	/**
	 * @brief Callback function for handling the disconnection status of the Redis server.
	 *
	 * @param c The redisAsyncContext object.
	 * @param status The disconnection status.
	 */
	static void on_disconnect_cb(const redisAsyncContext* c, int status);

	/**
	 * @brief Callback function for handling the response from Redis.
	 *
	 * @param c The redisAsyncContext object.
	 * @param reply The reply from Redis.
	 * @param priv A pointer to private data.
	 */
	static void on_redis_event_cb(struct redisAsyncContext* c, void* reply, void* priv);

	static void set_notify_keyspace_events_cb(redisAsyncContext* context, void* reply, void* privdata);

	static void get_value_async_lambda_callback(redisAsyncContext* context, void* reply, void* privdata);

	redisAsyncContext* async_context = nullptr;
	qstring redis_ip = "127.0.0.1";
	uint16_t redis_port;
	qstring redis_user_name = "";
	qstring redis_user_password = "";
	interface_qhiredis_async* interface = nullptr;
	bool connected = false;
	qstring key_event_config;
};

#endif /* qhiredis_async_hpp */
