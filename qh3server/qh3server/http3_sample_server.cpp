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
    if (conn_io->http_request.path.compare("/whoami")==0){
        conn_io->http_response.clear_payload();
        conn_io->http_response.payload = "{\"name\" : \"http3_sanple_server\"}";
    }
}
