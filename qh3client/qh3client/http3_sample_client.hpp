//
//  http3_sample_client.hpp
//  qh3client
//
//  Created by Arun A on 03/11/23.
//

#ifndef http3_sample_client_hpp
#define http3_sample_client_hpp

#include "qh3client_helper.hpp"

#define SAMPLE_SERVER_SALT "lkfm7q3a"

class http3_sample_client : public qtimer_sceduler {
public:
    http3_sample_client(const qstring& host, const qstring& port);
    ~http3_sample_client();
    void init_connection();
    void on_login_complete(const qstring& token, bool result);

private:
    qtimer* keep_alive_loop = nullptr;
    qstring host = "192.168.0.230";
    qstring port = "4004";
    static int live_connections;
    static int total_connections_returned;
    void create_connections();

    qstring session_token;
    qstring pid;
};
#endif /* http3_sample_client_hpp */
