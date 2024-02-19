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

class game_room : public room {
public:
    game_room(roomserver_interface*, const roomconfig& room_config);
    virtual ~game_room();
    
protected:
    void onroom_create() override;
    void onroom_start() override;
    void onroom_player_added(player* p) override;
    void onroom_message(player* p, const qstring& msg) override;
    void onroom_player_removed(player* p) override;
    void onroom_end() override;
};

class gameserver : public roomserver {
protected:
    room* create_room() override;
};

#endif /* gameserver_hpp */
