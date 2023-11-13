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
    http3_sample_client();
    ~http3_sample_client();
    
    void init_connection();
    
private:
    qtimer* keep_alive_loop = nullptr;
};
#endif /* http3_sample_client_hpp */
