//
//  http3_sample_server.cpp
//  qh3server
//
//  Created by Arun A on 31/10/23.
//

#include "http3_sample_server.hpp"
#include "../../common/gxcrc32.h"
#include "../../networkcommon/source/qbuffer.hpp"
#include "../../common/crypto_helper.hpp"

http3_sample_server::http3_sample_server(const char* mongodb_uri) {
    mongo = DEBUG_NEW qmongo(this, "qh3", "db_name", mongodb_uri);
    hiredis = DEBUG_NEW qhiredis();
    hiredis->connect_redis();
}

http3_sample_server::~http3_sample_server() {
    GX_DELETE(hiredis);
    GX_DELETE(mongo);
}

void http3_sample_server::on_run_started() {
    
}

void http3_sample_server::on_run_end() {
    
}

void http3_sample_server::parse_header(const qstring& name, const qstring& value, struct conn_io *conn_io) {
    qh3server::parse_header(name, value, conn_io);
}

void http3_sample_server::parse(struct conn_io *conn_io) {
    conn_io_req_res::header* path_header = conn_io->http_request->get_header(":path");
    if (path_header == nullptr) {
        qh3server::get_stats_loggeer()->server_count("parse", "", "", "", "error", "http3_sample_server", "", "path_not_found");
        return;
    }
    if (path_header->value.length()>1 && path_header->value.compare("/shutdown-test")==0) {
        bool validate = conn_io->http_request->validate();
        assert(validate);
        ev_break(conn_io->bridge->get_mainloop(), EVBREAK_ONE);
        return;
    }
    
    const conn_io_req_res::payload& payload = conn_io->http_request->get_payload();
    
    if (path_header->value.length()>1 && path_header->value.compare("/whoami")==0) {
        bool validate = conn_io->http_request->validate();
        assert(validate);
        if (!validate) {
            qh3server::get_stats_loggeer()->server_count("parse", "", "", "", "error", "http3_sample_server", path_header->value.c_str(), "crc_fail");
        }
        const char* res_string = "{\"name\" : \"http3_sample_server\"}";
        conn_io->http_response->set_payload(qstring(res_string, strlen(res_string)));
        qh3server::get_file_logger()->log(qlogfile::level_0, __LOGTAG__, "%s - whoami - %s", path_header->value.c_str(), res_string);
        qh3server::get_stats_loggeer()->server_count("parse", "", "", "", "hit", "http3_sample_server", path_header->value.c_str());
    } else if (path_header->value.length()>1 && path_header->value.compare("/user_get")==0) {
        bool validate = conn_io->http_request->validate();
        assert(validate);
        if (!validate) {
            qh3server::get_stats_loggeer()->server_count("parse", "", "", "", "error", "http3_sample_server", path_header->value.c_str(), "crc_fail");
        }
//        DEBUG_PRINT_IMPORTANT(__LOGTAG__, "user_get - %s", conn_io->http_request.payload.c_str());
        bson_t bson;
        bson_error_t error;
        if (!bson_init_from_json (&bson, payload.buffer.c_str(), payload.buffer.length(), &error)) {
            DEBUG_PRINT_ERROR(__LOGTAG__, "%s", error.message);
            qh3server::get_file_logger()->log(qlogfile::level_0, __LOGTAG__, "%s - ERROR - %s", path_header->value.c_str(), error.message);
            qh3server::get_stats_loggeer()->server_count("parse", "", "", "", "error", "http3_sample_server", path_header->value.c_str(), "payload_deserialise_fail");
           return;
        }
        
        // parse
        bson_iter_t iter;
        bson_iter_t sub_iter;
        qstring sys_name;
        qstring node_name;
        qstring release;
        qstring arch;
        qbuffer buffer;
        buffer.allocate(128);
        qbuffer_writer writer;
        if (bson_iter_init (&iter, &bson) && bson_iter_find_descendant (&iter, "details.sys_name", &sub_iter)) {
            sys_name = bson_iter_utf8 (&sub_iter, NULL);
            if (sys_name.length()) {
                writer.write(buffer, sys_name);
            }
        }
        if (bson_iter_init (&iter, &bson) && bson_iter_find_descendant (&iter, "details.node_name", &sub_iter)) {
            node_name = bson_iter_utf8 (&sub_iter, NULL);
            if (node_name.length()) {
                writer.write(buffer, node_name);
            }
        }
        if (bson_iter_init (&iter, &bson) && bson_iter_find_descendant (&iter, "details.release", &sub_iter)) {
            release = bson_iter_utf8 (&sub_iter, NULL);
            if (release.length()) {
                writer.write(buffer, release);
            }
        }
        if (bson_iter_init (&iter, &bson) && bson_iter_find_descendant (&iter, "details.arch", &sub_iter)) {
            arch = bson_iter_utf8 (&sub_iter, NULL);
            if (arch.length()) {
                writer.write(buffer, arch);
            }
        }
        bson_destroy (&bson);
        
        unsigned long  crc = crc32(0L, Z_NULL, 0);
        crc = crc32_z(crc, buffer.data, buffer.index);
        writer.write(buffer, crc);
        DEBUG_PRINT_IMPORTANT(__LOGTAG__, "user id : %x", crc);
        
        qstring pid;
        pid.format("%lx", crc);
        qstring user_name;
        user_name.format("guest-%lx", crc);
        
        time_t givemetime = time(NULL);
        qstring login_time_str(strtok(ctime(&givemetime), "\n"));
        writer.write(buffer, login_time_str);
        
        // calcualte sha
        crypto_helper::sha256_data sha_data((const char*)buffer.data, (int)buffer.index);
        crypto_helper::sha256(sha_data);
        // session token header
        qstring token(sha_data.out, strlen(sha_data.out));
        conn_io->http_response->add_or_get_header("token", token);
        
        // try find the user. (This needs to improve)
        bool found = false;
        bson_t find_query;
        bson_init(&find_query);
        bson_append_utf8(&find_query, "user.pid", strlen("user.pid"), pid.c_str(), (int)pid.length());
        mongoc_cursor_t* cursor = mongo->find("users", find_query);
        const bson_t *doc = nullptr;
        while (mongoc_cursor_next(cursor, &doc)) {
            found = true;
            char* json_string = bson_as_json(doc, nullptr);
            conn_io->http_response->set_payload(qstring(json_string, strlen(json_string)));
            qh3server::get_file_logger()->log(qlogfile::level_0, __LOGTAG__, "%s - user-found - %s", path_header->value.c_str(), json_string);
            bson_free(json_string);
        }
        mongoc_cursor_destroy(cursor);
        
        // if not found try insert. (This needs to improve)
        if (!found) {
            // mongo
            bson_t res_bson;
            bson_init(&res_bson);
            bson_t meta;
            bson_init(&meta);
            bson_append_document_begin (&res_bson, "user", 4, &meta);
            bson_append_utf8(&meta, "pid", strlen("pid"), pid.c_str(), (int)pid.length());
            bson_append_utf8(&meta, "name", strlen("name"), user_name.c_str(), (int)user_name.length());
            bson_append_utf8(&meta, "sys_name", strlen("sys_name"), sys_name.c_str(), (int)sys_name.length());
            bson_append_utf8(&meta, "node_name", strlen("node_name"), node_name.c_str(), (int)node_name.length());
            bson_append_utf8(&meta, "arch", strlen("arch"), arch.c_str(), (int)arch.length());
            bson_append_utf8(&meta, "last_login", strlen("last_login"), login_time_str.c_str(), (int)login_time_str.length());
            bson_append_document_end (&res_bson, &meta);
            if (mongo->insert("users", res_bson) == EXIT_SUCCESS) {
                char* json_string = bson_as_json(&res_bson, nullptr);
                conn_io->http_response->set_payload(qstring(json_string, strlen(json_string)));
                qh3server::get_file_logger()->log(qlogfile::level_0, __LOGTAG__, "%s - new-user - %s", path_header->value.c_str(), json_string);
                bson_free(json_string);
            } else {
                qh3server::get_file_logger()->log(qlogfile::level_0, __LOGTAG__, "%s - new-user failed", path_header->value.c_str());
            }
            bson_destroy(&meta);
            bson_destroy (&res_bson);
        }
        
        bson_t* update = BCON_NEW ("$set", "{", "user.last_login", BCON_UTF8 (login_time_str.c_str()), "}");
        if (mongo->update("users", find_query, *update) == EXIT_SUCCESS) {
            qh3server::get_file_logger()->log(qlogfile::level_0, __LOGTAG__, "user-last-login - %s, pid:%s", login_time_str.c_str(), pid.c_str());
        } else {
            qh3server::get_file_logger()->log(qlogfile::level_0, __LOGTAG__, "%s - %s", path_header->value.c_str(), pid.c_str());
        }
        bson_destroy(update);
        bson_destroy (&find_query);
        //
        
        // set session token on redis
        hiredis->set_value(pid, token, 5*60);   // 5 minutes
        //
        
        qh3server::get_stats_loggeer()->server_count("parse", "", "", "", "hit", "http3_sample_server", path_header->value.c_str());
    } else if (path_header->value.length()>1 && path_header->value.compare("/user_details")==0) {
        bool validate = conn_io->http_request->validate();
        assert(validate);
        if (!validate) {
            qh3server::get_stats_loggeer()->server_count("parse", "", "", "", "error", "http3_sample_server", path_header->value.c_str(), "crc_fail");
        }
        conn_io_req_res::header* token_header = conn_io->http_request->get_header("token");
        if (token_header == nullptr) {
            return;
        }
        
        bson_t bson;
        bson_error_t error;
        if (!bson_init_from_json (&bson, payload.buffer.c_str(), payload.buffer.length(), &error)) {
            DEBUG_PRINT_ERROR(__LOGTAG__, "%s", error.message);
            qh3server::get_file_logger()->log(qlogfile::level_0, __LOGTAG__, "%s - ERROR - %s", path_header->value.c_str(), error.message);
            qh3server::get_stats_loggeer()->server_count("parse", "", "", "", "error", "http3_sample_server", path_header->value.c_str(), "payload_deserialise_fail");
           return;
        }
        
        // parse
        bson_iter_t iter;
        bson_iter_t sub_iter;
        qstring pid;
        qbuffer buffer;
        buffer.allocate(128);
        qbuffer_writer writer;
        if (bson_iter_init (&iter, &bson) && bson_iter_find_descendant (&iter, "user.pid", &sub_iter)) {
            pid = bson_iter_utf8 (&sub_iter, NULL);
            if (pid.length()) {
                writer.write(buffer, pid);
            }
        } else {
            bson_destroy (&bson);
            return;
        }
        bson_destroy (&bson);

        // check with redis
        qstring token_in_redis;
        hiredis->get_value(pid, token_in_redis);
        if (token_in_redis==token_header->value) {
            DEBUG_PRINT(LOG_LEVEL_0, __LOGTAG__, "Valid user");
        } else {
            DEBUG_PRINT_ERROR(__LOGTAG__, "NOT a Valid user !!!");
        }
    }
}

void http3_sample_server::test_mongo_db() {
    qmongo mongo(this, "qh3", "test", "mongodb://192.168.0.230:6006");
    
    bson_t bson;
    bson_t meta;
    bson_init(&bson);
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
void http3_sample_server::on_mongo_create_index_keys(const qstring& collection_name, bson_t* indexkey, mongoc_index_opt_t* opt) {
    UNUSED(collection_name);
    BSON_APPEND_INT32(indexkey, "user.pid", 1);
    char *idx2_name = mongoc_collection_keys_to_index_string(indexkey);
    assert(strcmp(idx2_name, "user.pid_1") == 0);
    if (opt) {
        opt->unique = true;
//        BSON_APPEND_BOOL (opt, "unique", true);
    }
}
