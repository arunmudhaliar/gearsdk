//
//  UDPSocket.hpp
//  networkcommon
//
//  Created by Arun A on 26/09/23.
//

#ifndef UDPSocket_hpp
#define UDPSocket_hpp


#include <iostream>
#include <pthread.h>
#include "Address.hpp"
#include "UDPPacket.hpp"

#undef __LOGTAG__
#define __LOGTAG__ "UDPSocket"

using namespace UDPPacketSerialisation;

class UDPSocket;
class MSocketListener {
public:
    virtual void OnSocketOpen(const UDPSocket* socket) = 0;
    virtual void OnSocketBind(const UDPSocket* socket) = 0;
    virtual void OnSocketClose(const UDPSocket* socket) = 0;
    virtual void OnSocketMessage(const UDPSocket* socket, Address& sender, Buffer& buffer) = 0;
    virtual bool OnConstructPacket(const UDPSocket* socket, Address& sender, UDPPacket* packet, Buffer& buffer) = 0;
    virtual void OnReceivePayLoad(const UDPSocket* socket, Address& sender, const PayLoad* payload) = 0;
    virtual void OnSocketError(const UDPSocket* socket) = 0;
    virtual void OnUpdateLoop() = 0;
};

class UDPSocket {
public:
    enum SOCKET_STATE {
        SOCKET_UNINITIALIZED,
        SOCKET_OPEN,
        SOCKET_RECV_LOOP,
        SOCKET_CLOSE
    };
    
    UDPSocket();
    ~UDPSocket();
    bool Open(MSocketListener* listener);
    bool Bind(const Address& destination);
    int Close();
    long Send(const Address& destination, Buffer& buffer );
    long Receive(Address& sender, void* data, int32_t size );
    bool IsActive() const;
    bool IsBinded() const;
    void ReceiveLoop();
    SOCKET_STATE GetState() { return state; }
    
private:
    long Send(const Address& destination, const void* data, int32_t size );
    static void* ReceiveLoopInternal(void* vargp);
    MSocketListener* listener = nullptr;
    int32_t handle = 0;
    SOCKET_STATE state = SOCKET_UNINITIALIZED;
    bool isBinded = false;
};

#endif /* UDPSocket_hpp */
