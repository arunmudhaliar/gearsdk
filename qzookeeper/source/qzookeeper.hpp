/**
 * @file qzookeeper.hpp
 * @brief This file contains the declaration of the qzookeeper class and its associated interfaces.
 *
 * The qzookeeper class provides a C++ wrapper for the ZooKeeper C API. It allows connecting to a ZooKeeper server,
 * retrieving and setting data, deleting paths, and registering value change callbacks.
 *
 * The qzookeeper class implements the interface_qzookeeper interface, which defines the methods for registering and
 * unregistering value change callbacks.
 *
 * The qzookeeper class also inherits from the qtimer_sceduler class, which provides functionality for scheduling
 * timer events.
 *
 * @copyright 2024 homenet25
 * @note This file is part of the qzookeeper library.
 * @author Arun A
 * @date 05/01/24
 */
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
#include <atomic>
#include <errno.h>
#include <getopt.h>
#include <map>
#include <mutex>
#include <proto.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

#undef __LOGTAG__
#define __LOGTAG__ "qzookeeper"

#define QZK_MAX_RETRIES_FOR_API 5
#define QZK_RETRY_BASE_DELAY_MS 100

typedef void (*type_qzk_value_changed)(const qstring& path, const qstring& data, void* context);

/**
 * @brief The interface_qzookeeper class represents an interface for interacting with a ZooKeeper client.
 *
 * This class provides methods for registering and unregistering value change callbacks.
 */
class interface_qzookeeper {
   public:
	/**
	 * @brief Registers a callback function to be called when a value changes in the ZooKeeper client.
	 *
	 * @param callback The callback function to be registered.
	 * @param context A pointer to user-defined context data that will be passed to the callback function.
	 */
	virtual void register_value_change_callback(type_qzk_value_changed callback, void* context) = 0;

	/**
	 * @brief Unregisters a previously registered value change callback function.
	 *
	 * @param callback The callback function to be unregistered.
	 * @param context A pointer to the user-defined context data that was passed to the callback function.
	 */
	virtual void unregister_value_change_callback(type_qzk_value_changed callback, void* context) = 0;
};

class qzookeeper : public qtimer_sceduler, public interface_qzookeeper {
   public:
	qzookeeper(const qstring& name);
	~qzookeeper();

	/**
	 * Establishes a connection to the specified URL.
	 *
	 * @param url The URL to connect to.
	 * @return An integer representing the connection status.
	 */
	int connect(const qstring& url);
	/**
	 * @brief Shuts down the ZooKeeper client.
	 *
	 * This function is used to gracefully shut down the ZooKeeper client and release any resources
	 * held by it. After calling this function, the client will no longer be able to perform any
	 * operations.
	 */
	void shutdown();

	/**
	 * @brief Checks if the ZooKeeper instance is currently running.
	 *
	 * @return true if the ZooKeeper instance is running, false otherwise.
	 */
	bool is_running() { return running; }

	/**
	 * Check if the ZooKeeper connection is active.
	 *
	 * @return true if the ZooKeeper connection is active, false otherwise.
	 */
	bool is_zk_active() { return zh != nullptr; }
	int get_connection_state() { return connection_state; }

	/**
	 * Retrieves the data associated with the specified ZooKeeper path.
	 *
	 * @param zk_path The path of the ZooKeeper node.
	 * @param result  A reference to a qstring object to store the retrieved data.
	 * @param default_value The default value to be returned if the specified path does not exist.
	 * @return An integer value indicating the result of the operation. Zero indicates success, while
	 *         a non-zero value indicates an error.
	 */
	int get_data(const qstring& zk_path, qstring& result, const qstring& default_value = "{}");

	/**
	 * @brief Sets the data for a given ZooKeeper path.
	 *
	 * This function sets the data for the specified ZooKeeper path with the provided data.
	 *
	 * @param zk_path The path of the ZooKeeper node.
	 * @param data The data to be set for the ZooKeeper node.
	 * @return An integer value indicating the success or failure of the operation.
	 *         Returns 0 on success, and a negative value on failure.
	 */
	int set_data(const qstring& zk_path, const qstring& data);

	/**
	 * Deletes the specified path in the ZooKeeper server.
	 *
	 * @param zk_path The path to be deleted.
	 * @return Returns 0 if the path is successfully deleted, otherwise returns an error code.
	 */
	int delete_path(const qstring& zk_path);

	/**
	 * Registers a callback function to be called when the value changes.
	 *
	 * @param callback The callback function to be called when the value changes.
	 * @param context  A pointer to user-defined context data that will be passed to the callback function.
	 */
	void register_value_change_callback(type_qzk_value_changed callback, void* context) final;

	/**
	 * Unregisters a value change callback.
	 *
	 * This function is used to unregister a previously registered value change callback.
	 * The callback function and the context associated with it must match the ones used during registration.
	 *
	 * @param callback The callback function to unregister.
	 * @param context The context associated with the callback function.
	 */
	void unregister_value_change_callback(type_qzk_value_changed callback, void* context) final;

	/**
	 * @brief Broadcasts a value change to all connected clients.
	 *
	 * This function sends a value change notification to all clients connected to the ZooKeeper server.
	 * The value change is associated with the specified `path` and `data`.
	 *
	 * @param path The path associated with the value change.
	 * @param data The new data to be associated with the value change.
	 */
	void broadcast_value_change_to_all(const qstring& path, const qstring& data);

	/**
	 * Converts the given state value to its corresponding string representation.
	 *
	 * @param state The state value to convert.
	 * @return The string representation of the state value.
	 */
	static const char* state2String(int state);

	/**
	 * Converts the given state value to its corresponding string representation.
	 *
	 * @param state The state value to convert.
	 * @return The string representation of the state value.
	 */
	static const char* type2String(int state);

   private:
	/**
	 * @brief Retries the connection to the ZooKeeper server.
	 *
	 * This function attempts to reconnect to the ZooKeeper server in case the connection
	 * was lost or failed. It implements a retry mechanism to ensure a successful connection
	 * is established.
	 *
	 * @return An integer value indicating the result of the connection retry attempt.
	 *         A value of 0 indicates a successful connection, while a non-zero value
	 *         indicates a failure to establish a connection.
	 */
	int retry_connection();

	/**
	 * @brief Closes the ZooKeeper connection.
	 *
	 * This function closes the ZooKeeper connection and releases any resources associated with it.
	 *
	 * @param state The state of the ZooKeeper connection.
	 */
	void close_zk(const int state);

	const char* get_name() const;
	const char* get_wname() const;

	/**
	 * @brief Callback function for handling ZooKeeper events.
	 *
	 * This function is called when a ZooKeeper event occurs, such as a change in connection state or a node change.
	 *
	 * @param zzh The ZooKeeper handle.
	 * @param type The type of the event.
	 * @param state The state of the ZooKeeper connection.
	 * @param path The path of the node associated with the event.
	 * @param context The user-defined context data.
	 */
	static void watcher(zhandle_t* zzh, int type, int state, const char* path, void* context);

	static void my_stat_completion(int rc, const struct Stat* stat, const void* data);
	static void my_data_completion(int rc, const char* value, int value_len, const struct Stat* stat, const void* data);
	static void my_void_completion(int rc, const void* data);
	static void dumpStat(const struct Stat* stat);
	static void millisleep(int ms);

	/**
	 * @brief Establishes a connection to the ZooKeeper server.
	 *
	 * This function is responsible for establishing a connection to the ZooKeeper server.
	 * It is a static member function and should be called using the class name.
	 *
	 * @param data A pointer to the data needed for establishing the connection.
	 * @return A void pointer representing the connection object.
	 */
	static void* connect_internal(void* data);

	/**
	 * Retries an operation with backoff.
	 *
	 * This function retries the specified operation with exponential backoff. The operation is invoked using the provided
	 * `operation` function object. The maximum number of retries and the base delay between retries are specified by the
	 * `max_retries` and `base_delay_ms` parameters, respectively.
	 *
	 * @param operation The operation to be retried. This should be a function object that takes no arguments and returns
	 *                  an integer result.
	 * @param max_retries The maximum number of retries to attempt.
	 * @param base_delay_ms The base delay in milliseconds between retries. The actual delay increases exponentially with
	 *                      each retry.
	 * @return The result of the operation if it succeeds within the specified number of retries, or an error code if all
	 *         retries fail.
	 */
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
