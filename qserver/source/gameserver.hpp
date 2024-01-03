//
//  gameserver.hpp
//  NetworkServer
//
//  Created by Arun A on 15/10/23.
//

#ifndef gameserver_hpp
#define gameserver_hpp

#include "roomserver.hpp"

#undef __LOGTAG__
#define __LOGTAG__ "gameserver"

class gameserver : public roomserver {
protected:
    room* create_room() override;
};

#endif /* gameserver_hpp */
