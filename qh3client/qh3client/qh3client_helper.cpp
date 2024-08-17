//
//  Copyright 2024 homenet25
//  qh3client_helper.cpp
//  qh3client
//
//  Created by Arun A on 04/11/23.
//

#include <future>  // Include necessary header for std::async
#include "qh3client_helper.hpp"

using namespace client;

template int qh3client_helper::send_async_request<qh3client>(const qstring host, const qstring port, const conn_io_req_res* data_getorpost_, void* arg, type_qh3client_helper_cb async_cb, int retry);
template int qh3client_helper::send_request<qh3client>(const qstring host, const qstring port, const conn_io_req_res* data_getorpost_, type_qh3client_helper_cb async_cb, int retry);


#if PLATFORM == PLATFORM_ANDROID
#include "qh3client-android.h"
template int qh3client_helper::send_async_request<qh3client_android>(const qstring host, const qstring port, const conn_io_req_res* data_getorpost_, void* arg, type_qh3client_helper_cb async_cb, int retry);
template int qh3client_helper::send_request<qh3client_android>(const qstring host, const qstring port, const conn_io_req_res* data_getorpost_, type_qh3client_helper_cb async_cb, int retry);
#endif

template <typename T>
int qh3client_helper::send_request(const qstring host, const qstring port, const conn_io_req_res* data_getorpost_, type_qh3client_helper_cb async_cb, int retry) {
	std::shared_ptr<qh3_req_obj> req_obj(DEBUG_NEW qh3_req_obj(host, port, data_getorpost_), [](qh3_req_obj* obj) {
		GX_DELETE(obj);
	});
	req_obj->async_cb = async_cb;
	req_obj->retry = retry;

	try {
		std::future<void> future = std::async(std::launch::async, [req_obj]() {
			qh3client_helper::run_internal<T>(req_obj.get());
		});
		future.get(); // Wait for the async task to complete
		DEBUG_RAW(LOG_LEVEL_4, "Future get completed, async task finished");
	} catch (const std::exception& e) {
		DEBUG_PRINT_ERROR(__LOGTAG__, "Exception creating or joining thread: %s", e.what());
		return -1;
	} catch (...) {
		DEBUG_PRINT_ERROR(__LOGTAG__, "Unknown exception creating or joining thread");
		return -2;
	}
    return 0;
}

template <typename T>
int qh3client_helper::send_async_request(const qstring host, const qstring port, const conn_io_req_res* data_getorpost_, void* arg, type_qh3client_helper_cb async_cb, int retry) {
	std::shared_ptr<qh3_req_obj> req_obj(DEBUG_NEW qh3_req_obj(host, port, data_getorpost_), [](qh3_req_obj* obj) {
        GX_DELETE(obj);
    });
	req_obj->async_cb = async_cb;
	req_obj->arg = arg;
	req_obj->retry = retry;
    try {
        std::async(std::launch::async, qh3client_helper::run_internal<T>, req_obj.get());
    } catch (const std::exception& e) {
        DEBUG_PRINT_ERROR(__LOGTAG__, "Exception caught while launching async task: %s", e.what());
        return -1;
    } catch (...) {
        DEBUG_PRINT_ERROR(__LOGTAG__, "Unknown error caught while launching async task");
        return -2;
    }
	return 0;
}
//
template <typename T>
void qh3client_helper::respond_with_empty_response(qh3_req_obj* req_obj, T* client) {
	respond_with_empty_response(const_cast<conn_io_req_res*>(req_obj->data), req_obj->async_cb, req_obj->arg, client);
}

template <typename T>
void qh3client_helper::respond_with_empty_response(conn_io_req_res* request, type_qh3client_helper_cb async_cb, void* arg, T* client) {
	if (async_cb != nullptr) {
		auto empty_response = conn_io_req_res::create();
		async_cb(request, empty_response, client != nullptr ? client->get_client_specific_data() : nullptr, arg, false);
		GX_DELETE(empty_response);
		DEBUG_PRINT(LOG_LEVEL_0, __LOGTAG__, "send_request empty response sent to client");
	}
}

template <typename T>
void* qh3client_helper::run_internal(void* data) {
	qh3_req_obj* req_obj = (qh3_req_obj*) data;
	bool response_received = false;
	for (int x = 0; x < req_obj->retry + 1; x++) {
		if (x > 0) {
			DEBUG_PRINT(LOG_LEVEL_0, __LOGTAG__, "run_internal : retrying - %d", x);
		}
		T* new_client = DEBUG_NEW T(req_obj->host, req_obj->port, req_obj->arg);
		new_client->send_request(req_obj->data, req_obj->async_cb);
		response_received = new_client->conn_io->res_received;
		if (response_received || x == req_obj->retry) {	 // if no response even after last try just return the callback with empty response.
			DEBUG_PRINT(LOG_LEVEL_3, __LOGTAG__, "send_request returned with response_received %d", response_received);
			if (!response_received && req_obj->async_cb != nullptr) {
				respond_with_empty_response(req_obj, new_client);
			}
		}
		new_client->on_post_send_cleanup();
		GX_DELETE(new_client);
		if (response_received) {
			break;
		}
	}
	return nullptr;
}
