//
//  http3_sample_server.cpp
//  qh3server
//
//  Created by Arun A on 31/10/23.
//

#include "http3_sample_server.hpp"
#include "../../common/gxcrc32.h"
#include "../../networkcommon/source/qbuffer.hpp"

http3_sample_server::http3_sample_server() {
    mongo = new qmongo(this, "qh3", "db_name");
}

http3_sample_server::~http3_sample_server() {
    GX_DELETE(mongo);
}

void http3_sample_server::parse_header(const uint8_t *name, size_t name_len,
                  const uint8_t *value, size_t value_len, struct conn_io *conn_io) {
    qh3server::parse_header(name, name_len, value, value_len, conn_io);
}

void http3_sample_server::parse(struct conn_io *conn_io) {
    if (conn_io->http_request.path.compare("/whoami")==0) {
        conn_io->http_response.clear_payload();
        conn_io->http_response.payload = "{\"name\" : \"http3_sample_server\"}";
        logger.log(qlogfile::level_0, __LOGTAG__, "%s - whoami - %s", conn_io->http_request.path.c_str(), conn_io->http_response.payload.c_str());
    } else if (conn_io->http_request.path.compare("/user_get")==0) {
        DEBUG_PRINT_IMPORTANT(__LOGTAG__, "user_get - %s", conn_io->http_request.payload.c_str());
        bson_t bson;
        bson_error_t error;
        const char* json = conn_io->http_request.payload.c_str();
        if (!bson_init_from_json (&bson, json, conn_io->http_request.payload.size(), &error)) {
            DEBUG_PRINT_ERROR(__LOGTAG__, "%s", error.message);
            logger.log(qlogfile::level_0, __LOGTAG__, "%s - ERROR - %s", conn_io->http_request.path.c_str(), error.message);
           return;
        }
        
        // parse
        bson_iter_t iter;
        bson_iter_t sub_iter;
        const char* sys_name = nullptr;
        const char* node_name = nullptr;
        const char* release = nullptr;
        const char* arch = nullptr;
        qbuffer buffer;
        qbuffer_writer writer;
        if (bson_iter_init (&iter, &bson) && bson_iter_find_descendant (&iter, "details.sys_name", &sub_iter)) {
            sys_name = bson_iter_utf8 (&sub_iter, NULL);
            if (sys_name) {
                writer.write(buffer, (const uint8_t*)sys_name, strlen(sys_name));
            }
        }
        if (bson_iter_init (&iter, &bson) && bson_iter_find_descendant (&iter, "details.node_name", &sub_iter)) {
            node_name = bson_iter_utf8 (&sub_iter, NULL);
            if (node_name) {
                writer.write(buffer, (const uint8_t*)node_name, strlen(node_name));
            }
        }
        if (bson_iter_init (&iter, &bson) && bson_iter_find_descendant (&iter, "details.release", &sub_iter)) {
            release = bson_iter_utf8 (&sub_iter, NULL);
            if (release) {
                writer.write(buffer, (const uint8_t*)release, strlen(release));
            }
        }
        if (bson_iter_init (&iter, &bson) && bson_iter_find_descendant (&iter, "details.arch", &sub_iter)) {
            arch = bson_iter_utf8 (&sub_iter, NULL);
            if (arch) {
                writer.write(buffer, (const uint8_t*)arch, strlen(arch));
            }
        }
        bson_destroy (&bson);
        
        int crc = gxcrc32::Calc(buffer.data, 0, (int)buffer.index);
        DEBUG_PRINT_IMPORTANT(__LOGTAG__, "user id : %0x", crc);
        
        char user_id[32];
        memset(user_id, 0, sizeof(user_id));
        snprintf(user_id, sizeof(user_id), "%0x", crc);
        
        char user_name[32];
        memset(user_name, 0, sizeof(user_name));
        snprintf(user_name, sizeof(user_name), "guest-%0x", crc);
        
        time_t givemetime = time(NULL);
        char* login_time_str = strtok(ctime(&givemetime), "\n");
        
        // try find the user. (This needs to improve)
        bool found = false;
        bson_t find_query;
        bson_init(&find_query);
        bson_append_utf8(&find_query, "user.pid", strlen("user.pid"), user_id, (int)strlen(user_id));
        mongoc_cursor_t* cursor = mongo->find("users", find_query);
        const bson_t *doc;
        while (mongoc_cursor_next (cursor, &doc)) {
            found = true;
            char* json_string = bson_as_json(doc, nullptr);
            conn_io->http_response.clear_payload();
            conn_io->http_response.payload = json_string;
            bson_free(json_string);
            logger.log(qlogfile::level_0, __LOGTAG__, "%s - user-found - %s", conn_io->http_request.path.c_str(), conn_io->http_response.payload.c_str());
        }
        mongoc_cursor_destroy (cursor);
        
        // if not found try insert. (This needs to improve)
        if (!found) {
            // mongo
            bson_t res_bson;
            bson_init(&res_bson);
            bson_t meta;
            bson_init(&meta);
            bson_append_document_begin (&res_bson, "user", 4, &meta);
            bson_append_utf8(&meta, "pid", 3, user_id, (int)strlen(user_id));
            bson_append_utf8(&meta, "name", 4, user_name, (int)strlen(user_name));
            bson_append_utf8(&meta, "sys_name", strlen("sys_name"), sys_name, (int)strlen(sys_name));
            bson_append_utf8(&meta, "node_name", strlen("node_name"), node_name, (int)strlen(node_name));
            bson_append_utf8(&meta, "arch", strlen("arch"), arch, (int)strlen(arch));
            bson_append_utf8(&meta, "last_login", strlen("last_login"), login_time_str, (int)strlen(login_time_str));
            bson_append_document_end (&res_bson, &meta);
            if (mongo->insert("users", res_bson) == EXIT_SUCCESS) {
                char* json_string = bson_as_json(&res_bson, nullptr);
                conn_io->http_response.clear_payload();
                conn_io->http_response.payload = json_string;
                bson_free(json_string);
                logger.log(qlogfile::level_0, __LOGTAG__, "%s - new-user - %s", conn_io->http_request.path.c_str(), conn_io->http_response.payload.c_str());
            } else {
                logger.log(qlogfile::level_0, __LOGTAG__, "%s - new-user failed - %s", conn_io->http_request.path.c_str(), conn_io->http_response.payload.c_str());
            }
            bson_destroy(&meta);
            bson_destroy (&res_bson);
        }
        
        bson_t* update = BCON_NEW ("$set", "{", "user.last_login", BCON_UTF8 (login_time_str), "}");
        if (mongo->update("users", find_query, *update) == EXIT_SUCCESS) {
            logger.log(qlogfile::level_0, __LOGTAG__, "%s - user-last-login - %s, pid:%s", login_time_str, user_id);
        } else {
            logger.log(qlogfile::level_0, __LOGTAG__, "%s - %s", conn_io->http_request.path.c_str(), user_id);
        }
        bson_destroy(update);
        bson_destroy (&find_query);
        //
    }
}

void http3_sample_server::test_mongo_db() {
    qmongo mongo(this, "qh3", "db_name");
    
    bson_t bson;
    bson_t meta;
    bson_append_document_begin (&bson, "meta", 4, &meta);
    bson_append_utf8(&meta, "pid", 3, "mypid1", 6);
    bson_append_utf8(&meta, "name", 4, "myname", 6);
    bson_append_int32(&meta, "age", 3, 40);
    bson_append_utf8(&meta, "loc", 3, "palakkad", 8);
    bson_append_document_end (&bson, &meta);
    
    mongo.insert("users", bson);
    bson_destroy (&bson);
}

void http3_sample_server::on_mongo_connect() {

}
void http3_sample_server::on_mongo_create_index_keys(const char* collection_name, bson_t* indexkey, mongoc_index_opt_t* opt) {
    BSON_APPEND_INT32(indexkey, "user.pid", 1);
    char *idx2_name = mongoc_collection_keys_to_index_string(indexkey);
    assert(strcmp(idx2_name, "user.pid_1") == 0);
    mongoc_index_opt_init(opt);
    opt->unique = true;
}
