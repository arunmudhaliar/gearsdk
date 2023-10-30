//
//  main.cpp
//  qh3server
//
//  Created by Arun A on 30/10/23.
//

#include "http3_sample_server.hpp"

static std::string version_string = "0.1";
static unsigned version_code = 1;

int main(int argc, const char *argv[]) {
    std::string host = "localhost";
    std::string port = "6121";
    fs::path rootDir;
    essentials::resolve_cmd_line_args(__LOGTAG__, argc, argv,
                          version_string, version_code,
                          host, port, rootDir);
    http3_sample_server server;
    server.run(host, port, rootDir);
    
    return 0;
}
