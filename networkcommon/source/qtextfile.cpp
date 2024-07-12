//
//  Copyright 2024 homenet25
//  qtextfile.cpp
//  networkcommon
//
//  Created by Arun A on 28/12/23.
//

#include "qtextfile.hpp"

qtextfile::qtextfile() {}

qtextfile::qtextfile(const fs::path& path, const qstring& mode) {
	open(path, mode);
}

qtextfile::~qtextfile() {
	close();
}

int qtextfile::open(const fs::path& path, const qstring& mode) {
	if (fp != nullptr) {
		DEBUG_PRINT_ERROR(__LOGTAG__, "file open failed - already opened a file. fp != null !!!");
		return -1;
	}
	fp = fopen(path.c_str(), mode.c_str());
	if (fp == nullptr) {
		DEBUG_PRINT_ERROR(__LOGTAG__, "file open failed - %s !!!", path.c_str());
	}
	return fp == nullptr ? -1 : 0;
}

void qtextfile::close() {
	if (fp) {  // just close the file.
		fclose(fp);
		fp = nullptr;
	}
}

int qtextfile::seek_to_begining() {
	if (fp == nullptr) {
		return -1;
	}
	// Move to the beginning of the file
	if (fseek(fp, 0, SEEK_SET) != 0) {
		DEBUG_PRINT_ERROR(__LOGTAG__, "Error seeking to the beginning of the file");
		close();
		return -1;
	}
	return 0;
}

ssize_t qtextfile::read() {
	if (fp == nullptr) {
		DEBUG_PRINT_ERROR(__LOGTAG__, "file to read - fp == nullptr !!!");
		return -1;
	}

	buffer.clear();
	const int str_sz = 1024;
	char str[str_sz];
	if (seek_to_begining() != 0) {
		return -1;
	}
	while (fgets(str, str_sz, fp)) {
		buffer += str;
	}
	return buffer.length() > 0 ? buffer.length() : -1;
}

int qtextfile::get_content(const fs::path& path, qstring& out) {
	qtextfile file(path);
	if (file.read() < 0) {
		return -1;
	}
	out = file.get_buffer();
	return 0;
}
