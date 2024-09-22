//
//  Copyright 2024 homenet25
//  serverconfig.cpp
//  networkcommon
//
//  Created by Arun A on 08/03/24.
//

#include "serverconfig.hpp"

#include <rapidjson/error/en.h>	 // Include for GetParseError_En

serverconfig::serverconfig(interface_qzookeeper* interface, observer_serverconfig* observer) : zk_interface(interface), config_change_observer(observer) {
	zk_interface->register_value_change_callback(serverconfig::zk_value_change_listener, this);
}

serverconfig::~serverconfig() {
	clear();
	zk_interface->unregister_value_change_callback(serverconfig::zk_value_change_listener, this);
}

void serverconfig::clear() {
	configs.clear();
}

bool serverconfig::load(const fs::path& path, qzookeeper* qzk, const qstring& zk_root_folder) {
	qstring buffer;
	bool result = false;
	if (qtextfile::get_content(path, buffer) == 0) {
		result = iterate_and_load_keys(buffer, qzk, zk_root_folder);
	} else {
		debug_print_error(__LOGTAG__, "couldn't read zk config - %s", path.string().c_str());
	}
	return result;
}

int serverconfig::get_config(const qstring& key, const qstring& default_value, qstring& result) {
	result = default_value;
	std::map<qstring, qstring>::iterator itr = configs.find(key);
	if (itr == configs.end()) {
		debug_warn(LOG_LEVEL_0, __LOGTAG__, "zk config not found for key %s. setting default value of %s !!!.", key.c_str(), default_value.c_str());
		return -1;
	}
	result = itr->second;
	return 0;
}

int serverconfig::get_int32(const qstring& key, const int32_t DEFAULT_VALUE) {
	std::map<qstring, qstring>::iterator itr = configs.find(key);
	if (itr == configs.end()) {
		debug_warn(LOG_LEVEL_0, __LOGTAG__, "get_int32 : zk config not found for key %s. setting default value of %d !!!.", key.c_str(), DEFAULT_VALUE);
		return DEFAULT_VALUE;
	}
	int result = DEFAULT_VALUE;
	if (gsdk::str2int(&result, itr->second.c_str(), itr->second.length(), 10) != gsdk::STR2INT_SUCCESS) {
		debug_print_error(__LOGTAG__, "Unable to parse %s value - %s. setting default value of %d !!!.", key.c_str(), itr->second.c_str(), DEFAULT_VALUE);
	}
	return result;
}

qstring serverconfig::get_string(const qstring& key, const qstring& default_value) {
	std::map<qstring, qstring>::iterator itr = configs.find(key);
	if (itr == configs.end()) {
		debug_warn(LOG_LEVEL_0, __LOGTAG__, "get_string : zk config not found for key %s. setting default value of %s !!!.", key.c_str(), default_value.c_str());
		return default_value;
	}
	return itr->second;
}

bool serverconfig::iterate_and_load_keys(const qstring& buffer, qzookeeper* qzk, const qstring& zk_root_folder) {
	rapidjson::Document doc;
	rapidjson::ParseResult ok = doc.Parse((char*) buffer.c_str(), buffer.length());
	if (!ok) {
		debug_print_error(__LOGTAG__, "JSON parse error: %s (Offset: %d)", rapidjson::GetParseError_En(ok.Code()), ok.Offset());
		return false;
	}

	std::map<qstring, std::vector<qstring>> config_keys;
	for (auto& m : doc.GetObject()) {
		debug_print(LOG_LEVEL_0, __LOGTAG__, "%s", m.name.GetString());
		if (!m.value.IsArray()) {
			debug_warn(LOG_LEVEL_0, __LOGTAG__, "root key(%s) must be an array !!!", m.name.GetString());
			return false;
		}
		for (const auto& n : m.value.GetArray()) {
			if (!n.IsString()) {
				debug_warn(LOG_LEVEL_0, __LOGTAG__, "value (%s) must be a string !!!", n.GetString());
				return false;
			}
			// debug_print(LOG_LEVEL_0, __LOGTAG__, "keys %s", n.GetString());
			config_keys[m.name.GetString()].push_back(n.GetString());
		}
	}

	// load for zk
	for (auto kv : config_keys) {
		for (auto c : kv.second) {
			qstring zk_key = qstring::format_string("%s/%s/%s", zk_root_folder.c_str(), kv.first.c_str(), c.c_str());
			qstring zk_res;
			int result = qzk->get_data(zk_key, zk_res);
			if (result == ZNONODE) {
				//                debug_print_important2(__LOGTAG__, "Node does not exist (ZNONODE) for key %s. continuing...", zk_key.c_str());
				continue;
			}
			if (result != 0) {
				return false;
			}
			qstring mod_zk_key = qstring::format_string("%s/%s", kv.first.c_str(), c.c_str());
			configs[mod_zk_key] = zk_res;
		}
	}
	// debug_print(LOG_LEVEL_0, __LOGTAG__, "%d keys loaded", configs.size());
	return true;
}

bool serverconfig::try_update_value(const qstring& path, const qstring& data) {
	std::vector<qstring> array;
	path.split("/", array);
	qstring mod_zk_key(path);
	if (array.size() > 1) {
		mod_zk_key.replace(qstring::format_string("/%s/", array[1].c_str()), "");
	}
	if (configs.find(mod_zk_key) != configs.end()) {
		configs[mod_zk_key] = data;
		return true;
	}
	return false;
}
void serverconfig::zk_value_change_listener(const qstring& path, const qstring& data, void* context) {
	serverconfig* config = reinterpret_cast<serverconfig*>(context);
	if (config->try_update_value(path, data)) {
		debug_print(LOG_LEVEL_2, __LOGTAG__, "config updated k:%s", path.c_str());
		if (config->config_change_observer) {
			config->config_change_observer->configchanged(path, data);
		}
	} else {
		debug_warn(LOG_LEVEL_0, __LOGTAG__, "config not found for k:%s", path.c_str());
	}
}
