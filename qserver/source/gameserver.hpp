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
    void onroom_create(room*) override;
    void onroom_start(room*) override;
    void onroom_player_added(room*, player*) override;
    void onroom_message(room*, player*, const qstring& msg) override;
    void onroom_player_removed(room*, player*) override;
    void onroom_end(room*) override;

};

#endif /* gameserver_hpp */
