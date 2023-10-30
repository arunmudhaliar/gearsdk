//
//  main.cpp
//  qh3client
//
//  Created by Arun A on 30/10/23.
//

#include "qh3client.hpp"

int main(int argc, const char *argv[]) {
    std::string host = "localhost";
    std::string port = "4004";
    
    qh3client client(host, port);
    client.send_request(getorpost_reqdata("/whoami", "{}"));
    
//    client.get(getorpost_data("/"));
    
    return 0;
}

