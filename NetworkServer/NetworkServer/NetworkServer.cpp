//
//  main.cpp
//  GNetwork
//
//  Created by Arun A on 20/09/23.
//
#include "NetworkServer.hpp"
#include "GameServer.hpp"


#undef __LOGTAG__
#define __LOGTAG__ "NetworkServer"

#if PLATFORM == PLATFORM_LINUX
#include <linux/limits.h>
#endif
#include <netdb.h>

#if 0
#include <unistd.h>
#include "../../Common/gxCrc32.h"

#define NETWORK_PROTOCOL_ID 0x7f8c

NetworkServer::NetworkServer() : NetworkInterface() {
    
}

NetworkServer::~NetworkServer() {
    
}


void NetworkServer::OnReceivePayLoad(const UDPSocket* socket, Address& sender, const PayLoad* payload) {
    int random = 120;//std::min(250, std::max(85, rand()%250));
    usleep(random*2000);    // 10ms
    SendPayload(sender, new PayLoad(*payload));
}

bool NetworkServer::OnConstructPacket(const UDPSocket* socket, Address& sender, UDPPacket* packet, Buffer& buffer) {
    uint16_t type = 0;
    BufferReader reader;
    buffer.Stash();
    reader.Read( buffer, type);
    
    PayLoad* payload = nullptr;
    switch (type) {
        case PayLoad::TypeID: {
            payload = new PayLoad(type);
        }
        break;
        case SimplePayLoad::TypeID: {
            payload = new SimplePayLoad();
        }
        break;
        default:
            buffer.Pop();
            return false;
    }
    payload->Read(buffer);
    GX_DELETE(packet->payload);
    packet->payload = payload;
    return true;
}
////    long bytesSent = GetSocket().Send( sender, data, (int)size );
//    long bytesSent = SendPayload(sender, PayLoad &payload)
//    DEBUG_PRINT(LOG_LEVEL_1, __LOGTAG__, "bytesSent %ld", bytesSent);
#endif

int32_t main(int32_t argc, const char * argv[]) {
    PrintCommonInfo();
    
    std::string host = "localhost";
    std::string port = "4000";
    if (argc==3) {
        host = argv[1];
        port = argv[2];
        const struct addrinfo hints = {
            .ai_family = PF_UNSPEC,
            .ai_socktype = SOCK_DGRAM,
            .ai_protocol = IPPROTO_UDP
        };
        struct addrinfo *peer = nullptr;
        if (getaddrinfo(host.c_str(), port.c_str(), &hints, &peer) != 0) {
            DEBUG_PRINT_ERROR(__LOGTAG__, "Failed to resolve host. Exiting !!!");
            DEBUG_PRINT_IMPORTANT2(__LOGTAG__, "Usage : <executable> 'ip address' 'port'");
            return -1;
        }
        if (peer) {
            freeaddrinfo(peer);
            peer = nullptr;
        }
    } else {
        DEBUG_PRINT_IMPORTANT2(__LOGTAG__, "Usage : <executable> 'ip address' 'port'. Ignore for debug builds running locally.");
    }
    DEBUG_PRINT_IMPORTANT(__LOGTAG__, "host:%s, port:%s", host.c_str(), port.c_str());
    
    
    fs::path executablePath(argc>0 ? argv[0] : "");
    fs::path rootDir = executablePath.parent_path();
    DEBUG_PRINT_IMPORTANT(__DEFAULT_LOG_TAG__, "Root dir : %s", rootDir.c_str());
    GameServer server;
    server.run(host, port, rootDir);
    
    /*
     PrintCommonInfo();
    // insert code here...
    DEBUG_PRINT(LOG_LEVEL_1, __LOGTAG__, "Server Program, running...");
    
    UDPSocket socket;
    
    if ( !socket.Open(nullptr) )
    {
        DEBUG_PRINT(LOG_LEVEL_1, __LOGTAG__, "failed to create socket!");
        return 0;
    }

    if ( !socket.Bind( Address(INADDR_ANY, GSDK_UDP_DEFAULT_PORT) ) )
    {
        DEBUG_PRINT(LOG_LEVEL_1, __LOGTAG__, "failed to create socket!");
        return 0;
    }
    
    // receive packets
    while ( true )
    {
        Address sender;
        unsigned char buffer[1024];
        long bytes_read = socket.Receive( sender, buffer, sizeof( buffer ) );
        if ( !bytes_read )
            break;
        
        if (bytes_read>0) {
            DEBUG_PRINT(LOG_LEVEL_1, __LOGTAG__, "received %ld %s from %s", bytes_read , buffer, sender.ToString().c_str());
            
            long bytesSent = socket.Send( sender, buffer, (int)bytes_read );
            DEBUG_PRINT(LOG_LEVEL_1, __LOGTAG__, "bytesSent %ld", bytesSent);
        }
        // process packet
        usleep(10*1000);    // 10 ms
    }
    DEBUG_PRINT(LOG_LEVEL_1, __LOGTAG__, "Server Program, shut down...");
    
     */
    return 0;
}

