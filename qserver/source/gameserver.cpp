//
//  gameserver.cpp
//  NetworkServer
//
//  Created by Arun A on 15/10/23.
//

#include "gameserver.hpp"

room* gameserver::create_room() {
    return DEBUG_NEW room(this, roomconfig(2, 4, false));
}
