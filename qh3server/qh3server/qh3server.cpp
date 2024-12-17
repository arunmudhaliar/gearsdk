//
//  Copyright 2024 homenet25
//  qh3server.cpp
//  qh3server
//
//  Created by Arun A on 30/10/23.
//

#include "qh3server.hpp"

qh3server::qh3server() {}

qh3server::~qh3server() {
	GX_DELETE(relay_through_router_info);
	debug_print_important2(logtag.c_str(), "qh3server destroyed %s:%s !!!", host_id.c_str(), port_id.c_str());
}

void qh3server::debug_quiche_log(const uint8_t* line, void* argp) {
	qh3server* server = reinterpret_cast<qh3server*>(argp);
	if (server != nullptr && server->is_log_quiche()) {
		debug_print(LOG_LEVEL_0, server->logtag.c_str(), (char*) line);
	}
}

ssize_t qh3server::flush_egress(struct conn_io_qh3* conn_io) {
	const char* const_logtag = logtag.c_str();
	const bool VIA_ROUTER = relay_through_router_info && relay_through_router_info->serialised_buffer.length() >= ORIGINAL_CLIENT_ADDR_SZ;
	quiche_send_info send_info;
	ssize_t total_bytes_sent = 0;

	while (1) {
		ssize_t written = quiche_conn_send(conn_io->conn, out, sizeof(out), &send_info);

		if (written == QUICHE_ERR_DONE) {
			DEBUG_PRINT2(LOG_LEVEL_5, const_logtag, "done writing");
			break;
		}

		if (written < 0) {
			debug_print_error(const_logtag, "failed to create packet: %zd", written);
			return -1;
		}

		// if relay through router
		if (VIA_ROUTER) {
			memcpy(reinterpret_cast<void*>(&out[written]), reinterpret_cast<const void*>(conn_io->original_client_serialised_buffer.c_str()), ORIGINAL_CLIENT_ADDR_SZ);
			written += ORIGINAL_CLIENT_ADDR_SZ;
		}
		//

		ssize_t sent = sendto(conn_io->sock, out, written, 0, (struct sockaddr*) &conn_io->peer_addr, conn_io->peer_addr_len);

#if LOG_LEVEL >= LOG_LEVEL_5
		char name[INET6_ADDRSTRLEN];
		char port[10];
		getnameinfo((struct sockaddr*) &conn_io->peer_addr, sizeof(struct sockaddr), name, sizeof(name), port, sizeof(port), NI_NUMERICHOST | NI_NUMERICSERV);
		debug_print(LOG_LEVEL_0, const_logtag, "send to %s:%s bytes:%d", name, port, sent);
#endif

		if (sent != written) {
			char name[INET6_ADDRSTRLEN];
			char port[10];
			getnameinfo((struct sockaddr*) &conn_io->peer_addr, sizeof(struct sockaddr), name, sizeof(name), port, sizeof(port), NI_NUMERICHOST | NI_NUMERICSERV);
			debug_print_error(const_logtag, "ERROR (flush_egress) sending to %s:%s", name, port);
			debug_print_error(const_logtag, "failed to send - flush_egress %d<>%d", sent, written);
			return -1;
		}

#if LOG_LEVEL >= LOG_LEVEL_4
		unsigned long send_bytes_crc = crc32(0L, Z_NULL, 0);
		send_bytes_crc = essentials::mod_crc32_z(send_bytes_crc, reinterpret_cast<const uint8_t*>(out), sent);
		DEBUG_PRINT2(LOG_LEVEL_4, const_logtag, "sent %zd bytes - crc: %lx", sent, send_bytes_crc);
#endif

		qh3server::get_stats_loggeer()->server_count("flush_egress", sent, "", "", "", "tx", "qh3server", "", port_id.c_str());
		total_bytes_sent += sent;
	}

	double t = quiche_conn_timeout_as_nanos(conn_io->conn) / 1e9f;

	conn_io->timer.repeat = t;
#if USE_UV_MAIN_LOOP
	uv_timer_start(&conn_io->timer, timeout_cb, t * 1000, 0);
#else
	ev_timer_again(conn_io->bridge->get_mainloop(), &conn_io->timer);
#endif
	return total_bytes_sent;
}

void qh3server::mint_token(const uint8_t* dcid, size_t dcid_len, struct sockaddr_storage* addr, socklen_t addr_len, uint8_t* token, size_t* token_len) {
	memcpy(token, "quiche", sizeof("quiche") - 1);
	memcpy(token + sizeof("quiche") - 1, addr, addr_len);
	memcpy(token + sizeof("quiche") - 1 + addr_len, dcid, dcid_len);

	*token_len = sizeof("quiche") - 1 + addr_len + dcid_len;
}

bool qh3server::validate_token(const uint8_t* token, size_t token_len, struct sockaddr_storage* addr, socklen_t addr_len, uint8_t* odcid, size_t* odcid_len) {
	if ((token_len < sizeof("quiche") - 1) || memcmp(token, "quiche", sizeof("quiche") - 1)) {
		return false;
	}

	token += sizeof("quiche") - 1;
	token_len -= sizeof("quiche") - 1;

	if ((token_len < addr_len) || memcmp(token, addr, addr_len)) {
		return false;
	}

	token += addr_len;
	token_len -= addr_len;

	if (*odcid_len < token_len) {
		return false;
	}

	memcpy(odcid, token, token_len);
	*odcid_len = token_len;

	return true;
}

uint8_t* qh3server::gen_cid(uint8_t* cid, size_t cid_len) {
	int rng = open("/dev/urandom", O_RDONLY);
	if (rng < 0) {
		debug_print_error(__LOGTAG__, "failed to open /dev/urandom");
		return NULL;
	}

	ssize_t rand_len = read(rng, cid, cid_len);
	if (rand_len < 0) {
		close(rng);
		debug_print_error(__LOGTAG__, "failed to create connection ID");
		return NULL;
	}
	close(rng);
	return cid;
}

struct conn_io_qh3* qh3server::create_conn(uint8_t* scid, size_t scid_len, uint8_t* odcid, size_t odcid_len, struct sockaddr* local_addr, socklen_t local_addr_len, struct sockaddr_storage* peer_addr, socklen_t peer_addr_len,
										   struct sockaddr_storage* peer_original_client_addr) {
	const char* const_logtag = logtag.c_str();
#if USE_UV_MAIN_LOOP
	if (!uv_loop_alive(mainloop)) {
		debug_print_error(const_logtag, "mainloop is not alive, returning. !!!");
		return nullptr;
	}
#endif

	if (scid_len != LOCAL_CONN_ID_LEN) {
		debug_print_error(const_logtag, "failed, scid length too short");
	}

    quiche_conn* conn = quiche_accept(scid, LOCAL_CONN_ID_LEN, odcid, odcid_len, local_addr, local_addr_len, (struct sockaddr*) peer_original_client_addr, peer_addr_len, config);
	if (conn == NULL) {
		debug_print_error(const_logtag, "failed to create connection");
		return NULL;
	}

	struct conn_io_qh3* new_conn_io = DEBUG_NEW struct conn_io_qh3(this);
	if (new_conn_io == NULL) {
		quiche_conn_free(conn);
		debug_print_error(const_logtag, "failed to allocate connection IO");
		return NULL;
	}
	memcpy(new_conn_io->cid, scid, LOCAL_CONN_ID_LEN);
#if USE_UV_MAIN_LOOP
	new_conn_io->creation_time = uv_now(mainloop);
#else
	new_conn_io->creation_time = ev_now(mainloop);
#endif
	new_conn_io->sock = conns->sock;
	new_conn_io->conn = conn;
	HASH_VALUE(scid, scid_len, new_conn_io->cid_hash_val);
	memcpy(&new_conn_io->peer_addr, peer_addr, peer_addr_len);
	new_conn_io->peer_addr_len = peer_addr_len;

#if USE_UV_MAIN_LOOP
	uv_timer_init(mainloop, &new_conn_io->timer);
#else
	ev_init(&new_conn_io->timer, timeout_cb);
#endif

	new_conn_io->timer.data = new_conn_io;
	HASH_ADD(hh, conns->h, cid, LOCAL_CONN_ID_LEN, new_conn_io);

	debug_print(LOG_LEVEL_4, const_logtag, "new connection");

	return new_conn_io;
}

void qh3server::parse_header(const qstring& name, const qstring& value, struct conn_io_qh3* conn_io) {
	const char* const_logtag = logtag.c_str();
	if (name.compare(":path") == 0) {
		debug_print(LOG_LEVEL_4, const_logtag, "got HTTP header: %s=%s", name.c_str(), value.c_str());
	} else {
		debug_print(LOG_LEVEL_5, const_logtag, "got HTTP header: %s=%s", name.c_str(), value.c_str());
	}
	conn_io->http_request->add_or_get_header(name, value);
}

void qh3server::parse(struct conn_io_qh3* conn_io) {
	UNUSED(conn_io);
}

int qh3server::for_each_header(uint8_t* name, size_t name_len, uint8_t* value, size_t value_len, void* argp) {
	struct conn_io_qh3* conn_io = (struct conn_io_qh3*) argp;
	conn_io->bridge->parse_header(qstring(name, name_len), qstring(value, value_len), conn_io);
	return 0;
}

#if USE_UV_MAIN_LOOP
void qh3server::recv_cb(uv_poll_t* w, int status, int events) {
	UNUSED(events);
	// debug_raw(LOG_LEVEL_0, "recv_cb");
	if (status < 0) {
		debug_print_error(__LOGTAG__, "failed to recv");
		return;
	}
#else
void qh3server::recv_cb(EV_P_ ev_io* w, int revents) {
	UNUSED(revents);
#endif
	qh3server* server = reinterpret_cast<qh3server*>(w->data);
	struct connections* conns = server->conns;
	struct conn_io_qh3 *tmp, *conn_io = NULL;
	const char* const_logtag = server->logtag.c_str();
	const char* port_id_cstr = server->port_id.c_str();
	const bool VIA_ROUTER = server->relay_through_router_info && server->relay_through_router_info->serialised_buffer.length() >= ORIGINAL_CLIENT_ADDR_SZ;
	while (1) {
		struct sockaddr_storage peer_addr;
		struct sockaddr_storage peer_original_client_addr;
		socklen_t peer_addr_len = sizeof(peer_addr);
		memset(&peer_addr, 0, peer_addr_len);
		qstring original_client_serialised_buffer;

		ssize_t read = recvfrom(conns->sock, server->buf, sizeof(buf), 0, (struct sockaddr*) &peer_addr, &peer_addr_len);

		if (read < 0) {
			if ((errno == EWOULDBLOCK) || (errno == EAGAIN)) {
				debug_print(LOG_LEVEL_5, const_logtag, "recv would block");
				break;
			}

			debug_print_error(const_logtag, "failed to read");
			return;
		}

		server->get_stats_loggeer()->server_count("recv_cb", read, "", "", "", "rx", "qh3server", "", port_id_cstr);

#if LOG_LEVEL >= LOG_LEVEL_5
		char name[INET6_ADDRSTRLEN];
		char port[10];
		getnameinfo((struct sockaddr*) &peer_addr, sizeof(struct sockaddr), name, sizeof(name), port, sizeof(port), NI_NUMERICHOST | NI_NUMERICSERV);
		debug_print(LOG_LEVEL_0, const_logtag, "peer addr %s:%s read:%d", name, port, read);
#endif

		// if relay through router
		if (server->relay_through_router_info) {
			memset(&peer_original_client_addr, 0, peer_addr_len);
			read = read - peer_addr_len;  // remove the client info
			struct sockaddr* client_info = (struct sockaddr*) &peer_original_client_addr;
			memcpy(reinterpret_cast<void*>(client_info), reinterpret_cast<const void*>(&server->buf[read]), peer_addr_len);

			// serialize the original client address for later use
			qaddress original_client_address((struct sockaddr*) &peer_original_client_addr);
			original_client_address.serialise(original_client_serialised_buffer);

			// update the peer address port (return port)
			essentials::update_port((struct sockaddr*) &peer_addr, server->relay_through_router_info->port_return);
#if LOG_LEVEL >= LOG_LEVEL_5
			debug_print(LOG_LEVEL_0, const_logtag, "crc of orinal-client addr (last %d bytes) = 0x%x", peer_addr_len, essentials::get_crc(&server->buf[read], peer_addr_len));
			getnameinfo((struct sockaddr*) &peer_original_client_addr, sizeof(struct sockaddr), name, sizeof(name), port, sizeof(port), NI_NUMERICHOST | NI_NUMERICSERV);
			debug_print_important2(const_logtag, "original-client-address %s:%s", name, port);
			getnameinfo((struct sockaddr*) &peer_addr, sizeof(struct sockaddr), name, sizeof(name), port, sizeof(port), NI_NUMERICHOST | NI_NUMERICSERV);
			debug_print_important2(const_logtag, "modified-peer address %s:%s", name, port);
#endif
		} else {
			memcpy(&peer_original_client_addr, &peer_addr, peer_addr_len);
		}
		//

		uint8_t type;
		uint32_t version;

		uint8_t scid[QUICHE_MAX_CONN_ID_LEN];
		size_t scid_len = sizeof(scid);

		uint8_t dcid[QUICHE_MAX_CONN_ID_LEN];
		size_t dcid_len = sizeof(dcid);

		uint8_t odcid[QUICHE_MAX_CONN_ID_LEN];
		size_t odcid_len = sizeof(odcid);

		uint8_t token[MAX_TOKEN_LEN];
		size_t token_len = sizeof(token);

		int rc = quiche_header_info(server->buf, read, LOCAL_CONN_ID_LEN, &version, &type, scid, &scid_len, dcid, &dcid_len, token, &token_len);
		if (rc < 0) {
			debug_print_error(const_logtag, "failed to parse header: %d", rc);
			server->get_stats_loggeer()->server_count("recv_cb", 1, "", "", "", "error", "qh3server", "parse_header_fail", port_id_cstr);
			return;
		}

		HASH_FIND(hh, conns->h, dcid, dcid_len, conn_io);

		if (conn_io == NULL) {
			if (!quiche_version_is_supported(version)) {
				debug_print(LOG_LEVEL_4, const_logtag, "version negotiation");

				ssize_t written = quiche_negotiate_version(scid, scid_len, dcid, dcid_len, server->out, sizeof(server->out));

				if (written < 0) {
					debug_print_error(const_logtag, "failed to create vneg packet: %zd", written);
					server->get_stats_loggeer()->server_count("recv_cb", 1, "", "", "", "error", "qh3server", "version_negotiation_fail", port_id_cstr, qstring::format_string("failed to create vneg packet: %zd", written));
					continue;
				}

				// if relay through router
				if (VIA_ROUTER) {
					memcpy(reinterpret_cast<void*>(&server->out[written]), reinterpret_cast<const void*>(original_client_serialised_buffer.c_str()), ORIGINAL_CLIENT_ADDR_SZ);
					written += ORIGINAL_CLIENT_ADDR_SZ;
				}
				//
				ssize_t sent = sendto(conns->sock, server->out, written, 0, (struct sockaddr*) &peer_addr, peer_addr_len);

#if LOG_LEVEL >= LOG_LEVEL_5
				char name[INET6_ADDRSTRLEN];
				char port[10];
				getnameinfo((struct sockaddr*) &peer_addr, sizeof(struct sockaddr), name, sizeof(name), port, sizeof(port), NI_NUMERICHOST | NI_NUMERICSERV);
				debug_print(LOG_LEVEL_0, const_logtag, "send to %s:%s bytes:%d", name, port, sent);
#endif
				if (sent != written) {
					char name[INET6_ADDRSTRLEN];
					char port[10];
					getnameinfo((struct sockaddr*) &peer_addr, sizeof(struct sockaddr), name, sizeof(name), port, sizeof(port), NI_NUMERICHOST | NI_NUMERICSERV);
					debug_print_error(const_logtag, "ERROR (conn_io == NULL) sending to %s:%s", name, port);
					debug_print_error(const_logtag, "failed to send - recv_cb (conn_io == NULL) %d<>%d", sent, written);
					continue;
				}

				server->get_stats_loggeer()->server_count("recv_cb", sent, "", "", "", "tx", "qh3server", "", port_id_cstr);
#if LOG_LEVEL >= LOG_LEVEL_4
				unsigned long send_bytes_crc = crc32(0L, Z_NULL, 0);
				send_bytes_crc = essentials::mod_crc32_z(send_bytes_crc, reinterpret_cast<const uint8_t*>(server->out), sent);
				DEBUG_PRINT2(LOG_LEVEL_4, const_logtag, "sent %zd bytes - crc: %lx", sent, send_bytes_crc);
#endif
				continue;
			}

			if (token_len == 0) {
				debug_print(LOG_LEVEL_4, const_logtag, "stateless retry");

				server->mint_token(dcid, dcid_len, &peer_original_client_addr, peer_addr_len, token, &token_len);

				uint8_t new_cid[LOCAL_CONN_ID_LEN];

				if (gen_cid(new_cid, LOCAL_CONN_ID_LEN) == NULL) {
					continue;
				}

				ssize_t written = quiche_retry(scid, scid_len, dcid, dcid_len, new_cid, LOCAL_CONN_ID_LEN, token, token_len, version, server->out, sizeof(server->out));

				if (written < 0) {
					debug_print_error(const_logtag, "failed to create retry packet: %zd", written);
					continue;
				}

				// if relay through router
				if (VIA_ROUTER) {
					memcpy(reinterpret_cast<void*>(&server->out[written]), reinterpret_cast<const void*>(original_client_serialised_buffer.c_str()), ORIGINAL_CLIENT_ADDR_SZ);
					written += ORIGINAL_CLIENT_ADDR_SZ;
				}
				//
				ssize_t sent = sendto(conns->sock, server->out, written, 0, (struct sockaddr*) &peer_addr, peer_addr_len);

#if LOG_LEVEL >= LOG_LEVEL_5
				char name[INET6_ADDRSTRLEN];
				char port[10];
				getnameinfo((struct sockaddr*) &peer_addr, sizeof(struct sockaddr), name, sizeof(name), port, sizeof(port), NI_NUMERICHOST | NI_NUMERICSERV);
				debug_print(LOG_LEVEL_0, const_logtag, "send to %s:%s bytes:%d", name, port, sent);
#endif

				if (sent != written) {
					char name[INET6_ADDRSTRLEN];
					char port[10];
					getnameinfo((struct sockaddr*) &peer_addr, sizeof(struct sockaddr), name, sizeof(name), port, sizeof(port), NI_NUMERICHOST | NI_NUMERICSERV);
					debug_print_error(const_logtag, "ERROR sending to %s:%s", name, port);
					debug_print_error(const_logtag, "failed to send %d<>%d", sent, written);
					continue;
				}

				server->get_stats_loggeer()->server_count("recv_cb", sent, "", "", "", "tx", "qh3server", "", port_id_cstr);
#if LOG_LEVEL >= LOG_LEVEL_4
				unsigned long send_bytes_crc = crc32(0L, Z_NULL, 0);
				send_bytes_crc = essentials::mod_crc32_z(send_bytes_crc, reinterpret_cast<const uint8_t*>(server->out), sent);
				DEBUG_PRINT2(LOG_LEVEL_4, const_logtag, "sent %zd bytes - crc: %lx", sent, send_bytes_crc);
#endif
				continue;
			}

			if (!server->validate_token(token, token_len, &peer_original_client_addr, peer_addr_len, odcid, &odcid_len)) {
				debug_print_warn(const_logtag, "invalid address validation token");
				continue;
			}

			conn_io = server->create_conn(dcid, dcid_len, odcid, odcid_len, conns->local_addr, conns->local_addr_len, &peer_addr, peer_addr_len, &peer_original_client_addr);

			if (conn_io == NULL) {
				continue;
			}
			// cache the original client adress for later use. (flush_engress)
			conn_io->original_client_serialised_buffer.bin_copy((const uint8_t*) original_client_serialised_buffer.c_str(), original_client_serialised_buffer.length());

			server->get_stats_loggeer()->set_total_ram(static_cast<int>((essentials::get_process_used_mem())));
			server->get_stats_loggeer()->server_count("recv_cb", 1, "", "", "", "", "qh3server", "create_conn_io", port_id_cstr);
		}

		quiche_recv_info recv_info = {
			(struct sockaddr*) &peer_addr,
			peer_addr_len,

			conns->local_addr,
			conns->local_addr_len,
		};

		ssize_t done = quiche_conn_recv(conn_io->conn, server->buf, read, &recv_info);

		if (done < 0) {
			debug_print_error(const_logtag, "failed to process packet: %zd", done);
			server->get_stats_loggeer()->server_count("recv_cb", 1, "", "", "", "error", "qh3server", "process_packet_fail", port_id_cstr);
			continue;
		}

#if LOG_LEVEL >= LOG_LEVEL_4
		unsigned long recv_bytes_crc = crc32(0L, Z_NULL, 0);
		recv_bytes_crc = essentials::mod_crc32_z(recv_bytes_crc, reinterpret_cast<const uint8_t*>(server->buf), done);
		DEBUG_PRINT2(LOG_LEVEL_4, const_logtag, "q-recv %zd bytes - crc: %lx", done, recv_bytes_crc);
#endif

		if (quiche_conn_is_established(conn_io->conn)) {
            quiche_h3_event* ev;

			if (conn_io->http3 == NULL) {
				conn_io->http3 = quiche_h3_conn_new_with_transport(conn_io->conn, server->http3_config);
				if (conn_io->http3 == NULL) {
					server->get_stats_loggeer()->server_count("recv_cb", 1, "", "", "", "error", "qh3server", "http3_conn_fail", port_id_cstr);
					debug_print_error(const_logtag, "failed to create HTTP/3 connection");
					continue;
				}
			}

			// pending
			const conn_io_req_res::payload& payload = conn_io->http_response->get_payload();
			if (conn_io->total_sent_bytes < (ssize_t) payload.buffer.length()) {
				server->send_in_chunks(conn_io);
				if (conn_io->total_sent_bytes == (ssize_t) payload.buffer.length()) {
					debug_print(LOG_LEVEL_4, __LOGTAG__, "FINISH Stream sending .... [%d] [%d]", conn_io->total_sent_bytes, payload.buffer.length());
				}
			}
			//

			while (1) {
				int64_t s = quiche_h3_conn_poll(conn_io->http3, conn_io->conn, (struct quiche_h3_event**) &ev);

				if (s < 0) {
					break;
				}

				switch (quiche_h3_event_type(ev)) {
					case quiche_h3_event_type::QUICHE_H3_EVENT_HEADERS: {
						int rc = quiche_h3_event_for_each_header((struct quiche_h3_event*) ev, for_each_header, conn_io);

						if (rc != 0) {
							debug_print_error(const_logtag, "failed to process headers");
							server->get_stats_loggeer()->server_count("recv_cb", 1, "", "", "", "error", "qh3server", "process_header_fail", port_id_cstr);
						}
						break;
					}

					case quiche_h3_event_type::QUICHE_H3_EVENT_DATA: {
						//                        debug_print(LOG_LEVEL_1, __LOGTAG__, "got
						//                        HTTP req body");
						//                        conn_io->http_request.clear_payload();
						for (;;) {
							ssize_t len = quiche_h3_recv_body(conn_io->http3, conn_io->conn, s, server->buf, sizeof(server->buf));
							if (len <= 0) {
								break;
							}
							conn_io->http_request->set_payload(qstring(server->buf, len));
							debug_print(LOG_LEVEL_4, const_logtag, "<<<<< (q) %.*s", (int) len, server->buf);
						}
						break;
					}

					case quiche_h3_event_type::QUICHE_H3_EVENT_FINISHED: {
						EV_START_RECORD(parse_start_time);
						conn_io->bridge->parse(conn_io);
						EV_PRINT_IF_ELAPSED_AND_CLEAR(parse_start_time, const_logtag, "parse-time t:%lu ms", 1200);

						EV_START_RECORD(send_start_time);
						if (payload.buffer.length() == 0) {
							debug_print(LOG_LEVEL_4, const_logtag, "no-response. ignoring the request!!!");
							conn_io->http_response->set_payload(qstring("{}", strlen("{}")));
						}
						const qstring& content_length_data = qstring::format_string("%d", static_cast<int>(payload.buffer.length()));
						const qstring& crc = payload.get_crc_string();

						int header_size = 5;
						conn_io_req_res::header* status_header = conn_io->http_response->get_header(":status");
                        quiche_h3_header* headers = DEBUG_NEW quiche_h3_header[header_size + conn_io->http_response->headers.size()];
						headers[0] = {
							.name = (uint8_t*) ":status",
							.name_len = sizeof(":status") - 1,

							.value = status_header ? (uint8_t*) status_header->value.c_str() : (uint8_t*) "200",
							.value_len = status_header ? status_header->value.length() : sizeof("200") - 1,
						};
						headers[1] = {
							.name = (uint8_t*) "Alternate-Protocol",
							.name_len = sizeof("Alternate-Protocol") - 1,

							.value = (uint8_t*) conns->quic_alternate_protocol_str.c_str(),
							.value_len = conns->quic_alternate_protocol_str.length() - 1,
						};

						headers[2] = {
							.name = (uint8_t*) "server",
							.name_len = sizeof("server") - 1,

							.value = (uint8_t*) "quiche",
							.value_len = sizeof("quiche") - 1,
						};
						headers[3] = {
							.name = (uint8_t*) "content-length",
							.name_len = sizeof("content-length") - 1,

							.value = (uint8_t*) content_length_data.c_str(),
							.value_len = content_length_data.length(),
						};
						headers[4] = {
							.name = (uint8_t*) "crc",
							.name_len = sizeof("crc") - 1,

							.value = (uint8_t*) crc.c_str(),
							.value_len = crc.length(),
						};

						int additional_header_index = 0;
						for (auto it : conn_io->http_response->headers) {
							headers[header_size + additional_header_index] = {
								.name = (uint8_t*) it.second->name.c_str(),
								.name_len = it.second->name.length(),

								.value = (uint8_t*) it.second->value.c_str(),
								.value_len = it.second->value.length(),
							};
							debug_print(LOG_LEVEL_4, const_logtag, "custom header %s - %s", it.second->name.c_str(), it.second->value.c_str());
							additional_header_index++;
						}
						quiche_h3_send_response(conn_io->http3, conn_io->conn, s, headers, header_size + conn_io->http_response->headers.size(), false);
						GX_DELETE_ARY(headers);

						EV_PRINT_IF_ELAPSED_AND_CLEAR(parse_start_time, const_logtag, "q-send_response t:%lu ms", 50);

						// payload
						conn_io->total_sent_bytes = 0;	// reset the total bytes sent over network
						ssize_t bytes_to_send = payload.buffer.length();
						if (bytes_to_send < SEND_CHUNK_SIZE) {	// if small chunk then try issue in one go.
							ssize_t sent = quiche_h3_send_body(conn_io->http3, conn_io->conn, s, reinterpret_cast<const uint8_t*>(payload.buffer.c_str()), bytes_to_send, true);
							if (sent < 0) {
								debug_print_error(const_logtag, "HTTP response send failure. quiche_h3_send_body returned %d", sent);
								break;
							}
							conn_io->total_sent_bytes += sent;
							if (conn_io->total_sent_bytes != (ssize_t) payload.buffer.length()) {
								debug_print_error(const_logtag, "HTTP response send failure %d<>%d", conn_io->total_sent_bytes, payload.buffer.length());
								server->get_stats_loggeer()->server_count("recv_cb", 1, "", conn_io->total_sent_bytes, (ssize_t) payload.buffer.length(), "error", "qh3server", "response_send_fail", port_id_cstr);
								break;
							}
						} else {
							conn_io->stream_id = s;
							server->send_in_chunks(conn_io);
							debug_print(LOG_LEVEL_4, __LOGTAG__, "START Stream sending .... [%d] [%d]", conn_io->total_sent_bytes, payload.buffer.length());
							if (conn_io->total_sent_bytes < (ssize_t) payload.buffer.length()) {
								debug_print(LOG_LEVEL_4, const_logtag, "(Partial) HTTP response send %d<>%d", conn_io->total_sent_bytes, payload.buffer.length());
							}
						}

						EV_PRINT_IF_ELAPSED_AND_CLEAR(send_start_time, const_logtag, "q-send_body t:%lu ms", 30);
#if LOG_LEVEL >= LOG_LEVEL_4
						DEBUG_PRINT2(LOG_LEVEL_4, const_logtag, "q-sent HTTP response over %" PRId64 " with body %s\n\t%zd bytes - crc: %lx", s, payload.buffer.c_str(), payload.get_size(), payload.get_crc_value());
#endif
					} break;

					case quiche_h3_event_type::QUICHE_H3_EVENT_RESET:
						break;

					case quiche_h3_event_type::QUICHE_H3_EVENT_PRIORITY_UPDATE:
						break;

					case quiche_h3_event_type::QUICHE_H3_EVENT_GOAWAY: {
						debug_print(LOG_LEVEL_1, const_logtag, "got GOAWAY");
						break;
					}
				}

				quiche_h3_event_free(ev);
			}
		}
	}

	HASH_ITER(hh, conns->h, conn_io, tmp) {
		server->flush_egress(conn_io);

		if (quiche_conn_is_closed(conn_io->conn)) {
            quiche_stats stats;
            quiche_path_stats path_stats;

			quiche_conn_stats(conn_io->conn, &stats);
			quiche_conn_path_stats(conn_io->conn, 0, &path_stats);

			debug_print(LOG_LEVEL_4, const_logtag, "connection closed, recv=%zu sent=%zu lost=%zu rtt=%" PRIu64 "ns cwnd=%zu", stats.recv, stats.sent, stats.lost, path_stats.rtt, path_stats.cwnd);

			HASH_DELETE(hh, conns->h, conn_io);
			GX_DELETE(conn_io);
		}
	}
}

void qh3server::send_in_chunks(struct conn_io_qh3* conn_io) {
	const conn_io_req_res::payload& payload = conn_io->http_response->get_payload();
	size_t chunk_size = SEND_CHUNK_SIZE;
	const uint8_t* data = reinterpret_cast<const uint8_t*>(payload.buffer.c_str());
	size_t start_index = conn_io->total_sent_bytes;
	size_t total_payload_size = static_cast<size_t>(payload.buffer.length());

	// Send the data in chunks
	for (size_t offset = start_index; offset < total_payload_size; offset += chunk_size) {
		size_t remaining = total_payload_size - offset;
		size_t chunk = remaining < chunk_size ? remaining : chunk_size;
		// Send a chunk of the data
		bool fin = offset + chunk >= total_payload_size;
		ssize_t sent = quiche_h3_send_body(conn_io->http3, conn_io->conn, conn_io->stream_id, data + offset, chunk, fin);
		if (sent < 0) {
			//            fprintf(stderr, "Error sending body: %zd\n", sent);
			break;
		}
		if (fin) {
			DEBUG_PRINT2(LOG_LEVEL_4, __LOGTAG__, "fin");
		}
		conn_io->total_sent_bytes += sent;
	}
}

void qh3server::destroy_connection(struct conn_io_qh3* conn_io) {
	HASH_DELETE(hh, conns->h, conn_io);
	GX_DELETE(conn_io);
}

#if USE_UV_MAIN_LOOP
void qh3server::timeout_cb(uv_timer_t* w) {
#else
void qh3server::timeout_cb(EV_P_ ev_timer* w, int revents) {
	UNUSED(revents);
#endif
	struct conn_io_qh3* conn_io = (struct conn_io_qh3*) w->data;
	quiche_conn_on_timeout(conn_io->conn);
	DEBUG_PRINT2(LOG_LEVEL_5, __LOGTAG__, "timeout - %lx", conn_io->cid_hash_val);
	conn_io->bridge->flush_egress(conn_io);

	if (quiche_conn_is_closed(conn_io->conn)) {
        quiche_stats stats;
        quiche_path_stats path_stats;

		quiche_conn_stats(conn_io->conn, &stats);
		quiche_conn_path_stats(conn_io->conn, 0, &path_stats);

		debug_print(LOG_LEVEL_4, __LOGTAG__, "connection closedA, recv=%zu sent=%zu lost=%zu rtt=%" PRIu64 "ns cwnd=%zu", stats.recv, stats.sent, stats.lost, path_stats.rtt, path_stats.cwnd);

		conn_io->bridge->destroy_connection(conn_io);
		return;
	}
}

int qh3server::run(const qstring& host, const qstring& port, const fs::path& root_dir, struct addrinfo* router, uint16_t command_center_feedback_port, uint16_t router_port_return, const qstring& app_id) {
	app_directory = root_dir;
	host_id = host;
	port_id = port;
	GX_DELETE(relay_through_router_info);
	if (router != nullptr) {
		relay_through_router_info = DEBUG_NEW struct routerinfo(router, router_port_return);
	}
	logtag = qstring::format_string("%s:%s", __LOGTAG__, port.c_str());
	const char* const_logtag = logtag.c_str();
#if ENABLE_QUICHE_LOG
	quiche_enable_debug_logging(debug_quiche_log, this);
	debug_print_warn(const_logtag,
					 "quiche log is enabled. Perfomance may get "
					 "affected due to excess logs !!!");
#endif

	const struct addrinfo HINTS = {.ai_family = PF_UNSPEC, .ai_socktype = SOCK_DGRAM, .ai_protocol = IPPROTO_UDP};
	struct addrinfo* local;
	if (getaddrinfo(host.c_str(), port.c_str(), &HINTS, &local) != 0) {
		debug_print_error(const_logtag, "failed to resolve host - port[%s]", port.c_str());
		GX_DELETE(relay_through_router_info);
		return -1;
	}

	int sock = socket(local->ai_family, SOCK_DGRAM, 0);
	if (sock < 0) {
		debug_print_error(const_logtag, "failed to create socket - port[%s]", port.c_str());
		freeaddrinfo(local);
		GX_DELETE(relay_through_router_info);
		return -1;
	}

	if (fcntl(sock, F_SETFL, O_NONBLOCK) != 0) {
		debug_print_error(const_logtag, "failed to make socket non-blocking - port[%s]", port.c_str());
		close(sock);  // (amudaliar) : Needed for running as virtual servers. Else
		// new servers wont be able to bind.
		freeaddrinfo(local);
		GX_DELETE(relay_through_router_info);
		return -1;
	}

	if (bind(sock, local->ai_addr, local->ai_addrlen) < 0) {
		debug_print_error(const_logtag, "failed to bind socket - port[%s]", port.c_str());
		close(sock);
		freeaddrinfo(local);
		GX_DELETE(relay_through_router_info);
		return -1;
	}

	config = quiche_config_new(QUICHE_PROTOCOL_VERSION);
	if (config == NULL) {
		debug_print_error(const_logtag, "failed to create config");
		close(sock);
		freeaddrinfo(local);
		GX_DELETE(relay_through_router_info);
		return -1;
	}

	fs::path cert_file(root_dir / "cert.crt");
	fs::path key_file(root_dir / "cert.key");
	debug_print(LOG_LEVEL_2, const_logtag, "cert file %s, key file %s", cert_file.c_str(), key_file.c_str());
	int res_crt_load = quiche_config_load_cert_chain_from_pem_file(config, cert_file.c_str());
	if (res_crt_load != 0) {
		debug_print_error(const_logtag, "CERT load error - %s, err %d", cert_file.c_str(), res_crt_load);
		close(sock);
		freeaddrinfo(local);
		GX_DELETE(relay_through_router_info);
		return -1;
	}
	int res_key_load = quiche_config_load_priv_key_from_pem_file(config, key_file.c_str());
	if (res_key_load != 0) {
		debug_print_error(const_logtag, "KEY load error - %s", key_file.c_str());
		close(sock);
		freeaddrinfo(local);
		GX_DELETE(relay_through_router_info);
		return -1;
	}

	quiche_config_set_application_protos(config, reinterpret_cast<const uint8_t*>(QUICHE_H3_APPLICATION_PROTOCOL), sizeof(QUICHE_H3_APPLICATION_PROTOCOL) - 1);

	quiche_config_set_max_idle_timeout(config, 25000);
	quiche_config_set_max_recv_udp_payload_size(config, MAX_DATAGRAM_SIZE);
	quiche_config_set_max_send_udp_payload_size(config, MAX_DATAGRAM_SIZE);
	quiche_config_set_initial_max_data(config, 10000000);
	quiche_config_set_initial_max_stream_data_bidi_local(config, 1000000);
	quiche_config_set_initial_max_stream_data_bidi_remote(config, 1000000);
	quiche_config_set_initial_max_stream_data_uni(config, 1000000);
	quiche_config_set_initial_max_streams_bidi(config, 100);
	quiche_config_set_initial_max_streams_uni(config, 100);
	quiche_config_set_disable_active_migration(config, true);
	quiche_config_set_cc_algorithm(config, quiche_cc_algorithm::QUICHE_CC_RENO);

	// Generate a 16-byte token for stateless reset
	uint8_t stateless_reset_token[16] = {0x1a, 0x2b, 0x3c, 0x4d, 0x5e, 0x6f, 0x7a, 0x8b, 0x9c, 0xad, 0xbe, 0xcf, 0xda, 0xeb, 0xfc, 0x0d};

	// Set the stateless reset token in the QUIC config
	quiche_config_set_stateless_reset_token(config, stateless_reset_token);

	http3_config = quiche_h3_config_new();
	if (http3_config == NULL) {
		debug_print_error(const_logtag, "failed to create HTTP/3 config");
		close(sock);
		freeaddrinfo(local);
		GX_DELETE(relay_through_router_info);
		return -1;
	}

	struct connections c;
	c.sock = sock;
	c.h = NULL;
	c.local_addr = local->ai_addr;
	c.local_addr_len = local->ai_addrlen;
	c.server_port = port;
	c.quic_alternate_protocol_str = qstring("quic:") + port;

	conns = &c;

#if USE_UV_MAIN_LOOP
	debug_print_important(const_logtag, "USE_UV_MAIN_LOOP is enabled");
	mainloop = uv_loop_new();

	int poll_status = uv_poll_init_socket(mainloop, &c.poll_handle, sock);
	if (poll_status < 0) {
		debug_print_error(const_logtag, "Poll init socket error: %s", uv_strerror(poll_status));
	}
	c.poll_handle.data = this;
	int poll_start_status = uv_poll_start(&c.poll_handle, UV_READABLE, recv_cb);
	if (poll_start_status < 0) {
		debug_print_error(const_logtag, "Poll start error: %s", uv_strerror(poll_start_status));
	}
#else
	ev_io watcher;
	mainloop = ev_loop_new();
	ev_io_init(&watcher, recv_cb, sock, EV_READ);
	ev_io_start(mainloop, &watcher);
	watcher.data = this;
#endif

	//
	GX_DELETE(logger);
	GX_DELETE(stats_logger);
	logger = DEBUG_NEW qcustomlogger();
	stats_logger = DEBUG_NEW qstatslogger();

	if (!on_server_pre_init()) {
		debug_print_error(const_logtag, "on_server_pre_init failed !!!, Exiting.");
		on_server_uninitialise();
		stop_services_and_report(sock, command_center_feedback_port, 3.0f);
		GX_DELETE(stats_logger);
		GX_DELETE(logger);
		close(sock);
		freeaddrinfo(local);
		GX_DELETE(relay_through_router_info);
		return -1;
	}

	// logs and stats service
	qstring log_path = qstring::format_string("./logs/%s/qh3_logfile", port.c_str());
	qstring stats_path = qstring::format_string("./stats/%s/qh3_statfile", port.c_str());
	qh3server::get_file_logger()->start_session(log_path, log_path.length());
	qh3server::get_stats_loggeer()->init(essentials::get_sysname(), essentials::get_device_name(), "", app_id, 0);
	qh3server::get_stats_loggeer()->start_session(stats_path, stats_path.length());

	// dangling connection check timer
	TIMER_SCHEDuLER_TYPE close_dangling_connections_scheduler;
	close_dangling_connections_scheduler.set_loop(mainloop);
	TIMER_TYPE* dangling_connections_check_timer = dangling_connections_check_loop(close_dangling_connections_scheduler, 3.0f);

	// router hb timer
	TIMER_SCHEDuLER_TYPE router_hb_scheduler;
	router_hb_scheduler.set_loop(mainloop);
	TIMER_TYPE* router_hb_timer = router_hb_loop(router_hb_scheduler, host, port, sock, command_center_feedback_port);

	on_run_started();

	// main event loop
#if USE_UV_MAIN_LOOP
	uv_run(mainloop, UV_RUN_DEFAULT);
#else
	ev_loop(mainloop, 0);
#endif
	// destroy pending connections
	destroy_pending_connections();
	//

	on_run_end();
	on_server_uninitialise();

	// timer cleanups
	router_hb_scheduler.cancel_and_destroy_timer(router_hb_timer);
	close_dangling_connections_scheduler.cancel_and_destroy_timer(dangling_connections_check_timer);

	// ------------- cleanups starts from here -------------
	// ------------- cleanups starts from here -------------
	// ------------- cleanups starts from here -------------

	// cleaning and destroying main event loop
	debug_print(LOG_LEVEL_0, __LOGTAG__, "pre-cleanup and destroy %s:%s", host_id.c_str(), port_id.c_str());
#if USE_UV_MAIN_LOOP
	if (!essentials::cleanup_and_destroy_uv_loop(mainloop)) {
		debug_print_error(__LOGTAG__, "Failed to delete mainloop !!!");
	} else {
		mainloop = nullptr;
	}
#else
	ev_loop_destroy(mainloop);
#endif
	debug_print(LOG_LEVEL_0, __LOGTAG__, "post-cleanup and destroy %s:%s", host_id.c_str(), port_id.c_str());

	// quiche cleanups
	quiche_h3_config_free(http3_config);
	quiche_config_free(config);

	// logs and stats shutdown command (async)
	get_stats_loggeer()->end_session();
	get_file_logger()->end_session();

	// async shutdown of services
	stop_services_and_report(sock, command_center_feedback_port, 3.0f);

	// destroying vars
	close(sock);
	freeaddrinfo(local);
	GX_DELETE(relay_through_router_info);
	GX_DELETE(logger);
	GX_DELETE(stats_logger);
	debug_print(LOG_LEVEL_0, __LOGTAG__, "exiting from run loop %s:%s", host_id.c_str(), port_id.c_str());
	return 0;
}

void qh3server::stop_services_and_report(int sock, uint16_t command_center_feedback_port, float interval) {
	const char* const_logtag = logtag.c_str();
	debug_print_important(const_logtag, "waiting for services to finish !!!");
	struct ev_loop* wait_loop = ev_loop_new();
	qtimer_scheduler wait_scheduler;
	wait_scheduler.set_loop(wait_loop);
	qtimer* wait_timer = wait_scheduler.schedule_repeat_timer(
		[this, wait_loop, const_logtag, sock, command_center_feedback_port](qtimer& timer) {
			UNUSED(timer);
			int service_shutdown_cnt = 0;
			if (get_stats_loggeer()->config.finished) {
				debug_print_important(const_logtag, "stats service finished !!!");
				service_shutdown_cnt++;
			}
			if (get_file_logger()->config.finished) {
				debug_print_important(const_logtag, "logger service finished !!!");
				service_shutdown_cnt++;
			}
			if (service_shutdown_cnt >= 2) {
				const struct addrinfo HINTS = {.ai_family = PF_UNSPEC, .ai_socktype = SOCK_DGRAM, .ai_protocol = IPPROTO_UDP};
				qstring command_center_feedback_port_str = qstring::format_string("%d", command_center_feedback_port);
				struct addrinfo* cmd_center_feedback_address;
				if (getaddrinfo(host_id.c_str(), command_center_feedback_port_str.c_str(), &HINTS, &cmd_center_feedback_address) != 0) {
					debug_print_error(const_logtag, "failed to resolve host - port[%s]", command_center_feedback_port_str.c_str());
					return;
				}
				debug_print(LOG_LEVEL_0, __LOGTAG__, "Sending shutdown-ack to %s:%s", host_id.c_str(), command_center_feedback_port_str.c_str());
				qstring shut_cmd = qstring::format_string("shut-ack-%s", port_id.c_str());
				ssize_t sent = sendto(sock, shut_cmd.c_str(), shut_cmd.length(), 0, cmd_center_feedback_address->ai_addr, cmd_center_feedback_address->ai_addrlen);
				if (sent != (ssize_t) shut_cmd.length()) {
					debug_print_error(const_logtag, "ERROR sending shutdown event to command center !!!");
				}
				freeaddrinfo(cmd_center_feedback_address);
				ev_break(wait_loop, EVBREAK_ONE);
			}
		},
		interval);

	ev_run(wait_loop, 0);
	wait_scheduler.cancel_and_destroy_timer(wait_timer);
	ev_loop_destroy(wait_loop);
	debug_print_important(const_logtag, "services finish successfully !!!");
}

void qh3server::destroy_pending_connections() {
	const char* const_logtag = logtag.c_str();
	struct conn_io_qh3 *tmp, *conn_io = NULL;
	int pending_connections = 0;
	HASH_ITER(hh, conns->h, conn_io, tmp) {
		ssize_t sent_bytes = flush_egress(conn_io);
		if (sent_bytes) {
			DEBUG_PRINT2(LOG_LEVEL_3, const_logtag, "force close --> try flush : sent bytes %zd", sent_bytes);
		}
		if (quiche_conn_is_closed(conn_io->conn)) {
			debug_print(LOG_LEVEL_0, const_logtag, "force close : connection is already closed");
		}
        quiche_stats stats;
        quiche_path_stats path_stats;
		quiche_conn_stats(conn_io->conn, &stats);
		quiche_conn_path_stats(conn_io->conn, 0, &path_stats);
		debug_print(LOG_LEVEL_3, const_logtag, "connection force closed, recv=%zu sent=%zu lost=%zu rtt=%" PRIu64 "ns cwnd=%zu", stats.recv, stats.sent, stats.lost, path_stats.rtt, path_stats.cwnd);
		HASH_DELETE(hh, conns->h, conn_io);
		GX_DELETE(conn_io);
		pending_connections++;
	}
	if (pending_connections > 0) {
		debug_print(LOG_LEVEL_0, const_logtag, "Force closed %d pending connections.", pending_connections);
	}
	//
}

TIMER_TYPE* qh3server::dangling_connections_check_loop(TIMER_SCHEDuLER_TYPE& close_dangling_connections_scheduler, float interval) {
	const char* const_logtag = logtag.c_str();
	TIMER_TYPE* dangling_connections_check_timer = close_dangling_connections_scheduler.schedule_repeat_timer(
		[this, const_logtag](TIMER_TYPE& timer) {
			UNUSED(timer);
			//            debug_print(LOG_LEVEL_0, __LOGTAG__, "dangling_connections_check_timer callback");
			int dangling_connections = 0;
			int dangling_with_response = 0;
			int flushed_on_exit = 0;
			struct conn_io_qh3 *tmp, *conn_io = NULL;
			HASH_ITER(hh, conns->h, conn_io, tmp) {
#if USE_UV_MAIN_LOOP
				uint64_t elapsed = (uv_now(mainloop) - conn_io->creation_time) / 1000;	// Elapsed time in seconds
#else
				ev_tstamp elapsed = ev_now(mainloop) - conn_io->creation_time;
#endif
				if (elapsed > DROP_CONNECTION_AFTER && conn_io->timer.repeat == 0) {  // DROP_CONNECTION_AFTER seconds after connection
					// creation time.
					bool is_closed = quiche_conn_is_closed(conn_io->conn);
					if (!is_closed) {
						debug_print(LOG_LEVEL_4, const_logtag, "dangling : try flush : connection is still open");
						ssize_t sent_bytes = flush_egress(conn_io);
						if (sent_bytes) {
							DEBUG_PRINT2(LOG_LEVEL_4, const_logtag, "dangling : try flush : sent bytes %zd", sent_bytes);
							flushed_on_exit++;
						}
					}

					if (conn_io->http_response->get_payload().buffer.length() > 3) {
						dangling_with_response++;
					}
					debug_print(LOG_LEVEL_4, const_logtag, "closing dangling connection !!!");
                    quiche_stats stats;
                    quiche_path_stats path_stats;
					quiche_conn_stats(conn_io->conn, &stats);
					quiche_conn_path_stats(conn_io->conn, 0, &path_stats);
#if USE_UV_MAIN_LOOP
					uint64_t elapsed = (uv_now(mainloop) - conn_io->creation_time) / 1000;	// Elapsed time in seconds
#else
					ev_tstamp elapsed = ev_now(mainloop) - conn_io->creation_time;
#endif
					debug_print(LOG_LEVEL_4, const_logtag,
								"dangling connection force closed, recv=%zu sent=%zu "
								"lost=%zu rtt=%" PRIu64 "ns cwnd=%zu elapsed:%10.2fs",
								stats.recv, stats.sent, stats.lost, path_stats.rtt, path_stats.cwnd, elapsed);

					if (!is_closed) {
						int close_result = quiche_conn_close(conn_io->conn, true, 0, NULL, 0);
						if (close_result < 0) {
							conn_io->skip_destroy_counter++;
							if (conn_io->skip_destroy_counter < 3) {
								continue;
							}
							bool is_draining = quiche_conn_is_draining(conn_io->conn);
							if (!is_draining) {
								debug_print_error(const_logtag, "failed to close dangling connection, err %d", close_result);
							}
						}
					}

					HASH_DELETE(hh, conns->h, conn_io);
					GX_DELETE(conn_io);
					dangling_connections++;
				}
			}
			if (dangling_connections > 0) {
				if (dangling_connections < 10) {
					debug_print(flushed_on_exit ? LOG_LEVEL_0 : LOG_LEVEL_3, const_logtag,
								"Force closed %d dangling connections, with "
								"response %d. flushed_on_exit(%d)",
								dangling_connections, dangling_with_response, flushed_on_exit);
				} else if (dangling_connections >= 10 && dangling_connections < 20) {
					debug_print_important2(const_logtag,
										   "Force closed %d dangling connections, "
										   "with response %d. flushed_on_exit(%d)",
										   dangling_connections, dangling_with_response, flushed_on_exit);
				} else if (dangling_connections >= 20) {
					debug_print_warn(const_logtag,
									 "Force closed %d dangling connections, with "
									 "response %d. flushed_on_exit(%d)",
									 dangling_connections, dangling_with_response, flushed_on_exit);
				}
			}
		},
		interval);

	return dangling_connections_check_timer;
}

TIMER_TYPE* qh3server::router_hb_loop(TIMER_SCHEDuLER_TYPE& router_hb_scheduler, const qstring& host, const qstring& port, int sock, uint16_t command_center_feedback_port) {
	float router_hb_interval_in_sec = get_router_hb_interval_in_sec();
	debug_print_important(__LOGTAG__, "router_hb_interval_in_sec timer %5.1f", router_hb_interval_in_sec);
	const char* const_logtag = logtag.c_str();
	TIMER_TYPE* router_hb_timer = router_hb_scheduler.schedule_repeat_timer(
		[this, const_logtag, host, sock, command_center_feedback_port](TIMER_TYPE& timer) {
			int new_timer_val = get_router_hb_interval_in_sec();
			float diff = new_timer_val - timer.delay;
			if (GX_ABS(diff) > 1.0f) {
				debug_print_important(__LOGTAG__, "router_hb_timer updated from %5.1f to %d", timer.delay, new_timer_val);
				timer.update_delay(new_timer_val);
			}
			const struct addrinfo HINTS = {.ai_family = PF_UNSPEC, .ai_socktype = SOCK_DGRAM, .ai_protocol = IPPROTO_UDP};
			qstring command_center_feedback_port_str = qstring::format_string("%d", command_center_feedback_port);
			struct addrinfo* cmd_center_feedback_address;
			if (getaddrinfo(host.c_str(), command_center_feedback_port_str.c_str(), &HINTS, &cmd_center_feedback_address) != 0) {
				debug_print_error(const_logtag, "failed to resolve host - port[%s]", command_center_feedback_port_str.c_str());
				return;
			}
			debug_print(LOG_LEVEL_5, __LOGTAG__, "Sending heartbeat to %s:%s", host.c_str(), command_center_feedback_port_str.c_str());
			qstring hb_cmd = qstring::format_string("hb-%s", port_id.c_str());
			ssize_t sent = sendto(sock, hb_cmd.c_str(), hb_cmd.length(), 0, cmd_center_feedback_address->ai_addr, cmd_center_feedback_address->ai_addrlen);
			if (sent != (ssize_t) hb_cmd.length()) {
				debug_print_error(const_logtag, "ERROR sending hb event to command center !!!");
			}
			freeaddrinfo(cmd_center_feedback_address);
		},
		router_hb_interval_in_sec);
	return router_hb_timer;
}
