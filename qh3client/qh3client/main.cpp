//
//  main.cpp
//  qh3client
//
//  Created by Arun A on 30/10/23.
//

#include "http3_sample_client.hpp"

int main(int argc, const char *argv[]) {
//    std::string host = "192.168.0.230";


    http3_sample_client client;
    client.init_connection();
    return 0;
}

