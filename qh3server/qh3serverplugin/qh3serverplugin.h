//
//  qh3serverplugin.h
//  qh3server
//
//  Created by Arun A on 22/12/24.
//

#ifndef QH3SERVERPLUGIN_H
#define QH3SERVERPLUGIN_H

#include "../../../common/sdktypes.hpp"
#include "../qh3server/qh3router.hpp"
#include "../qh3server/qh3server.hpp"

#undef __LOGTAG__
#define __LOGTAG__ "qh3serverplugin"

namespace gsdk {
namespace server {

#if PLATFORM == PLATFORM_WINDOWS
#define EXPORT __declspec(dllexport)
#else
#define EXPORT __attribute__((visibility("default"))) __attribute__((unused))
#endif

class qh3plugin_router_event_listener : public observer_router_events {
   public:
	~qh3plugin_router_event_listener() {}
	typedef void (*type_on_router_pre_start)(qh3router* router);
	typedef void (*type_on_router_start)(qh3router* router);
	typedef void (*type_on_router_stop)();
	typedef void (*type_on_router_error)(int error_code);
	qh3plugin_router_event_listener(type_on_router_pre_start pre_start_cb, type_on_router_start start_cb, type_on_router_stop stop_cb, type_on_router_error error_cb)
		: cb_on_router_pre_start(pre_start_cb), cb_on_router_start(start_cb), cb_on_router_stop(stop_cb), cb_on_router_error(error_cb) {}

   protected:
	void on_router_pre_start(qh3router* router) override;
	void on_router_start(qh3router* router) override;
	void on_router_stop() override;
	void on_router_error(int error_code) override;

   private:
	type_on_router_pre_start cb_on_router_pre_start = nullptr;
	type_on_router_start cb_on_router_start = nullptr;
	type_on_router_stop cb_on_router_stop = nullptr;
	type_on_router_error cb_on_router_error = nullptr;
};

class qh3plugin_server_event_listener : public observer_qh3server_events {
   public:
	~qh3plugin_server_event_listener() {}
	typedef void (*type_on_server_pre_start)(qh3server* server);
	typedef void (*type_on_server_start)(qh3server* server);
	typedef void (*type_on_server_stop)(qh3server* server);
	typedef void (*type_on_server_error)(qh3server* server, int error_code);
	typedef char* (*type_on_server_parse)(qh3server*, const char* path, const char* buffer, unsigned long len);
	qh3plugin_server_event_listener(type_on_server_pre_start pre_start_cb, type_on_server_start start_cb, type_on_server_stop stop_cb, type_on_server_error error_cb, type_on_server_parse parse_cb)
		: cb_on_server_pre_start(pre_start_cb), cb_on_server_start(start_cb), cb_on_server_stop(stop_cb), cb_on_server_error(error_cb), cb_on_server_parse(parse_cb) {}

   protected:
	void on_server_pre_start(qh3server*) override;
	void on_server_start(qh3server*) override;
	void on_server_stop(qh3server*) override;
	void on_server_error(qh3server*, int error_code) override;
	char* on_serevr_parse(qh3server*, const char* path, const char* buffer, unsigned long len) override;

   private:
	type_on_server_pre_start cb_on_server_pre_start = nullptr;
	type_on_server_start cb_on_server_start = nullptr;
	type_on_server_stop cb_on_server_stop = nullptr;
	type_on_server_error cb_on_server_error = nullptr;
	type_on_server_parse cb_on_server_parse = nullptr;
};

class qh3plugin_server : public qh3server {
   public:
	qh3plugin_server(const server_config_in& config);
	static inline const char* get_server_name() { return "qh3plugin_server"; }

   protected:
	void parse_header(const qstring& name, const qstring& value, struct conn_io_qh3* conn_io) override;
	void parse(struct conn_io_qh3* conn_io) override;

	bool on_server_pre_init() override;
	void on_server_uninitialise() override;
	void on_run_started() override;
	void on_run_end() override;
};

extern "C" {
EXPORT void setup_signal_handler();
EXPORT void pre_init_qh3serverplugin_sdk();
EXPORT void spawn_qh3router(const char* router_address, const char* mongodb_uri, const char* redis_address, const char* zk_uri, const char* root_dir, uint16_t command_port, uint16_t router_port_return, const char* app_id,
							qh3plugin_router_event_listener::type_on_router_pre_start pre_start_cb, qh3plugin_router_event_listener::type_on_router_start start_cb, qh3plugin_router_event_listener::type_on_router_stop stop_cb,
							qh3plugin_router_event_listener::type_on_router_error error_cb);
EXPORT void spawn_qh3server(qh3router* router, const char* server_address, const char* mongodb_uri, const char* redis_address, const char* zk_uri, const char* root_dir, uint16_t command_port, uint16_t router_port_return, const char* app_id,
							qh3plugin_server_event_listener::type_on_server_pre_start pre_start_cb, qh3plugin_server_event_listener::type_on_server_start start_cb, qh3plugin_server_event_listener::type_on_server_stop stop_cb,
							qh3plugin_server_event_listener::type_on_server_error error_cb, qh3plugin_server_event_listener::type_on_server_parse parse_cb);
EXPORT int test_func();
}

}  // namespace server
}  // namespace gsdk

#endif /* QH3SERVERPLUGIN_H */
