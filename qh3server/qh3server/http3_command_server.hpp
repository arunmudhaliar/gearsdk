//
//  http3_command_server.hpp
//  qh3server
//
//  Created by Arun A on 31/10/23.
//

#ifndef http3_command_server_hpp
#define http3_command_server_hpp

#include "qh3server.hpp"
#include "../../qhiredis/source/qhiredis.hpp"

#undef __LOGTAG__
#define __LOGTAG__ "http3_command_server"

class http3_command_server : public qh3server {
protected:
    void parse_header(const qstring& name, const qstring& value, struct conn_io *conn_io) override;
    void parse(struct conn_io *conn_io) override;
    inline bool is_log_quiche() override;
    
    void on_run_started() override;
    void on_run_end() override;
    
    qhiredis* hiredis = nullptr;
public:
    http3_command_server(const qstring& redis_url, int redis_port);
    ~http3_command_server();
    
private:
    void parse_shutdown_test(conn_io_req_res::header* path_header, struct conn_io *conn_io);
    void parse_whoami(conn_io_req_res::header* path_header, struct conn_io *conn_io);
};
#endif /* http3_command_server_hpp */
