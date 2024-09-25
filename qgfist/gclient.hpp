//
//  Copyright 2024 homenet25
//  gclient.hpp
//  qgfist
//
//  Created by Arun A on 17/09/2024.
//

#ifndef gclient_hpp
#define gclient_hpp

#include "../qclient/source/qnetworkclient.hpp"

#include <functional>
#include <mutex>
#include <queue>
#include <tuple>
#include <uv.h>

#undef __LOGTAG__
#define __LOGTAG__ "gclient"

using namespace client;

class gclient;
typedef std::function<void(gclient* c, conn_io_client* qconnection)> type_qclient_onconnect_cb;
typedef std::function<void(gclient* c, ssize_t recv_len, uint8_t* buf, conn_io_client* qconnection)> type_qclient_onmessage_cb;
typedef std::function<void(gclient* c, unsigned cid_hash_val)> type_qclient_onreleaseconnection_cb;
typedef std::function<void(gclient* c, conn_io_client* qconnection)> type_qclient_onclose_cb;

class gclient : public qnetworkclient {
   public:
	gclient(uv_loop_t* loop, type_qclient_onconnect_cb onconnect_cb, type_qclient_onmessage_cb onmessage_cb, type_qclient_onreleaseconnection_cb onreleaseconnection_cb, type_qclient_onclose_cb onclose_cb, size_t worker_id);
	virtual ~gclient();

	size_t get_user_data() const { return worker_id; }
	bool is_finished() { return finished; }

   protected:
	void onconnect(conn_io_client* qconnection) override;
	void onmessage(ssize_t recv_len, uint8_t* buf, conn_io_client* qconnection) override;
	void onreleaseconnection(conn_io_client* qconnection) override;
	void onclose(conn_io_client* qconnection) override;

   private:
	gclient() = delete;
	type_qclient_onconnect_cb onconnect_cb = nullptr;
	type_qclient_onmessage_cb onmessage_cb = nullptr;
	type_qclient_onreleaseconnection_cb onreleaseconnection_cb = nullptr;
	type_qclient_onclose_cb onclose_cb = nullptr;

	size_t worker_id = -1;
	std::atomic<bool> finished = {false};
};

#endif /* gclient_hpp */
