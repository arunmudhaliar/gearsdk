//
//  NetworkClientTester.hpp
//  NetworkClientTester
//
//  Created by Arun A on 29/09/23.
//

#ifndef NetworkClientTester_hpp
#define NetworkClientTester_hpp

#include "GameClient.hpp"


class NetworkClientTester {
public:
    void run(const std::string& host, const std::string& port, float sendInterval, float closeTimeout);
    
private:
    static void send_msg_timer_cb(EV_P_ ev_timer *w, int revents);
    static void delete_cb(EV_P_ ev_timer *w, int revents);
    std::vector<GameClient*> clientList;
};

#endif /* NetworkClientTester_hpp */
