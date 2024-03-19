//
//  gameclient.cpp
//  networkclient
//
//  Created by Arun A on 15/10/23.
//

#include "gameclient.hpp"
#include "../../networkcommon/source/message.hpp"

void gameclient::onconnect(conn_io_client* qconnection) {
    if (shutdown_client) {
        test_send_shutdown_event();
        return;
    }
    Document doc;
    msg_room_config* msg = dynamic_cast<msg_room_config*>(msg_room_config::create());
    qstring msg_room_config_packet;
    msg->serialize(doc);
    essentials::get_json_string(doc, msg_room_config_packet);
    DEBUG_PRINT(LOG_LEVEL_2, __LOGTAG__, "room config --> %s", msg_room_config_packet.c_str());
    sendMessage(msg_room_config_packet, true);
}

void gameclient::test_send_shutdown_event() {
    Document doc;
    msg_room_server_shutdown* msg = dynamic_cast<msg_room_server_shutdown*>(msg_room_server_shutdown::create());
    qstring msg_room_server_shutdown_packet;
    msg->serialize(doc);
    essentials::get_json_string(doc, msg_room_server_shutdown_packet);
    DEBUG_PRINT(LOG_LEVEL_2, __LOGTAG__, "room shutdown --> %s", msg_room_server_shutdown_packet.c_str());
    sendMessage(msg_room_server_shutdown_packet, true);
}
