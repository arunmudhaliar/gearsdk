//
//  UDPPacket.hpp
//  networkcommon
//
//  Created by Arun A on 30/09/23.
//

#ifndef UDPPacket_hpp
#define UDPPacket_hpp

#include <stdio.h>
#include <stdint.h>
#include "../../common/sdktypes.hpp"
#include "Address.hpp"

#undef __LOGTAG__
#define __LOGTAG__ "UDPPacketSerialisation"

namespace UDPPacketSerialisation {
struct Buffer {
    ~Buffer() {
        GX_DELETE_ARY(data);
        DEBUG_PRINT(LOG_LEVEL_3, __LOGTAG__, "Buffer destroyed.");
    }
    void Allocate(int32_t sz) {
        GX_DELETE_ARY(data);
        data = new uint8_t[sz];
        size = sz;
        index = pushedIndex = 0;
    }
    void SeekToBegin() {
        index = 0;
    }
    void DoResizeIfRequired(uint32_t bytesNeededMore);
    
    void Stash() {
        pushedIndex = index;
    }
    int32_t Pop() {
        if (pushedIndex>size) {
            return -1;
        }
        index = pushedIndex;
        return index;
    }
    
    uint8_t* data = nullptr;    // pointer to buffer data
    int32_t size = 0;           // size of buffer data (bytes)
    int32_t index = 0;          // index of next byte to be read/written
    int32_t pushedIndex = 0;
};

class BufferReader {
public:
    void Read( Buffer& buffer, uint32_t& value );
    void Read( Buffer& buffer, uint16_t& value );
    void Read( Buffer& buffer, uint8_t& value );
    void Read( Buffer& buffer, unsigned long& value );
}; //class BufferReader

class BufferWriter {
public:
    void Write( Buffer& buffer, uint32_t value ) const;
    void Write( Buffer& buffer, uint16_t value ) const;
    void Write( Buffer& buffer, uint8_t value ) const;
    void Write( Buffer& buffer, unsigned long value ) const;
}; //class BufferWriter

#define PAYLOAD_TYPEID(x) static const uint16_t TypeID = x
class PayLoad {
public:
    uint16_t type = TypeID;
    uint32_t msg = 0;
    PayLoad(uint16_t type) : type(type) {
    }
    PayLoad(const PayLoad& payload) {
        type = payload.type;
        msg = payload.msg;
    }
    virtual ~PayLoad() {
        DEBUG_PRINT(LOG_LEVEL_3, __LOGTAG__, "Payload destroyed. type[%d]", type);
    }
    virtual void Write( Buffer& buffer ) const {
        BufferWriter writer;
        writer.Write( buffer, type );
        writer.Write( buffer, msg );
    }
    virtual void Read( Buffer& buffer ) {
        BufferReader reader;
        reader.Read( buffer, msg);
    }
    PAYLOAD_TYPEID(0x0001);
private:
    PayLoad(){}
};

class SimplePayLoad : public PayLoad {
public:
    SimplePayLoad() : PayLoad(TypeID) {
    }
    ~SimplePayLoad() {
    }
    PAYLOAD_TYPEID(0x0002);
};

class UDPPacket {
public:
    enum kPacketNumberSpace {
        Initial = 0,
        Handshake = 1,
        ApplicationData = 2,
        kPacketNumberSpaceMAX = 3
    };
    uint16_t protocolID = 0;
    uint16_t sequenceID = 0;
    uint16_t ack = 0;
    uint32_t ackBitField = 0;
    PayLoad* payload = nullptr;
    unsigned long sendTimeStamp = 0;
    bool acked = false;
    uint8_t space = Initial;
    unsigned long ackDelay = 0;
    uint8_t flag = 0;
    Address validForDiffered;
    
    UDPPacket() {
    }
    ~UDPPacket() {
        GX_DELETE(payload);
        DEBUG_PRINT(LOG_LEVEL_3, __LOGTAG__, "Packet destroyed.");
    }
    UDPPacket(uint16_t protocolID, uint16_t sequenceID, uint16_t ack, uint32_t ackBitField, PayLoad* payload):
        protocolID(protocolID),
        sequenceID(sequenceID),
        ack(ack),
        ackBitField(ackBitField),
        payload(payload) {
    }
    void Write( Buffer& buffer ) {
        BufferWriter writer;
        writer.Write( buffer, protocolID );
        writer.Write( buffer, sequenceID );
        writer.Write( buffer, ack );
        writer.Write( buffer, ackBitField);
        writer.Write( buffer, sendTimeStamp);
        writer.Write( buffer, flag);
        if (payload){
            payload->Write(buffer);
        } else {
            writer.Write( buffer, (uint16_t)0);
        }
    }

    void ReadHeader( Buffer& buffer ) {
        BufferReader reader;
        reader.Read( buffer, protocolID);
        reader.Read( buffer, sequenceID);
        reader.Read( buffer, ack);
        reader.Read( buffer, ackBitField);
        reader.Read( buffer, sendTimeStamp);
        reader.Read( buffer, flag);
    }
    static int32_t GetHeaderSize() {
        return sizeof(uint32_t) + (3 * sizeof(uint16_t) + (1 * sizeof(uint8_t)) + (1 * sizeof(unsigned long)));
    }
    
    void Read( Buffer& buffer ) {
        BufferReader reader;
        reader.Read( buffer, protocolID);
        reader.Read( buffer, sequenceID);
        reader.Read( buffer, ack);
        reader.Read( buffer, ackBitField);
        uint16_t type = 0;
        reader.Read( buffer, type);
        switch (type) {
            case 1: {
                GX_DELETE(payload);
                payload = new PayLoad(type);
            }
            break;
            default:
                return;
        }
        payload->Read(buffer);
    }
};
}; //namespace UDPPacketSerialisation

#endif /* UDPPacket_hpp */
