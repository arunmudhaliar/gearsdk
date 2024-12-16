//
//  Copyright 2024 homenet25
//  qh3client_helper.cpp
//  qh3client
//
//  Created by Arun A on 04/11/23.
//

#include "qh3client_helper.hpp"

#include <future>  // Include necessary header for std::async

#define PTHREAD_IMPL 1

using namespace client;

template int qh3client_helper::send_async_request<qh3client>(const qstring host, const qstring port, const conn_io_req_res* data_getorpost_, void* arg, type_qh3client_helper_cb async_cb, int retry, float connection_establishment_timeout,
															 type_qh3client_helper_finalize_cb finalize_cb);
template int qh3client_helper::send_request<qh3client>(const qstring host, const qstring port, const conn_io_req_res* data_getorpost_, type_qh3client_helper_cb async_cb, int retry, float connection_establishment_timeout);

#if PLATFORM == PLATFORM_ANDROID
#include "qh3client-android.h"
template int qh3client_helper::send_async_request<qh3client_android>(const qstring host, const qstring port, const conn_io_req_res* data_getorpost_, void* arg, type_qh3client_helper_cb async_cb, int retry,
																	 float connection_establishment_timeout, type_qh3client_helper_finalize_cb finalize_cb = nullptr);
template int qh3client_helper::send_request<qh3client_android>(const qstring host, const qstring port, const conn_io_req_res* data_getorpost_, type_qh3client_helper_cb async_cb, int retry, float connection_establishment_timeout);
#endif

template <typename T>
int qh3client_helper::send_request(const qstring HOST, const qstring PORT, const conn_io_req_res* data_getorpost, type_qh3client_helper_cb async_cb, int retry, float connection_establishment_timeout) {
	std::shared_ptr<qh3_req_obj> req_obj(DEBUG_NEW qh3_req_obj(HOST, PORT, data_getorpost), [](qh3_req_obj* obj) { GX_DELETE(obj); });
	req_obj->async_cb = async_cb;
	req_obj->retry = retry;
	req_obj->connection_establishment_timeout = connection_establishment_timeout;

#if PTHREAD_IMPL
	if (pthread_create(&req_obj->run_thread_id, nullptr, qh3client_helper::run_internal<T>, (void*) thread_data_t::create(req_obj)) < 0) {
		debug_print_error(__LOGTAG__, "could not create thread: %s - %d", strerror(errno), errno);
		return -1;
	}
	pthread_join(req_obj->run_thread_id, nullptr);
#else
	try {
		auto thread_data = thread_data_t::create(req_obj);
		std::future<void> future = std::async(std::launch::async, [thread_data]() { qh3client_helper::run_internal<T>(thread_data); });
		future.get();  // Wait for the async task to complete
		debug_raw(LOG_LEVEL_4, "Future get completed, async task finished");
	} catch (const std::exception& e) {
		debug_print_error(__LOGTAG__, "Exception creating or joining thread: %s", e.what());
		return -1;
	} catch (...) {
		debug_print_error(__LOGTAG__, "Unknown exception creating or joining thread");
		return -2;
	}
#endif
	return 0;
}

template <typename T>
int qh3client_helper::send_async_request(const qstring HOST, const qstring PORT, const conn_io_req_res* data_getorpost, void* arg, type_qh3client_helper_cb async_cb, int retry, float connection_establishment_timeout,
										 type_qh3client_helper_finalize_cb finalize_cb) {
	std::shared_ptr<qh3_req_obj> req_obj(DEBUG_NEW qh3_req_obj(HOST, PORT, data_getorpost), [](qh3_req_obj* obj) { GX_DELETE(obj); });
	req_obj->async_cb = async_cb;
	req_obj->finalize_cb = finalize_cb;
	req_obj->arg = arg;
	req_obj->retry = retry;
	req_obj->connection_establishment_timeout = connection_establishment_timeout;
#if PTHREAD_IMPL
	if (pthread_create(&req_obj->run_thread_id, nullptr, qh3client_helper::run_internal<T>, (void*) thread_data_t::create(req_obj)) < 0) {
		debug_print_error(__LOGTAG__, "could not create thread: %s - %d", strerror(errno), errno);
		return -1;
	}
	// pthread_detach(req_obj->run_thread_id);
#else
	try {
		std::async(std::launch::async, qh3client_helper::run_internal<T>, thread_data_t::create(req_obj));
	} catch (const std::exception& e) {
		debug_print_error(__LOGTAG__, "Exception caught while launching async task: %s", e.what());
		return -1;
	} catch (...) {
		debug_print_error(__LOGTAG__, "Unknown error caught while launching async task");
		return -2;
	}
#endif
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
		debug_print(LOG_LEVEL_3, __LOGTAG__, "send_request empty response sent to client");
	}
}

// // Define cleanup handler
// void qh3client_helper::cleanup_handler(void* arg) {
//     // std::shared_ptr<qh3_req_obj>* req_obj_ptr = static_cast<std::shared_ptr<qh3_req_obj>*>(arg);
//     // Cleanup logic, if necessary
//     debug_raw(LOG_LEVEL_0, "Cleaning up shared_ptr in thread.");
//     // delete req_obj_ptr; // Free the wrapper struct
// }

template <typename T>
void* qh3client_helper::run_internal(void* data) {
	// Register cleanup handler
	// pthread_cleanup_push(qh3client_helper::cleanup_handler, data);
	// std::shared_ptr<qh3_req_obj>& req_obj = *(reinterpret_cast<std::shared_ptr<qh3_req_obj>*>(data));
	thread_data_t* thread_data = reinterpret_cast<thread_data_t*>(data);
	qh3_req_obj* req_obj = thread_data->req_obj.get();

	bool response_received = false;
	for (int x = 0; x < req_obj->retry + 1; x++) {
		if (x > 0) {
			debug_print(LOG_LEVEL_0, __LOGTAG__, "run_internal : retrying - %d", x);
		}
		T* new_client = DEBUG_NEW T(req_obj->host, req_obj->port, req_obj->arg);
		new_client->send_request(req_obj->data, req_obj->async_cb, req_obj->connection_establishment_timeout);
		if (new_client->conn_io) {
			response_received = new_client->conn_io->res_received;
			if (response_received || x == req_obj->retry) {	 // if no response even after last try just return the callback with empty response.
				debug_print(LOG_LEVEL_3, __LOGTAG__, "send_request returned with response_received %d", response_received);
				if (!response_received && req_obj->async_cb != nullptr) {
					respond_with_empty_response(req_obj, new_client);
				}
			}
		}
		new_client->on_post_send_cleanup();
		GX_DELETE(new_client);
		if (response_received) {
			break;
		}
	}
	if (req_obj->finalize_cb != nullptr) {
		req_obj->finalize_cb(req_obj->arg);
	}
	GX_DELETE(thread_data);

	// debug_raw(LOG_LEVEL_0, "Exiting thread");
	// pthread_cleanup_pop(1); // Pop and execute cleanup handler if the second argument is non-zero
	return nullptr;
}
