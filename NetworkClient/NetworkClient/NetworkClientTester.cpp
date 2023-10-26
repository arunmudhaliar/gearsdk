//
//  NetworkClientTester.cpp
//  NetworkClientTester
//
//  Created by Arun A on 20/09/23.
//

#include "NetworkClientTester.hpp"
#undef __LOGTAG__
#define __LOGTAG__ "NetworkClient"
#include <unistd.h>
#include <algorithm>

void NetworkClientTester::send_msg_timer_cb(EV_P_ ev_timer *w, int revents) {
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

void NetworkClientTester::delete_cb(EV_P_ ev_timer *w, int revents) {
    NetworkClientTester* tester = (NetworkClientTester*)w->data;
    DEBUG_PRINT_IMPORTANT(__LOGTAG__, "TIMEOUT DELETE %d", tester->clientList.size());
//    int finished = 0;
    for(int x=0;x<tester->clientList.size();x++) {
        GameClient* client = tester->clientList[x];
        client->Close();
//        //client->ForceRelease();
        break;
//        if (client->IsRunFinished()) {
//            finished++;
//        }
    }
//    ev_break(loop, EVBREAK_ONE);
}

void NetworkClientTester::run(const std::string& host, const std::string& port, float sendInterval, float closeTimeout) {
    const int total_loop = 7;
    const int clients_per_loop = 5;
    const int wait_sec_after_loop = 6;
    for (int y=0; y<total_loop; y++) {
        for (int x=0; x<clients_per_loop; x++) {
            GameClient* newClient = new GameClient();
            newClient->run(host, port);
            clientList.push_back(newClient);
        }
        
        // send timer
        struct ev_loop* loop = ev_default_loop(0);
        static ev_timer tw;
        ev_timer_init (&tw, send_msg_timer_cb, sendInterval, 1);
        tw.data = this;
        ev_timer_start(loop, &tw);
        
        //close timer
        static ev_timer tw2;
        ev_timer_init (&tw2, delete_cb, closeTimeout, 1);
        tw2.data = this;
        ev_timer_start(loop, &tw2);
        
        ev_run(loop, 0);
        
        // destroy the list
        for (int x=0; x<clientList.size(); x++) {
            GameClient* client = clientList[x];
            GX_DELETE(client);
        }
        clientList.clear();
        usleep(wait_sec_after_loop*1000*1000);    // wait for wait_sec_after_loop sec.
    }
}
