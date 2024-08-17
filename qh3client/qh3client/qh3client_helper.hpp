//
//  Copyright 2024 homenet25
//  qh3client_helper.hpp
//  qh3client
//
//  Created by Arun A on 04/11/23.
//

#ifndef qh3client_helper_hpp
#define qh3client_helper_hpp

#include "qh3client.hpp"

#include <functional>
#include <pthread.h>

#undef __LOGTAG__
#define __LOGTAG__ "qh3client_helper"

namespace client {
class qh3client_helper {
   public:
	struct qh3_req_obj {
		qh3_req_obj(const qstring host, const qstring port, const conn_io_req_res* data) : host(host), port(port), data(data) {}
		~qh3_req_obj() { GX_DELETE(data); }
		qstring host;
		qstring port;
		const conn_io_req_res* data = nullptr;
		pthread_t run_thread_id;
		type_qh3client_helper_cb async_cb = nullptr;
		type_qh3client_helper_finalize_cb finalize_cb = nullptr;
		void* arg = nullptr;
		int retry = 0;
	};

	struct thread_data_t {
		std::shared_ptr<qh3_req_obj> req_obj;
		thread_data_t(std::shared_ptr<qh3_req_obj> req) : req_obj(req) {}
		static thread_data_t* create(std::shared_ptr<qh3_req_obj> req) { return DEBUG_NEW thread_data_t(req); }
	};

	template <typename T>
	static int send_request(const qstring host, const qstring port, const conn_io_req_res* data_getorpost_, type_qh3client_helper_cb async_cb, int retry);
	template <typename T>
	static int send_async_request(const qstring host, const qstring port, const conn_io_req_res* data_getorpost_, void* arg, type_qh3client_helper_cb async_cb, int retry, type_qh3client_helper_finalize_cb finalize_cb = nullptr);

   private:
	template <typename T>
	static void* run_internal(void* data);
	template <typename T>
	static void respond_with_empty_response(conn_io_req_res* request, type_qh3client_helper_cb async_cb, void* arg, T* client);
	template <typename T>
	static void respond_with_empty_response(qh3_req_obj* req_obj, T* client);
	// static void cleanup_handler(void* arg);
};
};	// namespace client
#endif /* qh3client_helper_hpp */
