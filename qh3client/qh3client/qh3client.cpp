//
//  Copyright 2024 homenet25
//  qh3client.cpp
//  qh3client
//
//  Created by Arun A on 30/10/23.
//

#include "qh3client.hpp"

#include "../../common/sdktypes.hpp"

#include <zlib.h>
#if PLATFORM == PLATFORM_WINDOWS
#include <corecrt_io.h>
#endif

using client::qh3client;

void qh3client::debug_log(const char* line, void* argp) {
	UNUSED(argp);
	debug_print(LOG_LEVEL_0, __LOGTAG__, line);
}

void qh3client::flush_egress(struct ev_loop* loop, struct conn_io_qh3_client* conn_io) {
	quiche_send_info send_info;

	while (1) {
		ssize_t written = quiche_conn_send(conn_io->conn, conn_io->out, sizeof(conn_io->out), &send_info);

		if (written == QUICHE_ERR_DONE) {
			DEBUG_PRINT2(LOG_LEVEL_5, __LOGTAG__, "done writing");
			break;
		}

		if (written < 0) {
			fprintf(stderr, "failed to create packet: %zd\n", written);
			return;
		}

#if PLATFORM == PLATFORM_WINDOWS
		ssize_t sent = sendto(conn_io->sock, (const char*) conn_io->out, written, 0, (struct sockaddr*) &send_info.to, send_info.to_len);
#else
		ssize_t sent = sendto(conn_io->sock, conn_io->out, written, 0, (struct sockaddr*) &send_info.to, send_info.to_len);
#endif

		if (sent != written) {
			perror("failed to send");
			return;
		}

#if LOG_LEVEL >= LOG_LEVEL_4
		unsigned long send_bytes_crc = crc32(0L, Z_NULL, 0);
		send_bytes_crc = essentials::mod_crc32_z(send_bytes_crc, reinterpret_cast<const uint8_t*>(conn_io->out), sent);
		DEBUG_PRINT2(LOG_LEVEL_4, __LOGTAG__, "sent %zd bytes - crc: %lx", sent, send_bytes_crc);
#endif
	}

	double t = quiche_conn_timeout_as_nanos(conn_io->conn) / 1e9f;
	conn_io->timer.repeat = t;
	ev_timer_again(loop, &conn_io->timer);
}

int qh3client::for_each_setting(uint64_t identifier, uint64_t value, void* argp) {
	UNUSED(argp);
	debug_print(LOG_LEVEL_4, __LOGTAG__, "got HTTP/3 SETTING: %" PRIu64 "=%" PRIu64 "", identifier, value);

	return 0;
}

/*
 int (*cb)(const uint8_t *name,
   size_t name_len,
   const uint8_t *value,
   size_t value_len,
   void *argp)
 */
int qh3client::for_each_header(uint8_t* name, size_t name_len, uint8_t* value, size_t value_len, void* argp) {
	debug_print(LOG_LEVEL_5, __LOGTAG__, "got HTTP header: %.*s=%.*s", (int) name_len, name, (int) value_len, value);

	struct conn_io_qh3_client* conn_io = (struct conn_io_qh3_client*) argp;
	if (conn_io->response == nullptr) {
		return 0;
	}
	conn_io->response->add_or_get_header(qstring(name, name_len), qstring(value, value_len));
	return 0;
}

int64_t qh3client::send_get_http_request(const conn_io_req_res* data_getorpost, struct conn_io_qh3_client* conn_io) {
	const qstring& crc_buffer = data_getorpost->get_payload().get_crc_string();
	int header_size = 5;
	quiche_h3_header* headers = DEBUG_NEW quiche_h3_header[header_size + data_getorpost->headers.size()];
	headers[0] = {
		.name = (uint8_t*) ":method",
		.name_len = sizeof(":method") - 1,

		.value = (uint8_t*) "GET",
		.value_len = sizeof("GET") - 1,
	};
	headers[1] = {
		.name = (uint8_t*) ":scheme",
		.name_len = sizeof(":scheme") - 1,

		.value = (uint8_t*) "https",
		.value_len = sizeof("https") - 1,
	};
	headers[2] = {
		.name = (uint8_t*) ":authority",
		.name_len = sizeof(":authority") - 1,

		.value = (uint8_t*) conn_io->host,
		.value_len = strlen(conn_io->host),
	};
	headers[3] = {
		.name = (uint8_t*) "user-agent",
		.name_len = sizeof("user-agent") - 1,

		.value = (uint8_t*) "quiche",
		.value_len = sizeof("quiche") - 1,
	};
	headers[4] = {
		.name = (uint8_t*) "crc",
		.name_len = sizeof("crc") - 1,

		.value = (uint8_t*) crc_buffer.c_str(),
		.value_len = crc_buffer.length(),
	};

	int additional_header_index = 0;
	for (auto it : data_getorpost->headers) {
		headers[header_size + additional_header_index] = {
			.name = (uint8_t*) it.second->name.c_str(),
			.name_len = it.second->name.length(),

			.value = (uint8_t*) it.second->value.c_str(),
			.value_len = it.second->value.length(),
		};
		debug_print(LOG_LEVEL_4, __LOGTAG__, "custom header %s - %s", it.second->name.c_str(), it.second->value.c_str());
		additional_header_index++;
	}

	int64_t stream_id = quiche_h3_send_request(conn_io->http3, conn_io->conn, headers, header_size + data_getorpost->headers.size(), true);
	GX_DELETE_ARY(headers);
	debug_print(LOG_LEVEL_4, __LOGTAG__, "sent HTTP GET request %" PRId64 "", stream_id);
	return stream_id;
}

int64_t qh3client::send_post_http_request(const conn_io_req_res* data_getorpost, struct conn_io_qh3_client* conn_io) {
	const qstring& content_length_data = qstring::format_string("%d", static_cast<int>(data_getorpost->data.buffer.length()));
	const qstring& crc_buffer = data_getorpost->get_payload().get_crc_string();
	debug_print(LOG_LEVEL_4, __LOGTAG__, "crc %lx - %s, payload sz %d", data_getorpost->get_payload().get_crc_value(), crc_buffer.c_str(), data_getorpost->data.buffer.length());

	int header_size = 6;
	quiche_h3_header* headers = DEBUG_NEW quiche_h3_header[header_size + data_getorpost->headers.size()];
	headers[0] = {
		.name = (uint8_t*) ":method",
		.name_len = sizeof(":method") - 1,

		.value = (uint8_t*) "POST",
		.value_len = sizeof("POST") - 1,
	};
	headers[1] = {
		.name = (uint8_t*) ":scheme",
		.name_len = sizeof(":scheme") - 1,

		.value = (uint8_t*) "https",
		.value_len = sizeof("https") - 1,
	};
	headers[2] = {
		.name = (uint8_t*) ":authority",
		.name_len = sizeof(":authority") - 1,

		.value = (uint8_t*) conn_io->host,
		.value_len = strlen(conn_io->host),
	};
	headers[3] = {
		.name = (uint8_t*) "user-agent",
		.name_len = sizeof("user-agent") - 1,

		.value = (uint8_t*) "quiche",
		.value_len = sizeof("quiche") - 1,
	};
	headers[4] = {
		.name = (uint8_t*) "content-length",
		.name_len = sizeof("content-length") - 1,

		.value = (uint8_t*) content_length_data.c_str(),
		.value_len = content_length_data.length(),
	};
	headers[5] = {
		.name = (uint8_t*) "crc",
		.name_len = sizeof("crc") - 1,

		.value = (uint8_t*) crc_buffer.c_str(),
		.value_len = crc_buffer.length(),
	};

	int additional_header_index = 0;
	for (auto it : data_getorpost->headers) {
		headers[header_size + additional_header_index] = {
			.name = (uint8_t*) it.second->name.c_str(),
			.name_len = it.second->name.length(),

			.value = (uint8_t*) it.second->value.c_str(),
			.value_len = it.second->value.length(),
		};
		debug_print(LOG_LEVEL_4, __LOGTAG__, "custom header %s - %s", it.second->name.c_str(), it.second->value.c_str());
		additional_header_index++;
	}

	int64_t stream_id = quiche_h3_send_request(conn_io->http3, conn_io->conn, headers, header_size + data_getorpost->headers.size(), false);
	ssize_t send_len = quiche_h3_send_body(conn_io->http3, conn_io->conn, stream_id, reinterpret_cast<const uint8_t*>(data_getorpost->get_payload().buffer.c_str()), data_getorpost->get_payload().buffer.length(), true);
	GX_DELETE_ARY(headers);
	debug_print(LOG_LEVEL_4, __LOGTAG__, "sent HTTP POST request %" PRId64 " with body %ld", stream_id, send_len);
	return stream_id;
}

void qh3client::recv_cb(EV_P_ ev_io* w, int revents) {
	UNUSED(loop);
	UNUSED(revents);
	qh3client* client = reinterpret_cast<qh3client*>(w->data);
	struct conn_io_qh3_client* conn_io = client->conn_io;

	while (1) {
		struct sockaddr_storage peer_addr;
		socklen_t peer_addr_len = sizeof(peer_addr);
		memset(&peer_addr, 0, peer_addr_len);

#if PLATFORM == PLATFORM_WINDOWS
		ssize_t read = recvfrom(conn_io->sock, (char*) conn_io->buf, sizeof(conn_io->buf), 0, (struct sockaddr*) &peer_addr, &peer_addr_len);
		if (read < 0) {
			int error = WSAGetLastError();
			if (error == WSAEWOULDBLOCK) {
				debug_print(LOG_LEVEL_6, __LOGTAG__, "recv would block");
				break;
			}
			debug_print_error(__LOGTAG__, "failed to read");
			return;
		}
#else
		ssize_t read = recvfrom(conn_io->sock, conn_io->buf, sizeof(conn_io->buf), 0, (struct sockaddr*) &peer_addr, &peer_addr_len);
		if (read < 0) {
			if ((errno == EWOULDBLOCK) || (errno == EAGAIN)) {
				debug_print(LOG_LEVEL_6, __LOGTAG__, "recv would block");
				break;
			}

			debug_print_error(__LOGTAG__, "failed to read");
			return;
		}
#endif

#if LOG_LEVEL >= LOG_LEVEL_5
		char name[INET6_ADDRSTRLEN];
		char port[10];
		getnameinfo((struct sockaddr*) &peer_addr, sizeof(struct sockaddr), name, sizeof(name), port, sizeof(port), NI_NUMERICHOST | NI_NUMERICSERV);
		debug_print(LOG_LEVEL_0, __LOGTAG__, "from server unmodified %s:%s read:%d", name, port, read);
#endif

#if 0
        if (conn_io->two_byte_port_check) {
            EV_START_RECORD(server_port_deserialise_time);
            read = read - ORIGINAL_CLIENT_ADDR_SZ;    // remove the size of port bytes from actual packet (quiche packet)
            const uint8_t* port_number_info = &conn_io->buf[read];
            uint16_t port_from_packet = ntohs(*(reinterpret_cast<uint16_t*>(port_number_info)));    // can do verification here
            essentials::update_port((struct sockaddr*)&peer_addr, port_from_packet);
            EV_PRINT_IF_ELAPSED(server_port_deserialise_time, __LOGTAG__, "server_port_deserialise_time t:%lu ms", 10);

#if LOG_LEVEL >= LOG_LEVEL_5
        getnameinfo((struct sockaddr*)&peer_addr, sizeof(struct sockaddr), name, sizeof(name), port, sizeof(port), NI_NUMERICHOST | NI_NUMERICSERV);
        debug_print(LOG_LEVEL_0, __LOGTAG__, "from server modified %s:%s read:%d", name, port, read);
#endif
        }
#endif

		quiche_recv_info recv_info = {
			(struct sockaddr*) &peer_addr,
			peer_addr_len,

			(struct sockaddr*) &conn_io->local_addr,
			conn_io->local_addr_len,
		};

		ssize_t done = quiche_conn_recv(conn_io->conn, conn_io->buf, read, &recv_info);

		if (done < 0) {
			fprintf(stderr, "failed to process packet: %zd\n", done);
			continue;
		}

#if LOG_LEVEL >= LOG_LEVEL_4
		unsigned long recv_bytes_crc = crc32(0L, Z_NULL, 0);
		recv_bytes_crc = essentials::mod_crc32_z(recv_bytes_crc, reinterpret_cast<const uint8_t*>(conn_io->buf), done);
		DEBUG_PRINT2(LOG_LEVEL_4, __LOGTAG__, "q-recv %zd bytes - crc: %lx", done, recv_bytes_crc);
#endif
	}

	DEBUG_PRINT2(LOG_LEVEL_5, __LOGTAG__, "done reading");

	if (quiche_conn_is_closed(conn_io->conn)) {
		debug_print(LOG_LEVEL_4, __LOGTAG__, "connection closed");

		ev_break(EV_A_ EVBREAK_ONE);
		return;
	}

	if (quiche_conn_is_established(conn_io->conn) && !conn_io->req_sent) {
		// Stop the connection establishment timeout timer if connection is established
		ev_timer_stop(loop, &conn_io->connection_establishment_timeout_timer);

		const uint8_t* app_proto;
		size_t app_proto_len;

		quiche_conn_application_proto(conn_io->conn, &app_proto, &app_proto_len);

		debug_print(LOG_LEVEL_4, __LOGTAG__, "connection established: %.*s", (int) app_proto_len, app_proto);

		quiche_h3_config* config = quiche_h3_config_new();
		if (config == NULL) {
			fprintf(stderr, "failed to create HTTP/3 config\n");
			return;
		}

		conn_io->http3 = quiche_h3_conn_new_with_transport(conn_io->conn, config);
		if (conn_io->http3 == NULL) {
			fprintf(stderr, "failed to create HTTP/3 connection\n");
			return;
		}

		quiche_h3_config_free(config);

		const conn_io_req_res* data_getorpost = conn_io->bridge->get_getorpost_http_request();
		if (data_getorpost->is_postrequest()) {
			conn_io->bridge->send_post_http_request(data_getorpost, conn_io);
		} else {
			conn_io->bridge->send_get_http_request(data_getorpost, conn_io);
		}
		conn_io->req_sent = true;
	}

	if (quiche_conn_is_established(conn_io->conn)) {
		quiche_h3_event* ev;

		while (1) {
			int64_t s = quiche_h3_conn_poll(conn_io->http3, conn_io->conn, (struct quiche_h3_event**) &ev);

			if (s < 0) {
				break;
			}

			if (!conn_io->settings_received) {
				int rc = quiche_h3_for_each_setting(conn_io->http3, for_each_setting, NULL);

				if (rc == 0) {
					conn_io->settings_received = true;
				}
			}

			switch (quiche_h3_event_type(ev)) {
				case quiche_h3_event_type::QUICHE_H3_EVENT_HEADERS: {
					int rc = quiche_h3_event_for_each_header((struct quiche_h3_event*) ev, for_each_header, conn_io);

					if (rc != 0) {
						fprintf(stderr, "failed to process headers");
					}

					break;
				}

				case quiche_h3_event_type::QUICHE_H3_EVENT_DATA: {
					for (;;) {
						ssize_t len = quiche_h3_recv_body(conn_io->http3, conn_io->conn, s, conn_io->buf, sizeof(conn_io->buf));

						if (len <= 0) {
							break;
						}
						debug_print(LOG_LEVEL_4, __LOGTAG__, "<<<<< (q) %.*s", (int) len, conn_io->buf);

						if (conn_io->response) {
							conn_io->response->append_to_payload(conn_io->buf, static_cast<int>(len));
						}
					}

#if LOG_LEVEL >= LOG_LEVEL_4
					DEBUG_PRINT2(LOG_LEVEL_4, __LOGTAG__, "payload q-received %zd bytes - crc: %lx", conn_io->response->get_payload().get_size(), conn_io->response->get_payload().get_crc_value());
#endif
					conn_io->res_received = true;

					break;
				}

				case quiche_h3_event_type::QUICHE_H3_EVENT_FINISHED: {
					if (client->response_cb && client->conn_io && client->conn_io->response) {
						client->response_cb(const_cast<conn_io_req_res*>(client->http_request), client->conn_io->response, client->get_client_specific_data(), client->arg, conn_io->res_received);
					}
					if (quiche_conn_close(conn_io->conn, true, 0, reinterpret_cast<const uint8_t*>("finished"), strlen("finished")) < 0) {
						fprintf(stderr, "failed to close connection\n");
					}
					break;
				}

				case quiche_h3_event_type::QUICHE_H3_EVENT_RESET:
					debug_print(LOG_LEVEL_3, __LOGTAG__, "request was reset");

					if (quiche_conn_close(conn_io->conn, true, 0, reinterpret_cast<const uint8_t*>("reset"), strlen("reset")) < 0) {
						fprintf(stderr, "failed to close connection\n");
					}
					break;

				case quiche_h3_event_type::QUICHE_H3_EVENT_PRIORITY_UPDATE:
					break;

				case quiche_h3_event_type::QUICHE_H3_EVENT_GOAWAY: {
					debug_print(LOG_LEVEL_3, __LOGTAG__, "got GOAWAY");
					break;
				}
			}

			quiche_h3_event_free(ev);
		}
	}

	conn_io->bridge->flush_egress(loop, conn_io);
}

void qh3client::timeout_cb(EV_P_ ev_timer* w, int revents) {
	UNUSED(revents);
	struct conn_io_qh3_client* conn_io = (struct conn_io_qh3_client*) w->data;
	quiche_conn_on_timeout(conn_io->conn);

	DEBUG_PRINT2(LOG_LEVEL_5, __LOGTAG__, "timeout");

	conn_io->bridge->flush_egress(loop, conn_io);

	if (quiche_conn_is_closed(conn_io->conn)) {
		quiche_stats stats;
		quiche_path_stats path_stats;

		quiche_conn_stats(conn_io->conn, &stats);
		quiche_conn_path_stats(conn_io->conn, 0, &path_stats);

		debug_print(LOG_LEVEL_4, __LOGTAG__, "connection closed, recv=%zu sent=%zu lost=%zu rtt=%" PRIu64 "ns", stats.recv, stats.sent, stats.lost, path_stats.rtt);

		ev_break(EV_A_ EVBREAK_ONE);
		return;
	}
}

qh3client::qh3client(const qstring& host, const qstring& port, void* arg) : HOST(host), PORT(port), arg(arg) {}

qh3client::~qh3client() {
	GX_DELETE(conn_io);
	debug_print(LOG_LEVEL_4, __LOGTAG__, "qh3client destroyed");
}

int qh3client::close_socket(int sock) {
	int result = closesocket(sock);
	if (result < 0) {
		debug_print_error(__LOGTAG__, "Socket closure failed: %s", strerror(errno));
	}
	return result;
}

void qh3client::on_prepare_client_send() {}
void qh3client::on_post_send_cleanup() {}
void* qh3client::get_client_specific_data() {
	return this;
}

void qh3client::connection_establishment_timeout_cb(EV_P_ ev_timer* w, int revents) {
	UNUSED(revents);

	qh3client* client = reinterpret_cast<qh3client*>(w->data);
	struct conn_io_qh3_client* conn_io = client->conn_io;

	debug_print(LOG_LEVEL_3, __LOGTAG__, "Connection establishment timeout");

	// Clean up and close the connection
	if (conn_io->conn) {
		quiche_conn_close(conn_io->conn, true, 0, reinterpret_cast<const uint8_t*>("tout"), strlen("tout"));
		debug_print_error(__LOGTAG__, "connection couldn't establish due to timeout. t:%5.2fs", ev_now(loop) - conn_io->creation_time);
	}

	ev_break(EV_A_ EVBREAK_ONE);
}

int qh3client::send_request(const conn_io_req_res* data_get, type_qh3client_helper_cb response_cb, float connection_timeout) {
	this->on_prepare_client_send();
	this->http_request = data_get;
	this->response_cb = response_cb;

#if ENABLE_QUICHE_LOG
	quiche_enable_debug_logging(debug_log, NULL);
#endif

	const struct addrinfo HINTS = {.ai_family = PF_UNSPEC, .ai_socktype = SOCK_DGRAM, .ai_protocol = IPPROTO_UDP};
	struct addrinfo* peer;
	if (getaddrinfo(HOST.c_str(), PORT.c_str(), &HINTS, &peer) != 0) {
		fprintf(stderr, "failed to resolve host");
		return -1;
	}

	int sock = socket(peer->ai_family, SOCK_DGRAM, 0);
#if PLATFORM == PLATFORM_WINDOWS
	if (sock == INVALID_SOCKET) {
		fprintf(stderr, "Socket creation failed %d", sock);
		freeaddrinfo(peer);
		return -1;
	}
#endif
	if (sock < 0) {
		fprintf(stderr, "failed to create socket");
		freeaddrinfo(peer);
		return -1;
	}

	if (essentials::set_non_blocking(sock) != 0) {
		debug_print_error(__LOGTAG__, "failed to make socket non-blocking");
		freeaddrinfo(peer);
		close_socket(sock);
		return -1;
	}

	/*
	if (fcntl(sock, F_SETFL, O_NONBLOCK) != 0) {
		fprintf(stderr, "failed to make socket non-blocking");
		freeaddrinfo(peer);
		close_socket(sock);
		return -1;
	}
	*/

	quiche_config* config = quiche_config_new(0xbabababa);
	if (config == NULL) {
		fprintf(stderr, "failed to create config\n");
		freeaddrinfo(peer);
		close_socket(sock);
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

	if (getenv("SSLKEYLOGFILE")) {
		quiche_config_log_keys(config);
	}

	// ABC: old config creation here
	uint8_t scid[LOCAL_CONN_ID_LEN];
	if (essentials::generate_random_data(scid, LOCAL_CONN_ID_LEN) < 0) {
		debug_print_error(__LOGTAG__, "generate_random_data failed. returning.");
		freeaddrinfo(peer);
		close_socket(sock);
		return -1;
	}

	/*
	int rng = open("/dev/urandom", O_RDONLY);
	if (rng < 0) {
		fprintf(stderr, "failed to open /dev/urandom");
		freeaddrinfo(peer);
		close_socket(sock);
		return -1;
	}

	ssize_t rand_len = read(rng, &scid, sizeof(scid));
	if (rand_len < 0) {
		close(rng);
		fprintf(stderr, "failed to create connection ID");
		freeaddrinfo(peer);
		close_socket(sock);
		return -1;
	}
	close(rng);
	*/

	GX_DELETE(conn_io);
	conn_io = DEBUG_NEW struct conn_io_qh3_client();
	if (conn_io == NULL) {
		fprintf(stderr, "failed to allocate connection IO\n");
		freeaddrinfo(peer);
		close_socket(sock);
		return -1;
	}

#if PLATFORM == PLATFORM_WINDOWS
	// in windows we need to bind the socket before calling 'getsockname'.
	struct sockaddr_in local_addr;
	int local_addr_len = sizeof(local_addr);
	memset(&local_addr, 0, sizeof(local_addr));
	local_addr.sin_family = AF_INET;
	local_addr.sin_port = htons(0);			  // Let the system pick an available port
	local_addr.sin_addr.s_addr = INADDR_ANY;  // Bind to any local address

	// Bind the socket
	if (bind(sock, (struct sockaddr*) &local_addr, sizeof(local_addr)) < 0) {
		fprintf(stderr, "bind failed");
		freeaddrinfo(peer);
		close_socket(sock);
		return -1;
	}
#endif

	conn_io->local_addr_len = sizeof(conn_io->local_addr);
	memset(&conn_io->local_addr, 0, sizeof(conn_io->local_addr));
	if (getsockname(sock, (struct sockaddr*) &conn_io->local_addr, &conn_io->local_addr_len) != 0) {
#if PLATFORM != PLATFORM_WINDOWS
		fprintf(stderr, "failed to get local address of socket - %s", strerror(errno));
#else
		fprintf(stderr, "failed to get local address of socket - error code: %d\n", WSAGetLastError());
#endif
		freeaddrinfo(peer);
		close_socket(sock);
		return -1;
	}

	quiche_conn* conn = quiche_connect(HOST.c_str(), (const uint8_t*) scid, sizeof(scid), (struct sockaddr*) &conn_io->local_addr, conn_io->local_addr_len, peer->ai_addr, peer->ai_addrlen, config);

	if (conn == NULL) {
		fprintf(stderr, "failed to create connection\n");
		freeaddrinfo(peer);
		close_socket(sock);
		return -1;
	}

#if LOG_LEVEL >= LOG_LEVEL_5
	char name[INET6_ADDRSTRLEN];
	char port[10];
	getnameinfo((struct sockaddr*) &conn_io->local_addr, sizeof(struct sockaddr), name, sizeof(name), port, sizeof(port), NI_NUMERICHOST | NI_NUMERICSERV);
	debug_print(LOG_LEVEL_0, __LOGTAG__, "local client address %s:%s", name, port);
    debug_print_scid(LOG_LEVEL_0, (const uint8_t*) scid, sizeof(scid));
#endif
	//    conn_io->two_byte_port_check = two_byte_port_check;
	conn_io->sock = sock;
	conn_io->conn = conn;
	conn_io->host = HOST.c_str();
	conn_io->bridge = this;

	// Initialize connection establishment timeout timer
	ev_timer_init(&conn_io->connection_establishment_timeout_timer, qh3client::connection_establishment_timeout_cb, connection_timeout, 0.0);  // 10 seconds timeout
	conn_io->connection_establishment_timeout_timer.data = this;																			   // Store pointer to the client object

	mainloop = ev_loop_new(0);

	ev_io watcher;

#if PLATFORM != PLATFORM_WINDOWS
	ev_io_init(&watcher, recv_cb, conn_io->sock, EV_READ);
#else
	int sock_fd = _open_osfhandle(conn_io->sock, 0);
	ev_io_init(&watcher, recv_cb, sock_fd, EV_READ);
#endif

	ev_io_start(mainloop, &watcher);
	watcher.data = this;

	ev_init(&conn_io->timer, timeout_cb);
	conn_io->timer.data = conn_io;
	conn_io->creation_time = ev_now(mainloop);

	flush_egress(mainloop, conn_io);

	// Start connection establishment timeout timer
	ev_timer_stop(mainloop, &conn_io->connection_establishment_timeout_timer);
	ev_timer_start(mainloop, &conn_io->connection_establishment_timeout_timer);
	//

	ev_loop(mainloop, 0);

	ev_timer_stop(mainloop, &conn_io->connection_establishment_timeout_timer);
	ev_timer_stop(mainloop, &conn_io->timer);
	ev_io_stop(mainloop, &watcher);
	ev_loop_destroy(mainloop);

	freeaddrinfo(peer);

	if (conn_io->http3) {
		quiche_h3_conn_free(conn_io->http3);
	}

	quiche_conn_free(conn);

	quiche_config_free(config);

	close_socket(sock);

	return 0;
}
