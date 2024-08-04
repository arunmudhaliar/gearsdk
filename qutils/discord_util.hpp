//
//  Copyright 2024 homenet25
//  discord_util.hpp
//  servercommon
//
//  Created by Arun A on 16/02/24.
//

#ifndef discord_util_hpp
#define discord_util_hpp

#include "../common/qstring.h"

#include <pthread.h>
#include <atomic>

#undef __LOGTAG__
#define __LOGTAG__ "discord_util"

class discord_util {
   public:
	struct discord_async_data {
	   private:
		discord_async_data() {};

	   public:
		discord_async_data(const qstring& msg, uint64_t msg_id) : msg(msg), msg_id(msg_id) {}
		pthread_t tid;
		qstring msg;
        uint64_t msg_id = 0;
	};

    
	static void set_web_hook(const qstring& web_hook);
	static int send(const qstring& msg, int retry_count = 0);
	static void send_async(const qstring& msg);

   private:
    static std::atomic<bool> inited;
//    static pthread_once_t init_once;
    static void initialize_webhook_url();
    
	static void* send_async_internal(void* data);
    static qstring current_web_hook;
    static pthread_mutex_t webhook_mutex;
    static uint64_t counter;
};

#endif /* discord_util_hpp */
