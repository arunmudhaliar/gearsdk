//
//  qunityplugin.cpp
//  qunityplugin
//
//  Created by Arun A on 24/01/24.
//

#include "qunitysdk.hpp"

using namespace qunitysdk;

qsocket::qsocket(type_qsocket_destroy_qsocket cb) : cb_destroy_qsocket(cb) {
    
}
qsocket::~qsocket() {
    DEBUG_PRINT(LOG_LEVEL_0, __LOGTAG__, "native qsocket destroyed !!!");
}

bool qsocket::connect(const char* host, const char* port, void* arg,
                 type_qsocket_onconnect cb_connect, type_qsocket_onmessage cb_message,
                 type_qsocket_onreleaseconnection cb_release_connection, type_qsocket_onclose cb_close) {
    this->cb_connect = cb_connect;
    this->cb_message = cb_message;
    this->cb_release_connection = cb_release_connection;
    this->cb_close = cb_close;
    if (run(host, port) != 0) {
        return false;
    }
    return true;
}

void qsocket::onconnect(conn_io_client* qconnection) {
    cb_connect();
}
void qsocket::onmessage(ssize_t recv_len, uint8_t* buf, conn_io_client* qconnection) {
    DEBUG_PRINT(LOG_LEVEL_0, __LOGTAG__, "native onmessage %.*s", recv_len, buf);
    cb_message(recv_len, buf);
}
void qsocket::onreleaseconnection(conn_io_client* qconnection) {
    cb_release_connection();
    cb_destroy_qsocket(this);
}
void qsocket::onclose(conn_io_client* qconnection) {
    cb_close();
}

extern "C" {
int send_async_request(const char* host, const char* port,
                                     const char* path, const char* payload, void* arg, type_qh3client_plugin_helper_cb callback) {
    DEBUG_PRINT(LOG_LEVEL_0, __LOGTAG__, "host %s, port %s, path %s, payload %s", host, port, path, payload);
    return qh3client_helper::send_async_request<client::qh3client>(host, port, conn_io_req_res::create(path, payload), arg,
            [callback](conn_io_req_res *response, void* client_specific_data, void* arg) {
                const conn_io_req_res::payload &payload = response->data;
                callback(payload.buffer.c_str(), arg, 0);
                DEBUG_PRINT(LOG_LEVEL_0, __LOGTAG__, "payload %s", payload.buffer.c_str());
            });
}

void destroy_qsocket(qsocket* qs) {
    std::map<unsigned long, qsocket*>::iterator it_qsocket;
    for(std::map<unsigned long, qsocket*>::iterator it = qsockets.begin(); it!=qsockets.end(); it++) {
        if (it->second == qs) {
            it_qsocket = it;
        }
    }
    
    if (it_qsocket!=qsockets.end()) {
        // GX_DELETE(it_qsocket->second); // do not delete since the qsocket get destroyed internally.
        DEBUG_PRINT(LOG_LEVEL_0, __LOGTAG__, "qsocket removed from the list !!!");
        qsockets.erase(it_qsocket);
    }
}

int qsocket_sendMessage(const char* guid, int guid_len, const char* buffer, unsigned long size, bool flush) {
    if (guid == nullptr) {
        DEBUG_WARN(LOG_LEVEL_0, __LOGTAG__, "qsocket_sendMessage failed - guid is null");
        return -1;
    }
    
    DEBUG_PRINT(LOG_LEVEL_0, __LOGTAG__, "try send %.*s", size, buffer);
    
    unsigned long crc = essentials::get_crc((const uint8_t*)guid, guid_len);
    std::map<unsigned long, qsocket*>::iterator it = qsockets.find(crc);
    if (it==qsockets.end()) {
        DEBUG_WARN(LOG_LEVEL_0, __LOGTAG__, "qsocket_sendMessage failed - guid not exist %.*s", guid_len, guid);
        return -1;
    }
    return it->second->sendMessage("qstring(buffer, size)", flush);   // TODO (amudaliar) - have to optimise. qstring will do a copy
}

bool qsocket_connect(const char* guid, int guid_len, const char* host, const char* port, void* arg,
                     qsocket::type_qsocket_onconnect cb_connect, qsocket::type_qsocket_onmessage cb_message,
                     qsocket::type_qsocket_onreleaseconnection cb_release_connection, qsocket::type_qsocket_onclose cb_close) {
    DEBUG_WARN(LOG_LEVEL_0, __LOGTAG__, "qsocket_connect guid - %s", guid);
    if (guid == nullptr) {
        DEBUG_WARN(LOG_LEVEL_0, __LOGTAG__, "qsocket_connect failed - guid is null");
        return false;
    }
    unsigned long crc = essentials::get_crc((const uint8_t*)guid, guid_len);
    if (qsockets.find(crc)!=qsockets.end()) {
        DEBUG_WARN(LOG_LEVEL_0, __LOGTAG__, "qsocket_connect failed - guid already exist %.*s", guid_len, guid);
        return false;
    }
    qsocket* newsocket = DEBUG_NEW qsocket(destroy_qsocket);
    if (!newsocket->connect(host, port, arg, cb_connect, cb_message, cb_release_connection, cb_close)) {
        GX_DELETE(newsocket);
    }
    qsockets[crc] = newsocket;
    return true;
}

bool qsocket_is_run_finished(const char* guid, int guid_len) {
    if (guid == nullptr) {
        DEBUG_WARN(LOG_LEVEL_0, __LOGTAG__, "qsocket_is_run_finished - guid is null");
        return false;
    }
    unsigned long crc = essentials::get_crc((const uint8_t*)guid, guid_len);
    std::map<unsigned long, qsocket*>::iterator it = qsockets.find(crc);
    if (it==qsockets.end()) {
        DEBUG_WARN(LOG_LEVEL_0, __LOGTAG__, "qsocket_is_run_finished - guid not found %.*s", guid_len, guid);
        return false;
    }
    return it->second->is_runfinished();
}

void destroy_finished_qsockets() {
    std::vector<unsigned long> finishedList;
    for(std::map<unsigned long, qsocket*>::iterator it = qsockets.begin(); it!=qsockets.end(); it++) {
        if (it->second->is_runfinished()) {
            finishedList.push_back(it->first);
        }
    }

    for (auto it = finishedList.cbegin();it != finishedList.cend();it++) {
        unsigned long guid_crc = *it;
        std::map<unsigned long, qsocket*>::iterator it_qsocket = qsockets.find(guid_crc);
        if (it_qsocket!=qsockets.end()) {
            int oldSz = (int)qsockets.size();
            qsockets.erase(it_qsocket);
            if (oldSz != qsockets.size()) {
                DEBUG_PRINT(LOG_LEVEL_0, __LOGTAG__, "destroy_finished_qsockets - qsocket for guid_crc %x deleted !!!", guid_crc);
                GX_DELETE(it_qsocket->second);
            }
        }
    }
}

}
