//
//  discord_util.hpp
//  servercommon
//
//  Created by Arun A on 16/02/24.
//

#ifndef discord_util_hpp
#define discord_util_hpp

#include "../common/qstring.h"
#include <pthread.h>

#undef __LOGTAG__
#define __LOGTAG__ "discord_util"

class discord_util {
public:
    struct discord_async_data {
    private:
        discord_async_data(){};
    public:
        discord_async_data(const qstring& msg) : msg(msg) {
        }
        pthread_t tid;
        qstring msg;
    };
    
    static void set_web_hook(const qstring& web_hook);
    static int send(const qstring& msg);
    static void send_async(const qstring& msg);
    
private:
    static void* send_async_internal(void* data);
    static qstring current_web_hook;
};

#endif /* discord_util_hpp */
