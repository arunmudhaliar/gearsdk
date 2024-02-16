//
//  discord_util.hpp
//  servercommon
//
//  Created by Arun A on 16/02/24.
//

#ifndef discord_util_hpp
#define discord_util_hpp

#include "../../common/qstring.h"

#undef __LOGTAG__
#define __LOGTAG__ "discord_util"

class discord_util {
public:
    static void set_web_hook(const qstring& web_hook);
    static int send(const qstring& msg);
    
private:
    static qstring current_web_hook;
};

#endif /* discord_util_hpp */
