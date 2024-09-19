//
//  Copyright 2024 homenet25
//  gclient.cpp
//  qgfist
//
//  Created by Arun A on 17/09/2024.
//

#include "gclient.hpp"

gclient::gclient(uv_loop_t* loop, type_qclient_onconnect_cb onconnect_cb, 
      type_qclient_onmessage_cb onmessage_cb, 
      type_qclient_onreleaseconnection_cb onreleaseconnection_cb, 
      type_qclient_onclose_cb onclose_cb, void* arg): qnetworkclient(), 
	  	onconnect_cb(onconnect_cb), onmessage_cb(onmessage_cb), onreleaseconnection_cb(onreleaseconnection_cb), onclose_cb(onclose_cb) {
	uv_async_init(loop, &async_onconnect, async_qclient_onconnect_cb);
	uv_async_init(loop, &async_onmessage, async_qclient_onmessage_cb);
	uv_async_init(loop, &async_onreleaseconnection, async_qclient_onreleaseconnection_cb);
	uv_async_init(loop, &async_onclose, async_qclient_onclose_cb);
	user_data = arg;
}

gclient::~gclient() {
	uv_close(reinterpret_cast<uv_handle_t*>(&async_onconnect), nullptr);
	uv_close(reinterpret_cast<uv_handle_t*>(&async_onmessage), nullptr);
	uv_close(reinterpret_cast<uv_handle_t*>(&async_onreleaseconnection), nullptr);
	uv_close(reinterpret_cast<uv_handle_t*>(&async_onclose), nullptr);
}

void gclient::onconnect(conn_io_client* qconnection) {
	async_onconnect.data = DEBUG_NEW std::tuple<gclient*, conn_io_client*>(this, qconnection);
	uv_async_send(&async_onconnect);
}

void gclient::onmessage(ssize_t recv_len, uint8_t* buf, conn_io_client* qconnection) {
	async_onmessage.data = DEBUG_NEW std::tuple<gclient*, ssize_t, uint8_t*, conn_io_client*>(this, recv_len, buf, qconnection);
	uv_async_send(&async_onmessage);
}

void gclient::onreleaseconnection(conn_io_client* qconnection) {
	async_onreleaseconnection.data = DEBUG_NEW std::tuple<gclient*, conn_io_client*>(this, qconnection);
	uv_async_send(&async_onreleaseconnection);
}

void gclient::onclose(conn_io_client* qconnection) {
	async_onclose.data = DEBUG_NEW std::tuple<gclient*, conn_io_client*>(this, qconnection);
	uv_async_send(&async_onclose);
}

void gclient::async_qclient_onconnect_cb(uv_async_t* handle) {
	std::tuple<gclient*, conn_io_client*> *data = static_cast<std::tuple<gclient*, conn_io_client*>*>(handle->data);
	gclient* client = std::get<0>(*data);
	conn_io_client* qconnection = std::get<1>(*data);
	if (client->onconnect_cb) {
		client->onconnect_cb(client, qconnection);
	}
	GX_DELETE(data);
}

void gclient::async_qclient_onmessage_cb(uv_async_t* handle) {
	std::tuple<gclient*, ssize_t, uint8_t*, conn_io_client*> *data = static_cast<std::tuple<gclient*, ssize_t, uint8_t*, conn_io_client*>*>(handle->data);
	gclient* client = std::get<0>(*data);
	ssize_t recv_len = std::get<1>(*data);
	uint8_t* buf = std::get<2>(*data);
	conn_io_client* qconnection = std::get<3>(*data);
	if (client->onmessage_cb) {
		client->onmessage_cb(client, recv_len, buf, qconnection);
	}
	GX_DELETE(data);
}

void gclient::async_qclient_onreleaseconnection_cb(uv_async_t* handle) {
	std::tuple<gclient*, conn_io_client*> *data = static_cast<std::tuple<gclient*, conn_io_client*>*>(handle->data);
	gclient* client = std::get<0>(*data);
	conn_io_client* qconnection = std::get<1>(*data);
	if (client->onreleaseconnection_cb) {
		client->onreleaseconnection_cb(client, qconnection);
	}
	GX_DELETE(data);
}

void gclient::async_qclient_onclose_cb(uv_async_t* handle) {
	std::tuple<gclient*, conn_io_client*> *data = static_cast<std::tuple<gclient*, conn_io_client*>*>(handle->data);
	gclient* client = std::get<0>(*data);
	conn_io_client* qconnection = std::get<1>(*data);
	if (client->onclose_cb) {
		client->onclose_cb(client, qconnection);
	}
	GX_DELETE(data);
}