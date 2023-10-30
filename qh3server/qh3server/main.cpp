//
//  main.cpp
//  qh3server
//
//  Created by Arun A on 30/10/23.
//

#include "qh3server.hpp"

int main(int argc, const char *argv[]) {
    std::string host = "localhost";
    std::string port = "4004";
    
    qh3server::run(host, port);
    
    return 0;
}
