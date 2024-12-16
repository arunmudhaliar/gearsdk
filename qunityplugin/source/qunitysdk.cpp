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
	// debug_print(LOG_LEVEL_5, __LOGTAG__, "native onmessage %.*s : sz(%d)", recv_len, buf, recv_len);
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
EXPORT void pre_init_sdk() {
	// To prevent exceptions on editor. bcz when we stop a game in editor the managed instances will get destroyed, but not the native objects.
	for (auto q : qsockets) {
		debug_print(LOG_LEVEL_0, __LOGTAG__, "qsocket - clearing all callbacks for %x state:%d !!!", q.first, q.second->getstate());
		q.second->clear_callbacks();
	}
	qsockets.clear();
}

EXPORT int send_async_request(const char* host, const char* port, const char* path, const char* payload, void* arg, type_qh3client_plugin_helper_cb callback, int retry) {
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

EXPORT int qsocket_sendMessage(unsigned long guid_crc, const char* buffer, unsigned long size, bool flush) {
	std::map<unsigned long, qsocket*>::iterator it = qsockets.find(guid_crc);
	if (it == qsockets.end()) {
		debug_warn(LOG_LEVEL_0, __LOGTAG__, "qsocket_sendMessage failed - guid_crc not exist %d", guid_crc);
		return -1;
	}
	return it->second->send_message((uint8_t*) buffer, size, flush);
}

EXPORT int qsocket_close(unsigned long guid_crc) {
	std::map<unsigned long, qsocket*>::iterator it = qsockets.find(guid_crc);
	if (it == qsockets.end()) {
		debug_warn(LOG_LEVEL_0, __LOGTAG__, "qsocket_close failed - guid_crc not exist %d", guid_crc);
		return -1;
	}
	return it->second->close();
}

EXPORT unsigned long get_crc32(const char* guid, int guid_len) {
	if (guid == nullptr) {
		debug_warn(LOG_LEVEL_0, __LOGTAG__, "get_crc32 failed - guid is null");
		return false;
	}
	return essentials::get_crc((const uint8_t*) guid, guid_len);
}

EXPORT bool qsocket_connect(unsigned long guid_crc, const char* host, const char* port, void* arg, qsocket::type_qsocket_onconnect cb_connect, qsocket::type_qsocket_onmessage cb_message,
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

	debug_print(LOG_LEVEL_0, __LOGTAG__, "qsocket_connect guid_crc %d", guid_crc);
	qsockets[guid_crc] = newsocket;
	return true;
}

EXPORT bool qsocket_is_run_finished(unsigned long guid_crc) {
	std::map<unsigned long, qsocket*>::iterator it = qsockets.find(guid_crc);
	if (it == qsockets.end()) {
		debug_warn(LOG_LEVEL_0, __LOGTAG__, "qsocket_is_run_finished - guid_crc not exist %d", guid_crc);
		return false;
	}
	return it->second->is_runfinished();
}

EXPORT void destroy_finished_qsockets() {
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

EXPORT void qsocket_print_info() {
	for (auto q : qsockets) {
		debug_print(LOG_LEVEL_0, __LOGTAG__, "qsocket - guid_crc %d state:%d !!!", q.first, q.second->getstate());
	}
}
}

// #define QH3TEST_APP 1
#if PLATFORM == PLATFORM_WINDOWS && QH3TEST_APP
// Callback function definition
void async_request_callback(const char* payload, void* arg, bool success) {
	debug_print(LOG_LEVEL_0, __LOGTAG__, "async_request_callback called with result %d", success);
}

// example code for stateless server socket
void test_qh3client() {
	// The server details
	const char* host = "15.206.79.30";
	const char* port = "4004";
	const char* path = "/whoami";
	const char* payload = "{}";
	void* user_arg = nullptr;  // You can pass any user-defined argument here

	int result = send_async_request(host, port, path, payload, user_arg, async_request_callback, 3);
	if (result != 0) {
		debug_print(LOG_LEVEL_0, __LOGTAG__, "Request initiation failed with error code %d", result);
	}
}

// example code for statefull server socket
void test_qclient() {
	const char* host = "15.206.79.30";
	const char* port = "4000";
	void* user_arg = nullptr;  // You can pass any user-defined argument here

	const int CONNECTION1_GUID = 12345;
	const int CONNECTION2_GUID = 12347;
	bool result1 = qsocket_connect(
		CONNECTION1_GUID, host, port, user_arg,
		[](unsigned long guid_crc, void* v) {
			debug_print(LOG_LEVEL_0, __LOGTAG__, "qsocket connected %d", guid_crc);
			const char* payload = "{\"sig\":31387,\"t_crc\":3673067835,\"room_config\":{\"min\":2,\"max\":4,\"betx\":0,\"rewardx\":8192,\"allow_after_start\":false},\"prev_cid_hash_val\":0,\"room_id\":-1,\"pid\":\"57f5159b\"}";
			qsocket_sendMessage(guid_crc, payload, strlen(payload), true);
		},
		[](unsigned long guid_crc, unsigned long recv_len, uint8_t* buf) { debug_print(LOG_LEVEL_0, __LOGTAG__, "msg recv %d - %.*s", guid_crc, recv_len, buf); },
		[](unsigned long guid_crc) { debug_print(LOG_LEVEL_0, __LOGTAG__, "qsocket release connection %d", guid_crc); }, [](unsigned long guid_crc) { debug_print(LOG_LEVEL_0, __LOGTAG__, "qsocket connection closed %d", guid_crc); });
	if (!result1) {
		debug_print(LOG_LEVEL_0, __LOGTAG__, "Connection1 request failed");
	}

	bool result2 = qsocket_connect(
		CONNECTION2_GUID, host, port, user_arg,
		[](unsigned long guid_crc, void* v) {
			debug_print(LOG_LEVEL_0, __LOGTAG__, "qsocket connected %d", guid_crc);
			const char* payload = "{\"sig\":31387,\"t_crc\":3673067835,\"room_config\":{\"min\":2,\"max\":4,\"betx\":0,\"rewardx\":8192,\"allow_after_start\":false},\"prev_cid_hash_val\":0,\"room_id\":-1,\"pid\":\"57f515a1\"}";
			qsocket_sendMessage(guid_crc, payload, strlen(payload), true);
		},
		[](unsigned long guid_crc, unsigned long recv_len, uint8_t* buf) { debug_print(LOG_LEVEL_0, __LOGTAG__, "msg recv %d - %.*s", guid_crc, recv_len, buf); },
		[](unsigned long guid_crc) { debug_print(LOG_LEVEL_0, __LOGTAG__, "qsocket release connection %d", guid_crc); }, [](unsigned long guid_crc) { debug_print(LOG_LEVEL_0, __LOGTAG__, "qsocket connection closed %d", guid_crc); });
	if (!result2) {
		debug_print(LOG_LEVEL_0, __LOGTAG__, "Connection2 request failed");
	}

	debug_print(LOG_LEVEL_0, __LOGTAG__, "Press ENTER key to close connections");
	getchar();
	qsocket_close(CONNECTION1_GUID);
	qsocket_close(CONNECTION2_GUID);
	qsocket_print_info();
}

int main() {
	debug_print(LOG_LEVEL_0, __LOGTAG__, "qunity test app");
#if PLATFORM == PLATFORM_WINDOWS
	// Initialize Winsock
	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
		fprintf(stderr, "WSAStartup failed\n");
		return -1;
	}
#endif

	test_qh3client();  // uncomment this to test stateless request
	// test_qclient();		// uncomment this to test statefull request

	// Keep the program running until the async request completes and the callback is invoked.
	// (This is just a placeholder and should be handled more elegantly in a real app).

	debug_print(LOG_LEVEL_0, __LOGTAG__, "Press ENTER key to exit");
	getchar();
#if PLATFORM == PLATFORM_WINDOWS
	WSACleanup();  // Clean up Winsock before exiting
#endif
	return 0;
}
#endif