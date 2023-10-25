//
//  UDPPacket.cpp
//  NetworkCommon
//
//  Created by Arun A on 30/09/23.
//

#include "UDPPacket.hpp"
#include <assert.h>
#include <string>

namespace UDPPacketSerialisation {

// --------------------------BufferReader--------------------------------
// --------------------------BufferReader--------------------------------
// --------------------------BufferReader--------------------------------

void Buffer::DoResizeIfRequired(uint32_t bytesNeededMore) {
    if (index + bytesNeededMore >= size) {
        // resize
        int resizeTo = (size == 0) ? sizeof(uint32_t) * 8 : size*2;
        uint8_t* newData = new uint8_t[resizeTo];
        if (newData == nullptr) {
            DEBUG_ASSERT(__LOGTAG__, false, "newData == nullptr");
        }
        memset(newData, 0, resizeTo);
        if (data) {
            memcpy(newData, data, index);
            GX_DELETE_ARY(data);
        }
        if (size > 0) { // only warn if the buffer is resizing.
            DEBUG_PRINT_WARN(__LOGTAG__, "Buffer resize %d-->%d", size, resizeTo);
        }
        data = newData;
        size = resizeTo;
    }
}

void BufferReader::Read( Buffer& buffer, uint32_t& value ) {
    uint32_t sizeOfType = sizeof(uint32_t);
    assert((buffer.index + sizeOfType) <= buffer.size);
    value = *((uint32_t*)(buffer.data+buffer.index));
    buffer.index += sizeOfType;
}

void BufferReader::Read( Buffer& buffer, uint16_t& value ) {
    uint32_t sizeOfType = sizeof(uint16_t);
    assert((buffer.index + sizeOfType) <= buffer.size);
    value = *((uint16_t*)(buffer.data+buffer.index));
    buffer.index += sizeOfType;
}
void BufferReader::Read( Buffer& buffer, uint8_t& value ) {
    uint32_t sizeOfType = sizeof(uint8_t);
    assert((buffer.index + sizeOfType) <= buffer.size);
    value = *((uint8_t*)(buffer.data+buffer.index));
    buffer.index += sizeOfType;
}
void BufferReader::Read( Buffer& buffer, unsigned long& value ) {
    uint32_t sizeOfType = sizeof(unsigned long);
    assert((buffer.index + sizeOfType) <= buffer.size);
    value = *((unsigned long*)(buffer.data+buffer.index));
    buffer.index += sizeOfType;
}

// --------------------------BufferWriter--------------------------------
// --------------------------BufferWriter--------------------------------
// --------------------------BufferWriter--------------------------------

void BufferWriter::Write( Buffer& buffer, uint32_t value ) const {
    uint32_t sizeOfType = sizeof(uint32_t);
    buffer.DoResizeIfRequired(sizeOfType);
    *((uint32_t*)(buffer.data+buffer.index)) = value;
    buffer.index += sizeOfType;
}
void BufferWriter::Write( Buffer& buffer, uint16_t value ) const {
    uint32_t sizeOfType = sizeof(uint16_t);
    buffer.DoResizeIfRequired(sizeOfType);
    *((uint16_t*)(buffer.data+buffer.index)) = value;
    buffer.index += sizeOfType;
}
void BufferWriter::Write( Buffer& buffer, uint8_t value ) const {
    uint32_t sizeOfType = sizeof(uint8_t);
    buffer.DoResizeIfRequired(sizeOfType);
    *((uint8_t*)(buffer.data+buffer.index)) = value;
    buffer.index += sizeOfType;
}
void BufferWriter::Write( Buffer& buffer, unsigned long value ) const {
    uint32_t sizeOfType = sizeof(unsigned long);
    buffer.DoResizeIfRequired(sizeOfType);
    *((unsigned long*)(buffer.data+buffer.index)) = value;
    buffer.index += sizeOfType;
}
}; //namespace UDPPacketSerialisation
