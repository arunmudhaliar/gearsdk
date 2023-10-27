//
//  NetworkInterface.cpp
//  networkcommon
//
//  Created by Arun A on 29/09/23.
//

#include "NetworkInterface.hpp"
#include "../../common/gxCrc32.h"
#include "../../common/Timer.h"
#include <unistd.h>
#include <sstream>

NetworkInterface::NetworkInterface() {
    for(int x=0;x<UDPPacket::kPacketNumberSpaceMAX;x++) {
        largest_acked_packet[x] = 0xFFFF;
    }
}

NetworkInterface::~NetworkInterface() {
    while (!receiveQ.empty()) {
        GX_DELETE(receiveQ.front());
        receiveQ.pop_front();
    }
    
    while (!sendQ.empty()) {
        GX_DELETE(sendQ.front());
        sendQ.pop_front();
    }
    
    DEBUG_PRINT(LOG_LEVEL_1, __LOGTAG__, "NetworkInterface destroyed");
}

void NetworkInterface::Init(uint16_t protocolID, unsigned short port) {
    if ( !socket.Open(this) )
    {
        DEBUG_PRINT_ERROR(__LOGTAG__, "failed to open socket!");
        return;
    }
    
    if (port>0 && !socket.Bind( Address(INADDR_ANY, port) ) )
    {
        DEBUG_PRINT_ERROR(__LOGTAG__, "failed to bind socket!");
        return;
    }
    this->protocolID = protocolID;
}

void NetworkInterface::BeginReceive() {
    socket.ReceiveLoop();
}

UDPSocket::SOCKET_STATE NetworkInterface::GetSocketState() {
    return socket.GetState();
}

void NetworkInterface::OnSocketOpen(const UDPSocket* socket) {
    
}
void NetworkInterface::OnSocketBind(const UDPSocket* socket) {
    
}
void NetworkInterface::OnSocketClose(const UDPSocket* socket) {
    
}
inline bool NetworkInterface::sequence_greater_than(uint16_t s1, uint16_t s2) {
    return ( ( s1 > s2 ) && ( s1 - s2 <= 32768 ) ) ||
           ( ( s1 < s2 ) && ( s2 - s1  > 32768 ) );
}

uint32_t NetworkInterface::ConstructAckField() {
    std::stringstream ss;
    std::stringstream ss_send_ack;
    uint16_t rSeq_minus_32 = std::max(0, remoteSeqNumber - 32);
    uint32_t ackField = 0;
    std::deque<UDPPacket*> oldReceivedPackets;
    for(auto it = receiveQ.cbegin();it!=receiveQ.cend();it++) {
        UDPPacket* p = *it;
        if (p->sequenceID>rSeq_minus_32 && p->sequenceID<remoteSeqNumber)
        {
            uint32_t bitIndex = std::max(0, (remoteSeqNumber-p->sequenceID)-1);
            ackField = ackField | (1<<bitIndex);
            ss_send_ack<<p->sequenceID<<",";
        } else if (p->sequenceID<=rSeq_minus_32){
            oldReceivedPackets.push_back(p);
            ss<<p->sequenceID<<",";
            DEBUG_PRINT(LOG_LEVEL_3, __LOGTAG__, "receivedQ older seqId %d. Will be dropped from the Q", p->sequenceID);
        }
    }
    
    if (ackField) {
        DEBUG_PRINT(LOG_LEVEL_1, __LOGTAG__, "[%s] sending ACK[locSeq:%d][%s].",GetSocket().IsBinded() ? "S" :"C", localSeqNumber, ss_send_ack.str().c_str());
    }
    if (oldReceivedPackets.size()) {
        DEBUG_PRINT(LOG_LEVEL_1, __LOGTAG__, "receivedQ[%s] PURGE_OLD_SEQID[locSeq:%d][%s]. Will be dropped from the Q",GetSocket().IsBinded() ? "S" :"C", localSeqNumber, ss.str().c_str());
    }
    
    for(auto it = oldReceivedPackets.cbegin();it!=oldReceivedPackets.cend();it++) {
        UDPPacket* p = *it;
        int oldSz = (int)receiveQ.size();
        receiveQ.erase(std::remove(receiveQ.begin(), receiveQ.end(), p), receiveQ.end());
        if(oldSz!=receiveQ.size()) {
            GX_DELETE(p);
        }
    }
    
    return ackField;
}

void NetworkInterface::OnUpdateLoop() {
    unsigned long currentTime = Timer::getCurrentTimeInMilliSec();
    int ackNotReceived = 0;
    std::stringstream ss;
    int topDiffOfSeq = 0;

    for(auto it = sendQ.cbegin();it!=sendQ.cend();it++) {
        UDPPacket* p = *it;
        unsigned long elapsedTime = currentTime - p->sendTimeStamp;
        if (elapsedTime > ackDelayThresholdInMs) {
            topDiffOfSeq = std::max(topDiffOfSeq, remoteSeqNumber-p->sequenceID);
            ackNotReceived++;
            //ss<<p->sequenceID<<",";
            DEBUG_PRINT(LOG_LEVEL_3, __LOGTAG__, "[%s] ACK_NOT_RECEIVED for seqID %d, timestamp %ld, elapsed %ld", GetSocket().IsBinded() ? "S" : "C", p->sequenceID, p->sendTimeStamp, elapsedTime);
        }
    }
    
    if(GetSocket().IsBinded()) {
        DEBUG_PRINT(LOG_LEVEL_3, __LOGTAG__, "Server");
    } else {
        DEBUG_PRINT(LOG_LEVEL_3, __LOGTAG__, "Client");
    }
    
    long diff = Timer::getCurrentTimeInMilliSec() - ack_send_time;
    long thresholdTime = smoothed_rtt/2; //std::min(200, (int)(smoothed_rtt/2));
    if (diff<thresholdTime) {
        return;
    }
    
    if ((smoothed_rtt>300 || sendQ.size()==0) && receiveQ.size()>1) {

        localSeqNumber++;
        uint32_t ackField = ConstructAckField();
        UDPPacket* packet = new UDPPacket(protocolID, localSeqNumber, remoteSeqNumber, ackField, nullptr);
        packet->sendTimeStamp = Timer::getCurrentTimeInMilliSec();
        packet->flag=1;
        Buffer buffer;
        packet->Write(buffer);
        socket.Send(packet->validForDiffered, buffer);
        GX_DELETE(packet);
        //PushToSendQ(packet);
        int crc = gxCrc32::Calc(buffer.data, 0, (int)buffer.index, true);
        DEBUG_PRINT(LOG_LEVEL_0, __LOGTAG__, "sending ACK_PACK, localSeqNumber %d crc[0x%0x]", localSeqNumber, crc);
        ack_send_time = Timer::getCurrentTimeInMilliSec();
        return;
    }
    
    if(differedSendQ.size()) {
        UDPPacket* packet = differedSendQ.front();
        packet->sendTimeStamp = Timer::getCurrentTimeInMilliSec();
        Buffer buffer;
        packet->Write(buffer);
        long bytesSent = socket.Send(packet->validForDiffered, buffer);
        
        PushToSendQ(packet);
        int crc = gxCrc32::Calc(buffer.data, 0, (int)buffer.index, true);
        DEBUG_PRINT(LOG_LEVEL_1, __LOGTAG__, "DIFFERED --> bytesSent %ld crc[0x%0x]", bytesSent, crc);
        DEBUG_PRINT(LOG_LEVEL_0, __LOGTAG__, "---->>>> DIFFERED SEND -->>> [%s] seqId %d. waiting for ack on sendQ.",GetSocket().IsBinded() ? "S" :"C",  packet->sequenceID);
        differedSendQ.pop_front();
        ack_send_time = Timer::getCurrentTimeInMilliSec();
    }

//    if (ackNotReceived>sendQ.size()) {
//        ackNotReceived=ackNotReceived;
//    }
//    if (ackNotReceived>=483) {
//        ackNotReceived=ackNotReceived;
//    }
    
//    if (ackNotReceived) {
//        DEBUG_PRINT_WARN(__LOGTAG__, "[%s] TOTAL ACK_NOT_RECEIVED [%d][rSeq:%d, topDiff:%d] - [%s]", GetSocket().IsBinded() ? "S" : "C", ackNotReceived, remoteSeqNumber, topDiffOfSeq, ss.str().c_str());
//    }
}

long NetworkInterface::SendPayload(const Address &destination, PayLoad* payload) {
    if(GetSocket().IsBinded()) {
        DEBUG_PRINT(LOG_LEVEL_3, __LOGTAG__, "Server");
    } else {
        DEBUG_PRINT(LOG_LEVEL_3, __LOGTAG__, "Client");
    }
    
    localSeqNumber++;
    // compose
    uint32_t ackField = ConstructAckField();
    UDPPacket* packet = new UDPPacket(protocolID, localSeqNumber, remoteSeqNumber, ackField, payload);
    if (smoothed_rtt>300) {
        packet->validForDiffered=destination;
        differedSendQ.push_back(packet);
        return 0;
    }
    packet->sendTimeStamp = Timer::getCurrentTimeInMilliSec();
    Buffer buffer;
    packet->Write(buffer);
    long bytesSent = socket.Send(destination, buffer);
    
    PushToSendQ(packet);
    int crc = gxCrc32::Calc(buffer.data, 0, (int)buffer.index, true);
    DEBUG_PRINT(LOG_LEVEL_1, __LOGTAG__, "bytesSent %ld crc[0x%0x]", bytesSent, crc);
    DEBUG_PRINT(LOG_LEVEL_0, __LOGTAG__, "---->>>>[%s] seqId %d. waiting for ack on sendQ.",GetSocket().IsBinded() ? "S" :"C",  localSeqNumber);
    
    return bytesSent;
}

void NetworkInterface::OnSocketMessage(const UDPSocket* socket, Address& sender, Buffer& buffer) {
    int packHeaderSize = UDPPacket::GetHeaderSize();
    if (buffer.data == nullptr || buffer.index < packHeaderSize) {
        DEBUG_PRINT_WARN(__LOGTAG__, "OnSocketMessage - buffer.data == nullptr || buffer.index < packHeaderSize : %d, %d", buffer.index, packHeaderSize);
        return;
    }
    
    UDPPacket* packet = new UDPPacket();
    buffer.SeekToBegin();
    packet->ReadHeader(buffer);
    
    if (packet->protocolID != protocolID) {
        DEBUG_PRINT_WARN(__LOGTAG__, "OnSocketMessage - packet.protocolID != protocolID : %d, %d", packet->protocolID, protocolID);
        GX_DELETE(packet);
        return;
    }
    
    if (packet->flag ==0) {
        remoteSeqNumber = sequence_greater_than(packet->sequenceID, remoteSeqNumber) ? packet->sequenceID : remoteSeqNumber;
    }
    
    if(GetSocket().IsBinded()) {
        DEBUG_PRINT(LOG_LEVEL_3, __LOGTAG__, "Server");
    } else {
        DEBUG_PRINT(LOG_LEVEL_3, __LOGTAG__, "Client");
    }
    //
    std::stringstream ss;
    std::deque<UDPPacket*> ackedPackets;
    UDPPacket* largestAckPacket = nullptr;
    for(auto it = sendQ.cbegin();it!=sendQ.cend();it++) {
        UDPPacket* p = *it;
        if (p->sequenceID==packet->ack) {
            if(std::find(ackedPackets.begin(), ackedPackets.end(), p)==ackedPackets.end()) {
                p->acked = true;
                if (largestAckPacket == nullptr) {
                    largestAckPacket = p;
                } else if (largestAckPacket->sequenceID<p->sequenceID) {
                    largestAckPacket = p;
                }
                ackedPackets.push_back(p);
                DEBUG_PRINT(LOG_LEVEL_3, __LOGTAG__, "sendQ[%s] acked for seqId %d. Will be dropped from the Q",socket->IsBinded() ? "S" :"C",  p->sequenceID);
                ss<<p->sequenceID<<",";
            }
        }
        for (int x=0;x<32;x++) {
            uint16_t ackForSeqNumber = std::max(0, packet->ack-(x+1));
            bool isAckSet = (packet->ackBitField & (1<<x));
            if (!isAckSet) {
                continue;
            }
            if (p->sequenceID == ackForSeqNumber /*|| p->sequenceID==packet->ack*/) {
                if(std::find(ackedPackets.begin(), ackedPackets.end(), p)==ackedPackets.end()) {
                    p->acked = true;
                    if (largestAckPacket == nullptr) {
                        largestAckPacket = p;
                    } else if (largestAckPacket->sequenceID<p->sequenceID) {
                        largestAckPacket = p;
                    }
                    ackedPackets.push_back(p);
                    DEBUG_PRINT(LOG_LEVEL_3, __LOGTAG__, "sendQ[%s] acked for seqId %d. Will be dropped from the Q",socket->IsBinded() ? "S" :"C", p->sequenceID);
                    ss<<p->sequenceID<<",";
                }
            }
        }
    }
    
    if (ackedPackets.size()) {
        if (largestAckPacket) {
            latest_rtt = Timer::getCurrentTimeInMilliSec() - largestAckPacket->sendTimeStamp;
            UpdateRtt(25);
            handShakeConfirmed = true;  // TODO : This needs to put on proper place once handshake is implemented.
        }
        DEBUG_PRINT(LOG_LEVEL_0, __LOGTAG__, "sendQ[%s] [rSeq:%d, smoothed_rtt:%lu, latest_rtt:%lu] ACKED_SEQID[%s]. Will be dropped from the Q",socket->IsBinded() ? "S" :"C", remoteSeqNumber, smoothed_rtt, latest_rtt, ss.str().c_str());
    }
    
    for(auto it = ackedPackets.cbegin();it!=ackedPackets.cend();it++) {
        UDPPacket* p = *it;
        int oldSz = (int)sendQ.size();
        sendQ.erase(std::remove(sendQ.begin(), sendQ.end(), p), sendQ.end());
        if(oldSz!=sendQ.size()) {
            GX_DELETE(p);
        }
    }
    //
    
    if (packet->flag !=0) {
        PushToReceiveQ(packet);
    }
    
    if (OnConstructPacket(socket, sender, packet, buffer)) {
        OnReceivePayLoad(socket, sender, packet->payload);
    }
    if (packet->flag !=0) {
        GX_DELETE(packet);
    }

//    for(auto it = receiveQ.cbegin();it!=receiveQ.cend();it++)
//    {
//        //cout << *it << " ";
//    }
}

void NetworkInterface::UpdateRtt(unsigned long ack_delay) {
    if (first_rtt_sample == 0) {
        min_rtt = latest_rtt;
        smoothed_rtt = latest_rtt;
        rttvar = latest_rtt / 2;
        first_rtt_sample = Timer::getCurrentTimeInMilliSec();
        return;
    }
  // min_rtt ignores acknowledgment delay.
    min_rtt = std::min(min_rtt, latest_rtt);
  // Limit ack_delay by max_ack_delay after handshake
  // confirmation.
    if (handShakeConfirmed) {
        ack_delay = std::min(ack_delay, max_ack_delay);
    }

  // Adjust for acknowledgment delay if plausible.
    adjusted_rtt = latest_rtt;
    if (latest_rtt >= min_rtt + ack_delay) {
        adjusted_rtt = latest_rtt - ack_delay;
    }

    rttvar = (3.0f/4.0f) * (float)rttvar + (1.0f/4.0f * (float)(std::abs((long)(smoothed_rtt - adjusted_rtt))));
    smoothed_rtt = (7.0f/8.0f) * (float)smoothed_rtt + ((1.0f/8.0f) * (float)adjusted_rtt);
}

bool NetworkInterface::PushToReceiveQ(UDPPacket* packet) {
//    DEBUG_ASSERT(__LOGTAG__, this->receiveQ.size()<=32, "PushToReceiveQ Failed !!! %d", this->receiveQ.size());
//    if (this->receiveQ.size()>32) {
//        return false;
//    }
    this->receiveQ.push_back(packet);
    return true;
}

bool NetworkInterface::PushToSendQ(UDPPacket* packet) {
//    DEBUG_ASSERT(__LOGTAG__, this->sendQ.size()<=32, "PushToSendQ Failed !!! %d", this->sendQ.size());
//    if (this->sendQ.size()>32) {
//        return false;
//    }
    this->sendQ.push_back(packet);
    return true;
}

bool NetworkInterface::OnConstructPacket(const UDPSocket* socket, Address& sender, UDPPacket* packet, Buffer& buffer) {
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
        default:
            buffer.Pop();
            return false;
    }
    payload->Read(buffer);
    GX_DELETE(packet->payload);
    packet->payload = payload;
    return true;
}

void NetworkInterface::OnReceivePayLoad(const UDPSocket* socket, Address& sender, const PayLoad* payload) {
    
}
void NetworkInterface::OnSocketError(const UDPSocket* socket) {
    
}
