//
//  http3_sample_client.hpp
//  qh3client
//
//  Created by Arun A on 03/11/23.
//

#ifndef http3_sample_client_hpp
#define http3_sample_client_hpp

#include "qh3client.hpp"

class http3_sample_client : public qh3client {
public:
    http3_sample_client(const std::string& host, const std::string& port);
    ~http3_sample_client();
    
    void init_connection();
};
#endif /* http3_sample_client_hpp */
