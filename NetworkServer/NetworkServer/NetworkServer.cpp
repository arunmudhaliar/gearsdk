//
//  main.cpp
//  GNetwork
//
//  Created by Arun A on 20/09/23.
//
#include "NetworkServer.hpp"
#include <unistd.h>
#include "../../Common/gxCrc32.h"

#include "QNetworkServer.hpp"

//extern "C" {
//#include "../../NetworkCommon/Source/quiche/server.h"
//}

#undef __LOGTAG__
#define __LOGTAG__ "NetworkServer"

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

int32_t main(int32_t argc, const char * argv[]) {
    PrintCommonInfo();
//    NetworkServer server;
//    server.Init(NETWORK_PROTOCOL_ID, GSDK_UDP_DEFAULT_PORT);
//    server.BeginReceive();
//
//
//    while (server.GetSocket().IsActive())
//    {
//        usleep(10*1000);    // 10ms
//    }
    
//    printf("hello quinche");
//    char **argv2 = (char**)malloc(sizeof(char*)*3);
//    argv2[0] = (char*)"TestQuiche";
//    argv2[1] = (char*)"localhost";
//    argv2[2] = (char*)"4000";
//    test_server_main(3, argv2);
    QNetworkServer::run("localhost", "4000");
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

