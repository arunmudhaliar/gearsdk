//
//  Copyright 2024 homenet25
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
		ssize_t resize_to = (size == 0) ? sizeof(uint32_t) * 8 : size * 2;
		uint8_t* new_data = DEBUG_NEW uint8_t[resize_to];
		if (new_data == nullptr) {
			DEBUG_ASSERT(__LOGTAG__, false, "newData == nullptr");
		}
		memset(new_data, 0, resize_to);
		if (data) {
			memcpy(new_data, data, index);
			GX_DELETE_ARY(data);
		}
		if (size > 0) {	 // only warn if the buffer is resizing.
			debug_print_warn(__LOGTAG__, "Buffer resize %d-->%d", size, resize_to);
		}
		data = new_data;
		size = resize_to;
	}
}

void qbuffer_reader::read(qbuffer& buffer, uint32_t& value) {
	uint32_t size_of_type = sizeof(uint32_t);
	DEBUG_ASSERT(__LOGTAG__, ((buffer.index + size_of_type) <= buffer.size), "((buffer.index + sizeOfType) <= buffer.size)");
	value = *((uint32_t*) (buffer.data + buffer.index));
	buffer.index += size_of_type;
}

void qbuffer_reader::read(qbuffer& buffer, uint16_t& value) {
	uint32_t size_of_type = sizeof(uint16_t);
	DEBUG_ASSERT(__LOGTAG__, ((buffer.index + size_of_type) <= buffer.size), "((buffer.index + sizeOfType) <= buffer.size)");
	value = *((uint16_t*) (buffer.data + buffer.index));
	buffer.index += size_of_type;
}
void qbuffer_reader::read(qbuffer& buffer, uint8_t& value) {
	uint32_t size_of_type = sizeof(uint8_t);
	DEBUG_ASSERT(__LOGTAG__, ((buffer.index + size_of_type) <= buffer.size), "((buffer.index + sizeOfType) <= buffer.size)");
	value = *((uint8_t*) (buffer.data + buffer.index));
	buffer.index += size_of_type;
}
void qbuffer_reader::read(qbuffer& buffer, unsigned long& value) {
	uint32_t size_of_type = sizeof(unsigned long);
	DEBUG_ASSERT(__LOGTAG__, ((buffer.index + size_of_type) <= buffer.size), "((buffer.index + sizeOfType) <= buffer.size)");
	value = *((unsigned long*) (buffer.data + buffer.index));
	buffer.index += size_of_type;
}

void qbuffer_reader::read(qbuffer& buffer, uint8_t* data, ssize_t length) {
	DEBUG_ASSERT(__LOGTAG__, ((buffer.index + length) <= buffer.size), "((buffer.index + length) <= buffer.size)");
	uint8_t* src = ((uint8_t*) (buffer.data + buffer.index));
	memcpy(data, src, length);
	buffer.index += length;
}
// --------------------------BufferWriter--------------------------------
// --------------------------BufferWriter--------------------------------
// --------------------------BufferWriter--------------------------------

void qbuffer_writer::write(qbuffer& buffer, int32_t value) const {
	uint32_t size_of_type = sizeof(int32_t);
	buffer.do_resize_if_required(size_of_type);
	*((int32_t*) (buffer.data + buffer.index)) = value;
	buffer.index += size_of_type;
}
void qbuffer_writer::write(qbuffer& buffer, uint32_t value) const {
	uint32_t size_of_type = sizeof(uint32_t);
	buffer.do_resize_if_required(size_of_type);
	*((uint32_t*) (buffer.data + buffer.index)) = value;
	buffer.index += size_of_type;
}
void qbuffer_writer::write(qbuffer& buffer, uint16_t value) const {
	uint32_t size_of_type = sizeof(uint16_t);
	buffer.do_resize_if_required(size_of_type);
	*((uint16_t*) (buffer.data + buffer.index)) = value;
	buffer.index += size_of_type;
}
void qbuffer_writer::write(qbuffer& buffer, uint8_t value) const {
	uint32_t size_of_type = sizeof(uint8_t);
	buffer.do_resize_if_required(size_of_type);
	*((uint8_t*) (buffer.data + buffer.index)) = value;
	buffer.index += size_of_type;
}
void qbuffer_writer::write(qbuffer& buffer, unsigned long value) const {
	uint32_t size_of_type = sizeof(unsigned long);
	buffer.do_resize_if_required(size_of_type);
	//    *((unsigned long*)(buffer.data + buffer.index)) = value;
	memcpy(buffer.data + buffer.index, &value, sizeof(unsigned long));
	buffer.index += size_of_type;
}

void qbuffer_writer::write(qbuffer& buffer, const uint8_t* data, ssize_t length) const {
	buffer.do_resize_if_required(length);
	uint8_t* dest = ((uint8_t*) (buffer.data + buffer.index));
	memcpy(dest, data, length);
	buffer.index += length;
}

void qbuffer_writer::write(qbuffer& buffer, const qstring& data) const {
	buffer.do_resize_if_required(data.length());
	uint8_t* dest = ((uint8_t*) (buffer.data + buffer.index));
	memcpy(dest, data.c_str(), data.length());
	buffer.index += data.length();
}
