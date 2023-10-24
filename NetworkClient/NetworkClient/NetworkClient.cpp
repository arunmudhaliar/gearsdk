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
//    ev_timer_stop(loop, w);
//    ev_break(loop, EVBREAK_ONE);
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
        } else {
            finished = finished;
        }
    }
    
//    if (tester->clientList.size()==finished) {
//        ev_break(loop, EVBREAK_ALL);
//    }
//    tester->clientList.clear();
////    client->Close();
}

int32_t main(int32_t argc, const char * argv[]) {
    PrintCommonInfo();
    
    NetworkClientTester tester;
    
    for (int y=0; y<5; y++) {
        for (int x=0; x<100; x++) {
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
        
        static ev_timer tw2;
        ev_timer_init (&tw2, delete_cb, 7, 1);
        tw2.data = &tester;
        ev_timer_start(loop, &tw2);
        
        ev_run(loop, 0);
        
        
        for (int x=0; x<tester.clientList.size(); x++) {
            GameClient* client = tester.clientList[x];
            GX_DELETE(client);
        }
        tester.clientList.clear();
        usleep(5*1000*1000);    // 10ms
    }
    return 0;
}

