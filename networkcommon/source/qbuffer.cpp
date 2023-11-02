//
//  qbuffer.cpp
//  networkcommon
//
//  Created by Arun A on 02/11/23.
//

#include "qbuffer.hpp"
#include <string.h>
// --------------------------BufferReader--------------------------------
// --------------------------BufferReader--------------------------------
// --------------------------BufferReader--------------------------------

void qbuffer::do_resize_if_required(ssize_t bytes_needed) {
    if (index + bytes_needed >= size) {
        // resize
        ssize_t resizeTo = (size == 0) ? sizeof(uint32_t) * 8 : size*2;
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

void qbuffer_reader::read( qbuffer& buffer, uint32_t& value ) {
    uint32_t sizeOfType = sizeof(uint32_t);
    DEBUG_ASSERT(__LOGTAG__, ((buffer.index + sizeOfType) <= buffer.size), "((buffer.index + sizeOfType) <= buffer.size)");
    value = *((uint32_t*)(buffer.data+buffer.index));
    buffer.index += sizeOfType;
}

void qbuffer_reader::read( qbuffer& buffer, uint16_t& value ) {
    uint32_t sizeOfType = sizeof(uint16_t);
    DEBUG_ASSERT(__LOGTAG__, ((buffer.index + sizeOfType) <= buffer.size), "((buffer.index + sizeOfType) <= buffer.size)");
    value = *((uint16_t*)(buffer.data+buffer.index));
    buffer.index += sizeOfType;
}
void qbuffer_reader::read( qbuffer& buffer, uint8_t& value ) {
    uint32_t sizeOfType = sizeof(uint8_t);
    DEBUG_ASSERT(__LOGTAG__, ((buffer.index + sizeOfType) <= buffer.size), "((buffer.index + sizeOfType) <= buffer.size)");
    value = *((uint8_t*)(buffer.data+buffer.index));
    buffer.index += sizeOfType;
}
void qbuffer_reader::read( qbuffer& buffer, unsigned long& value ) {
    uint32_t sizeOfType = sizeof(unsigned long);
    DEBUG_ASSERT(__LOGTAG__, ((buffer.index + sizeOfType) <= buffer.size), "((buffer.index + sizeOfType) <= buffer.size)");
    value = *((unsigned long*)(buffer.data+buffer.index));
    buffer.index += sizeOfType;
}

void qbuffer_reader::read( qbuffer& buffer, uint8_t* data, ssize_t length ) {
    DEBUG_ASSERT(__LOGTAG__, ((buffer.index + length) <= buffer.size), "((buffer.index + length) <= buffer.size)");
    uint8_t* src = ((uint8_t*)(buffer.data+buffer.index));
    memcpy(data, src, length);
    buffer.index += length;
}
// --------------------------BufferWriter--------------------------------
// --------------------------BufferWriter--------------------------------
// --------------------------BufferWriter--------------------------------

void qbuffer_writer::write( qbuffer& buffer, uint32_t value ) const {
    uint32_t sizeOfType = sizeof(uint32_t);
    buffer.do_resize_if_required(sizeOfType);
    *((uint32_t*)(buffer.data+buffer.index)) = value;
    buffer.index += sizeOfType;
}
void qbuffer_writer::write( qbuffer& buffer, uint16_t value ) const {
    uint32_t sizeOfType = sizeof(uint16_t);
    buffer.do_resize_if_required(sizeOfType);
    *((uint16_t*)(buffer.data+buffer.index)) = value;
    buffer.index += sizeOfType;
}
void qbuffer_writer::write( qbuffer& buffer, uint8_t value ) const {
    uint32_t sizeOfType = sizeof(uint8_t);
    buffer.do_resize_if_required(sizeOfType);
    *((uint8_t*)(buffer.data+buffer.index)) = value;
    buffer.index += sizeOfType;
}
void qbuffer_writer::write( qbuffer& buffer, unsigned long value ) const {
    uint32_t sizeOfType = sizeof(unsigned long);
    buffer.do_resize_if_required(sizeOfType);
    *((unsigned long*)(buffer.data+buffer.index)) = value;
    buffer.index += sizeOfType;
}

void qbuffer_writer::write( qbuffer& buffer, const uint8_t* data, ssize_t length ) const {
    buffer.do_resize_if_required(length);
    uint8_t* dest = ((uint8_t*)(buffer.data+buffer.index));
    memcpy(dest, data, length);
    buffer.index += length;
}
