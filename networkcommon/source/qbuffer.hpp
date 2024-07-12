//
//  Copyright 2024 homenet25
//  qbuffer.hpp
//  networkcommon
//
//  Created by Arun A on 02/11/23.
//

#ifndef qbuffer_hpp
#define qbuffer_hpp

#include "../../common/qstring.h"
#include "../../common/sdktypes.hpp"

#include <stdint.h>
#include <stdio.h>

#undef __LOGTAG__
#define __LOGTAG__ "qbuffer"

struct qbuffer {
	~qbuffer() {
		GX_DELETE_ARY(data);
		DEBUG_PRINT(LOG_LEVEL_5, __LOGTAG__, "Buffer destroyed.");
	}
	void allocate(ssize_t sz) {
		GX_DELETE_ARY(data);
		data = DEBUG_NEW uint8_t[sz];
		size = sz;
		index = pushed_index = 0;
	}
	void seek_to_begin() { index = 0; }
	void do_resize_if_required(ssize_t bytes_needed);

	void stash() { pushed_index = index; }
	ssize_t pop() {
		if (pushed_index > size) {
			return -1;
		}
		index = pushed_index;
		return index;
	}

	uint8_t* data = nullptr;  // pointer to buffer data
	ssize_t size = 0;		  // size of buffer data (bytes)
	ssize_t index = 0;		  // index of next byte to be read/written
	ssize_t pushed_index = 0;
};

class qbuffer_reader {
   public:
	void read(qbuffer& buffer, uint32_t& value);
	void read(qbuffer& buffer, uint16_t& value);
	void read(qbuffer& buffer, uint8_t& value);
	void read(qbuffer& buffer, uint8_t* data, ssize_t length);
	void read(qbuffer& buffer, unsigned long& value);
};	// class BufferReader

class qbuffer_writer {
   public:
	void write(qbuffer& buffer, int32_t value) const;
	void write(qbuffer& buffer, uint32_t value) const;
	void write(qbuffer& buffer, uint16_t value) const;
	void write(qbuffer& buffer, uint8_t value) const;
	void write(qbuffer& buffer, const uint8_t* data, ssize_t length) const;
	void write(qbuffer& buffer, unsigned long value) const;
	void write(qbuffer& buffer, const qstring& data) const;
};	// class BufferWriter

#endif /* qbuffer_hpp */
