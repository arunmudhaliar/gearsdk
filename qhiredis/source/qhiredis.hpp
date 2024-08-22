/**
 * @file qhiredis.hpp
 * @brief This file contains the declaration of the `qhiredis` class, which provides a C++ wrapper for the hiredis library.
 *
 * The `qhiredis` class allows connecting to a Redis server, performing various operations like setting and getting values,
 * working with hash fields, incrementing and decrementing values, and more.
 * It also provides iterators for iterating over hash fields and scanning keys with a specific prefix.
 * The class uses the hiredis library for interacting with Redis.
 *
 * @note This file includes other headers and defines some types and macros specific to the qhiredis project.
 * It also contains commented-out code that can be used for testing or as an example.
 *
 * @note For more information on Redis, refer to the official Redis documentation: https://redis.io/documentation
 * For more information on the hiredis library, refer to the hiredis GitHub repository: https://github.com/redis/hiredis
 *
 * @author Arun A
 * @date 2023
 * @copyright 2024 homenet25
 */

#ifndef qhiredis_hpp
#define qhiredis_hpp

#include "../../common/qstring.h"
#include "../../common/sdktypes.hpp"

#include <functional>
#include <hiredis.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#undef __LOGTAG__
#define __LOGTAG__ "qhiredis"

/**
 * @typedef type_redis_hash_iterator_field_value_cb
 * @brief Callback function type for iterating over hash fields.
 * @param field The field name.
 * @param value The field value.
 * @param arg Additional argument passed to the callback function.
 */
typedef std::function<void(const char* field, const char* value, void* arg)> type_redis_hash_iterator_field_value_cb;

/**
 * @typedef type_redis_scan_iterator_key_field_value_cb
 * @brief Callback function type for scanning keys with a specific prefix.
 * @param key The key name.
 * @param field The field name.
 * @param value The field value.
 * @param arg Additional argument passed to the callback function.
 */
typedef std::function<void(const char* key, const char* field, const char* value, void* arg)> type_redis_scan_iterator_key_field_value_cb;

/**
 * @class qhiredis
 * @brief Provides a C++ wrapper for the hiredis library.
 *
 * The `qhiredis` class allows connecting to a Redis server, performing various operations like setting and getting values,
 * working with hash fields, incrementing and decrementing values, and more.
 * It also provides iterators for iterating over hash fields and scanning keys with a specific prefix.
 * The class uses the hiredis library for interacting with Redis.
 */

class qhiredis {
   public:
	/**
	 * @brief Constructs a `qhiredis` object.
	 * @param name The name of the `qhiredis` object.
	 * @param redis_ip The IP address of the Redis server.
	 * @param redis_port The port number of the Redis server.
	 */
	qhiredis(const qstring name, const qstring& redis_ip, uint16_t redis_port);

	/**
	 * @brief Destroys the `qhiredis` object.
	 */
	~qhiredis();

	/**
	 * @brief Connects to the Redis server.
	 * @param unix_socket Flag indicating whether to use a Unix socket for connection.
	 * @return 0 if the connection is successful, -1 otherwise.
	 */
	int connect_redis(bool unix_socket = false);

	/**
	 * @brief Disconnects from the Redis server.
	 */
	void disconnect_redis();

	/**
	 * @brief Sets the value of a key in Redis.
	 * @param key The key name.
	 * @param value The value to set.
	 * @return 0 if the operation is successful, -1 otherwise.
	 */
	int set_value(const qstring& key, const qstring& value);

	/**
	 * @brief Sets the value of a key in Redis with an expiry time.
	 * @param key The key name.
	 * @param value The value to set.
	 * @param expiry_in_sec The expiry time in seconds.
	 * @return 0 if the operation is successful, -1 otherwise.
	 */
	int set_value(const qstring& key, const qstring& value, int expiry_in_sec);

	/**
	 * @brief Gets the value of a key from Redis.
	 * @param key The key name.
	 * @param value The variable to store the retrieved value.
	 * @return 0 if the operation is successful, -1 otherwise.
	 */
	int get_value(const qstring& key, qstring& value);

	/**
	 * @brief Sets the value of a field in a Redis hash.
	 * @param hashkey The hash key name.
	 * @param field The field name.
	 * @param value The value to set.
	 * @return 0 if the operation is successful, -1 otherwise.
	 */
	int set_hash_value(const qstring& hashkey, const qstring& field, const qstring& value);

	/**
	 * @brief Gets the value of a field from a Redis hash.
	 * @param hashkey The hash key name.
	 * @param field The field name.
	 * @param value The variable to store the retrieved value.
	 * @return 0 if the operation is successful, -1 otherwise.
	 */
	int get_hash_value(const qstring& hashkey, const qstring& field, qstring& value);

	/**
	 * @brief Deletes a field from a Redis hash.
	 * @param hashkey The hash key name.
	 * @param field The field name to delete.
	 * @return 0 if the operation is successful, -1 otherwise.
	 */
	int delete_hash_field(const qstring& hashkey, const qstring& field);

	/**
	 * @brief Increments the value of a key in Redis.
	 * @param key The key name.
	 * @param value The variable to store the incremented value.
	 * @return 0 if the operation is successful, -1 otherwise.
	 */
	int incr(const qstring& key, long long& value);

	/**
	 * @brief Decrements the value of a key in Redis.
	 * @param key The key name.
	 * @param value The variable to store the decremented value.
	 * @return 0 if the operation is successful, -1 otherwise.
	 */
	int decr(const qstring& key, long long& value);

	/**
	 * @brief Increments the value of a key in Redis by a specified delta.
	 * @param key The key name.
	 * @param delta The delta value to increment by.
	 * @param value The variable to store the incremented value.
	 * @return 0 if the operation is successful, -1 otherwise.
	 */
	int incr_by(const qstring& key, const int delta, long long& value);

	/**
	 * @brief Decrements the value of a key in Redis by a specified delta.
	 * @param key The key name.
	 * @param delta The delta value to decrement by.
	 * @param value The variable to store the decremented value.
	 * @return 0 if the operation is successful, -1 otherwise.
	 */
	int decr_by(const qstring& key, const int delta, long long& value);

	/**
	 * @brief Sets an expiry time for a key in Redis.
	 * @param key The key name.
	 * @param expiry_in_sec The expiry time in seconds.
	 * @return 0 if the operation is successful, -1 otherwise.
	 */
	int expire_key(const qstring& key, int expiry_in_sec);

	/**
	 * @brief Iterates over the fields of a Redis hash.
	 * @param key The hash key name.
	 * @param arg Additional argument to pass to the callback function.
	 * @param callback The callback function to invoke for each field-value pair.
	 */
	void iterate_hash(const qstring& key, void* arg, type_redis_hash_iterator_field_value_cb callback);

	/**
	 * @brief Deletes a key from Redis.
	 * @param key The key name to delete.
	 * @return 0 if the operation is successful, -1 otherwise.
	 */
	int delete_key(const qstring& key);

	/**
	 * @brief Scans keys with a specific prefix in Redis.
	 * @param prefix_key The prefix of the keys to scan.
	 * @param arg Additional argument to pass to the callback function.
	 * @param callback The callback function to invoke for each key-field-value triple.
	 */
	void scan(const qstring& prefix_key, void* arg, type_redis_scan_iterator_key_field_value_cb callback);

   private:
	/**
	 * @brief Retries the connection to the Redis server.
	 * @return 0 if the connection is successful, -1 otherwise.
	 */
	int retry_connection();

	/**
	 * @brief Connects to the Redis server.
	 * @param hostname The hostname or IP address of the Redis server.
	 * @param port The port number of the Redis server.
	 * @param unix_socket Flag indicating whether to use a Unix socket for connection.
	 * @return 0 if the connection is successful, -1 otherwise.
	 */
	int connect_redis_internal(const qstring& hostname = "127.0.0.1", uint16_t port = 6379, bool unix_socket = false);

	/**
	 * @brief Iterates over the fields of a Redis hash.
	 * @param context The Redis context.
	 * @param hash_key The hash key name.
	 * @param arg Additional argument to pass to the callback function.
	 * @param callback The callback function to invoke for each field-value pair.
	 */
	static void iterate_hash(redisContext* context, const char* hash_key, void* arg, type_redis_hash_iterator_field_value_cb callback);

	redisContext* context = nullptr; /**< The Redis context. */
	const qstring name;				 /**< The name of the `qhiredis` object. */
	qstring redis_ip = "127.0.0.1";	 /**< The IP address of the Redis server. */
	uint16_t redis_port;			 /**< The port number of the Redis server. */
};

#endif /* qhiredis_hpp */
