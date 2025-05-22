//
//  Copyright 2024 homenet25
//  qunityplugin.hpp
//  qunityplugin
//
//  Created by Arun A on 24/01/24.
//

#ifndef qunityplugin_hpp
#define qunityplugin_hpp

#include "../../common/sdktypes.hpp"
#include "../../qclient/source/qnetworkclient.hpp"
#include "../../qh3client/qh3client/qh3client.hpp"
#include "../../qh3client/qh3client/qh3client_helper.hpp"

#include <algorithm>
#include <map>

#undef __LOGTAG__
#define __LOGTAG__ "qunitysdk"

namespace qunitysdk {
using namespace client;

class qsocket : public qnetworkclient {
   public:
	typedef void (*type_qsocket_destroy_qsocket)(qsocket*);

	qsocket(type_qsocket_destroy_qsocket cb);
	virtual ~qsocket();

	typedef void (*type_qsocket_onconnect)(unsigned long guid_crc, void*);
	typedef void (*type_qsocket_onmessage)(unsigned long guid_crc, unsigned long recv_len, uint8_t* buf);
	typedef void (*type_qsocket_onreleaseconnection)(unsigned long guid_crc);
	typedef void (*type_qsocket_onclose)(unsigned long guid_crc);
	bool connect(const char* host, const char* port, void* arg, type_qsocket_onconnect cb_connect, type_qsocket_onmessage cb_message, type_qsocket_onreleaseconnection cb_release_connection, type_qsocket_onclose cb_close);
	void clear_callbacks();
	void set_guid_crc(unsigned long guid_crc) { this->guid_crc = guid_crc; }

   protected:
	void onconnect(conn_io_client* qconnection) override;
	void onmessage(ssize_t recv_len, uint8_t* buf, conn_io_client* qconnection) override;
	void onreleaseconnection(conn_io_client* qconnection) override;
	void onclose(conn_io_client* qconnection) override;

   private:
	type_qsocket_onconnect cb_connect = nullptr;
	type_qsocket_onmessage cb_message = nullptr;
	type_qsocket_onreleaseconnection cb_release_connection = nullptr;
	type_qsocket_onclose cb_close = nullptr;
	type_qsocket_destroy_qsocket cb_destroy_qsocket = nullptr;
	unsigned long guid_crc = 0;
	void* user_data = nullptr;
};

std::map<unsigned long, qsocket*> qsockets;

#if PLATFORM == PLATFORM_WINDOWS
#define EXPORT __declspec(dllexport)
#else
#define EXPORT __attribute__((visibility("default"))) __attribute__((unused))
#endif

extern "C" {
EXPORT void pre_init_sdk();

typedef void (*type_qh3client_plugin_helper_cb)(const char* payload, void* arg, bool success);
EXPORT int send_async_request(const char* host, const char* port, const char* path, const char* payload, void* arg, type_qh3client_plugin_helper_cb callback, int retry);
void destroy_qsocket(qsocket* qs);
EXPORT bool qsocket_connect(unsigned long guid_crc, const char* host, const char* port, void* arg, qsocket::type_qsocket_onconnect cb_connect, qsocket::type_qsocket_onmessage cb_message,
							qsocket::type_qsocket_onreleaseconnection cb_release_connection, qsocket::type_qsocket_onclose cb_close);
EXPORT bool qsocket_is_run_finished(unsigned long guid_crc);
EXPORT int qsocket_sendMessage(unsigned long guid_crc, const char* buffer, unsigned long size, bool flush);
EXPORT int qsocket_close(unsigned long guid_crc);
EXPORT void destroy_finished_qsockets();
EXPORT void qsocket_print_info();
EXPORT unsigned long get_crc32(const char* guid, int guid_len);
EXPORT void qsocket_send_ping(unsigned long guid_crc);
}
};	// namespace qunitysdk

#endif /* qunityplugin_hpp */
