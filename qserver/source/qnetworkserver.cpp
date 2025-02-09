//
//  Copyright 2024 homenet25
//  qnetworkserver.cpp
//  NetworkServer
//
//  Created by Arun A on 12/10/23.
//

#include "qnetworkserver.hpp"

#include <errno.h>
#include <ev.h>
#include <fcntl.h>
#include <inttypes.h>
#include <netdb.h>
#include <quiche.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <uthash.h>

#define QSERVER_EVENT_ERROR(thiz, observer, error_code)  \
	do {                                                 \
		if (observer && thiz) {                          \
			observer->on_server_error(thiz, error_code); \
		}                                                \
	} while (0)

#define QSERVER_EVENT_PRE_START(thiz, observer)  \
	do {                                         \
		if (observer && thiz) {                  \
			observer->on_server_pre_start(thiz); \
		}                                        \
	} while (0)

#define QSERVER_EVENT_START(thiz, observer, ip, port)  \
	do {                                               \
		if (observer && thiz) {                        \
			observer->on_server_start(thiz, ip, port); \
		}                                              \
	} while (0)

#define QSERVER_EVENT_STOP(thiz, observer)  \
	do {                                    \
		if (observer && thiz) {             \
			observer->on_server_stop(thiz); \
		}                                   \
	} while (0)

// MARK: - conn_io
int qnetworkserver::run_id = 0;

qconn_io::qconn_io(bridge_qpeerconnection* bridge, uint8_t* scid, size_t scid_len, int sock) : bridge(bridge), sock(sock) {
	if (scid_len != Q_LOCAL_CONN_ID_LEN) {
		debug_print_warn(__LOGTAG__, "failed, scid length too short");
	}
	memcpy(cid, scid, Q_LOCAL_CONN_ID_LEN);
	HASH_VALUE(cid, Q_LOCAL_CONN_ID_LEN, cid_hash_val);
	last_heartbeat_time = ev_now(bridge->get_mainloop());
	connection_start_time = ev_now(bridge->get_mainloop());
}

qconn_io::~qconn_io() {
	ev_timer_stop(bridge->get_mainloop(), &timer);
	if (conn) {
		quiche_conn_free(conn);
		conn = nullptr;
	}
}

void qconn_io::sendmessage(const qstring& buffer, bool flush) {
	sendmessage(buffer.c_str(), buffer.length(), flush);
}

void qconn_io::sendmessage(const char* buf, size_t buflen, bool flush) {
	if (!quiche_conn_is_established(conn)) {
		debug_print_important(__LOGTAG__, "Cant send !!!, connection not established - %s", (char*) buf);
		return;
	}
	bool success = false;
	uint64_t s = 0;
	quiche_stream_iter* writable = quiche_conn_writable(conn);
	while (quiche_stream_iter_next(writable, &s)) {
		if (last_stream_s == s) {
			continue;
		}
		debug_print(LOG_LEVEL_3, __LOGTAG__, "stream %" PRIu64 " is writable", s);
		uint64_t out_error_code = 0;
		ssize_t sent_len = quiche_conn_stream_send(conn, s, reinterpret_cast<const uint8_t*>(buf), buflen, false, &out_error_code);
		if (sent_len != (ssize_t) buflen) {
			debug_print_error(__LOGTAG__, "send failure %d", sent_len);
			break;
		}
		success = true;
		last_stream_s = s;
		debug_print(LOG_LEVEL_3, __LOGTAG__, "--------->>>>>>>>>>>[%d] %s", s, (char*) buf);
		break;
	}

	const uint64_t MAX_SEND_STREAM_TO_TRY = 200;
	uint64_t next_s = last_stream_s;
	while (!success && next_s < MAX_SEND_STREAM_TO_TRY) {
		next_s = (next_s + 1) + (next_s % 2);
		uint64_t out_error_code = 0;
		ssize_t sent_len = quiche_conn_stream_send(conn, next_s, reinterpret_cast<const uint8_t*>(buf), buflen, false, &out_error_code);
		if (sent_len == (ssize_t) buflen) {
			debug_print(LOG_LEVEL_3, __LOGTAG__, "--------->>>>>>>>>>>[%d] %s", next_s, (char*) buf);
			last_stream_s = next_s;
			success = true;
		}
	}

	if (!success && next_s >= MAX_SEND_STREAM_TO_TRY) {
		debug_print_error(__LOGTAG__, "Send %s FAILED even after %d tries. Streams not available to send !!!", (char*) buf, next_s);
	}
	quiche_stream_iter_free(writable);
	if (flush) {
		bridge->flush_egress(bridge->get_mainloop(), this);
	}
}

void qconn_io::close() {
	if (conn == nullptr || !quiche_conn_is_established(conn)) {
		debug_print_important(__LOGTAG__, "f:close - Cant close !!!, connection not established. - connection %0x", cid_hash_val);
		return;
	}
	uint64_t s = 0;
	quiche_stream_iter* writable = quiche_conn_writable(conn);
	while (quiche_stream_iter_next(writable, &s)) {
		debug_print(LOG_LEVEL_3, __LOGTAG__, "f:close - stream %" PRIu64 " is writable - connection %0x", s, cid_hash_val);
		const uint8_t BYEZ[] = "byez";
		uint64_t out_error_code = 0;
		ssize_t bye_sent_len = quiche_conn_stream_send(conn, s, BYEZ, sizeof(BYEZ), true, &out_error_code);
		debug_print(LOG_LEVEL_3, __LOGTAG__, "f:close - sending 'byez' - connection %0x", cid_hash_val);
		if (bye_sent_len != 5) {
			debug_print_error(__LOGTAG__, "f:close - sending 'byez' failed !!! - connection %0x", cid_hash_val);
		}
		debug_print(LOG_LEVEL_3, __LOGTAG__, "--------->>>>>>>>>>>[%d] %s", s, (char*) BYEZ);
		break;
	}
	quiche_stream_iter_free(writable);
	bridge->flush_egress(bridge->get_mainloop(), this);

	// closing the connection
	if (!quiche_conn_is_closed(conn) && !quiche_conn_is_draining(conn)) {
		int close_result = quiche_conn_close(conn, true, 0, nullptr, 0);
		if (close_result < 0) {
			debug_print_error(__LOGTAG__, "f:close - failed to close connection %0x, err %d", cid_hash_val, close_result);
		} else {
			debug_print_important(__LOGTAG__, "f:close - closing... connection %0x", cid_hash_val);
		}
	}
}

// MARK: - qnetworkserver
qnetworkserver::~qnetworkserver() {
	debug_print_important2(__LOGTAG__, "qnetworkserver destroyed %s:%s !!!", host_id.c_str(), port_id.c_str());
}

void qnetworkserver::debug_quiche_log(const char* line, void* argp) {
	qnetworkserver* server = reinterpret_cast<qnetworkserver*>(argp);
	if (server != nullptr && server->is_log_quiche()) {
		debug_print(LOG_LEVEL_0, __LOGTAG__, line);
	}
}

void qnetworkserver::mint_token(const uint8_t* dcid, size_t dcid_len, struct sockaddr_storage* addr, socklen_t addr_len, uint8_t* token, size_t* token_len) {
	memcpy(token, "quiche", sizeof("quiche") - 1);
	memcpy(token + sizeof("quiche") - 1, addr, addr_len);
	memcpy(token + sizeof("quiche") - 1 + addr_len, dcid, dcid_len);

	*token_len = sizeof("quiche") - 1 + addr_len + dcid_len;
}

bool qnetworkserver::validate_token(const uint8_t* token, size_t token_len, struct sockaddr_storage* addr, socklen_t addr_len, uint8_t* odcid, size_t* odcid_len) {
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

uint8_t* qnetworkserver::gen_cid(uint8_t* cid, size_t cid_len) {
	int rng = open("/dev/urandom", O_RDONLY);
	if (rng < 0) {
		debug_print_error(__LOGTAG__, "failed to open /dev/urandom");
		return nullptr;
	}

	ssize_t rand_len = read(rng, cid, cid_len);
	if (rand_len < 0) {
		debug_print_error(__LOGTAG__, "failed to create connection ID");
		close(rng);
		return nullptr;
	}

	close(rng);
	return cid;
}

qconn_io* qnetworkserver::create_conn(uint8_t* scid, size_t scid_len, uint8_t* odcid, size_t odcid_len, struct sockaddr* local_addr, socklen_t local_addr_len, struct sockaddr_storage* peer_addr, socklen_t peer_addr_len) {
	qconn_io* qconnection = DEBUG_NEW qconn_io(this, scid, scid_len, conns->sock);
	if (qconnection == nullptr) {
		debug_print_error(__LOGTAG__, "failed to allocate qconnection");
		return nullptr;
	}

	quiche_conn* conn = quiche_accept(qconnection->cid, Q_LOCAL_CONN_ID_LEN, odcid, odcid_len, local_addr, local_addr_len, (struct sockaddr*) peer_addr, peer_addr_len, config);
	if (conn == nullptr) {
		debug_print_error(__LOGTAG__, "failed to create connection");
		GX_DELETE(qconnection);
		return nullptr;
	}

	qconnection->conn = conn;
	memcpy(&qconnection->peer_addr, peer_addr, peer_addr_len);
	qconnection->peer_addr_len = peer_addr_len;
	ev_init(&qconnection->timer, timeout_cb);
	qconnection->timer.data = qconnection;
	HASH_ADD(hh, conns->h, cid, Q_LOCAL_CONN_ID_LEN, qconnection);
	qconnection->bridge->onconnection_connect(qconnection);
	Q_INFO(__LOGTAG__, "new connection %0x", qconnection->cid_hash_val);
	return qconnection;
}

void qnetworkserver::onconnection_connect(qconn_io* qconnection) {
	UNUSED(qconnection);
	debug_print_important(__LOGTAG__, "<<<<< new connection");
}
void qnetworkserver::onconnection_connected(qconn_io* qconnection) {
	UNUSED(qconnection);
	debug_print_important(__LOGTAG__, "connection established");
}

void qnetworkserver::onconnection_message(ssize_t recv_len, uint8_t* buf, qconn_io* qconnection) {
#if DEV_BUILD && 0
	char hbuf[NI_MAXHOST], sbuf[NI_MAXSERV];
	uint8_t* copybuf = DEBUG_NEW uint8_t[recv_len + 1];
	memcpy(copybuf, buf, recv_len);
	copybuf[recv_len] = '\0';
	if (getnameinfo((struct sockaddr*) &qconnection->peer_addr, qconnection->peer_addr_len, hbuf, sizeof(hbuf), sbuf, sizeof(sbuf), NI_NUMERICHOST | NI_NUMERICSERV) == 0) {
		debug_print_important2(__LOGTAG__, "<<<<< %s [len %d], host : %s, serv : %s", copybuf, recv_len, hbuf, sbuf);
	} else {
		debug_print_important2(__LOGTAG__, "<<<<< %s [len %d]", copybuf, recv_len);
	}
	GX_DELETE_ARY(copybuf);

	// TODO(amudaliar) : Comment this for development.
	/*
	qstring ss = qstring::format_string("HELLO from server-%d", qconnection->itrmsg++);
	qconnection->sendmessage(ss, true);
	*/
#endif
}

void qnetworkserver::flush_egress(struct ev_loop* loop, qconn_io* qconnection) {
	quiche_send_info send_info;
	while (true) {
		ssize_t written = quiche_conn_send(qconnection->conn, qconnection->egress_out, sizeof(qconnection->egress_out), &send_info);
		if (written == QUICHE_ERR_DONE) {
			DEBUG_PRINT2(LOG_LEVEL_5, __LOGTAG__, "done writing");
			break;
		}
		if (written < 0) {
			debug_print_error(__LOGTAG__, "failed to create packet: %zd", written);
			return;
		}

		ssize_t sent = sendto(qconnection->sock, qconnection->egress_out, written, 0, (struct sockaddr*) &send_info.to, send_info.to_len);
		if (sent != written) {
			debug_print_error(__LOGTAG__, "f:flush_egress - failed to send");
			return;
		}

#if LOG_LEVEL >= LOG_LEVEL_4
		unsigned long send_bytes_crc = crc32(0L, Z_NULL, 0);
		send_bytes_crc = essentials::mod_crc32_z(send_bytes_crc, reinterpret_cast<const uint8_t*>(qconnection->egress_out), sent);
		DEBUG_PRINT2(LOG_LEVEL_4, __LOGTAG__, "sent %zd bytes - crc: %lx", sent, send_bytes_crc);
#endif
	}

	uint64_t timeout_in_nanos = quiche_conn_timeout_as_nanos(qconnection->conn);
	double t = static_cast<double>(timeout_in_nanos) / 1e9;
	qconnection->timer.repeat = t < 0.00001f ? 1.0f : t;
	ev_timer_again(loop, &qconnection->timer);
	debug_print(LOG_LEVEL_5, __LOGTAG__, "qconnection->timer.repeat %f - %" PRIu64 "", t, timeout_in_nanos);
}

void qnetworkserver::close_connection(qconn_io* qconnection) {
	qconnection->close();
}

void qnetworkserver::destroy_connection(struct ev_loop* loop, qconn_io* qconnection) {
	UNUSED(loop);
	close_connection(qconnection);
	onconnection_destroy(qconnection);
	HASH_DELETE(hh, conns->h, qconnection);
	Q_INFO(__LOGTAG__, "destroy connection %0x", qconnection->cid_hash_val);
	GX_DELETE(qconnection);
	debug_print_important(__LOGTAG__, "connection destroyed [pending %d]!!!", HASH_CNT(hh, conns->h));
}

void qnetworkserver::onconnection_destroy(qconn_io* qconnection) {
	UNUSED(qconnection);
	//    debug_print_important(__LOGTAG__, "Connection about to destroy !!!");
}

void qnetworkserver::timeout_cb(EV_P_ ev_timer* w, int revents) {
	UNUSED(revents);
	qconn_io* qconnection = reinterpret_cast<qconn_io*>(w->data);
	DEBUG_PRINT2(LOG_LEVEL_5, __LOGTAG__, "timeout - %lx", qconnection->cid_hash_val);
	quiche_conn_on_timeout(qconnection->conn);
	qconnection->bridge->flush_egress(loop, qconnection);

	if (quiche_conn_is_closed(qconnection->conn)) {
		quiche_stats stats;
		quiche_path_stats path_stats;

		quiche_conn_stats(qconnection->conn, &stats);
		quiche_conn_path_stats(qconnection->conn, 0, &path_stats);

		debug_print(LOG_LEVEL_4, __LOGTAG__, "connection closed, recv=%zu sent=%zu lost=%zu rtt=%" PRIu64 "ns cwnd=%zu\n", stats.recv, stats.sent, stats.lost, path_stats.rtt, path_stats.cwnd);

		qconnection->bridge->destroy_connection(loop, qconnection);
		return;
	} /*else {
		// force close here
		debug_print_important(__LOGTAG__, "Force close connection !!!");
		qconnection->bridge->DestroyConnection(loop, qconnection);
		return;
	}*/
}

void qnetworkserver::recv_cb_internal(EV_P_ ev_io* w, int revents) {
	UNUSED(w);
	UNUSED(revents);
	qconn_io* qconnection = nullptr;
	qconn_io* tmp = nullptr;

	while (true) {
		struct sockaddr_storage peer_addr;
		socklen_t peer_addr_len = sizeof(peer_addr);
		memset(&peer_addr, 0, peer_addr_len);

		ssize_t read = recvfrom(conns->sock, conns->buf, sizeof(conns->buf), 0, (struct sockaddr*) &peer_addr, &peer_addr_len);

		if (read < 0) {
			if ((errno == EWOULDBLOCK) || (errno == EAGAIN)) {
				debug_print(LOG_LEVEL_6, __LOGTAG__, "recv would block");
				break;
			}

			debug_print_error(__LOGTAG__, "failed to read");
			return;
		}

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

		int rc = quiche_header_info(conns->buf, read, Q_LOCAL_CONN_ID_LEN, &version, &type, scid, &scid_len, dcid, &dcid_len, token, &token_len);
		if (rc < 0) {
			debug_print_error(__LOGTAG__, "failed to parse header: %d", rc);
			continue;
		}

		HASH_FIND(hh, conns->h, dcid, dcid_len, qconnection);

		if (qconnection == nullptr) {
			if (!quiche_version_is_supported(version)) {
				debug_print(LOG_LEVEL_4, __LOGTAG__, "version negotiation");

				ssize_t written = quiche_negotiate_version(scid, scid_len, dcid, dcid_len, conns->out, sizeof(conns->out));

				if (written < 0) {
					debug_print_warn(__LOGTAG__, "failed to create vneg packet: %zd", written);
					continue;
				}

				ssize_t sent = sendto(conns->sock, conns->out, written, 0, (struct sockaddr*) &peer_addr, peer_addr_len);
				if (sent != written) {
					debug_print_error(__LOGTAG__, "version negotiation: failed to send");
					continue;
				}

#if LOG_LEVEL >= LOG_LEVEL_4
				unsigned long send_bytes_crc = crc32(0L, Z_NULL, 0);
				send_bytes_crc = essentials::mod_crc32_z(send_bytes_crc, reinterpret_cast<const uint8_t*>(conns->out), sent);
				DEBUG_PRINT2(LOG_LEVEL_4, __LOGTAG__, "sent %zd bytes - crc: %lx", sent, send_bytes_crc);
#endif
				continue;
			}

			if (token_len == 0) {
				debug_print(LOG_LEVEL_4, __LOGTAG__, "stateless retry");

				mint_token(dcid, dcid_len, &peer_addr, peer_addr_len, token, &token_len);

				uint8_t new_cid[Q_LOCAL_CONN_ID_LEN];

				if (gen_cid(new_cid, Q_LOCAL_CONN_ID_LEN) == nullptr) {
					continue;
				}

				ssize_t written = quiche_retry(scid, scid_len, dcid, dcid_len, new_cid, Q_LOCAL_CONN_ID_LEN, token, token_len, version, conns->out, sizeof(conns->out));

				if (written < 0) {
					debug_print_warn(__LOGTAG__, "stateless retry: failed to create retry packet: %zd", written);
					continue;
				}

				ssize_t sent = sendto(conns->sock, conns->out, written, 0, (struct sockaddr*) &peer_addr, peer_addr_len);
				if (sent != written) {
					debug_print_error(__LOGTAG__, "stateless retry: failed to send");
					continue;
				}

#if LOG_LEVEL >= LOG_LEVEL_4
				unsigned long send_bytes_crc = crc32(0L, Z_NULL, 0);
				send_bytes_crc = essentials::mod_crc32_z(send_bytes_crc, reinterpret_cast<const uint8_t*>(conns->out), sent);
				DEBUG_PRINT2(LOG_LEVEL_4, __LOGTAG__, "sent %zd bytes - crc: %lx", sent, send_bytes_crc);
#endif
				continue;
			}

			if (!validate_token(token, token_len, &peer_addr, peer_addr_len, odcid, &odcid_len)) {
				debug_print_warn(__LOGTAG__, "invalid address validation token");
				continue;
			}

#if DEV_BUILD && 0
			debug_print_scid(LOG_LEVEL_0, scid, scid_len);
			debug_print_hexadecimal_string(LOG_LEVEL_0, "dcid", dcid, dcid_len);
			debug_print_hexadecimal_string(LOG_LEVEL_0, "Token:", token, token_len);
#endif
			qconnection = create_conn(dcid, dcid_len, odcid, odcid_len, conns->local_addr, conns->local_addr_len, &peer_addr, peer_addr_len);

			if (qconnection == nullptr) {
				continue;
			}
		}

		quiche_recv_info recv_info = {
			(struct sockaddr*) &peer_addr,
			peer_addr_len,

			conns->local_addr,
			conns->local_addr_len,
		};

		ssize_t done = quiche_conn_recv(qconnection->conn, conns->buf, read, &recv_info);

		if (done < 0) {
			debug_print_error(__LOGTAG__, "failed to process packet: %zd", done);
			continue;
		}

#if LOG_LEVEL >= LOG_LEVEL_4
		unsigned long recv_bytes_crc = crc32(0L, Z_NULL, 0);
		recv_bytes_crc = essentials::mod_crc32_z(recv_bytes_crc, reinterpret_cast<const uint8_t*>(conns->buf), done);
		DEBUG_PRINT2(LOG_LEVEL_4, __LOGTAG__, "q-recv %zd bytes - crc: %lx", done, recv_bytes_crc);
#endif

		if (quiche_conn_is_established(qconnection->conn)) {
			if (!qconnection->connection_established) {
				qconnection->connection_established = true;
				Q_INFO(__LOGTAG__, "connected %0x", qconnection->cid_hash_val);
				qconnection->bridge->onconnection_connected(qconnection);
			}
			uint64_t s = 0;
			quiche_stream_iter* readable = quiche_conn_readable(qconnection->conn);
			while (quiche_stream_iter_next(readable, &s)) {
				debug_print(LOG_LEVEL_4, __LOGTAG__, "stream %" PRIu64 " is readable", s);
				bool fin = false;
				uint64_t out_error_code = 0;
				ssize_t recv_len = quiche_conn_stream_recv(qconnection->conn, s, conns->buf, sizeof(conns->buf), &fin, &out_error_code);
				if (recv_len < 0) {
					break;
				}
				if (fin) {
					const uint8_t BYEZ[] = "byez";
					ssize_t bye_sent_len = quiche_conn_stream_send(qconnection->conn, s, BYEZ, sizeof(BYEZ), true, &out_error_code);
					debug_print_important(__LOGTAG__, "fin received, sending 'byez' - %0x", qconnection->cid_hash_val);
					if (bye_sent_len != 5) {
						debug_print_error(__LOGTAG__, "sending 'byez' failed !!!");
					}
				}

				// heart-beat from client
				if (recv_len == 2 && conns->buf[0] == 'h' && conns->buf[1] == 'b') {
					qconnection->last_heartbeat_time = ev_now(loop);
					continue;
				}
				qconnection->bridge->onconnection_message(recv_len, conns->buf, qconnection);
			}
			quiche_stream_iter_free(readable);
		}
	}

	HASH_ITER(hh, conns->h, qconnection, tmp) {
		flush_egress(loop, qconnection);

		if (quiche_conn_is_closed(qconnection->conn)) {
			quiche_stats stats;
			quiche_path_stats path_stats;

			quiche_conn_stats(qconnection->conn, &stats);
			quiche_conn_path_stats(qconnection->conn, 0, &path_stats);

			debug_print_important(__LOGTAG__, "connection closed, recv=%zu sent=%zu lost=%zu rtt=%" PRIu64 "ns cwnd=%zu", stats.recv, stats.sent, stats.lost, path_stats.rtt, path_stats.cwnd);

			destroy_connection(loop, qconnection);
		}
	}
}

void qnetworkserver::broadcast_message(const qstring& buffer, bool flush) {
	qconn_io* qconnection = nullptr;
	qconn_io* tmp = nullptr;
	HASH_ITER(hh, conns->h, qconnection, tmp) {
		if (quiche_conn_is_established(qconnection->conn)) {
			qconnection->sendmessage(buffer, flush);
		}
	}
}

void qnetworkserver::force_disconnect_all() {
	debug_print_important(__LOGTAG__, "force disconnect all connections !!!");
	// destroy connections
	qconn_io *tmp, *conn_io_ptr = NULL;
	int pending_connections = 0;
	HASH_ITER(hh, conns->h, conn_io_ptr, tmp) {
		flush_egress(mainloop, conn_io_ptr);
		//        if (sent_bytes) {
		//            debug_print(LOG_LEVEL_3, __LOGTAG__, "force close --> try flush : sent bytes %zd", sent_bytes);
		//        }
		if (quiche_conn_is_closed(conn_io_ptr->conn)) {
			debug_print(LOG_LEVEL_0, __LOGTAG__, "force close : connection is already closed");
		}
		quiche_stats stats;
		quiche_path_stats path_stats;
		quiche_conn_stats(conn_io_ptr->conn, &stats);
		quiche_conn_path_stats(conn_io_ptr->conn, 0, &path_stats);
		debug_print(LOG_LEVEL_3, __LOGTAG__, "connection force closed, recv=%zu sent=%zu lost=%zu rtt=%" PRIu64 "ns cwnd=%zu", stats.recv, stats.sent, stats.lost, path_stats.rtt, path_stats.cwnd);
		HASH_DELETE(hh, conns->h, conn_io_ptr);
		GX_DELETE(conn_io_ptr);
		pending_connections++;
	}
	if (pending_connections > 0) {
		debug_print(LOG_LEVEL_0, __LOGTAG__, "Force closed %d pending connections.", pending_connections);
	}
}

void qnetworkserver::recv_cb(EV_P_ ev_io* w, int revents) {
	qnetworkserver* server = reinterpret_cast<qnetworkserver*>(w->data);
	server->recv_cb_internal(loop, w, revents);
}

bool qnetworkserver::network_server_begin() {
	if (!on_network_server_begin()) {
		return false;
	}
	on_network_server_init();
	return true;
}

void qnetworkserver::network_server_end() {
	on_network_server_end();
}

void qnetworkserver::on_heartbeat_check() {
	debug_raw_no_newline(LOG_LEVEL_0, "\r", "connections %zu\t\t", get_connection_count());
}

void qnetworkserver::heartbeat_check() {
	qconn_io *tmp, *conn_io_ptr = NULL;
	struct ev_loop* loop = get_mainloop();
	HASH_ITER(hh, conns->h, conn_io_ptr, tmp) {
		if (quiche_conn_is_closed(conn_io_ptr->conn)) {
			continue;
		}
		ev_tstamp elapsed_since_last_hb = ev_now(loop) - conn_io_ptr->last_heartbeat_time;
		if (elapsed_since_last_hb > 10.0) {
			debug_print(LOG_LEVEL_0, __LOGTAG__, "purging the connection due to inactivity -  connection %0x !!!", conn_io_ptr->cid_hash_val);
			destroy_connection(loop, conn_io_ptr);
		}
	}
	on_heartbeat_check();
}

void qnetworkserver::heart_beat_check_cb(EV_P_ ev_timer* w, int revents) {
	UNUSED(revents);
	qnetworkserver* server = reinterpret_cast<qnetworkserver*>(w->data);
	server->heartbeat_check();
}

void qnetworkserver::threadpool_mainthread_dispatcher_cb(EV_P_ ev_timer* w, int revents) {
	UNUSED(revents);
#if QTHREADPOOL
	qnetworkserver* server = reinterpret_cast<qnetworkserver*>(w->data);
	server->threadpool.process_in_main_thread();
#endif
}

#if QTHREADPOOL
// Initialization function for context
bool qnetworkserver::init_threadpool_context(thread_pool_context& context, const void* user_arg) {
	const runserverconfig* run_config = reinterpret_cast<const runserverconfig*>(user_arg);
	GX_DELETE(context.hiredis);
	context.hiredis = DEBUG_NEW qhiredis("threadpool:qserver_hiredis", run_config->redis_ip, run_config->redis_port, "gsdkuser", "Fr0gmoon123");
	if (context.hiredis->connect_redis() != 0) {
		debug_print_error(__LOGTAG__, "f:init_threadpool_context - failed to connect hiredis, Exiting !!!");
		GX_DELETE(context.hiredis);
		return false;
	}
	return true;
}

// Cleanup function for context
bool qnetworkserver::cleanup_threadpool_context(thread_pool_context& context) {
	GX_DELETE(context.hiredis);
	return true;
}
#endif

void qnetworkserver::exit_services_gracefully() {
	int log_status = logger.end_session();
	int stats_status = stats_logger.end_session();
	debug_print_important(__LOGTAG__, "waiting for services to finish !!!");
	struct ev_loop* wait_loop = ev_loop_new();
	qtimer_scheduler wait_scheduler;
	wait_scheduler.set_loop(wait_loop);
	qtimer* wait_timer = wait_scheduler.schedule_repeat_timer(
		[this, wait_loop, &log_status, &stats_status](qtimer& timer) {
			UNUSED(timer);
			if (log_status != 0) {	// in-case the internal thread is not yet started, we need to try calling end_session till we get a success.
				log_status = logger.end_session();
			}
			if (stats_status != 0) {  // in-case the internal thread is not yet started, we need to try calling end_session till we get a success.
				stats_status = stats_logger.end_session();
			}
			int service_shutdown_cnt = 0;
			if (logger.config.finished) {
				debug_print_important(__LOGTAG__, "logger service finished !!!");
				service_shutdown_cnt++;
			}
			if (stats_logger.config.finished) {
				debug_print_important(__LOGTAG__, "stats service finished !!!");
				service_shutdown_cnt++;
			}
			if (service_shutdown_cnt >= 2) {
				ev_break(wait_loop, EVBREAK_ONE);
			}
		},
		3);
	ev_run(wait_loop, 0);
	wait_scheduler.cancel_and_destroy_timer(wait_timer);
	ev_loop_destroy(wait_loop);
}

int qnetworkserver::run(qstring host, qstring port, fs::path root_dir, const qstring& redis_ip, const uint16_t REDIS_PORT, const qstring& app_id, observer_qserver_events* observer, void* user_arg_ptr) {
	user_arg = user_arg_ptr;
	run_server_config.host = host;
	run_server_config.port = port;
	run_server_config.redis_ip = redis_ip;
	run_server_config.redis_port = REDIS_PORT;
	run_server_config.root_dir = root_dir;
	run_server_config.id = qnetworkserver::run_id++;
	run_server_config.app_id = app_id;
	run_server_config.observer = observer;
    run_server_config.run_thread_id = pthread_self();
    
	qstring thread_name = qstring::format_string("qnetworkserver-%s %s:%s", app_id.c_str(), host.c_str(), port.c_str());
	PTHREAD_NAME(thread_name.c_str());
	server_event_observer = observer;
	host_id = host;
	port_id = port;

	qstring log_path = qstring::format_string("./glogs/%s/q_logfile", port.c_str());
	qstring stats_path = qstring::format_string("./gstats/%s/q_statfile", port.c_str());
	logger.start_session(log_path, log_path.length());
	stats_logger.init(essentials::get_sysname(), essentials::get_device_name(), "", app_id, 0);
	stats_logger.start_session(stats_path, stats_path.length());

#if ENABLE_QUICHE_LOG
	quiche_enable_debug_logging(debug_quiche_log, this);
	debug_print_warn(__LOGTAG__,
					 "quiche log is enabled. Perfomance may get "
					 "affected due to excess logs !!!");
#endif
	struct addrinfo* local;
	const struct addrinfo HINTS = {.ai_family = PF_UNSPEC, .ai_socktype = SOCK_DGRAM, .ai_protocol = IPPROTO_UDP};
	if (getaddrinfo(host.c_str(), port.c_str(), &HINTS, &local) != 0) {
		debug_print_error(__LOGTAG__, "f:run_internal - failed to resolve host");
		exit_services_gracefully();
		QSERVER_EVENT_ERROR(this, server_event_observer, -1);
		return -1;
	}

	int sock = socket(local->ai_family, SOCK_DGRAM, 0);
	if (sock < 0) {
		freeaddrinfo(local);
		debug_print_error(__LOGTAG__, "f:run_internal - failed to create socket");
		exit_services_gracefully();
		QSERVER_EVENT_ERROR(this, server_event_observer, -1);
		return -1;
	}

	if (fcntl(sock, F_SETFL, O_NONBLOCK) != 0) {
		freeaddrinfo(local);
		debug_print_error(__LOGTAG__, "f:run_internal - failed to make socket non-blocking");
		exit_services_gracefully();
		QSERVER_EVENT_ERROR(this, server_event_observer, -1);
		return -1;
	}

	if (bind(sock, local->ai_addr, local->ai_addrlen) < 0) {
		freeaddrinfo(local);
		debug_print_error(__LOGTAG__, "f:run_internal - failed to bind socket");
		exit_services_gracefully();
		QSERVER_EVENT_ERROR(this, server_event_observer, -1);
		return -1;
	}

	config = quiche_config_new(QUICHE_PROTOCOL_VERSION);
	if (config == nullptr) {
		freeaddrinfo(local);
		debug_print_error(__LOGTAG__, "f:run_internal - failed to create config");
		exit_services_gracefully();
		QSERVER_EVENT_ERROR(this, server_event_observer, -1);
		return -1;
	}

	fs::path cert_file(root_dir / "cert.crt");
	fs::path key_file(root_dir / "cert.key");
	debug_print(LOG_LEVEL_2, __LOGTAG__, "cert file %s, key file %s", cert_file.c_str(), key_file.c_str());
	int res_crt_load = quiche_config_load_cert_chain_from_pem_file(config, cert_file.c_str());
	if (res_crt_load != 0) {
		debug_print_error(__LOGTAG__, "CERT load error - %s, err %d", cert_file.c_str(), res_crt_load);
		freeaddrinfo(local);
		exit_services_gracefully();
		QSERVER_EVENT_ERROR(this, server_event_observer, -1);
		return -1;
	}
	int res_key_load = quiche_config_load_priv_key_from_pem_file(config, key_file.c_str());
	if (res_key_load != 0) {
		debug_print_error(__LOGTAG__, "KEY load error - %s", key_file.c_str());
		freeaddrinfo(local);
		exit_services_gracefully();
		QSERVER_EVENT_ERROR(this, server_event_observer, -1);
		return -1;
	}

	quiche_config_set_application_protos(config, reinterpret_cast<const uint8_t*>("\x0ahq-interop\x05hq-29\x05hq-28\x05hq-27\x08http/0.9"), 38);

	quiche_config_set_max_idle_timeout(config, 30000);
	quiche_config_set_max_recv_udp_payload_size(config, Q_MAX_DATAGRAM_SIZE);
	quiche_config_set_max_send_udp_payload_size(config, Q_MAX_DATAGRAM_SIZE);
	quiche_config_set_initial_max_data(config, 10000000);
	quiche_config_set_initial_max_stream_data_bidi_local(config, 1000000);
	quiche_config_set_initial_max_stream_data_bidi_remote(config, 1000000);
	quiche_config_set_initial_max_streams_bidi(config, 100);
	quiche_config_set_cc_algorithm(config, quiche_cc_algorithm::QUICHE_CC_RENO);

	// Generate a 16-byte token for stateless reset
	uint8_t stateless_reset_token[16] = {0x1a, 0x2b, 0x3c, 0x4d, 0x5e, 0x6f, 0x7a, 0x8b, 0x9c, 0xad, 0xbe, 0xcf, 0xda, 0xeb, 0xfc, 0x0d};

	// Set the stateless reset token in the QUIC config
	quiche_config_set_stateless_reset_token(config, stateless_reset_token);

	struct qconnections c;
	c.sock = sock;
	c.h = nullptr;
	c.local_addr = local->ai_addr;
	c.local_addr_len = local->ai_addrlen;

	conns = &c;

	mainloop = ev_loop_new(0);

#if QTHREADPOOL
	ev_init(&threadpool_mainthread_dispatcher_timer, threadpool_mainthread_dispatcher_cb);
	threadpool_mainthread_dispatcher_timer.data = this;
	threadpool_mainthread_dispatcher_timer.repeat = 1.5f;
	ev_timer_again(mainloop, &threadpool_mainthread_dispatcher_timer);
	if (threadpool.init(&run_server_config) <= 0) {
		debug_print_error(__LOGTAG__, "failed to init threadpool, Exiting !!!");
		threadpool.stop();
		ev_loop_destroy(mainloop);
		freeaddrinfo(local);
		exit_services_gracefully();
		QSERVER_EVENT_ERROR(this, server_event_observer, -1);
		return -1;
	}
#endif

	ev_io watcher;
	ev_io_init(&watcher, recv_cb, sock, EV_READ);
	ev_io_start(mainloop, &watcher);
	watcher.data = this;

	if (!network_server_begin()) {
		network_server_end();
		debug_print_error(__LOGTAG__, "network_server_begin failed, Exiting !!!");
#if QTHREADPOOL
		threadpool.stop();
#endif
		ev_loop_destroy(mainloop);
		freeaddrinfo(local);
		exit_services_gracefully();
		QSERVER_EVENT_ERROR(this, server_event_observer, -1);
		return -1;
	}

	QSERVER_EVENT_PRE_START(this, server_event_observer);

	// heartbeat check timer
	ev_init(&heartbeat_check_timer, heart_beat_check_cb);
	heartbeat_check_timer.data = this;
	heartbeat_check_timer.repeat = 10.0f;
	ev_timer_again(mainloop, &heartbeat_check_timer);

	int port_in_number = 0;
	if (gsdk::str2int(&port_in_number, port.c_str(), port.length(), 10) != gsdk::STR2INT_SUCCESS) {
		debug_print_error(__LOGTAG__, "Unable to parse server port, defaulting to %d !!!", port_in_number);
	}
	QSERVER_EVENT_START(this, server_event_observer, host.c_str(), port_in_number);

	ev_loop(mainloop, 0);

	ev_timer_stop(mainloop, &heartbeat_check_timer);
#if QTHREADPOOL
	// Wait for all tasks to complete before shutting down the pool
	essentials::sleep_for(5000);
	threadpool.stop();
	ev_timer_stop(mainloop, &threadpool_mainthread_dispatcher_timer);
#endif

	QSERVER_EVENT_STOP(this, server_event_observer);
	network_server_end();
	force_disconnect_all();

	ev_loop_destroy(mainloop);

	freeaddrinfo(local);
	quiche_config_free(config);

	exit_services_gracefully();
	return 0;
}
