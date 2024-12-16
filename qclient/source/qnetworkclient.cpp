//
//  Copyright 2024 homenet25
//  qnetworkclient.cpp
//  networkclient
//
//  Created by Arun A on 12/10/23.
//

#include "qnetworkclient.hpp"

#if PLATFORM == PLATFORM_WINDOWS
#include <wincrypt.h>
#include <corecrt_io.h>
#endif


using client::conn_io_client;
using client::qnetworkclient;

int qnetworkclient::connection_id = 0;

// MARK: - QConnection
conn_io_client::conn_io_client(bridge_qcommand* bridge, int id) : id(id), bridge(bridge) {}

conn_io_client::conn_io_client(bridge_qcommand* bridge, quiche_config* config, int id) : id(id), bridge(bridge), config(config) {}

conn_io_client::~conn_io_client() {
	release();
}

void conn_io_client::release() {
	close_socket();
	if (peer) {
		freeaddrinfo(peer);
		peer = nullptr;
	}
	ev_timer_stop(bridge->getmainloop(), &timer);
	ev_timer_stop(bridge->getmainloop(), &send_timer);

	watcher.data = nullptr;
	ev_io_stop(bridge->getmainloop(), &watcher);
	ev_break(bridge->getmainloop(), EVBREAK_ONE);
	if (conn) {
		close_connection();
		quiche_conn_free(conn);
		conn = nullptr;
	}

	for (auto it = send_buffer.cbegin(); it != send_buffer.cend(); it++) {
		qdata* sd = *it;
		GX_DELETE(sd);
	}
	send_buffer.clear();
}

// Note: This function is not fully tested.
void conn_io_client::close_connection() {
	int con_active = connection_active();
	if (con_active == 0) {
		int close_result = quiche_conn_close(conn, true, 0, reinterpret_cast<const uint8_t*>("close"), strlen("close"));
		if (close_result < 0) {
			debug_print_error(__LOGTAG__, "f:close - failed to close connection %0x, err %d", cid_hash_val, close_result);
		} else {
			debug_print_important(__LOGTAG__, "f:close - closing... connection %0x", cid_hash_val);
		}
	}
}

int conn_io_client::close_socket() {
	if (sock < 0) {
		return -1;
	}
	int result = closesocket(sock);
	if (result < 0) {
		debug_print_error(__LOGTAG__, "Socket closure failed - fd(%d): %s", sock, strerror(errno));
	} else {
		sock = -1;
	}
	return result;
}

int conn_io_client::connect(qstring host, qstring port) {
	const struct addrinfo HINTS = {.ai_family = PF_UNSPEC, .ai_socktype = SOCK_DGRAM, .ai_protocol = IPPROTO_UDP};

	issue_close = false;

	if (peer) {
		freeaddrinfo(peer);
		peer = nullptr;
	}
	if (getaddrinfo(host.c_str(), port.c_str(), &HINTS, &peer) != 0) {
		debug_print_error(__LOGTAG__, "failed to resolve host");
		return -1;
	}

	sock = socket(peer->ai_family, SOCK_DGRAM, 0);
	if (sock < 0) {
		debug_print_error(__LOGTAG__, "failed to create socket");
		return -1;
	}

#if PLATFORM == PLATFORM_WINDOWS
	win_sock_fd = _open_osfhandle(sock, 0);
	if (win_sock_fd > FD_SETSIZE) {
		debug_print_error(__LOGTAG__, "sock fd LIMIT %d reached !!!, FD_SETSIZE(%d)", win_sock_fd, FD_SETSIZE);
		return -1;
	}
#else
	if (sock > FD_SETSIZE) {
		debug_print_error(__LOGTAG__, "sock fd LIMIT %d reached !!!, FD_SETSIZE(%d)", sock, FD_SETSIZE);
		return -1;
	}
#endif

	if (essentials::set_non_blocking(sock) != 0) {
		debug_print_error(__LOGTAG__, "failed to make socket non-blocking");
		return -1;
	}

	uint8_t scid[Q_LOCAL_CONN_ID_LEN];
	if (essentials::generate_random_data(scid, Q_LOCAL_CONN_ID_LEN) < 0) {
		debug_print_error(__LOGTAG__, "generate_random_data failed. returning.");
		return -1;
	}
	/*
	int rng = open("/dev/urandom", O_RDONLY);
	if (rng < 0) {
		debug_print_error(__LOGTAG__, "failed to open /dev/urandom");
		return -1;
	}

	ssize_t rand_len = read(rng, &scid, sizeof(scid));
	if (rand_len < 0) {
		close(rng);
		debug_print_error(__LOGTAG__, "failed to create connection ID");
		return -1;
	}
	close(rng);
	*/

#if PLATFORM == PLATFORM_WINDOWS
	// in windows we need to bind the socket before calling 'getsockname'.
	struct sockaddr_in tmp_local_addr;
	int tmp_local_addr_len = sizeof(tmp_local_addr);
	memset(&tmp_local_addr, 0, sizeof(tmp_local_addr));
	tmp_local_addr.sin_family = AF_INET;
	tmp_local_addr.sin_port = htons(0);		  // Let the system pick an available port
	tmp_local_addr.sin_addr.s_addr = INADDR_ANY;  // Bind to any local address

	// Bind the socket
	if (bind(sock, (struct sockaddr*) &tmp_local_addr, sizeof(tmp_local_addr)) < 0) {
		debug_print_error(__LOGTAG__, "bind socket failed");
		return -1;
	}
#endif

	local_addr_len = sizeof(local_addr);
	if (getsockname(sock, (struct sockaddr*) &local_addr, &local_addr_len) != 0) {
		debug_print_error(__LOGTAG__, "failed to get local address of socket");
		return -1;
	}

	HASH_VALUE(scid, Q_LOCAL_CONN_ID_LEN, cid_hash_val);

	conn = quiche_connect(host.c_str(), (const uint8_t*) scid, sizeof(scid), (struct sockaddr*) &local_addr, local_addr_len, peer->ai_addr, peer->ai_addrlen, config);
#if DEV_BUILD
	debug_print_scid(LOG_LEVEL_0, scid, sizeof(scid));
#endif
	if (conn == NULL) {
		debug_print_error(__LOGTAG__, "failed to create connection");
		return -1;
	}

	return 0;
}

int conn_io_client::connection_active() {
	if (!conn) {
		return -1;
	}
	if (!quiche_conn_is_established(conn)) {
		return -2;
	}
	if (quiche_conn_is_closed(conn)) {
		return -3;
	}
	if (quiche_conn_is_draining(conn)) {
		return -4;
	}
	return 0;
}

ssize_t conn_io_client::send_message(const qstring& buffer, bool fin) {
	return send_message(buffer.c_str(), buffer.length(), fin);
}

ssize_t conn_io_client::send_message(const char* buf, size_t buflen, bool fin) {
	int conn_active = connection_active();
	if (conn_active < 0) {
		debug_print(LOG_LEVEL_3, __LOGTAG__, "Cant send !!!, conn not active = %d", conn_active);
		return conn_active;
	}

	ssize_t result = -5;
	uint64_t s = 0;
	quiche_stream_iter* writable = quiche_conn_writable(conn);
	while (quiche_stream_iter_next(writable, &s)) {
		//        debug_print(LOG_LEVEL_0, __LOGTAG__, "stream %" PRIu64 " is writable", s);
		uint64_t err_code = 0;
		ssize_t sent_len = quiche_conn_stream_send(conn, s, reinterpret_cast<const uint8_t*>(buf), buflen, fin, &err_code);
		if (sent_len != (ssize_t) buflen) {
			debug_print_error(__LOGTAG__, "send failure %d", sent_len);
			break;
		}
		// debug_print_important(__LOGTAG__, "--------->>>>>>>>>>>[%d] %s", s, (char*)buf);
		result = sent_len;
		break;
	}
	quiche_stream_iter_free(writable);

	return result;
}

void qnetworkclient::setstate(con_state state) {
	if (this->state >= state) {
		debug_print_warn(__LOGTAG__, "QConnection state >= state, this->state %d, incoming state %d", this->state, state);
	}
	this->state = state;
}

void qnetworkclient::debug_log(const char* line, void* argp) {
	UNUSED(argp);
	debug_print(LOG_LEVEL_0, __LOGTAG__, "%s", line);
}

void qnetworkclient::reset_sendto_retry_timer() {
	ev_timer_stop(getmainloop(), &sendto_retry_timer);
	sendto_retry_timer.repeat = SENDTO_INITIAL_RETRY_INTERVAL;
	sendto_retry_count = 0;
}

void qnetworkclient::sendto_retry_cb(EV_P_ ev_timer* w, int revents) {
	qnetworkclient* client = reinterpret_cast<qnetworkclient*>(w->data);
	if (client == nullptr || client->qclient_connection == nullptr) {
		if (client) {
			client->reset_sendto_retry_timer();
		}
		debug_print_error(__LOGTAG__, "f:sendto_retry_cb - cancelling the re-send !!!");
		return;
	}

	if (client->sendto_retry_count >= MAX_SENDTO_RETRY_COUNT) {
		client->reset_sendto_retry_timer();
		debug_print_error(__LOGTAG__, "f:sendto_retry_cb - all retry failed to send through socket. check netowrk !!!");
		return;
	}

	conn_io_client* qconnection = client->qclient_connection;
	client->sendto_retry_count++;
	debug_print_important(__LOGTAG__, "f:sendto_retry_cb - retrying (%d) for connection id %d", client->sendto_retry_count, qconnection->id);
	ssize_t bytes_sent = 0;
	while (client->pending_socket_data_buffer.size()) {
		socket_data* data = client->pending_socket_data_buffer.front();
#if PLATFORM != PLATFORM_WINDOWS
		ssize_t sent = sendto(data->sockfd, data->buf, data->len, data->flags, data->dest_addr, data->addrlen);
#else
		ssize_t sent = sendto(data->sockfd, (const char*)data->buf, data->len, data->flags, data->dest_addr, data->addrlen);
#endif
		bytes_sent += sent;
		if (sent < 0) {
			client->sendto_retry_timer.repeat *= 1.25;
			ev_timer_again(client->getmainloop(), &client->sendto_retry_timer);
			return;
		}
		client->pending_socket_data_buffer.pop_front();
		GX_DELETE(data);
	}

	// stop the retry timer on success.
	client->reset_sendto_retry_timer();
	debug_warn_cond(__LOGTAG__, bytes_sent < 0, "f:sendto_retry_cb - total bytes sent %ld", bytes_sent);
}

ssize_t qnetworkclient::socket_sendto(int sockfd, const void* buf, size_t len, int flags, const struct sockaddr* dest_addr, socklen_t addrlen) {
#if PLATFORM != PLATFORM_WINDOWS
	ssize_t sent = sendto(sockfd, buf, len, flags, dest_addr, addrlen);
#else
	ssize_t sent = sendto(sockfd, (const char*)buf, len, flags, dest_addr, addrlen);
#endif
	if (sent < 0) {
		debug_print_error(__LOGTAG__, "f:socket_sendto - failed to send");
		socket_data* data = new socket_data(sockfd, buf, len, flags, dest_addr, addrlen);
		pending_socket_data_buffer.push_back(data);
		ev_timer_start(mainloop, &sendto_retry_timer);
		debug_print(LOG_LEVEL_0, __LOGTAG__, "f:socket_sendto - started retry timer.");
	}
	return sent;
}

ssize_t qnetworkclient::flushegress(struct ev_loop* loop, conn_io_client* qconnection) {
	ssize_t sent_bytes = 0;
	quiche_send_info send_info;
	while (true) {
		ssize_t written = quiche_conn_send(qconnection->conn, qconnection->egress_out, sizeof(qconnection->egress_out), &send_info);

		if (written == QUICHE_ERR_DONE) {
			DEBUG_PRINT2(LOG_LEVEL_5, __LOGTAG__, "f:flushegress - done writing");
			break;
		}

		if (written < 0) {
			debug_print_error(__LOGTAG__, "f:flushegress - failed to create packet: %zd", written);
			return sent_bytes;
		}

		ssize_t sent = socket_sendto(qconnection->sock, qconnection->egress_out, written, 0, (struct sockaddr*) &send_info.to, send_info.to_len);
		sent_bytes += sent;
		if (sent != written) {
			debug_print_error(__LOGTAG__, "f:flushegress - failed to send");
			return sent_bytes;
		}

#if LOG_LEVEL >= LOG_LEVEL_4
		unsigned long send_bytes_crc = crc32(0L, Z_NULL, 0);
		send_bytes_crc = essentials::mod_crc32_z(send_bytes_crc, reinterpret_cast<const uint8_t*>(qconnection->egress_out), sent);
		DEBUG_PRINT2(LOG_LEVEL_4, __LOGTAG__, "f:flushegress - sent %zd bytes - crc: %lx", sent, send_bytes_crc);
#endif
	}

	uint64_t timeout_in_nanos = quiche_conn_timeout_as_nanos(qconnection->conn);
	double t = static_cast<double>(timeout_in_nanos) / 1e9;
	qconnection->timer.repeat = t;
	ev_timer_again(loop, &qconnection->timer);
	debug_print(LOG_LEVEL_5, __LOGTAG__, "qconnection->timer.repeat %f - %" PRIu64 "", t, timeout_in_nanos);
	return sent_bytes;
}

void qnetworkclient::event_connect(conn_io_client* qconnection) {
	setstate(STATE_CONNECT);
	onconnect(qconnection);
}

void qnetworkclient::event_msg_received(ssize_t recv_len, uint8_t* buf, conn_io_client* qconnection, bool fin) {
	onmessage(recv_len, buf, qconnection);
}

void qnetworkclient::event_close(conn_io_client* qconnection) {
	if (isclosed()) {
		debug_print_error(__LOGTAG__, "Connection already closed, Event_Close returning...");
		return;
	}
	setstate(STATE_CLOSE);
	onclose(qconnection);
}

void qnetworkclient::forcerelease() {
	if (release_connection(getmainloop(), qclient_connection) == 0) {
		GX_DELETE(qclient_connection);
	}
}

int qnetworkclient::release_connection(struct ev_loop* loop, conn_io_client* qconnection) {
	UNUSED(loop);
	int ret_val = 0;
	if (qconnection != nullptr) {
		qconnection->release();
		onreleaseconnection(qconnection);
		GX_DELETE(qclient_connection);
		debug_print(LOG_LEVEL_3, __LOGTAG__, "Connection released !!!");
	} else {
		ret_val = -1;
		debug_print(LOG_LEVEL_3, __LOGTAG__, "Already destroyed.. Ignoring...");
	}
	return ret_val;
}

int qnetworkclient::close() {
#if USE_PTHREAD
	// lock
	DEBUG_ASSERT(__LOGTAG__, (close_mutex.try_lock(__FUNCTION__) == 0), __FUNCTION__);
	close_mutex.conditional_wait(__FUNCTION__);

	// block
	send_mutex.block(__FUNCTION__);
	sendloop_mutex.block(__FUNCTION__);
#endif

	if (state == con_state::STATE_CONNECT) {
		if (qclient_connection) {
			int con_active = qclient_connection->connection_active();
			if (con_active == 0) {
				const uint8_t BYE[] = "Bye\r\n";
				qclient_connection->send_buffer.push_back(DEBUG_NEW qdata(reinterpret_cast<const uint8_t*>(BYE), sizeof(BYE), true));
			}
		}
	}
//    if (qclient_connection) {
//        qclient_connection->issue_close = true;
//    }
#if USE_PTHREAD
	sendloop_mutex.unblock(__FUNCTION__);
	send_mutex.unblock(__FUNCTION__);
	close_mutex.unlock();
#endif
	return 0;
}

int qnetworkclient::send_message(const uint8_t* buffer, ssize_t size, bool flush) {
#if USE_PTHREAD
	// lock
	DEBUG_ASSERT(__LOGTAG__, (send_mutex.try_lock(__FUNCTION__) == 0), __FUNCTION__);
	send_mutex.conditional_wait(__FUNCTION__);

	// block
	close_mutex.block(__FUNCTION__);
	sendloop_mutex.block(__FUNCTION__);
#endif

	if (qclient_connection) {
		qclient_connection->send_buffer.push_back(DEBUG_NEW qdata(buffer, size));
	}

#if USE_PTHREAD
	sendloop_mutex.unblock(__FUNCTION__);
	close_mutex.unblock(__FUNCTION__);
	send_mutex.unlock();
#endif
	return 0;
}

int qnetworkclient::send_message(const qstring& buffer, bool flush) {
	return send_message(reinterpret_cast<const uint8_t*>(buffer.c_str()), buffer.length(), flush);
}

void qnetworkclient::recv_cb(EV_P_ ev_io* w, int revents) {
	UNUSED(revents);
	conn_io_client* qconnection = reinterpret_cast<conn_io_client*>(w->data);
	if (qconnection->conn == nullptr) {
		return;
	}

	while (true) {
		struct sockaddr_storage peer_addr;
		socklen_t peer_addr_len = sizeof(peer_addr);
		memset(&peer_addr, 0, peer_addr_len);

#if PLATFORM == PLATFORM_WINDOWS
		ssize_t read = recvfrom(qconnection->sock, (char*) qconnection->recv_buf, sizeof(qconnection->recv_buf), 0, (struct sockaddr*) &peer_addr, &peer_addr_len);
		if (read < 0) {
			int error = WSAGetLastError();
			if (error == WSAEWOULDBLOCK) {
				debug_print(LOG_LEVEL_5, __LOGTAG__, "recv would block");
				break;
			}
			perror("failed to read");
			return;
		}
#else
		ssize_t read = recvfrom(qconnection->sock, qconnection->recv_buf, sizeof(qconnection->recv_buf), 0, (struct sockaddr*) &peer_addr, &peer_addr_len);
		if (read < 0) {
			if ((errno == EWOULDBLOCK) || (errno == EAGAIN)) {
				debug_print(LOG_LEVEL_5, __LOGTAG__, "recv would block");
				break;
			}

			perror("failed to read");
			return;
		}
#endif
		quiche_recv_info recv_info = {
			(struct sockaddr*) &peer_addr,
			peer_addr_len,

			(struct sockaddr*) &qconnection->local_addr,
			qconnection->local_addr_len,
		};

		ssize_t done = quiche_conn_recv(qconnection->conn, qconnection->recv_buf, read, &recv_info);
		if (done < 0) {
			debug_print_error(__LOGTAG__, "failed to process packet");
			continue;
		}

		debug_print(LOG_LEVEL_4, __LOGTAG__, "recv %zd bytes", done);
	}

	debug_print(LOG_LEVEL_4, __LOGTAG__, "done reading");

	if (quiche_conn_is_established(qconnection->conn) && qconnection->bridge->getstate() == con_state::STATE_OPEN) {
		const uint8_t* app_proto;
		size_t app_proto_len;

		quiche_conn_application_proto(qconnection->conn, &app_proto, &app_proto_len);

		debug_print(LOG_LEVEL_3, __LOGTAG__, "connection established: %.*s", (int) app_proto_len, app_proto);
		qconnection->bridge->event_connect(qconnection);

		const static uint8_t HI[] = "{\"m\":\"hi\"}";
		uint64_t error_code = 0;
		if (quiche_conn_stream_send(qconnection->conn, 4, HI, sizeof(HI), false, &error_code) < 0) {
			debug_print_error(__LOGTAG__, "failed to send Hi request");
			return;
		}
		debug_print(LOG_LEVEL_3, __LOGTAG__, "sent Hi request");
	}

	if (quiche_conn_is_established(qconnection->conn)) {
		uint64_t s = 0;
		quiche_stream_iter* readable = quiche_conn_readable(qconnection->conn);
		while (quiche_stream_iter_next(readable, &s)) {
			debug_print(LOG_LEVEL_4, __LOGTAG__, "stream %" PRIu64 " is readable", s);

			bool fin = false;
			uint64_t error_code = 0;
			ssize_t recv_len = quiche_conn_stream_recv(qconnection->conn, s, qconnection->recv_buf, sizeof(qconnection->recv_buf), &fin, &error_code);
			if (recv_len < 0) {
				break;
			}

			if (fin) {
				int close_result = quiche_conn_close(qconnection->conn, true, 0, reinterpret_cast<const uint8_t*>("fin"), strlen("fin"));
				if (close_result < 0) {
					debug_print_error(__LOGTAG__, "failed to close connection, err %d", close_result);
				} else {
					debug_print(LOG_LEVEL_2, __LOGTAG__, "fin received, closing...");
				}
			}
			qconnection->fin_received = fin;
			qconnection->bridge->event_msg_received(recv_len, qconnection->recv_buf, qconnection, fin);
		}
		quiche_stream_iter_free(readable);
	}

	if (qconnection->conn) {
		ssize_t bytes_sent = qconnection->bridge->flushegress(loop, qconnection);
		debug_warn_cond(__LOGTAG__, bytes_sent < 0, "f:recv_cb - flushegress returned %ld", bytes_sent);
	}
}

void qnetworkclient::send_cb(EV_P_ ev_timer* w, int revents) {
	UNUSED(loop);
	UNUSED(revents);
	conn_io_client* qconnection = reinterpret_cast<conn_io_client*>(w->data);
#if USE_PTHREAD
	// lock
	DEBUG_ASSERT(__LOGTAG__, (qconnection->bridge->get_sendloop_mutex()->try_lock(__FUNCTION__) == 0), __FUNCTION__);
	qconnection->bridge->get_sendloop_mutex()->conditional_wait(__FUNCTION__);

	// block
	qconnection->bridge->get_close_mutex()->block(__FUNCTION__);
	qconnection->bridge->get_send_mutex()->block(__FUNCTION__);
#endif

	if (qconnection->bridge->getstate() == con_state::STATE_CONNECT) {
		std::vector<qdata*> successfully_sent;
		for (auto it = qconnection->send_buffer.cbegin(); it != qconnection->send_buffer.cend(); it++) {
			qdata* sd = *it;
			ssize_t send_res = qconnection->send_message((const char*) sd->data, sd->size, sd->fin);
			if (sd->size != send_res) {
				debug_print(LOG_LEVEL_3, __LOGTAG__, "send_cb failed for %.*s, err %d, fin %d, pending %d", sd->size, sd->data, send_res, qconnection->fin_received, qconnection->send_buffer.size());
			} else {
				successfully_sent.push_back(sd);
				ssize_t bytes_sent = qconnection->bridge->flushegress(qconnection->bridge->getmainloop(), qconnection);
				debug_warn_cond(__LOGTAG__, bytes_sent < 0, "f:send_cb - flushegress returned %ld", bytes_sent);
			}
		}

		for (auto it = successfully_sent.cbegin(); it != successfully_sent.cend(); it++) {
			qdata* fd = *it;
			size_t old_sz = qconnection->send_buffer.size();
			qconnection->send_buffer.erase(std::remove(qconnection->send_buffer.begin(), qconnection->send_buffer.end(), fd), qconnection->send_buffer.end());
			if (old_sz != qconnection->send_buffer.size()) {
				GX_DELETE(fd);
			}
		}
	}

	//    if (qconnection->issue_close) {
	//        ssize_t bytes_sent = qconnection->bridge->flushegress(qconnection->bridge->getmainloop(), qconnection);
	//        debug_warn_cond(__LOGTAG__, bytes_sent < 0, "f:send_cb:issue_close - flushegress returned %ld", bytes_sent);
	//        qconnection->close_connection();
	//    }

#if USE_PTHREAD
	qconnection->bridge->get_send_mutex()->unblock(__FUNCTION__);
	qconnection->bridge->get_close_mutex()->unblock(__FUNCTION__);
	qconnection->bridge->get_sendloop_mutex()->unlock();
#endif
}

void qnetworkclient::heart_beat_cb(EV_P_ ev_timer* w, int revents) {
	UNUSED(revents);
	qnetworkclient* client = reinterpret_cast<qnetworkclient*>(w->data);
	if (client == nullptr) {
		return;
	}
	conn_io_client* qconnection = client->qclient_connection;
	if (qconnection->conn == nullptr) {
		return;
	}
	if (qconnection->connection_active() != 0) {
		return;
	}
	client->send_message("hb", true);
}

void qnetworkclient::timeout_cb(EV_P_ ev_timer* w, int revents) {
	UNUSED(revents);
	conn_io_client* qconnection = reinterpret_cast<conn_io_client*>(w->data);
	if (qconnection->conn == nullptr) {
		ev_break(EV_A_ EVBREAK_ONE);
		return;
	}

	DEBUG_PRINT2(LOG_LEVEL_5, __LOGTAG__, "timeout - %lx", qconnection->cid_hash_val);

	quiche_conn_on_timeout(qconnection->conn);
	ssize_t bytes_sent = qconnection->bridge->flushegress(loop, qconnection);
	debug_warn_cond(__LOGTAG__, bytes_sent < 0, "f:timeout_cb - flushegress returned %ld", bytes_sent);

	if (quiche_conn_is_closed(qconnection->conn)) {
		quiche_stats stats;
		quiche_path_stats path_stats;

		quiche_conn_stats(qconnection->conn, &stats);
		quiche_conn_path_stats(qconnection->conn, 0, &path_stats);

		debug_print(LOG_LEVEL_4, __LOGTAG__, "connection closed, recv=%zu sent=%zu lost=%zu rtt=%" PRIu64 "ns", stats.recv, stats.sent, stats.lost, path_stats.rtt);
		qconnection->bridge->event_close(qconnection);
		ev_break(EV_A_ EVBREAK_ONE);
		return;
	} else {
		if (quiche_conn_is_established(qconnection->conn)) {
			debug_print(LOG_LEVEL_3, __LOGTAG__, "connection not closed");
		}
	}
}

void qnetworkclient::onconnect(conn_io_client* qconnection) {
	debug_print_important(__LOGTAG__, "########## CONNECTED ########## - %d", qconnection->id);
}

void qnetworkclient::onclose(conn_io_client* qconnection) {
	debug_print_important(__LOGTAG__, "########## CLOSED ########## - %d", qconnection->id);
}

void qnetworkclient::onmessage(ssize_t recv_len, uint8_t* buf, conn_io_client* qconnection) {
	UNUSED(qconnection);
	uint8_t* copybuf = DEBUG_NEW uint8_t[recv_len + 1];
	memcpy(copybuf, buf, recv_len);
	copybuf[recv_len] = '\0';
	debug_print_important2(__LOGTAG__, "<<<<< %s [len:%d]", copybuf, recv_len);
	GX_DELETE_ARY(copybuf);
}

void qnetworkclient::onreleaseconnection(conn_io_client* qconnection) {
	UNUSED(qconnection);
	debug_print(LOG_LEVEL_3, __LOGTAG__, "Connection about to release !!!");
}

qnetworkclient::qnetworkclient() {
#if USE_PTHREAD
	DEBUG_ASSERT(__LOGTAG__, (run_mutex.init("run") == 0), "qnetworkclient Constructor - CHECK !!!");
	DEBUG_ASSERT(__LOGTAG__, (send_mutex.init("send") == 0), "qnetworkclient Constructor - CHECK !!!");
	DEBUG_ASSERT(__LOGTAG__, (sendloop_mutex.init("sendLoop") == 0), "qnetworkclient Constructor - CHECK !!!");
	DEBUG_ASSERT(__LOGTAG__, (close_mutex.init("close") == 0), "qnetworkclient Constructor - CHECK !!!");
	DEBUG_ASSERT(__LOGTAG__, (runconfig_mutex.init("run_config_data") == 0), "qnetworkclient Constructor - CHECK !!!");
#endif
	debug_print(LOG_LEVEL_3, __LOGTAG__, "qnetworkclient created !!!");
}

qnetworkclient::~qnetworkclient() {
	release_connection(mainloop, qclient_connection);
	debug_print(LOG_LEVEL_3, __LOGTAG__, "qnetworkclient destroyed !!!");
}

void qnetworkclient::destroy_pending_socket_data() {
	while (pending_socket_data_buffer.size()) {
		socket_data* data = pending_socket_data_buffer.front();
		pending_socket_data_buffer.pop_front();
		GX_DELETE(data);
	}
}

void* qnetworkclient::run_internal(void* data) {
	run_config* run_config_data = reinterpret_cast<run_config*>(data);
	qstring host = run_config_data->host;
	qstring port = run_config_data->port;
	qnetworkclient* thiz = run_config_data->thiz;
#if USE_PTHREAD
	if (thiz->run_mutex.try_lock(__FUNCTION__) != 0) {
		run_config_data->finished = true;
		run_config_data->pthread_return_value = -1;
		pthread_exit(&run_config_data->pthread_return_value);
	}
#endif

#if ENABLE_QUICHE_LOG
	quiche_enable_debug_logging(debug_log, nullptr);
#endif

	quiche_config* config = quiche_config_new(0xbabababa);
	if (config == NULL) {
		debug_print_error(__LOGTAG__, "failed to create config");
		run_config_data->pthread_return_value = -1;
		run_config_data->finished = true;
#if USE_PTHREAD
		DEBUG_ASSERT(__LOGTAG__, (thiz->run_mutex.unlock() == 0), "CHECK !!!");
		pthread_exit(&run_config_data->pthread_return_value);
#else
		return nullptr;
#endif
	}

	quiche_config_set_application_protos(config, reinterpret_cast<const uint8_t*>("\x0ahq-interop\x05hq-29\x05hq-28\x05hq-27\x08http/0.9"), 38);

	quiche_config_set_max_idle_timeout(config, CLIENT_IDLE_TIMEOUT_MS);
	quiche_config_set_max_recv_udp_payload_size(config, Q_MAX_DATAGRAM_SIZE);
	quiche_config_set_max_send_udp_payload_size(config, Q_MAX_DATAGRAM_SIZE);
	quiche_config_set_initial_max_data(config, 10000000);
	quiche_config_set_initial_max_stream_data_bidi_local(config, 1000000);
	quiche_config_set_initial_max_stream_data_uni(config, 1000000);
	//    quiche_config_set_initial_max_stream_data_bidi_remote(config, 1000000);
	quiche_config_set_initial_max_streams_bidi(config, 100);
	quiche_config_set_initial_max_streams_uni(config, 100);
	quiche_config_set_disable_active_migration(config, true);

	if (getenv("SSLKEYLOGFILE")) {
		quiche_config_log_keys(config);
	}

#if USE_PTHREAD
	thiz->sendloop_mutex.block(__FUNCTION__);
	thiz->close_mutex.block(__FUNCTION__);
	thiz->send_mutex.block(__FUNCTION__);
#endif

	thiz->qclient_connection = DEBUG_NEW conn_io_client(thiz, config, run_config_data->id);
	if (thiz->qclient_connection == NULL) {
		debug_print_error(__LOGTAG__, "failed to create qconnection");
		run_config_data->pthread_return_value = -1;
		run_config_data->finished = true;
#if USE_PTHREAD
		DEBUG_ASSERT(__LOGTAG__, (thiz->run_mutex.unlock() == 0), "CHECK !!!");
		pthread_exit(&run_config_data->pthread_return_value);
#else
		return nullptr;
#endif
	}
	int connection_result = thiz->qclient_connection->connect(host, port);
	if (connection_result != 0) {
		debug_print_error(__LOGTAG__, "failed to connect qconnection");
		run_config_data->pthread_return_value = -1;
		run_config_data->finished = true;
#if USE_PTHREAD
		DEBUG_ASSERT(__LOGTAG__, (thiz->run_mutex.unlock() == 0), "CHECK !!!");
		pthread_exit(&run_config_data->pthread_return_value);
#else
		return nullptr;
#endif
	}

	thiz->mainloop = ev_loop_new(0);

#if PLATFORM != PLATFORM_WINDOWS
	ev_io_init(&thiz->qclient_connection->watcher, recv_cb, thiz->qclient_connection->sock, EV_READ);
#else
	ev_io_init(&thiz->qclient_connection->watcher, recv_cb, thiz->qclient_connection->win_sock_fd, EV_READ);
#endif

	ev_io_start(thiz->mainloop, &thiz->qclient_connection->watcher);
	thiz->qclient_connection->watcher.data = thiz->qclient_connection;

	ev_init(&thiz->qclient_connection->timer, timeout_cb);
	thiz->qclient_connection->timer.data = thiz->qclient_connection;

	ev_init(&thiz->qclient_connection->send_timer, send_cb);
	thiz->qclient_connection->send_timer.data = thiz->qclient_connection;
	thiz->qclient_connection->send_timer.repeat = SEND_INTERVAL;
	ev_timer_again(thiz->mainloop, &thiz->qclient_connection->send_timer);
	//    ev_timer_start(thiz->mainloop, &thiz->qclientConnection->send_timer);

	ev_timer_init(&thiz->sendto_retry_timer, sendto_retry_cb, 1, SENDTO_INITIAL_RETRY_INTERVAL);
	thiz->sendto_retry_timer.data = thiz;

	ev_init(&thiz->heart_beat_timer, heart_beat_cb);
	thiz->heart_beat_timer.data = thiz;
	thiz->heart_beat_timer.repeat = HEARTBEAT_INTERVAL;
	ev_timer_again(thiz->mainloop, &thiz->heart_beat_timer);

	ssize_t bytes_sent = thiz->flushegress(thiz->mainloop, thiz->qclient_connection);
	debug_warn_cond(__LOGTAG__, bytes_sent < 0, "f:run_internal - flushegress returned %ld", bytes_sent);

#if USE_PTHREAD
	thiz->send_mutex.unblock(__FUNCTION__);
	thiz->close_mutex.unblock(__FUNCTION__);
	thiz->sendloop_mutex.unblock(__FUNCTION__);
#endif
	ev_loop(thiz->mainloop, 0);

	ev_timer_stop(thiz->mainloop, &thiz->heart_beat_timer);

	ev_timer_stop(thiz->mainloop, &thiz->sendto_retry_timer);

	debug_print(LOG_LEVEL_3, __LOGTAG__, "run_internal loop released !!!");
	thiz->release_connection(thiz->mainloop, thiz->qclient_connection);

	thiz->destroy_pending_socket_data();

	ev_loop_destroy(thiz->mainloop);

	quiche_config_free(config);

#if USE_PTHREAD
	DEBUG_ASSERT(__LOGTAG__, (thiz->get_runconfig_mutex().try_lock(__FUNCTION__) == 0), __FUNCTION__);
	run_config_data->pthread_return_value = 0;
	run_config_data->finished = true;
	DEBUG_ASSERT(__LOGTAG__, (thiz->get_runconfig_mutex().unlock() == 0), __FUNCTION__);
	DEBUG_ASSERT(__LOGTAG__, (thiz->run_mutex.unlock() == 0), "CHECK !!!");
	pthread_exit(0);
#else
	run_config_data->pthread_return_value = 0;
	run_config_data->finished = true;
	return nullptr;
#endif
	return nullptr;
}

bool qnetworkclient::is_runfinished() {
#if USE_PTHREAD
	DEBUG_ASSERT(__LOGTAG__, (runconfig_mutex.try_lock(__FUNCTION__) == 0), __FUNCTION__);
	bool ret_val = run_config_data.finished;
	DEBUG_ASSERT(__LOGTAG__, (runconfig_mutex.unlock() == 0), __FUNCTION__);
#else
	bool ret_val = run_config_data.finished;
#endif
	return ret_val;
}

int qnetworkclient::run(qstring host, qstring port) {
#if USE_PTHREAD
	DEBUG_ASSERT(__LOGTAG__, (runconfig_mutex.try_lock(__FUNCTION__) == 0), __FUNCTION__);
#endif
	run_config_data.host = host;
	run_config_data.port = port;
	run_config_data.thiz = this;
	run_config_data.finished = false;
	run_config_data.id = qnetworkclient::connection_id++;
#if USE_PTHREAD
	DEBUG_ASSERT(__LOGTAG__, (runconfig_mutex.unlock() == 0), __FUNCTION__);
	if (pthread_create(&run_thread_id, nullptr, qnetworkclient::run_internal, reinterpret_cast<void*>(&run_config_data)) < 0) {
		debug_print_error(__LOGTAG__, "could not create thread: %s - %d", strerror(errno), errno);
		return -1;
	}
#else
	qnetworkclient::run_internal(reinterpret_cast<void*>(&run_config_data));
#endif
	return 0;
}
