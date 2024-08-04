//
//  Copyright 2024 homenet25
//  discord_util.cpp
//  servercommon
//
//  Created by Arun A on 16/02/24.
//

#include "discord_util.hpp"
#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdocumentation"
#endif
#include <dpp/dpp.h>
#ifdef __clang__
#pragma clang diagnostic pop
#endif
#include "../common/sdktypes.hpp"

#define DISCORD_WEB_HOOK "https://discord.com/api/webhooks/1207911659214082058/A0S49aiBOJKVZJk5FUUQaAw3Qxl2oRmRFdf7R93B8Y60QPuagXS0F3gLKS3yYRQrTyo4"
qstring discord_util::current_web_hook = DISCORD_WEB_HOOK;
//pthread_once_t discord_util::init_once = PTHREAD_ONCE_INIT;
std::atomic<bool> discord_util::inited = false;
pthread_mutex_t discord_util::webhook_mutex = PTHREAD_MUTEX_INITIALIZER;
uint64_t discord_util::counter = 0;

void discord_util::initialize_webhook_url() {
    set_web_hook(DISCORD_WEB_HOOK);
    discord_util::inited = true;
}

void discord_util::set_web_hook(const qstring& web_hook) {
    pthread_mutex_lock(&webhook_mutex);
	current_web_hook = web_hook;
    pthread_mutex_unlock(&webhook_mutex);
}

int discord_util::send(const qstring& msg, int retry_count) {
    const int max_retries = 5;
    const int base_delay_ms = 1000; // base delay in milliseconds for exponential backoff

	dpp::cluster bot(""); /* Normally, you put your bot token in here, but its not
							 required for webhooks. */

	bot.on_log(dpp::utility::cout_logger());

	try {
        pthread_mutex_lock(&webhook_mutex);
        const char* url = current_web_hook.c_str();
        if (url == nullptr) {
            DEBUG_PRINT(LOG_LEVEL_0, __LOGTAG__, "url == null");
            return -1;
        }
        /* Construct a webhook object using the URL you got from Discord */
        dpp::webhook wh(current_web_hook.c_str());
        pthread_mutex_unlock(&webhook_mutex);
		/* Send a message with this webhook */
		bot.execute_webhook_sync(wh, dpp::message(msg.c_str()));
	} catch (const dpp::rest_exception& e) {
		DEBUG_PRINT(LOG_LEVEL_0, __LOGTAG__, "Caught exception: %s, msg: %s", e.what(), msg.c_str());
		// Implement retry logic here, respecting the Retry-After header
	} catch (const std::system_error& e) {
		DEBUG_PRINT(LOG_LEVEL_0, __LOGTAG__, "Caught std::system_error: %s, msg: %s", e.what(), msg.c_str());
		// Implement specific handling logic for std::system_error here
		// This could be related to thread join issues or other system-level errors
	} catch (const std::exception& e) {
//		DEBUG_PRINT(LOG_LEVEL_0, __LOGTAG__, "Caught std::exception: %s, msg: %s", e.what(), msg.c_str());
		// Handle other std::exception derived exceptions
        std::string error_message = e.what();
        // Check for rate limit error message
        if (error_message.find("You are being rate limited.") != std::string::npos) {
            if (retry_count < max_retries) {
                int delay = base_delay_ms * (1 << retry_count); // Exponential backoff
                DEBUG_PRINT(LOG_LEVEL_0, __LOGTAG__, "Rate limited. Retrying after %d milliseconds", delay);
                
                // Sleep for the specified delay time
                std::this_thread::sleep_for(std::chrono::milliseconds(delay));

                // Retry sending the message
                return send(msg, retry_count + 1);
            } else {
                DEBUG_PRINT(LOG_LEVEL_0, __LOGTAG__, "Exceeded maximum retry attempts.");
            }
        }
	} catch (...) {
		DEBUG_PRINT(LOG_LEVEL_0, __LOGTAG__, "Caught an unknown exception. msg: %s", msg.c_str());
		// Handle any non-standard exceptions
	}
	return 0;
}

void* discord_util::send_async_internal(void* data) {
	discord_util::discord_async_data* msg = (discord_util::discord_async_data*) data;
    PTHREAD_NAME(qstring::format_string("dpp-%d", msg->msg_id).c_str());
	discord_util::send(msg->msg);
	GX_DELETE(msg);
	pthread_exit(0);
}

void discord_util::send_async(const qstring& msg) {
    if (!discord_util::inited) {
//        DEBUG_PRINT(LOG_LEVEL_0, __LOGTAG__, "issue initialize_webhook_url");
        // Ensure the static variable is initialized before creating threads
//        pthread_once(&discord_util::init_once, initialize_webhook_url);
        initialize_webhook_url();
    }
    
	discord_util::discord_async_data* new_msg = DEBUG_NEW discord_util::discord_async_data(msg, counter++);
	if (pthread_create(&new_msg->tid, nullptr, discord_util::send_async_internal, (void*) new_msg) < 0) {
		DEBUG_PRINT_ERROR(__LOGTAG__, "could not create thread: %s - %d", strerror(errno), errno);
		GX_DELETE(new_msg);
		return;
	}
}
