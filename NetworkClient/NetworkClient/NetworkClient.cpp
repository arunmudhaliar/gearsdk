//
//  main.cpp
//  GNetwork
//
//  Created by Arun A on 20/09/23.
//

#include "NetworkClient.hpp"
#include <unistd.h>
#include "../../Common/gxCrc32.h"
#include "../../Common/Timer.h"

extern "C" {
#include "../../NetworkCommon/Source/quiche/client.h"
}

#include "QNetworkClient.hpp"

#undef __LOGTAG__
#define __LOGTAG__ "NetworkClient"

#define NETWORK_PROTOCOL_ID 0x7f8c

NetworkClient::NetworkClient() : NetworkInterface() {
    
}

NetworkClient::~NetworkClient() {
    
}

void NetworkClient::OnReceivePayLoad(const UDPSocket* socket, Address& sender, const PayLoad* payload) {
    //GetSocket().Close();
//    Buffer buffer;
//    payload->Write(buffer);
//    DEBUG_PRINT(LOG_LEVEL_1, __LOGTAG__, "bytesReceived %d", buffer.index);
}

bool NetworkClient::OnConstructPacket(const UDPSocket* socket, Address& sender, UDPPacket* packet, Buffer& buffer) {
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

int32_t main(int32_t argc, const char * argv[]) {
    
//    std::deque<UDPPacket*> receiveQ;
//    UDPPacket* p1 = new UDPPacket();
//    UDPPacket* p2 = new UDPPacket();
//    UDPPacket* p3 = new UDPPacket();
//    UDPPacket* p4 = new UDPPacket();
//    receiveQ.push_back(p1);
//    receiveQ.push_back(p2);
//    receiveQ.push_back(p3);
//    receiveQ.push_back(p4);
//
//    int size = (int)receiveQ.size();
//    receiveQ.erase(std::remove(receiveQ.begin(), receiveQ.end(), p2), receiveQ.end());
//    size = (int)receiveQ.size();
//
//    int cnt1=0;
//    for(auto it = receiveQ.cbegin();it!=receiveQ.cend();it++) {
//        UDPPacket* p = *it;
//        cnt1++;
//    }

    PrintCommonInfo();
    
    
//    char **argv2 = (char**)malloc(sizeof(char*)*3);
//    argv2[0] = (char*)"TestQuiche";
//    argv2[1] = (char*)"localhost";
//    argv2[2] = (char*)"4000";
//    test_client_main(3, argv2);
    
    QNetworkClient::run("localhost", "4000");
//    NetworkClient client;
//    client.Init(NETWORK_PROTOCOL_ID);
//    client.BeginReceive();
//
//    int32_t cnt=0;
//    int tt = 0;
//    unsigned long timeStarted = Timer::getCurrentTimeInMilliSec();
//    unsigned long sendForTime = 1000 * 60 * 20;
//    while(Timer::getCurrentTimeInMilliSec() < timeStarted+sendForTime){
////        if (tt==32) {
////            DEBUG_PRINT(LOG_LEVEL_0, __LOGTAG__, "Last packet about to send");
////        }
//        Address sendTo(127, 0, 0, 1, GSDK_UDP_DEFAULT_PORT);
//        SimplePayLoad* payload = new SimplePayLoad();
//        client.SendPayload(sendTo, payload);
//        tt++;
//        int random = std::min(150, std::max(65, rand()%150));
//        usleep(random*2000);    // 10ms
//    }
//    DEBUG_PRINT(LOG_LEVEL_0, __LOGTAG__, "Sent %d packets", tt);
//
//    while (client.GetSocket().IsActive())
//    {
//        usleep(10*1000);    // 10ms
//    }
    
    /*
    UDPSocket socket;

    if ( !socket.Open() )
    {
        DEBUG_PRINT(LOG_LEVEL_1, __LOGTAG__, "failed to create socket!");
        return 0;
    }
    
    socket.IsActive();
    int32_t cnt=0;
    while(cnt++ <15)
    {
        char data[256];
        snprintf(data, sizeof(data), "packet%d", cnt);
        Address sendTo(127, 0, 0, 1, GSDK_UDP_DEFAULT_PORT);
        long bytesSent = socket.Send( sendTo, data, sizeof( data ) );
        DEBUG_PRINT(LOG_LEVEL_1, __LOGTAG__, "bytesSent %ld", bytesSent);
    }
    
//    socket.IsActive();
    // usleep(10*1000*1000);    // 10s
    while ( true )
    {
        Address sender;
        unsigned char buffer[256];
        long bytes_read = socket.Receive( sender, buffer, sizeof( buffer ) );
        if ( !bytes_read )
            break;
        
        if (bytes_read>0) {
            DEBUG_PRINT(LOG_LEVEL_1, __LOGTAG__, "received %ld %s from %s", bytes_read , buffer, sender.ToString().c_str());
            socket.Close();
//            socket.IsActive();
        } else if (bytes_read<0) {
            if (errno == EBADF) {
                DEBUG_PRINT (__LOGTAG__, "Socket recv error: %s - %d", strerror (errno), errno);
                break;
            }
        }
        // process packet
        usleep(10*1000);    // 10ms
//        socket.IsActive();
    }
*/
    return 0;
}

