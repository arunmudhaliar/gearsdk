//
//  NetworkServer.hpp
//  NetworkServer
//
//  Created by Arun A on 29/09/23.
//

#ifndef NetworkServer_hpp
#define NetworkServer_hpp

#include "../../NetworkCommon/Source/NetworkInterface.hpp"

#if 0
class NetworkServer : public NetworkInterface {
public:
    NetworkServer();
    virtual ~NetworkServer();
    
protected:
    virtual bool OnConstructPacket(const UDPSocket* socket, Address& sender, UDPPacket* packet, Buffer& buffer) override;
    virtual void OnReceivePayLoad(const UDPSocket* socket, Address& sender, const PayLoad* payload) override;
};
#endif

#endif /* NetworkServer_hpp */
