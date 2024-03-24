//
//  qh3client_helper.cpp
//  qh3client
//
//  Created by Arun A on 04/11/23.
//

#include "qh3client_helper.hpp"

using namespace client;

template int qh3client_helper::send_async_request<qh3client>(
	const qstring host, const qstring port,
	const conn_io_req_res* data_getorpost_, void* arg, type_qh3client_helper_cb async_cb, int retry);

#if PLATFORM == PLATFORM_ANDROID
#include "qh3client-android.h"
template int qh3client_helper::send_async_request<qh3client_android>(
	const qstring host, const qstring port,
	const conn_io_req_res* data_getorpost_, void* arg, type_qh3client_helper_cb async_cb, int retry);
#endif

template <typename T>
int qh3client_helper::send_request(const qstring host, const qstring port,
								   const conn_io_req_res* data_getorpost_, type_qh3client_helper_cb async_cb, int retry) {
	qh3_req_obj* req_obj = DEBUG_NEW qh3_req_obj(host, port, data_getorpost_);
	req_obj->async_cb = async_cb;
	req_obj->retry = retry;
	if (pthread_create(&req_obj->run_thread_id, nullptr, qh3client_helper::run_internal<T>, (void*) req_obj) < 0) {
		DEBUG_PRINT_ERROR(__LOGTAG__, "could not create thread: %s - %d", strerror(errno), errno);
		return -1;
	}
	pthread_join(req_obj->run_thread_id, nullptr);
	return 0;
}

template <typename T>
int qh3client_helper::send_async_request(const qstring host, const qstring port,
										 const conn_io_req_res* data_getorpost_, void* arg, type_qh3client_helper_cb async_cb, int retry) {
	qh3_req_obj* req_obj = DEBUG_NEW qh3_req_obj(host, port, data_getorpost_);
	req_obj->async_cb = async_cb;
	req_obj->arg = arg;
	req_obj->retry = retry;
	if (pthread_create(&req_obj->run_thread_id, nullptr, qh3client_helper::run_internal<T>, (void*) req_obj) < 0) {
		DEBUG_PRINT_ERROR(__LOGTAG__, "could not create thread: %s - %d", strerror(errno), errno);
		return -1;
	}
	return 0;
}
//

template <typename T>
void* qh3client_helper::run_internal(void* data) {
	qh3_req_obj* req_obj = (qh3_req_obj*) data;
	bool response_received = false;
	for (int x = 0; x < req_obj->retry + 1; x++) {
		if (x > 0) {
			DEBUG_PRINT(LOG_LEVEL_0, __LOGTAG__, "run_internal : retrying - %d", x);
		}
		T* new_client = DEBUG_NEW T(req_obj->host, req_obj->port, req_obj->arg);
		new_client->send_request(req_obj->data);
		response_received = new_client->conn_io->res_received;
		if (response_received || x == req_obj->retry) { // if no response even after last try just return the callback with empty response.
			if (req_obj->async_cb && new_client->conn_io && new_client->conn_io->response) {
				req_obj->async_cb(new_client->conn_io->response, new_client->get_client_specific_data(), new_client->arg, response_received);
			}
		}
		new_client->on_post_send_cleanup();
		GX_DELETE(new_client);
		if (response_received) {
			break;
		}
	}
	GX_DELETE(req_obj);
	pthread_exit(0);
}
