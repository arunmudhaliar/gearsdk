//
//  serverconfig.cpp
//  networkcommon
//
//  Created by Arun A on 08/03/24.
//

#include "serverconfig.hpp"

#include <algorithm>
#include <functional>
#include <rapidjson/error/en.h>	 // Include for GetParseError_En

serverconfig::serverconfig() {}

serverconfig::~serverconfig() {
	clear();
}

void serverconfig::clear() {
	configs.clear();
}

void serverconfig::load(const fs::path& path, qzookeeper* qzk, const qstring& zk_root_folder) {
	qstring buffer;
	if (qtextfile::get_content(path, buffer) == 0) {
		iterate_and_load_keys(buffer, qzk, zk_root_folder);
	} else {
		DEBUG_PRINT_ERROR(__LOGTAG__, "couldn't read zk config - %s", path.string().c_str());
	}
}

int serverconfig::get_config(const qstring& key, const qstring& default_value, qstring& result) {
	result = default_value;
	std::map<qstring, qstring>::iterator itr = configs.find(key);
	if (itr == configs.end()) {
		DEBUG_WARN(LOG_LEVEL_0, __LOGTAG__, "zk config not found for key %s. setting default value of %s !!!.", key.c_str(), default_value.c_str());
		return -1;
	}
	result = itr->second;
	return 0;
}

int serverconfig::get_int32(const qstring& key, const int32_t default_value) {
	std::map<qstring, qstring>::iterator itr = configs.find(key);
	if (itr == configs.end()) {
		DEBUG_WARN(LOG_LEVEL_0, __LOGTAG__, "get_int32 : zk config not found for key %s. setting default value of %d !!!.", key.c_str(), default_value);
		return default_value;
	}
	int result = default_value;
	if (gsdk::str2int(&result, itr->second.c_str(), 10) != gsdk::STR2INT_SUCCESS) {
		DEBUG_PRINT_ERROR(__LOGTAG__, "Unable to parse %s value - %s. setting default value of %d !!!.", key.c_str(), itr->second.c_str(), default_value);
	}
	return result;
}

qstring serverconfig::get_string(const qstring& key, const qstring& default_value) {
	std::map<qstring, qstring>::iterator itr = configs.find(key);
	if (itr == configs.end()) {
		DEBUG_WARN(LOG_LEVEL_0, __LOGTAG__, "get_string : zk config not found for key %s. setting default value of %s !!!.", key.c_str(), default_value.c_str());
		return default_value;
	}
	return itr->second;
}

void serverconfig::iterate_and_load_keys(const qstring& buffer, qzookeeper* qzk, const qstring& zk_root_folder) {
	rapidjson::Document doc;
	rapidjson::ParseResult ok = doc.Parse((char*) buffer.c_str(), buffer.length());
	if (!ok) {
		DEBUG_PRINT_ERROR(__LOGTAG__, "JSON parse error: %s (Offset: %d)", rapidjson::GetParseError_En(ok.Code()), ok.Offset());
		return;
	}

	std::map<qstring, std::vector<qstring>> config_keys;
	for (auto& m : doc.GetObject()) {
		DEBUG_PRINT(LOG_LEVEL_0, __LOGTAG__, "%s", m.name.GetString());
		if (!m.value.IsArray()) {
			DEBUG_WARN(LOG_LEVEL_0, __LOGTAG__, "root key(%s) must be an array !!!", m.name.GetString());
			continue;
		}
		for (const auto& n : m.value.GetArray()) {
			if (!n.IsString()) {
				DEBUG_WARN(LOG_LEVEL_0, __LOGTAG__, "value (%s) must be a string !!!", n.GetString());
				continue;
			}
			config_keys[m.name.GetString()].push_back(n.GetString());
		}
	}

	// load for zk
	for (auto kv : config_keys) {
		for (auto c : kv.second) {
			qstring zk_key = qstring::format_string("%s/%s/%s", zk_root_folder.c_str(), kv.first.c_str(), c.c_str());
			qstring zk_res;
			int result = qzk->get_data(zk_key, zk_res);
			if (result != 0) {
				continue;
			}
			qstring mod_zk_key = qstring::format_string("%s/%s", kv.first.c_str(), c.c_str());
			configs[mod_zk_key] = zk_res;
		}
	}
}
