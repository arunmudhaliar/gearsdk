//
//  serverplugin.h
//  qh3server
//
//  Created by Arun A on 22/12/24.
//

#ifndef SERVERPLUGIN_H
#define SERVERPLUGIN_H

// Thread-safe queue
#include <queue>
#include <mutex>

#include "../common/sdktypes.hpp"
#include "../common/qstring.hpp"
#include "../qh3server/qh3server/qh3router.hpp"
#include "../qh3server/qh3server/qh3server.hpp"
#include "../networkcommon/source/qthreadpool.hpp"
#include "serverplugin_helper.hpp"

#undef __LOGTAG__
#define __LOGTAG__ "serverplugin"

namespace gsdk {
namespace server {

class qh3plugin_router_event_listener : public observer_router_events {
   public:
	~qh3plugin_router_event_listener() {}
	typedef void (*type_on_router_pre_start)(qh3router* router, void* user_arg);
	typedef void (*type_on_router_start)(qh3router* router, void* user_arg);
	typedef void (*type_on_router_stop)(qh3router* router, void* user_arg);
	typedef void (*type_on_router_error)(qh3router* router, void* user_arg, int error_code);
	qh3plugin_router_event_listener(type_on_router_pre_start pre_start_cb, type_on_router_start start_cb, type_on_router_stop stop_cb, type_on_router_error error_cb)
		: cb_on_router_pre_start(pre_start_cb), cb_on_router_start(start_cb), cb_on_router_stop(stop_cb), cb_on_router_error(error_cb) {}

   protected:
	void on_router_pre_start(qh3router* router) override;
	void on_router_start(qh3router* router) override;
	void on_router_stop(qh3router* router) override;
	void on_router_error(qh3router* router, int error_code) override;

   private:
	type_on_router_pre_start cb_on_router_pre_start = nullptr;
	type_on_router_start cb_on_router_start = nullptr;
	type_on_router_stop cb_on_router_stop = nullptr;
	type_on_router_error cb_on_router_error = nullptr;
};

class qh3plugin_server_event_listener : public observer_qh3server_events {
   public:
	~qh3plugin_server_event_listener() {}
	typedef void (*type_on_server_pre_start)(qh3server* server, void* user_arg);
	typedef void (*type_on_server_start)(qh3server* server, void* user_arg, const char* ip, uint16_t port);
	typedef void (*type_on_server_stop)(qh3server* server, void* user_arg);
	typedef void (*type_on_server_error)(qh3server* server, void* user_arg, int error_code);
	typedef void (*type_on_server_parse)(qh3server* server, void* user_arg, uint8_t *cid, uint16_t cid_len, const char* path, const char* buffer, unsigned long len, const char* headers_buffer, unsigned long headers_buffer_size);
	qh3plugin_server_event_listener(type_on_server_pre_start pre_start_cb, type_on_server_start start_cb, type_on_server_stop stop_cb, type_on_server_error error_cb, type_on_server_parse parse_cb)
		: cb_on_server_pre_start(pre_start_cb), cb_on_server_start(start_cb), cb_on_server_stop(stop_cb), cb_on_server_error(error_cb), cb_on_server_parse(parse_cb) {}

    // std::mutex callback_mutex;
    
   protected:
	void on_server_pre_start(qh3server*) override;
	void on_server_start(qh3server*, const char* ip, uint16_t port) override;
	void on_server_stop(qh3server*) override;
	void on_server_error(qh3server*, int error_code) override;
	void on_serevr_parse(qh3server*, const conn_io_qh3* conn, const char* path, const char* buffer, unsigned long len, const char* headers_buffer, unsigned long headers_buffer_size) override;

   private:
	type_on_server_pre_start cb_on_server_pre_start = nullptr;
	type_on_server_start cb_on_server_start = nullptr;
	type_on_server_stop cb_on_server_stop = nullptr;
	type_on_server_error cb_on_server_error = nullptr;
	type_on_server_parse cb_on_server_parse = nullptr;
    
};

struct st_response_packet {
    st_response_packet(qh3server* server, uint8_t *cid, uint16_t cid_len, const char* payload, size_t len) {
        this->server = server;
        this->cid_len = cid_len;
        if (cid && cid_len>0) {
            this->cid = (unsigned char *)malloc(cid_len);
            memcpy(this->cid, cid, cid_len);
        }
        this->payload.bin_copy((const uint8_t*)payload, len);
    }
    ~st_response_packet() {
        if (this->cid) {
            free(this->cid);
            this->cid = nullptr;
        }
    }
    qh3server* server = nullptr;
    uint8_t *cid = nullptr;
    uint16_t cid_len = 0;
    qstring payload;
};

class qh3plugin_server : public qh3server {
   public:
	qh3plugin_server(const server_config_in& config);
    ~qh3plugin_server();
    
	static inline const char* get_server_name() { return "qh3plugin_server"; }

    struct st_admin_cmd {
        st_admin_cmd(short type, qh3plugin_server* server): type(type), server(server) {
        }
        short type = 0;
        qh3plugin_server* server = nullptr;
    };
    
    ev_async async_cmd_watcher_notify_server;
    
//    // Thread-safe enqueue
//    void enqueue_response(st_response_packet* response_packet) {
//        std::lock_guard<std::mutex> lock(queue_mutex);
//        response_queue.push(response_packet);
//    }

//    // Thread-safe dequeue (main thread calls this)
//    st_response_packet* dequeue_response() {
//        std::lock_guard<std::mutex> lock(queue_mutex);
//        if (response_queue.empty()) return nullptr;
//        st_response_packet* response_packet = response_queue.front();
//        response_queue.pop();
//        return response_packet;
//    }

//    // Notify main thread
//    void notify_main_thread() {
//        total_response_notified.fetch_add(1, std::memory_order_relaxed);
//        ev_async_send(get_server_main_loop(), &async_watcher_notify_server);
//    }
    
    async_ev_notifier<struct st_response_packet>& get_ev_notifier()   { return ev_notifier; }
    
    void increment_request_counter() {
        total_requests.fetch_add(1, std::memory_order_relaxed);
    }
    void increment_response_served_counter() {
        total_response_served.fetch_add(1, std::memory_order_relaxed);
    }
    void increment_response_notified() {
        total_response_notified.fetch_add(1, std::memory_order_relaxed);
    }
    void print_request_response_summary() {
        debug_print(LOG_LEVEL_0, __LOGTAG__, "rq:%d, notify:%d, res:%d", total_requests.load(std::memory_order_relaxed), total_response_notified.load(std::memory_order_relaxed), total_response_served.load(std::memory_order_relaxed));
    }
    inline EVENT_LOOP_TYPE* get_server_main_loop() { return get_mainloop(); }
    
    void shutdown_server() {
        st_admin_cmd* new_cmd = DEBUG_NEW st_admin_cmd(1, this);
        async_cmd_watcher_notify_server.data = new_cmd;
        ev_async_send(get_server_main_loop(), &async_cmd_watcher_notify_server);
    }
    
   protected:
	void parse_header(const qstring& name, const qstring& value, struct conn_io_qh3* conn_io) override;
	parse_return parse(struct conn_io_qh3* conn_io) override;

	bool on_server_pre_init() override;
	void on_server_uninitialise() override;
	void on_run_started() override;
	void on_run_end() override;
    
private:
    async_ev_notifier<struct st_response_packet> ev_notifier;
    static void notify_server_async_cb(EV_P_ ev_async *w, int revents);
    static void notify_server_cmd_async_cb(EV_P_ ev_async *w, int revents);
    
    std::atomic<int> total_requests = {0};
    std::atomic<int> total_response_notified = {0};
    std::atomic<int> total_response_served = {0};
};

extern "C" {
EXPORT void setup_signal_handler();
EXPORT void pre_init_serverplugin_sdk();
EXPORT void spawn_qh3router(const char* router_address, const char* mongodb_uri, const char* redis_address, const char* zk_uri, const char* root_dir, const char* inf_file, uint16_t command_port, uint16_t router_port_return, const char* app_id,
							qh3plugin_router_event_listener::type_on_router_pre_start pre_start_cb, qh3plugin_router_event_listener::type_on_router_start start_cb, qh3plugin_router_event_listener::type_on_router_stop stop_cb,
							qh3plugin_router_event_listener::type_on_router_error error_cb, void* user_arg = nullptr);
EXPORT void spawn_qh3server(qh3router* router, const char* server_address, const char* mongodb_uri, const char* redis_address, const char* zk_uri, const char* root_dir, const char* inf_file, uint16_t command_port, uint16_t router_port_return, const char* app_id,
							qh3plugin_server_event_listener::type_on_server_pre_start pre_start_cb, qh3plugin_server_event_listener::type_on_server_start start_cb, qh3plugin_server_event_listener::type_on_server_stop stop_cb,
							qh3plugin_server_event_listener::type_on_server_error error_cb, qh3plugin_server_event_listener::type_on_server_parse parse_cb, void* user_arg);
EXPORT unsigned long get_crc32(const char* guid, int guid_len);
EXPORT unsigned long mod_crc32(uLong adler, const Bytef* buf, z_size_t len);
void* spawn_qh3router_internal(void* data);
void* spawn_qh3server_internal(void* data);
EXPORT void qh3server_try_send_response(qh3server*, uint8_t *cid, uint16_t cid_len, const char* payload, size_t len);
EXPORT unsigned int get_live_connection_count(qh3server*);
EXPORT const char* get_device_public_ip();
EXPORT uint64_t qh3server_logfile(qh3server*, qlogfile::log_lvls lvl, qcustomlogger::elog_type type, const char* tag, const char* pid, const char* roomid, const char* message);
EXPORT size_t qh3server_stats_count(qh3server* server, const char* counter, long count_val, const char* session, const char* pid, const char* version = "", const char* epic = "", const char* myth = "", const char* legend = "",
                    const char* story = "", const char* message = "");
EXPORT unsigned long get_current_time_in_ms();
EXPORT void sleep_for(int milliseconds);
}

}  // namespace server
}  // namespace gsdk

#endif /* SERVERPLUGIN_H */
