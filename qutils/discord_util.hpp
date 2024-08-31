#ifndef discord_util_hpp
#define discord_util_hpp

#include "../common/qstring.h"


#include <pthread.h>
#include <atomic>

#include <iostream>
#include <thread>
#include <vector>
#include <condition_variable>
#include <queue>
#include <mutex>
#include <cstring>
#include <stdexcept>
#include <chrono>

#undef __LOGTAG__
#define __LOGTAG__ "discord_util"

/**
 * @class discord_util
 * @brief A utility class for interacting with Discord.
 *
 * The discord_util class provides methods for sending messages to Discord channels
 * synchronously and asynchronously. It also allows setting a webhook URL for sending
 * messages to a specific channel.
 * @author Arun A
 * @date 2024
 * @copyright 2024 homenet25
 */

class discord_util {
   public:
#if FOR_OLD_DD_IMPL
	/**
	 * @struct discord_async_data
	 * @brief A structure to hold data for asynchronous message sending.
	 *
	 * The discord_async_data structure holds the message and message ID for sending
	 * messages asynchronously. It also stores the thread ID of the thread that sends
	 * the message.
	 */
	struct discord_async_data {
	   private:
		discord_async_data() {};

	   public:
		/**
		 * @brief Constructs a discord_async_data object with the given message and message ID.
		 * @param msg The message to send.
		 * @param msg_id The ID of the message.
		 */
		discord_async_data(const qstring& msg, uint64_t msg_id) : msg(msg), msg_id(msg_id) {}

		pthread_t tid; ///< The thread ID of the thread that sends the message.
		qstring msg; ///< The message to send.
		uint64_t msg_id = 0; ///< The ID of the message.
	};
#endif
	/**
	 * @brief Sets the webhook URL for sending messages to Discord.
	 * @param web_hook The webhook URL to set.
	 */
	static void set_web_hook(const qstring& web_hook);

	/**
	 * @brief Sends a message to Discord synchronously.
	 * @param msg The message to send.
	 * @param retry_count The number of times to retry sending the message in case of failure.
	 * @return The result of the message sending operation.
	 */
	static int send(const qstring& msg, int retry_count = 0);

	/**
	 * @brief Sends a message to Discord asynchronously.
	 * @param msg The message to send.
	 */
	static void send_async(const qstring& msg);

	static void shutdown();

   private:
	static std::atomic<bool> inited; ///< Flag indicating whether the class has been initialized.
	static void initialize_webhook_url(); ///< Initializes the webhook URL.

	/**
	 * @brief Internal function for sending messages asynchronously.
	 * @param data A pointer to the discord_async_data structure containing the message and message ID.
	 * @return A pointer to the result of the message sending operation.
	 */
	static void* send_async_internal(void* data);

	static void request_worker();
	

	static qstring current_web_hook; ///< The current webhook URL.
	static pthread_mutex_t webhook_mutex; ///< Mutex for thread-safe access to the webhook URL.
	static uint64_t counter; ///< Counter for generating unique message IDs.

	static std::queue<qstring> request_queue;
	static std::mutex queue_mutex;
	static std::condition_variable queue_cv;
	static std::atomic<bool> done;
	static std::vector<std::thread> worker_threads;
	static std::atomic<int> active_requests;
	static const int MAX_CONCURRENT_REQUESTS;
};

#endif /* discord_util_hpp */
