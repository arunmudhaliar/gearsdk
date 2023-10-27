//
//  NetworkInterface.hpp
//  networkcommon
//
//  Created by Arun A on 29/09/23.
//

#ifndef NetworkInterface_hpp
#define NetworkInterface_hpp

#include "UDPSocket.hpp"
#include "UDPPacket.hpp"
#include <queue>
#include <algorithm>

#undef __LOGTAG__
#define __LOGTAG__ "NetworkInterface"

#define DEFAULT_ACK_DELAY_THRESHOLD_IN_MS   3*1000

#define kInitialRtt 333 // 333ms

using namespace UDPPacketSerialisation;

class NetworkInterface : protected MSocketListener {
public:
    void Init(uint16_t protocolID, unsigned short port = 0);     // port = 0 for clients.
    void BeginReceive();
    UDPSocket::SOCKET_STATE GetSocketState();
    UDPSocket& GetSocket() { return socket; }
    
    long SendPayload(const Address &destination, PayLoad* payload);
protected:
    NetworkInterface();
    virtual ~NetworkInterface() = 0;
    virtual void OnSocketOpen(const UDPSocket* socket) override;
    virtual void OnSocketBind(const UDPSocket* socket) override;
    virtual void OnSocketClose(const UDPSocket* socket) override;
    virtual void OnSocketMessage(const UDPSocket* socket, Address& sender, Buffer& buffer) override;
    virtual void OnReceivePayLoad(const UDPSocket* socket, Address& sender, const PayLoad* payload) override;
    virtual bool OnConstructPacket(const UDPSocket* socket, Address& sender, UDPPacket* packet, Buffer& buffer) override;
    virtual void OnSocketError(const UDPSocket* socket) override;
    
    virtual void OnUpdateLoop() override;
    
    UDPSocket socket;

private:
    // protocol
    uint16_t protocolID;
    uint16_t localSeqNumber = 0;
    uint16_t remoteSeqNumber = 0;
    
    std::deque<UDPPacket*> receiveQ;
    std::deque<UDPPacket*> sendQ;
    std::deque<UDPPacket*> differedSendQ;
    
    bool PushToReceiveQ(UDPPacket* packet);
    bool PushToSendQ(UDPPacket* packet);
    inline bool sequence_greater_than(uint16_t s1, uint16_t s2);
    
    unsigned long ackDelayThresholdInMs = DEFAULT_ACK_DELAY_THRESHOLD_IN_MS;
    
    unsigned long latest_rtt = 0;
    unsigned long min_rtt = kInitialRtt / 2;
    unsigned long rttvar = 0;
    unsigned long smoothed_rtt = kInitialRtt;
    unsigned long first_rtt_sample = 0;
    unsigned long adjusted_rtt = 0;
    
    unsigned long max_ack_delay = 300;
    uint16_t largest_acked_packet[UDPPacket::kPacketNumberSpaceMAX];
    bool handShakeConfirmed = false;
    void UpdateRtt(unsigned long ack_delay);
    uint32_t ConstructAckField();
    
    unsigned long ack_send_time = 0;
};

#endif /* NetworkInterface_hpp */
