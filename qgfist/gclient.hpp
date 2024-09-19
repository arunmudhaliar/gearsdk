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
#include <uv.h>
#include <functional>
#include <tuple>

#undef __LOGTAG__
#define __LOGTAG__ "gclient"

using namespace client;

class gclient;
typedef std::function<void(gclient* c, conn_io_client* qconnection)> type_qclient_onconnect_cb;
typedef std::function<void(gclient* c, ssize_t recv_len, uint8_t* buf, conn_io_client* qconnection)> type_qclient_onmessage_cb;
typedef std::function<void(gclient* c, conn_io_client* qconnection)> type_qclient_onreleaseconnection_cb;
typedef std::function<void(gclient* c, conn_io_client* qconnection)> type_qclient_onclose_cb;

class gclient : public qnetworkclient {
   public:
   gclient(uv_loop_t* loop, type_qclient_onconnect_cb onconnect_cb, 
      type_qclient_onmessage_cb onmessage_cb, 
      type_qclient_onreleaseconnection_cb onreleaseconnection_cb, 
      type_qclient_onclose_cb onclose_cb, void* arg = nullptr);
	virtual ~gclient();

   void* get_user_data() const { return user_data; }
   
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

   static void async_qclient_onconnect_cb(uv_async_t* handle);
   static void async_qclient_onmessage_cb(uv_async_t* handle);
   static void async_qclient_onreleaseconnection_cb(uv_async_t* handle);
   static void async_qclient_onclose_cb(uv_async_t* handle);
   
   uv_async_t async_onconnect;
	uv_async_t async_onmessage;
	uv_async_t async_onreleaseconnection;
	uv_async_t async_onclose;
   void* user_data = nullptr;
};

#endif /* gclient_hpp */
