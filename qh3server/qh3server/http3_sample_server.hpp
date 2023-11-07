//
//  http3_sample_server.hpp
//  qh3server
//
//  Created by Arun A on 31/10/23.
//

#ifndef http3_sample_server_hpp
#define http3_sample_server_hpp

#include "qh3server.hpp"
#include "../../servercommon/source/qmongo/qmongo.hpp"

#define SAMPLE_SERVER_SALT "lkfm7q3a"

class http3_sample_server : public qh3server, interface_qmongo_connection {
protected:
    void parse_header(const uint8_t *name, size_t name_len,
                      const uint8_t *value, size_t value_len, struct conn_io *conn_io) override;
    void parse(struct conn_io *conn_io) override;
    
    // mongo callbacks
    void on_mongo_connect() override final;
    void on_mongo_create_index_keys(const char* collection_name, bson_t* indexkey, mongoc_index_opt_t* opt) override final;
    
    qmongo* mongo = nullptr;
    
public:
    http3_sample_server(const char* mongodb_uri);
    ~http3_sample_server();
    
    void test_mongo_db();

};
#endif /* http3_sample_server_hpp */
