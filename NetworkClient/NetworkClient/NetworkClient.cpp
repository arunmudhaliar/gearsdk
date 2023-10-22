//
//  main.cpp
//  GNetwork
//
//  Created by Arun A on 20/09/23.
//

#include "NetworkClient.hpp"
#undef __LOGTAG__
#define __LOGTAG__ "NetworkClient"
#include <unistd.h>

#if 0
#include <unistd.h>
#include "../../Common/gxCrc32.h"
#include "../../Common/Timer.h"
#include <vector>

extern "C" {
#include "../../NetworkCommon/Source/quiche/client.h"
}



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
#endif

void timeout_main__cb(EV_P_ ev_timer *w, int revents) {
    DEBUG_PRINT_IMPORTANT(__LOGTAG__, "TIMEOUT MAIN");
    NetworkClientTester* tester = (NetworkClientTester*)w->data;
    std::vector<GameClient*> finishedList;
    for(int x=0;x<tester->clientList.size();x++) {
        GameClient* client = tester->clientList[x];
        client->SendMessage("hello1 from client", true);
        client->SendMessage("hello12 from client", true);
        client->SendMessage("hello123 from client", true);
        client->SendMessage("hello1234 from client", true);
//        client->Close();
        if (client->IsRunFinished()) {
            finishedList.push_back(client);
        }
    }
    
    for(auto it = finishedList.cbegin();it!=finishedList.cend();it++) {
        GameClient* p = *it;
        int oldSz = (int)tester->clientList.size();
        tester->clientList.erase(std::remove(tester->clientList.begin(), tester->clientList.end(), p), tester->clientList.end());
        if(oldSz!=tester->clientList.size()) {
            GX_DELETE(p);
        }
    }
//    ev_timer_stop(loop, w);
//    ev_break(loop, EVBREAK_ONE);
    if (tester->clientList.size()==0) {
        ev_break(loop, EVBREAK_ALL);
    }
}

void delete_cb(EV_P_ ev_timer *w, int revents) {
    DEBUG_PRINT_IMPORTANT(__LOGTAG__, "TIMEOUT DELETE");
    NetworkClientTester* tester = (NetworkClientTester*)w->data;
//    while(tester->clientList.size()) {
//        tester->clientList.front()->ForceRelease();
//        break;
//    }
    int finished = 0;
    for(int x=0;x<tester->clientList.size();x++) {
        GameClient* client = tester->clientList[x];
        client->Close();
        //client->ForceRelease();
        if (client->IsRunFinished()) {
            finished++;
        } else {
            finished = finished;
        }
    }
    
    if (tester->clientList.size()==finished) {
        ev_break(loop, EVBREAK_ALL);
    }
//    tester->clientList.clear();
////    client->Close();
}

int32_t main(int32_t argc, const char * argv[]) {
    PrintCommonInfo();
    
    NetworkClientTester tester;
    
    for (int y=0; y<1; y++) {
        for (int x=0; x<1; x++) {
            GameClient* newClient = new GameClient();
            newClient->run("localhost", "4000");
    //        newClient->SendMessage("STAAART CLIENT from client", true);
    //        newClient->Close();
            tester.clientList.push_back(newClient);
        }
    //    GameClient client2;
    //    client2.run("localhost", "4000");
        
        struct ev_loop* loop = ev_default_loop(0);
        static ev_timer tw;
        ev_timer_init (&tw, timeout_main__cb, 1, 1);
        tw.data = &tester;
        ev_timer_start(loop, &tw);
        
//        static ev_timer tw2;
//        ev_timer_init (&tw2, delete_cb, 3, 1);
//        tw2.data = &tester;
//        ev_timer_start(loop, &tw2);
        
        ev_run(loop, 0);
        
        
        for (int x=0; x<tester.clientList.size(); x++) {
            GameClient* client = tester.clientList[x];
            GX_DELETE(client);
        }
        tester.clientList.clear();
        usleep(5*1000*1000);    // 10ms
    }
    
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

