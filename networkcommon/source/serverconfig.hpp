//
//  serverconfig.hpp
//  networkcommon
//
//  Created by Arun A on 08/03/24.
//

#ifndef serverconfig_hpp
#define serverconfig_hpp

#include <map>
#include "qtextfile.hpp"
#include "../../common/qstring.h"
#include "../../qzookeeper/source/qzookeeper.hpp"
#include <rapidjson/rapidjson.h>
#include <rapidjson/document.h>
#include <map>
#include <vector>
#include <algorithm>

#undef __LOGTAG__
#define __LOGTAG__ "serverconfig"

class serverconfig {
public:
    serverconfig();
    ~serverconfig();
    
    void clear();
    void load(const fs::path& path, qzookeeper* qzk, const qstring& zk_root_folder);
    int get_config(const qstring& key, const qstring& default_value, qstring& result);
    
private:
    void iterate_and_load_keys(const qstring& buffer, qzookeeper* qzk, const qstring& zk_root_folder);
    
    std::map<qstring, qstring> configs;
     
};
#endif /* serverconfig_hpp */
