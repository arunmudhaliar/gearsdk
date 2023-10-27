//
//  RemoteInterface.hpp
//  networkcommon
//
//  Created by Arun A on 03/10/23.
//

#ifndef RemoteInterface_hpp
#define RemoteInterface_hpp

#include "UDPSocket.hpp"
#include "UDPPacket.hpp"

#include <stdio.h>

#undef __LOGTAG__
#define __LOGTAG__ "RemoteInterface"

using namespace UDPPacketSerialisation;

class RemoteInterface {
public:
    RemoteInterface(const Address& adress);
    virtual ~RemoteInterface();
protected:
    Address remoteAddress;
};

#endif /* RemoteInterface_hpp */
