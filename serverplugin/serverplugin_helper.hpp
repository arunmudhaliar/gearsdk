//
//  serverplugin_helper.hpp
//  qh3server
//
//  Created by Arun A on 03/02/25.
//

#ifndef serverplugin_helper_hpp
#define serverplugin_helper_hpp

// Thread-safe queue
#include <queue>
#include <mutex>
#include <ev.h>
#include "../common/sdktypes.hpp"
#include "../common/qstring.hpp"

#undef __LOGTAG__
#define __LOGTAG__ "serverplugin_helper"

namespace gsdk {
namespace server {

template <class T>
class async_ev_notifier {
public:
    async_ev_notifier<T>() {
    }
    
    ~async_ev_notifier() {
        while (!response_queue.empty()) {
            GX_DELETE(response_queue.front())
            response_queue.pop();
        }
    }

    typedef void (*ev_notifier_cb)(EV_P_ ev_async *w, int revents);

    void init(struct ev_loop* loop, ev_notifier_cb callback, void* user_data) {
        if (inited) {
            debug_warn(LOG_LEVEL_0, __LOGTAG__, "async_ev_notifier already initialised !!!");
            return;
        }
        main_loop = loop;
        ev_async_callback = callback;
        ev_async_init(&async_watcher, callback);
        async_watcher.data = user_data;
        ev_async_start(main_loop, &async_watcher);
        inited = true;
        debug_print(LOG_LEVEL_0, __LOGTAG__, "async_ev_notifier initialised !!!");
    }
    
    // Thread-safe enqueue
    void enqueue_response(T* response_packet) {
        if (!inited) {
            debug_warn(LOG_LEVEL_0, __LOGTAG__, "async_ev_notifier not initialised !!!");
            return;
        }
        std::lock_guard<std::mutex> lock(queue_mutex);
        response_queue.push(response_packet);
    }

    // Thread-safe dequeue (main thread calls this)
    T* dequeue_response() {
        if (!inited) {
            debug_warn(LOG_LEVEL_0, __LOGTAG__, "async_ev_notifier not initialised !!!");
            return nullptr;
        }
        std::lock_guard<std::mutex> lock(queue_mutex);
        if (response_queue.empty()) return nullptr;
        T* response_packet = response_queue.front();
        response_queue.pop();
        return response_packet;
    }

    // Notify main thread
    void notify_main_thread() {
        if (!inited) {
            debug_warn(LOG_LEVEL_0, __LOGTAG__, "async_ev_notifier not initialised !!!");
            return;
        }
//        total_response_notified.fetch_add(1, std::memory_order_relaxed);
        ev_async_send(main_loop, &async_watcher);
    }
    
private:
    ev_async async_watcher;
    struct ev_loop* main_loop = nullptr;
    std::queue<T*> response_queue;
    std::mutex queue_mutex;
    ev_notifier_cb ev_async_callback = nullptr;
    bool inited = false;
};
}}
#endif /* serverplugin_helper_hpp */
