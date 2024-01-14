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
#include "../../qhiredis/source/qhiredis.hpp"
#include "../../networkcommon/source/qtextfile.hpp"
#include "../../qzookeeper/source/qzookeeper.hpp"

#define SAMPLE_SERVER_SALT "lkfm7q3a"

#undef __LOGTAG__
#define __LOGTAG__ "http3_sample_server"

class http3_sample_server : public qh3server, interface_qmongo_connection {
protected:
    void parse_header(const qstring& name, const qstring& value, struct conn_io *conn_io) override;
    void parse(struct conn_io *conn_io) override;
    inline bool is_log_quiche() override;
    
    bool on_server_pre_init() override;
    void on_run_started() override;
    void on_run_end() override;
    
    // mongo callbacks
    void on_mongo_connect() override final;
    void on_mongo_create_index_keys(const qstring& collection_name, bson_t* indexkey, mongoc_index_opt_t* opt) override final;
    
    qmongo* mongo = nullptr;
    qhiredis* hiredis = nullptr;
    qzookeeper* qzk = nullptr;
public:
    http3_sample_server(const qstring& mongodb_uri, const qstring& redis_url, int redis_port, const qstring& zk_uri);
    ~http3_sample_server();
    
    void test_mongo_db();
    
private:
    void parse_shutdown_test(conn_io_req_res::header* path_header, struct conn_io *conn_io);
    void parse_whoami(conn_io_req_res::header* path_header, struct conn_io *conn_io);
    void parse_user_get(conn_io_req_res::header* path_header, struct conn_io *conn_io);
    void parse_user_details(conn_io_req_res::header* path_header, struct conn_io *conn_io);
    
    qstring zk_uri;
    
#if TEST_RESPONSE
    qstring test_response;
#endif
};
#endif /* http3_sample_server_hpp */
