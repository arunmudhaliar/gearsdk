//
//  Copyright 2024 homenet25
//  gclient.cpp
//  qgfist
//
//  Created by Arun A on 17/09/2024.
//

#include "gclient.hpp"

gclient::gclient(uv_loop_t* loop, type_qclient_onconnect_cb onconnect_cb, type_qclient_onmessage_cb onmessage_cb, type_qclient_onreleaseconnection_cb onreleaseconnection_cb, type_qclient_onclose_cb onclose_cb, size_t worker_id)
	: qnetworkclient(), onconnect_cb(onconnect_cb), onmessage_cb(onmessage_cb), onreleaseconnection_cb(onreleaseconnection_cb), onclose_cb(onclose_cb), worker_id(worker_id) {}

gclient::~gclient() {}

void gclient::onconnect(conn_io_client* qconnection) {
	if (onconnect_cb) {
		onconnect_cb(this, qconnection);
	}
}

void gclient::onmessage(ssize_t recv_len, uint8_t* buf, conn_io_client* qconnection) {
	if (onmessage_cb) {
		onmessage_cb(this, recv_len, buf, qconnection);
	}
}

void gclient::onreleaseconnection(conn_io_client* qconnection) {
	if (onreleaseconnection_cb) {
		onreleaseconnection_cb(this, qconnection->cid_hash_val);
	}
	finished = true;
}

void gclient::onclose(conn_io_client* qconnection) {
	if (onclose_cb) {
		onclose_cb(this, qconnection);
	}
}
