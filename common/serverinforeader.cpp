//
//  serverinforeader.cpp
//  common
//
//  Created by Arun A on 10/02/25.
//

#include "serverinforeader.hpp"

using namespace gsdk::common;
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <map>
#include <mutex>

using namespace gsdk::common;

// define static members
server_info_reader* server_info_reader::instance = nullptr;
std::mutex server_info_reader::instance_mutex;

server_info_reader* server_info_reader::get_instance() {
	std::lock_guard<std::mutex> lock(instance_mutex);
	if (!instance) {
		instance = DEBUG_NEW server_info_reader();
		std::atexit(destroy_instance);	// auto-delete on exit
	}
	return instance;
}

void server_info_reader::destroy_instance() {
	std::lock_guard<std::mutex> lock(instance_mutex);
	GX_DELETE(instance);
}

bool server_info_reader::load_config(const qstring& file_path, bool force) {
	std::lock_guard<std::mutex> lock(config_mutex);
	if (loaded && !force) {
		debug_print(LOG_LEVEL_0, __LOGTAG__, "config file %s already loaded.", file_path.c_str());
		return true;
	}
	if (force) {
		debug_print(LOG_LEVEL_0, __LOGTAG__, "FORCE LOAD config file %s.", file_path.c_str());
	}

	FILE* file = fopen(file_path.c_str(), "r");
	if (!file) {
		debug_print_error(__LOGTAG__, "failed to open config file: %s", file_path.c_str());
		return false;
	}

	char* line = nullptr;  // must be NULL for 'getline' to allocate memory. consecutive calls may realloc the buffer if required.
	size_t len = 0;
	ssize_t read;

	int line_number = 0;
	while ((read = getline(&line, &len, file)) != -1) {
		line_number++;
		char* trimmed = trim(line);
		if (*trimmed == '\0' || *trimmed == '#')
			continue;

		char* delimiter = strchr(trimmed, '=');
		if (!delimiter) {
			if (strlen(trimmed) > 1) {
				debug_warn(LOG_LEVEL_0, __LOGTAG__, "invalid token at line %d", line_number);
			}
			continue;
		}

		*delimiter = '\0';
		char* key = trim(trimmed);
		char* value = trim(delimiter + 1);

		if (*key == '\0' || *value == '\0') {
			if (*key != '\0') {
				debug_warn(LOG_LEVEL_0, __LOGTAG__, "invalid value for key:'%s' at line %d", key, line_number);
			} else if (*value != '\0') {
				debug_warn(LOG_LEVEL_0, __LOGTAG__, "invalid token value '%s' at line %d", value, line_number);
			} else {
				debug_warn(LOG_LEVEL_0, __LOGTAG__, "invalid token at line %d", line_number);
			}
			continue;
		}

		size_t valueLen = strlen(value);
		if (valueLen >= 2 && ((value[0] == '"' && value[valueLen - 1] == '"') || (value[0] == '\'' && value[valueLen - 1] == '\''))) {
			value[valueLen - 1] = '\0';
			value++;
		}

		if ((value[0] == '"' && value[valueLen - 1] != '"') || (value[0] == '\'' && value[valueLen - 1] != '\'')) {
			continue;
		}

		config[key] = value;
	}
	fclose(file);

	if (line)
		free(line);	 // destroy the allocated memory by 'getline'
	loaded = true;
	debug_print(LOG_LEVEL_0, __LOGTAG__, "config file loaded - %s", file_path.c_str());
	return true;
}

const qstring server_info_reader::get_value(const qstring& key) {
	std::lock_guard<std::mutex> lock(config_mutex);
	auto it = config.find(key);
	return (it != config.end()) ? it->second : "";
}

void server_info_reader::print_config() {
	std::lock_guard<std::mutex> lock(config_mutex);
	debug_print(LOG_LEVEL_0, __LOGTAG__, "-- server configs --");
	for (const auto& pair : config) {
		debug_raw(LOG_LEVEL_0, "\t%s : %s", pair.first.c_str(), pair.second.c_str());
	}
}

char* server_info_reader::trim(char* str) {
	while (*str == ' ' || *str == '\t')
		str++;
	char* end = str + strlen(str) - 1;
	while (end > str && (*end == ' ' || *end == '\t' || *end == '\n' || *end == '\r')) {
		*end = '\0';
		end--;
	}
	return str;
}

const qstring server_info_reader::get_value_else_default(const qstring& key, const qstring& default_value) {
	std::lock_guard<std::mutex> lock(config_mutex);
	auto it = config.find(key);
	return (it != config.end()) ? it->second : default_value;
}

int server_info_reader::get_value_as_number(const qstring& key, int default_value) {
	const qstring& value = get_value(key);
	if (!value.isempty()) {
		return atoi(value.c_str());
	}
	return default_value;
}
