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
#include <netdb.h>

void timeout_main__cb(EV_P_ ev_timer *w, int revents) {
    NetworkClientTester* tester = (NetworkClientTester*)w->data;
    DEBUG_PRINT_IMPORTANT(__LOGTAG__, "TIMEOUT MAIN %d", tester->clientList.size());
    std::vector<GameClient*> finishedList;
    for(int x=0;x<tester->clientList.size();x++) {
        GameClient* client = tester->clientList[x];
        client->SendMessage("hello1 from client", true);
        client->SendMessage("hello12 from client", true);
        client->SendMessage("hello123 from client", true);
        client->SendMessage("hello1234 from client", true);
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
    if (tester->clientList.size()==0) {
        ev_break(loop, EVBREAK_ALL);
    }
}

void delete_cb(EV_P_ ev_timer *w, int revents) {
    NetworkClientTester* tester = (NetworkClientTester*)w->data;
    DEBUG_PRINT_IMPORTANT(__LOGTAG__, "TIMEOUT DELETE %d", tester->clientList.size());
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
        }
    }
}

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
    
    NetworkClientTester tester;
    
    for (int y=0; y<5; y++) {
        for (int x=0; x<100; x++) {
            GameClient* newClient = new GameClient();
            newClient->run(host, port);
    //        newClient->SendMessage("STAAART CLIENT from client", true);
    //        newClient->Close();
            tester.clientList.push_back(newClient);
        }
        
        // send timer
        struct ev_loop* loop = ev_default_loop(0);
        static ev_timer tw;
        ev_timer_init (&tw, timeout_main__cb, 1, 1);
        tw.data = &tester;
        ev_timer_start(loop, &tw);
        
        //close timer
        static ev_timer tw2;
        ev_timer_init (&tw2, delete_cb, 7, 1);
        tw2.data = &tester;
        ev_timer_start(loop, &tw2);
        
        ev_run(loop, 0);
        
        // destroy the list
        for (int x=0; x<tester.clientList.size(); x++) {
            GameClient* client = tester.clientList[x];
            GX_DELETE(client);
        }
        tester.clientList.clear();
        usleep(5*1000*1000);    // wait for 5 sec. 
    }
    return 0;
}

