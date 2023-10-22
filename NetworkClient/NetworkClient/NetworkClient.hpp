//
//  NetworkClient.hpp
//  NetworkClient
//
//  Created by Arun A on 29/09/23.
//

#ifndef NetworkClient_hpp
#define NetworkClient_hpp

#include "../../NetworkCommon/Source/NetworkInterface.hpp"
#include "GameClient.hpp"


class NetworkClientTester {
public:
    void Test();
    std::vector<GameClient*> clientList;
};

#if 0
class NetworkClient : public NetworkInterface {
public:
    NetworkClient();
    virtual ~NetworkClient();
    
protected:
    virtual void OnReceivePayLoad(const UDPSocket* socket, Address& sender, const PayLoad* payload) override;
    virtual bool OnConstructPacket(const UDPSocket* socket, Address& sender, UDPPacket* packet, Buffer& buffer) override;
};
#endif
#endif /* NetworkClient_hpp */
