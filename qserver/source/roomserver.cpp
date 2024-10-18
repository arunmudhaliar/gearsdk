//
//  Copyright 2024 homenet25
//  roomserver.cpp
//  roomserver
//
//  Created by Arun A on 26/10/23.
//

#include "roomserver.hpp"

// MARK: - roomserver
roomserver::roomserver() : qnetworkserver() {}

roomserver::~roomserver() {
    GX_DELETE(hiredis_async);
    GX_DELETE(hiredis);
}

void roomserver::on_timer_check_zombie_rooms(qtimer& timer) {
	UNUSED(timer);
	if (waiting_rooms.size()) {
		std::vector<room*> zombies;
		for (auto it = waiting_rooms.cbegin(); it != waiting_rooms.cend(); it++) {
			room* waiting_room = *it;
			if (waiting_room->since_creation() >= WAITING_ROOM_ZOMBIE_THRESHOLD) {
				zombies.push_back(waiting_room);
			}
		}
		if (zombie_rooms != (size_t) zombies.size()) {
			debug_print_important2(__LOGTAG__, "[%d] - zombies", zombies.size());
			for (auto it = zombies.cbegin(); it != zombies.cend(); it++) {
				room* zombie = *it;
				zombie->print_info();
			}

			zombie_rooms = zombies.size();
		}
	}
}

bool roomserver::on_network_server_begin() {
    const struct runserverconfig& run_config = get_run_server_config();
    GX_DELETE(hiredis);
    hiredis = DEBUG_NEW qhiredis("qserver_hiredis", run_config.redis_ip, run_config.redis_port);
    if (hiredis->connect_redis() != 0) {
        debug_print_error(__LOGTAG__, "failed to connect hiredis, Exiting !!!");
        GX_DELETE(hiredis);
        return false;
    }
    hiredis->set_hash_value(qstring::format_string("gservers:%s", gsdk::server::machine_public_ip), qstring::format_string("gserver-%s", run_config.port.c_str()), qstring::format_string("%s:%s", run_config.host.c_str(), run_config.port.c_str()));
    
    GX_DELETE(hiredis_async);
    hiredis_async = DEBUG_NEW qhiredis_async(run_config.redis_ip, run_config.redis_port, this, "CONFIG SET notify-keyspace-events KEA");
    if (hiredis_async->connect_async_redis(get_mainloop()) != 0) {
        debug_print_error(__LOGTAG__, "failed to connect async hiredis, Exiting !!!");
        GX_DELETE(hiredis);
        GX_DELETE(hiredis_async);
        return false;
    }
    
	scheduler.set_loop(get_mainloop());
	type_qtimer_cb timeout_callback = std::bind(&roomserver::on_timer_check_zombie_rooms, this, std::placeholders::_1);
	waiting_room_check_zombie_timer = scheduler.schedule_repeat_timer(timeout_callback, WAITING_ROOM_ZOMBIE_CHECK_TIMER);
	debug_print_important2(__LOGTAG__, "start");

	// for quiche logs
	check_and_update_is_log_quiche_flag();
    return true;
}

void roomserver::check_and_update_is_log_quiche_flag() {
	qstring is_log_quiche_value;
	hiredis->get_value("is_log_quiche", is_log_quiche_value);
	is_log_quiche_flag = is_log_quiche_value.compare("true") == 0;
}

bool roomserver::is_log_quiche() {
	return is_log_quiche_flag;
}

void roomserver::on_network_server_init() {
	debug_print_important2(__LOGTAG__, "roomserver::init");
	scheduler.cancel_and_destroy_timer(waiting_room_check_zombie_timer);

	//	msg_parser.register_message_type<msg_room_match_request>();
	//	msg_parser.register_message_type<msg_room_server_shutdown>();

	message_handlers.clear();
	message_handlers[msg_room_match_request::get_type_string_crc()] = std::bind(&roomserver::process_match_request, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5);
	message_handlers[msg_room_server_shutdown::get_type_string_crc()] =
		std::bind(&roomserver::process_shutdown_request, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5);
}

void roomserver::on_network_server_end() {
	message_handlers.clear();

	for (auto* wr : waiting_rooms) {
		GX_DELETE(wr);
	}
	waiting_rooms.clear();

	for (auto* r : rooms) {
		GX_DELETE(r);
	}
	rooms.clear();
	new_connections.clear();
	connection_map.clear();
    
    GX_DELETE(hiredis_async);
    GX_DELETE(hiredis);
    
	debug_print_important2(__LOGTAG__, "end");
}

void roomserver::onroom_pre_start(room* r) {
	// check and delete the room from waiting list
	int old_sz = (int) waiting_rooms.size();
	waiting_rooms.erase(std::remove(waiting_rooms.begin(), waiting_rooms.end(), r), waiting_rooms.end());
	if (old_sz <= (int) waiting_rooms.size()) {
		debug_print_error(__LOGTAG__, "f:onroom_pre_start - coudn't find the room in waiting rooms list. CHECK !!!");
		if (r) {
			r->print_info();
		}
		return;
	}

	// update the waiting room staus on redis
	long long count_waiting_room_of_this_type = 0;
	const qstring& wkey = r->get_room_signature("wroom:", host_id, port_id);
	int result = this->hiredis->decr_by(wkey, 1, count_waiting_room_of_this_type);
	debug_warn_cond(__LOGTAG__, result != 0, "hiredis decr_by failed for key %s, result %d", wkey.c_str(), result);
	if (count_waiting_room_of_this_type > 0) {
		result = this->hiredis->expire_key(wkey, 1 * 60);  // 1 minute(s)
		debug_warn_cond(__LOGTAG__, result != 0, "hiredis expire_key failed for key %s, result %d", wkey.c_str(), result);
	} else {
		result = this->hiredis->delete_key(wkey);
		debug_warn_cond(__LOGTAG__, result != 0, "hiredis delete_key failed for key %s, result %d", wkey.c_str(), result);
	}

	if (std::find(rooms.begin(), rooms.end(), r) == rooms.end()) {
		rooms.push_back(r);
		debug_print_important(__LOGTAG__, "room %d: removed from waiting list and pushed to rooms list", r->ROOM_ID);
		// update the room status on redis
		long long count_room_of_this_type = 0;
		const qstring& rkey = r->get_room_signature("room:", host_id, port_id);
		if (r->ROOM_ID == 0) {
			int rresult = this->hiredis->set_value(rkey, "1");
			debug_warn_cond(__LOGTAG__, rresult != 0, "hiredis set_value failed for key %s, result %d", rkey.c_str(), rresult);
		} else {
			int rresult = this->hiredis->incr_by(rkey, 1, count_room_of_this_type);
			debug_warn_cond(__LOGTAG__, rresult != 0, "hiredis incr_by failed for key %s, result %d", rkey.c_str(), rresult);
		}
	} else {
		debug_print_error(__LOGTAG__, "coudn't add the room to rooms list (Duplicate). CHECK !!!");
	}
}

void roomserver::process_match_request(ssize_t recv_len, uint8_t* buf, conn_io* qconnection, rapidjson::Document& doc, void* user_data) {
	msg_room_match_request rq;
	if (!rq.deserialize(doc)) {
		debug_warn(LOG_LEVEL_0, __LOGTAG__, "f:process_match_request - packet deserialize failed !!!. returning.");
		return;
	}
	const std::map<unsigned, ev_tstamp>::iterator& itr_found = *((std::map<unsigned, ev_tstamp>::iterator*) user_data);
	qconnection->user_data |= FLAG_ROOM_CONFIG_RECEIVED;
	new_connections.erase(itr_found);
	debug_print(LOG_LEVEL_3, __LOGTAG__, "msg_room_config received from client %0x - %.*s !!!", qconnection->cid_hash_val, recv_len, buf);
	do_process_roomjoin(qconnection, rq);
}

void roomserver::process_shutdown_request(ssize_t recv_len, uint8_t* buf, conn_io* qconnection, rapidjson::Document& doc, void* user_data) {
	UNUSED(doc);
	msg_room_server_shutdown* room_server_shutdown_msg = msg_parser.parse<msg_room_server_shutdown>(recv_len, buf);
	if (room_server_shutdown_msg) {
		const std::map<unsigned, ev_tstamp>::iterator& itr_found = *((std::map<unsigned, ev_tstamp>::iterator*) user_data);
		new_connections.erase(itr_found);
		debug_print(LOG_LEVEL_0, __LOGTAG__, "msg_room_server_shutdown received from client %0x - %.*s !!!", qconnection->cid_hash_val, recv_len, buf);
		GX_DELETE(room_server_shutdown_msg);
		ev_break(get_mainloop(), EVBREAK_ONE);
	} else {
		debug_warn(LOG_LEVEL_0, __LOGTAG__, "msg_room_config not yet received from client %0x - %.*s !!!", qconnection->cid_hash_val, recv_len, buf);
	}
}

void roomserver::onconnection_message(ssize_t recv_len, uint8_t* buf, conn_io* qconnection) {
	if ((qconnection->user_data & FLAG_ROOM_CONFIG_RECEIVED) == 0) {
		// check if he is a fresh connection or not
		std::map<unsigned, ev_tstamp>::iterator itr_found = new_connections.end();
		for (std::map<unsigned, ev_tstamp>::iterator itr_new_connection = new_connections.begin(); itr_new_connection != new_connections.end(); itr_new_connection++) {
			if (itr_new_connection->first == qconnection->cid_hash_val) {
				itr_found = itr_new_connection;
				// new connection
				break;
			}
		}
		if (itr_found != new_connections.end()) {
			rapidjson::Document doc;
			// check if its the first 'hi' message from client.
			if ((qconnection->user_data & FLAG_FIRST_HI_RECEIVED) == 0) {
				int parse_hi_result = message_room_base::deserialize_first_hi_message(recv_len, buf, doc);
				if (parse_hi_result == 0) {
					qconnection->user_data |= FLAG_FIRST_HI_RECEIVED;
					return;	 // no need to handle.
				}
			}
			unsigned short sig = 0;
			unsigned long t_crc = 0;
			int parse_result = message_room_base::deserialize_header(recv_len, buf, sig, t_crc, doc);
			if (parse_result != 0) {
				debug_warn(LOG_LEVEL_0, __LOGTAG__, "f:onconnection_message - room message header parse failed !!!. returning.");
				return;
			}
			auto handler = message_handlers.find(t_crc);
			if (handler != message_handlers.end()) {
				handler->second(recv_len, buf, qconnection, doc, (void*) &itr_found);
			} else {
				debug_warn(LOG_LEVEL_0, __LOGTAG__, "f:onconnection_message - handler not found for CRC: %d", t_crc);
			}
			return;
		}
	}

	// check if he was part of any active room.
	room* room_ptr = nullptr;
	std::map<unsigned, room*>::iterator iterator = connection_map.find(qconnection->cid_hash_val);
	if (iterator != connection_map.end()) {
		room_ptr = iterator->second;
	}
	if (room_ptr == nullptr) {
		debug_print_important2(__LOGTAG__, "f:onconnection_message - returning !!!, connection %0x not part of any room.", qconnection->cid_hash_val);
		// connection was not part of any room so far
		return;
	}
	player* player_ptr = room_ptr->get_player(qconnection);
	if (player_ptr == nullptr) {
		debug_print_error(__LOGTAG__, "f:onconnection_message - player not found in the room !!!");
		return;
	}
	room_ptr->pass_message_to_room(player_ptr, qstring(buf, recv_len));
}

void roomserver::onconnection_connect(conn_io* qconnection) {
	debug_print(LOG_LEVEL_3, __LOGTAG__, "f:onconnection_connect - incoming connection %0x", qconnection->cid_hash_val);
}

void roomserver::onconnection_connected(conn_io* qconnection) {
	debug_print(LOG_LEVEL_3, __LOGTAG__, "f:onconnection_connected - connected %0x", qconnection->cid_hash_val);
	new_connections[qconnection->cid_hash_val] = ev_now(get_netowrk_main_loop());
}

room* roomserver::find_room(int room_id) {
	for (auto it = rooms.cbegin(); it != rooms.cend(); it++) {
		room* room_ptr = *it;
		if (room_ptr->ROOM_ID == room_id) {
			return room_ptr;
		}
	}
	return nullptr;
}

void roomserver::do_process_roomjoin(conn_io* qconnection, const msg_room_match_request& room_match_request_msg) {
	const msg_room_config& room_config_msg = room_match_request_msg.room_config;
	// check if he was part of any active room.
	room* room_ptr = nullptr;
	std::map<unsigned, room*>::iterator iterator = connection_map.find(qconnection->cid_hash_val);
	if (iterator != connection_map.end()) {
		room_ptr = iterator->second;
	}
	// check if he was a disconnected player or not
	if (room_ptr == nullptr && room_match_request_msg.room_id >= 0) {
		room* found_room = find_room(room_match_request_msg.room_id);
		if (found_room != nullptr && found_room->get_state() < room::ROOM_END) {
			if (found_room->find_in_disconnected_players(room_match_request_msg.prev_cid_hash_val)) {
				room_ptr = found_room;
				debug_print(LOG_LEVEL_0, __LOGTAG__, "f:do_process_roomjoin - found previous room:%d for connection %0x. previous connection %0x", found_room->ROOM_ID, qconnection->cid_hash_val, room_match_request_msg.prev_cid_hash_val);
			}
		} else {
			debug_print_important2(__LOGTAG__, "f:do_process_roomjoin - reconnection failed for connection %0x  (prev:%0x)!!!. either prev room was destroyed or in end state.", qconnection->cid_hash_val,
								   room_match_request_msg.prev_cid_hash_val);
		}
	}
	bool connection_added_to_room = false;
	if (room_ptr == nullptr) {
		// may be a new connection.
		// so get a waiting room for him
		// search existing waiting rooms.
		for (auto it = waiting_rooms.cbegin(); it != waiting_rooms.cend(); it++) {
			room* waiting_room = *it;
			ssize_t old_count = waiting_room->get_playermap_count();
			bool replaced_by_disconnected_player = false;
			ssize_t new_count = waiting_room->try_add_connection(qconnection, room_match_request_msg.pid, replaced_by_disconnected_player);
			connection_added_to_room = new_count > old_count;
			if (connection_added_to_room) {
				room_ptr = waiting_room;
				connection_map[qconnection->cid_hash_val] = room_ptr;
				if (room_ptr->get_state() == room::states::ROOM_WAITING) {
					debug_print_important2(__LOGTAG__, "f:do_process_roomjoin - add to waiting room %d, map[after add sz:%d] !!! - connection %0x", room_ptr->ROOM_ID, connection_map.size(), qconnection->cid_hash_val);
				} else if (room_ptr->get_state() >= room::states::ROOM_START) {
					debug_print_important2(__LOGTAG__, "f:do_process_roomjoin - added to room %d, map[after add sz:%d] !!! - connection %0x", room_ptr->ROOM_ID, connection_map.size(), qconnection->cid_hash_val);
				}
				break;
			}
		}
	} else {
		// check if he still in the room or not
		player* already_in_room = room_ptr->get_player(qconnection);
		if (already_in_room) {
			debug_print_important2(__LOGTAG__, "f:do_process_roomjoin - already part of PREV room %d of user [m-sz:%d] !!! - connection %0x, returning.", room_ptr->ROOM_ID, qconnection->cid_hash_val, connection_map.size(),
								   qconnection->cid_hash_val);
			return;
		}
		// check if he can be added back to the same room.
		ssize_t old_count = room_ptr->get_playermap_count();
		bool replaced_by_disconnected_player = false;
		ssize_t new_count = room_ptr->try_add_connection(qconnection, room_match_request_msg.pid, replaced_by_disconnected_player, room_match_request_msg.prev_cid_hash_val);
		if (new_count == -2) {	// already part of the room
			debug_print_error(__LOGTAG__, "f:do_process_roomjoin - room %d - this can not happen !!!, returning.", room_ptr->ROOM_ID);
			return;
		}
		connection_added_to_room = new_count > old_count;
		if (!connection_added_to_room) {
			// remove from old list
			debug_print_important2(__LOGTAG__, "f:do_process_roomjoin - can't be added to his prev room %d, remove him from old hash list", room_ptr->ROOM_ID);
			if (iterator != connection_map.end()) {
				connection_map.erase(iterator);
			}
			room_ptr = nullptr;
		} else {
			if (iterator != connection_map.end()) {
				connection_map.erase(iterator);
			}
			connection_map[qconnection->cid_hash_val] = room_ptr;  // update the connection map
			debug_print_important2(__LOGTAG__, "f:do_process_roomjoin - add player to PREV room %d of user [m-sz:%d] !!! - connection %0x", room_ptr->ROOM_ID, connection_map.size(), qconnection->cid_hash_val);
		}
	}
	// create a new room and add him
	if (!connection_added_to_room) {
		room* waiting_room = create_waiting_room(&room_config_msg);
		bool replaced_by_disconnected_player = false;
		waiting_room->try_add_connection(qconnection, room_match_request_msg.pid, replaced_by_disconnected_player);	 // no need to check for limit since he is our first user in this room.
		room_ptr = waiting_room;
		connection_map[qconnection->cid_hash_val] = room_ptr;
		debug_print(LOG_LEVEL_3, __LOGTAG__, "f:do_process_roomjoin - add to hash[after add sz:%d] !!! - %0x", connection_map.size(), qconnection->cid_hash_val);
	}
}

room* roomserver::create_waiting_room(const msg_room_config* room_config_msg) {
	room* room_ptr = create_room(room_config_msg);

	waiting_rooms.push_back(room_ptr);
	long long count_waiting_room_of_this_type = 0;
	const qstring& key = room_ptr->get_room_signature("wroom:", host_id, port_id);
	int result = this->hiredis->incr_by(key, 1, count_waiting_room_of_this_type);
	debug_warn_cond(__LOGTAG__, result != 0, "hiredis incr_by failed for key %s, result %d", key.c_str(), result);
	result = this->hiredis->expire_key(key, 1 * 60);  // 1 minute(s)
	debug_warn_cond(__LOGTAG__, result != 0, "hiredis expire_key failed for key %s, result %d", key.c_str(), result);
	return room_ptr;
}

void roomserver::onconnection_destroy(conn_io* qconnection) {
	room* room_ptr = nullptr;
	std::map<unsigned, room*>::iterator iterator = connection_map.find(qconnection->cid_hash_val);
	if (iterator != connection_map.end()) {
		room_ptr = iterator->second;
	}
	if (room_ptr == nullptr) {
		debug_print(LOG_LEVEL_3, __LOGTAG__, "f:onconnection_destroy - qconnection not in any room !!! - %0x, [cnt %d]", qconnection->cid_hash_val, connection_map.size());
		return;
	}
	connection_map.erase(iterator);

	debug_print_important2(__LOGTAG__, "f:onconnection_destroy - qconnection removed, map sz [%d], %0x", connection_map.size(), qconnection->cid_hash_val);
	room_ptr->remove_connection(qconnection);
	if (room_ptr->get_state() == room::ROOM_END && room_ptr->get_playermap_count() == 0) {
		// delete the room, if the state is in 'END' and player count is zero.
		ssize_t waiting_room_size = waiting_rooms.size();
		waiting_rooms.erase(std::remove(waiting_rooms.begin(), waiting_rooms.end(), room_ptr), waiting_rooms.end());
		DEBUG_ASSERT(__LOGTAG__, (waiting_room_size == (ssize_t) waiting_rooms.size()), "check this");	// still in waiting room ???
		ssize_t rooms_size = rooms.size();
		rooms.erase(std::remove(rooms.begin(), rooms.end(), room_ptr), rooms.end());
		DEBUG_ASSERT(__LOGTAG__, (rooms_size > (ssize_t) rooms.size()), "check this");	// still in waiting room ???

		// update the room status on redis
		long long count_room_of_this_type = 0;
		const qstring& rkey = room_ptr->get_room_signature("room:", host_id, port_id);
		int rresult = this->hiredis->decr_by(rkey, 1, count_room_of_this_type);
		debug_warn_cond(__LOGTAG__, rresult != 0, "hiredis decr_by failed for key %s, result %d", rkey.c_str(), rresult);

		GX_DELETE(room_ptr);
		debug_print_important2(__LOGTAG__, "room size %d", rooms.size());
	}
}

void roomserver::on_qhiredis_async_key_expired(const qstring& expired_key) {
	int count = 0;
	for (auto it = waiting_rooms.cbegin(); it != waiting_rooms.cend(); it++) {
		room* waiting_room = *it;
		// if (waiting_room->)
		const qstring& key = waiting_room->get_room_signature("wroom:", host_id, port_id);
		if (key.compare(expired_key) == 0) {
			count++;
		}
	}

	// refresh the key
	if (count > 0) {
		long long count_waiting_room_of_this_type = 0;
		int result = this->hiredis->incr_by(expired_key, count, count_waiting_room_of_this_type);
		debug_warn_cond(__LOGTAG__, result != 0, "hiredis incr_by failed for key %s, result %d", expired_key.c_str(), result);
		result = this->hiredis->expire_key(expired_key, 1 * 60);  // 1 minute(s)
		debug_warn_cond(__LOGTAG__, result != 0, "hiredis expire_key failed for key %s, result %d", expired_key.c_str(), result);
	}
}

void roomserver::on_qhiredis_async_key_changed(const qstring& modified_key, const qstring& event) {
	if (modified_key.compare("is_log_quiche") == 0 && event.compare("set") == 0) {
		check_and_update_is_log_quiche_flag();
	}
}

void roomserver::on_qhiredis_connect() {
}

void roomserver::on_qhiredis_disconnect() {
}

void roomserver::on_heartbeat_check() {
	size_t curr_conns = get_connection_count();
	max_conns_reached = MAX(curr_conns, max_conns_reached);
	size_t curr_rooms = rooms.size();
	max_rooms_reached = MAX(curr_rooms, max_rooms_reached);
	size_t curr_wrooms = waiting_rooms.size();
	max_wrooms_reached = MAX(curr_wrooms, max_wrooms_reached);
	debug_raw_no_newline(LOG_LEVEL_0, "\r", "connections %zu (max %zu), rooms %zu (max %zu), wrooms %zu (max %zu), conn map %zu\t\t", curr_conns, max_conns_reached, curr_rooms, max_rooms_reached, curr_wrooms, max_wrooms_reached,
						 connection_map.size());
}
