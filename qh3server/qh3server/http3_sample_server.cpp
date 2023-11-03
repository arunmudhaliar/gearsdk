//
//  http3_sample_server.cpp
//  qh3server
//
//  Created by Arun A on 31/10/23.
//

#include "http3_sample_server.hpp"

void http3_sample_server::parse_header(const uint8_t *name, size_t name_len,
                  const uint8_t *value, size_t value_len, struct conn_io *conn_io) {
    qh3server::parse_header(name, name_len, value, value_len, conn_io);
}

void http3_sample_server::parse(struct conn_io *conn_io) {
    if (conn_io->http_request.path.compare("/whoami")==0) {
        conn_io->http_response.clear_payload();
        conn_io->http_response.payload = "{\"name\" : \"http3_sanple_server\"}";
    } else if (conn_io->http_request.path.compare("/user_get")==0) {
        DEBUG_PRINT_IMPORTANT(__LOGTAG__, "user_get - %s", conn_io->http_request.payload.c_str());
        bson_t bson;
        bson_error_t error;
        const char* json = conn_io->http_request.payload.c_str();
        if (!bson_init_from_json (&bson, json, conn_io->http_request.payload.size(), &error)) {
            DEBUG_PRINT_ERROR(__LOGTAG__, "%s", error.message);
           return;
        }
        
        // parse
        //
        
        bson_destroy (&bson);
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
void http3_sample_server::on_mongo_create_index_keys(const char* collection_name, bson_t& indexkey, mongoc_index_opt_t& opt) {
    BSON_APPEND_INT32 (&indexkey, "meta.pid", 1);
    char *idx2_name = mongoc_collection_keys_to_index_string (&indexkey);
    assert(strcmp (idx2_name, "meta.pid_1") == 0);
    mongoc_index_opt_init (&opt);
    opt.unique = true;
}
