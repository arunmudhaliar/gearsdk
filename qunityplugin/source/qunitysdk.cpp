//
//  Copyright 2024 homenet25
//  qunityplugin.cpp
//  qunityplugin
//
//  Created by Arun A on 24/01/24.
//

#include "qunitysdk.hpp"

using namespace qunitysdk;

qsocket::qsocket(type_qsocket_destroy_qsocket cb) : cb_destroy_qsocket(cb) {}
qsocket::~qsocket() {
	debug_print(LOG_LEVEL_0, __LOGTAG__, "native qsocket destroyed !!!");
}

bool qsocket::connect(const char* host, const char* port, void* arg, type_qsocket_onconnect cb_connect, type_qsocket_onmessage cb_message, type_qsocket_onreleaseconnection cb_release_connection, type_qsocket_onclose cb_close) {
	this->cb_connect = cb_connect;
	this->cb_message = cb_message;
	this->cb_release_connection = cb_release_connection;
	this->cb_close = cb_close;
	this->user_data = arg;
	if (run(host, port) != 0) {
		return false;
	}
	return true;
}

void qsocket::onconnect(conn_io_client* qconnection) {
	if (cb_connect == nullptr) {
		return;
	}
	cb_connect(guid_crc, user_data);
}
void qsocket::onmessage(ssize_t recv_len, uint8_t* buf, conn_io_client* qconnection) {
	if (cb_message == nullptr) {
		return;
	}
	debug_print(LOG_LEVEL_0, __LOGTAG__, "native onmessage %.*s : sz(%d)", recv_len, buf, recv_len);
	cb_message(guid_crc, recv_len, buf);
}
void qsocket::onreleaseconnection(conn_io_client* qconnection) {
	if (cb_release_connection) {
		cb_release_connection(guid_crc);
	}
	if (cb_destroy_qsocket) {
		cb_destroy_qsocket(this);
	}
}
void qsocket::onclose(conn_io_client* qconnection) {
	if (cb_close == nullptr) {
		return;
	}
	cb_close(guid_crc);
}

void qsocket::clear_callbacks() {
	cb_connect = nullptr;
	cb_message = nullptr;
	cb_release_connection = nullptr;
	cb_close = nullptr;
}

extern "C" {
void pre_init_sdk() {
	// To prevent exceptions on editor. bcz when we stop a game in editor the managed instances will get destroyed, but not the native objects.
	for (auto q : qsockets) {
		debug_print(LOG_LEVEL_0, __LOGTAG__, "qsocket - clearing all callbacks for %x state:%d !!!", q.first, q.second->getstate());
		q.second->clear_callbacks();
	}
	qsockets.clear();
}

int send_async_request(const char* host, const char* port, const char* path, const char* payload, void* arg, type_qh3client_plugin_helper_cb callback, int retry) {
	debug_print(LOG_LEVEL_0, __LOGTAG__, "host %s, port %s, path %s, payload %s", host, port, path, payload);
	return qh3client_helper::send_async_request<client::qh3client>(
		host, port, conn_io_req_res::create(path, payload), arg,
		[callback](conn_io_req_res* request, conn_io_req_res* response, void* client_specific_data, void* arg, bool success) {
			const conn_io_req_res::payload& payload = response->data;
			if (callback != nullptr) {
				callback(payload.buffer.c_str(), arg, success);
			}
			debug_print(LOG_LEVEL_0, __LOGTAG__, "payload %s", payload.buffer.c_str());
		},
		retry);
}

void destroy_qsocket(qsocket* qs) {
	debug_print(LOG_LEVEL_0, __LOGTAG__, "qsocket destroy_qsocket called.");
	std::map<unsigned long, qsocket*>::iterator it_qsocket = qsockets.end();
	for (std::map<unsigned long, qsocket*>::iterator it = qsockets.begin(); it != qsockets.end(); it++) {
		if (it->second == qs) {
			it_qsocket = it;
			break;
		}
	}

	if (it_qsocket != qsockets.end()) {
		// GX_DELETE(it_qsocket->second); // do not delete since the qsocket get destroyed internally.
		debug_print(LOG_LEVEL_0, __LOGTAG__, "qsocket removed from the list !!!");
		qsockets.erase(it_qsocket);
	}
}

int qsocket_sendMessage(unsigned long guid_crc, const char* buffer, unsigned long size, bool flush) {
	std::map<unsigned long, qsocket*>::iterator it = qsockets.find(guid_crc);
	if (it == qsockets.end()) {
		debug_warn(LOG_LEVEL_0, __LOGTAG__, "qsocket_sendMessage failed - guid_crc not exist %d", guid_crc);
		return -1;
	}
	return it->second->sendMessage((uint8_t*) buffer, size, flush);
}

int qsocket_close(unsigned long guid_crc) {
	std::map<unsigned long, qsocket*>::iterator it = qsockets.find(guid_crc);
	if (it == qsockets.end()) {
		debug_warn(LOG_LEVEL_0, __LOGTAG__, "qsocket_close failed - guid_crc not exist %d", guid_crc);
		return -1;
	}
	return it->second->close();
}

unsigned long get_crc32(const char* guid, int guid_len) {
	if (guid == nullptr) {
		debug_warn(LOG_LEVEL_0, __LOGTAG__, "get_crc32 failed - guid is null");
		return false;
	}
	return essentials::get_crc((const uint8_t*) guid, guid_len);
}

bool qsocket_connect(unsigned long guid_crc, const char* host, const char* port, void* arg, qsocket::type_qsocket_onconnect cb_connect, qsocket::type_qsocket_onmessage cb_message,
					 qsocket::type_qsocket_onreleaseconnection cb_release_connection, qsocket::type_qsocket_onclose cb_close) {
	if (qsockets.find(guid_crc) != qsockets.end()) {
		debug_warn(LOG_LEVEL_0, __LOGTAG__, "qsocket_connect failed - guid_crc already exist %d", guid_crc);
		return false;
	}
	qsocket* newsocket = DEBUG_NEW qsocket(qunitysdk::destroy_qsocket);
	newsocket->set_guid_crc(guid_crc);
	if (!newsocket->connect(host, port, arg, cb_connect, cb_message, cb_release_connection, cb_close)) {
		GX_DELETE(newsocket);
	}

	debug_warn(LOG_LEVEL_0, __LOGTAG__, "qsocket_connect guid_crc %d", guid_crc);
	qsockets[guid_crc] = newsocket;
	return true;
}

bool qsocket_is_run_finished(unsigned long guid_crc) {
	std::map<unsigned long, qsocket*>::iterator it = qsockets.find(guid_crc);
	if (it == qsockets.end()) {
		debug_warn(LOG_LEVEL_0, __LOGTAG__, "qsocket_is_run_finished - guid_crc not exist %d", guid_crc);
		return false;
	}
	return it->second->is_runfinished();
}

void destroy_finished_qsockets() {
	std::vector<unsigned long> finishedList;
	for (std::map<unsigned long, qsocket*>::iterator it = qsockets.begin(); it != qsockets.end(); it++) {
		if (it->second->is_runfinished()) {
			finishedList.push_back(it->first);
		}
	}

	for (auto it = finishedList.cbegin(); it != finishedList.cend(); it++) {
		unsigned long guid_crc = *it;
		std::map<unsigned long, qsocket*>::iterator it_qsocket = qsockets.find(guid_crc);
		if (it_qsocket != qsockets.end()) {
			size_t oldSz = qsockets.size();
			qsockets.erase(it_qsocket);
			if (oldSz != qsockets.size()) {
				debug_print(LOG_LEVEL_0, __LOGTAG__, "destroy_finished_qsockets - qsocket for guid_crc %x deleted !!!", guid_crc);
				GX_DELETE(it_qsocket->second);
			}
		}
	}
}

void qsocket_print_info() {
	for (auto q : qsockets) {
		debug_print(LOG_LEVEL_0, __LOGTAG__, "qsocket - guid_crc %d state:%d !!!", q.first, q.second->getstate());
	}
}
}
