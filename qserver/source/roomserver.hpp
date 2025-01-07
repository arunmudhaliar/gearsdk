//
//  Copyright 2024 homenet25
//  roomserver.hpp
//  roomserver
//
//  Created by Arun A on 26/10/23.
//

#ifndef roomserver_hpp
#define roomserver_hpp

#include "../../networkcommon/source/message.hpp"
#include "../../networkcommon/source/roommessage.hpp"
#include "../../networkcommon/source/serverconfig.hpp"
#include "../../qzookeeper/source/qzookeeper.hpp"
#include "qnetworkserver.hpp"
#include "room.hpp"

#include <map>
#include <vector>

#undef __LOGTAG__
#define __LOGTAG__ "roomserver"

#define WAITING_ROOM_ZOMBIE_CHECK_TIMER 30.0
#define WAITING_ROOM_ZOMBIE_THRESHOLD 200.0

// MARK: -
class roomserver : public qnetworkserver, public roomserver_interface, protected interface_qhiredis_async, observer_serverconfig {
   public:
	roomserver(const qstring& zk_uri);
	virtual ~roomserver();
	inline struct ev_loop* get_netowrk_main_loop() override final { return get_mainloop(); }
	inline observer_qserver_events* get_main_observer() override final { return get_observer(); }

   protected:
	bool on_network_server_begin() override final;
	void on_network_server_init() override;
	void on_network_server_end() override final;
	void onconnection_message(ssize_t recv_len, uint8_t* buf, qconn_io* qconnection) override final;
	void onconnection_connect(qconn_io* qconnection) override final;
	void onconnection_connected(qconn_io* qconnection) override final;
	void onconnection_destroy(qconn_io* qconnection) override final;
	void on_qhiredis_async_key_expired(const qstring& expired_key) override;
	void on_qhiredis_async_key_changed(const qstring& modified_key, const qstring& event) override;
	void on_qhiredis_connect() override;
	void on_qhiredis_disconnect() override;
	void on_heartbeat_check() override;
	void onroom_pre_start(room*) override final;
	bool is_log_quiche() override;

	void configchanged(const qstring& path, const qstring& data) override;

	virtual room* create_room(const msg_room_config* room_config_msg) = 0;

	// timers
	void on_timer_check_zombie_rooms(qtimer& timer);

	room* create_waiting_room(const msg_room_config* room_config);

	void process_match_request(ssize_t recv_len, uint8_t* buf, qconn_io* qconnection, rapidjson::Document& doc, void* user_data);
	void process_shutdown_request(ssize_t recv_len, uint8_t* buf, qconn_io* qconnection, rapidjson::Document& doc, void* user_data);

	room* find_room(int room_id);

	void check_and_update_is_log_quiche_flag();

	qtimer_scheduler scheduler;
	std::vector<room*> waiting_rooms;
	std::vector<room*> rooms;
	std::map<unsigned, room*> connection_map;
	std::map<unsigned, ev_tstamp> new_connections;
	size_t zombie_rooms = 0;
	size_t max_conns_reached = 0;
	size_t max_rooms_reached = 0;
	size_t max_wrooms_reached = 0;

	message_parser msg_parser;
	qtimer* waiting_room_check_zombie_timer = nullptr;
	qtimer* update_redis_about_gserver_timer = nullptr;
	bool is_log_quiche_flag = false;

	qhiredis* hiredis = nullptr;
	qhiredis_async* hiredis_async = nullptr;
	qzookeeper* qzk = nullptr;
	serverconfig* zkconfig = nullptr;

   private:
	const qstring zk_uri;
	enum conn_flags { FLAG_ROOM_CONFIG_RECEIVED = (1 << 0), FLAG_FIRST_HI_RECEIVED = (1 << 1) };
	std::map<unsigned long, std::function<void(ssize_t, uint8_t*, qconn_io*, rapidjson::Document&, void*)>> message_handlers;

	void do_process_roomjoin(qconn_io* qconnection, const msg_room_match_request& room_match_request_msg);
	qtimer* schedule_update_redis_about_gserver_timer();
};

#endif /* roomserver_hpp */
