//
//  http3_sample_server.hpp
//  qh3server
//
//  Created by Arun A on 31/10/23.
//

#ifndef http3_sample_server_hpp
#define http3_sample_server_hpp

#include "qh3server.hpp"

class http3_sample_server : public qh3server {
protected:
    void parse_header(const uint8_t *name, size_t name_len,
                      const uint8_t *value, size_t value_len, struct conn_io *conn_io) override;
    void parse(struct conn_io *conn_io) override;
};
#endif /* http3_sample_server_hpp */
