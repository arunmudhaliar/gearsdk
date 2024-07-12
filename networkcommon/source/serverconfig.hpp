//
//  Copyright 2024 homenet25
//  serverconfig.hpp
//  networkcommon
//
//  Created by Arun A on 08/03/24.
//

#ifndef serverconfig_hpp
#define serverconfig_hpp

#include "../../common/qstring.h"
#include "../../qzookeeper/source/qzookeeper.hpp"
#include "qtextfile.hpp"
#include <map>
#include <rapidjson/document.h>
#include <rapidjson/rapidjson.h>

#undef __LOGTAG__
#define __LOGTAG__ "serverconfig"

class serverconfig {
private:
    serverconfig(){}
   public:
	serverconfig(interface_qzookeeper* interface);
	~serverconfig();

	void clear();
	void load(const fs::path& path, qzookeeper* qzk, const qstring& zk_root_folder);
	int get_config(const qstring& key, const qstring& default_value, qstring& result);
	int get_int32(const qstring& key, const int32_t default_value);
	qstring get_string(const qstring& key, const qstring& default_value);
    
   private:
    static void zk_value_change_listener(const qstring& path, const qstring& data, void* context);
	void iterate_and_load_keys(const qstring& buffer, qzookeeper* qzk, const qstring& zk_root_folder);
    bool try_update_value(const qstring& path, const qstring& data);
    
	std::map<qstring, qstring> configs;
    interface_qzookeeper* zk_interface;
};
#endif /* serverconfig_hpp */
