#include "./discord_util.hpp"
#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdocumentation"
#endif
#include <dpp/dpp.h>
#ifdef __clang__
#pragma clang diagnostic pop
#endif
#include "../common/sdktypes.hpp"

#define DEFUALT_DISCORD_WEB_HOOK "https://discord.com/api/webhooks/1207911659214082058/A0S49aiBOJKVZJk5FUUQaAw3Qxl2oRmRFdf7R93B8Y60QPuagXS0F3gLKS3yYRQrTyo4"

qstring discord_util::current_web_hook = DEFUALT_DISCORD_WEB_HOOK;
// pthread_once_t discord_util::init_once = PTHREAD_ONCE_INIT;
std::atomic<bool> discord_util::inited = false;
// pthread_mutex_t discord_util::webhook_mutex = PTHREAD_MUTEX_INITIALIZER;
// uint64_t discord_util::counter = 0;

std::queue<qstring> discord_util::request_queue;
std::mutex discord_util::queue_mutex;
std::condition_variable discord_util::queue_cv;
std::atomic<bool> discord_util::done(false);
std::vector<std::thread> discord_util::worker_threads;
std::atomic<int> discord_util::active_requests(0);
const int discord_util::MAX_CONCURRENT_REQUESTS = 5;

void discord_util::request_worker() {
	PTHREAD_NAME("discord_util_worker");
	while (!done) {
		std::unique_lock<std::mutex> lock(queue_mutex);
		queue_cv.wait(lock, [] { return !request_queue.empty() || done; });

		if (done && request_queue.empty()) {
			break;
		}

		if (active_requests >= MAX_CONCURRENT_REQUESTS) {
			lock.unlock();
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
			continue;
		}

		qstring msg = request_queue.front();
		request_queue.pop();
		++active_requests;
		lock.unlock();

		send(msg);

		--active_requests;
	}
}

void discord_util::initialize_with_webhook_url(const qstring& web_hook) {
	// Initialization code if needed
	set_web_hook(web_hook);
	done = false;
	inited = true;

	// Start worker threads
	for (int i = 0; i < MAX_CONCURRENT_REQUESTS; ++i) {
		worker_threads.emplace_back(request_worker);
	}
}

void discord_util::set_web_hook(const qstring& web_hook) {
	std::lock_guard<std::mutex> lock(queue_mutex);
	current_web_hook = web_hook;
}

int discord_util::send(const qstring& msg, int retry_count) {
	const int MAX_RETRIES = 5;
	const int BASE_DELAY_MS = 1000;	 // base delay in milliseconds for exponential backoff

	try {
		dpp::cluster bot(""); /* Normally, you put your bot token in here, but it's not required for webhooks. */
		bot.on_log(dpp::utility::cout_logger());

		std::lock_guard<std::mutex> lock(queue_mutex);
		const char* url = current_web_hook.c_str();
		if (url == nullptr) {
			std::cerr << "url == null" << std::endl;
			return -1;
		}
		dpp::webhook wh(current_web_hook.c_str());
		bot.execute_webhook_sync(wh, dpp::message(msg.c_str()));
	} catch (const dpp::rest_exception& e) {
		std::cerr << "Caught exception: " << e.what() << ", msg: " << msg.c_str() << std::endl;
	} catch (const std::system_error& e) {
		std::cerr << "Caught std::system_error: " << e.what() << ", msg: " << msg.c_str() << std::endl;
	} catch (const std::exception& e) {
		std::string error_message = e.what();
		if (error_message.find("You are being rate limited.") != std::string::npos) {
			if (retry_count < MAX_RETRIES) {
				int delay = BASE_DELAY_MS * (1 << retry_count);	 // Exponential backoff
				std::cerr << "Rate limited. Retrying after " << delay << " milliseconds" << std::endl;
				std::this_thread::sleep_for(std::chrono::milliseconds(delay));
				return send(msg, retry_count + 1);
			} else {
				std::cerr << "Exceeded maximum retry attempts." << std::endl;
			}
		}
	} catch (...) {
		std::cerr << "Caught an unknown exception. msg: " << msg.c_str() << std::endl;
	}
	return 0;
}

void discord_util::send_async(const qstring& msg) {
	if (!discord_util::inited) {
		std::cerr << "discord_util. Not inited !!!" << std::endl;
		return;
	}

	if (!done) {
		std::lock_guard<std::mutex> lock(queue_mutex);
		if (request_queue.size() > 10) {
			std::cerr << "Discord request queue is full. Dropping message: " << msg.c_str() << std::endl;
			return;
		}
		request_queue.push(msg);
		queue_cv.notify_one();	// Notify a worker thread that a new request is available
	} else {
		std::cerr << "Discord utility is not initialized." << std::endl;
	}
}

void discord_util::shutdown() {
	if (!inited) {
		return;
	}
	debug_print(LOG_LEVEL_0, __LOGTAG__, "discord_util shut down");
	done = true;
	queue_cv.notify_all();	// Wake up all worker threads to shut down
	for (auto& t : worker_threads) {
		if (t.joinable()) {
			t.join();
		}
	}
	worker_threads.clear();
}
