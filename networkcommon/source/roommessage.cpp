//
//  roommessage.cpp
//  qserver
//
//  Created by Arun A on 02/03/24.
//

#include "roommessage.hpp"

// MARK: - room_message_base
room_message_base::room_message_base(unsigned long type_string_crc) : cached_type_string_crc(type_string_crc) {
}
room_message_base::~room_message_base() {
}

void room_message_base::serialize(rapidjson::Document& obj) {
    obj.SetObject();

    // Create a JSON object to hold the data
    Document::AllocatorType& allocator = obj.GetAllocator();
    obj.AddMember("sig", Value().SetUint(signature), allocator);
    obj.AddMember("t_crc", Value().SetInt64(get_type_crc()), allocator);
}

bool room_message_base::deserialize(rapidjson::Document& obj) {
    if (!obj.IsObject()) return false;

    if (obj.HasMember("sig") && obj["sig"].IsUint()) {
        signature = (unsigned short)obj["sig"].GetUint();
    } else return false;
    
    if (obj.HasMember("t_crc") && obj["t_crc"].IsInt64()) {
        cached_type_string_crc = obj["t_crc"].GetInt64();
    } else return false;
    
    return true;
}

// MARK: - room_config_message
DEFINE_ROOM_MESSAGE_PRE_REQUISITES(msg_room_config)
msg_room_config::msg_room_config() : room_message_base(msg_room_config::get_type_string_crc()) {
}

bool msg_room_config::deserialize(rapidjson::Document& obj) {
    if (!room_message_base::deserialize(obj)) {
        return false;
    }
    
    if (!obj.IsObject()) return false;

    // Check and assign "min"
    if (obj.HasMember("min") && obj["min"].IsInt()) {
        min = obj["min"].GetInt();
    } else return false;

    // Check and assign "max"
    if (obj.HasMember("max") && obj["max"].IsInt()) {
        max = obj["max"].GetInt();
    } else return false;

    // Check and assign "betx"
    if (obj.HasMember("betx") && obj["betx"].IsInt()) {
        betx = obj["betx"].GetInt();
    } else return false;
    
    // Check and assign "rewardx"
    if (obj.HasMember("rewardx") && obj["rewardx"].IsInt()) {
        rewardx = obj["rewardx"].GetInt();
    } else return false;
    
    // Check and assign "allow_after_start"
    if (obj.HasMember("allow_after_start") && obj["allow_after_start"].IsBool()) {
        allow_after_start = obj["allow_after_start"].GetBool();
    } else return false;

    return true;
}

void msg_room_config::serialize(rapidjson::Document& obj) {
    room_message_base::serialize(obj);
//    obj.SetObject();

    // Create a JSON object to hold the data
    Document::AllocatorType& allocator = obj.GetAllocator();
    obj.AddMember("min", Value().SetInt(min), allocator);
    obj.AddMember("max", Value().SetInt(max), allocator);
    obj.AddMember("betx", Value().SetInt(betx), allocator);
    obj.AddMember("rewardx", Value().SetInt(rewardx), allocator);
    obj.AddMember("allow_after_start", Value().SetBool(allow_after_start), allocator);
}

DEFINE_ROOM_MESSAGE_PRE_REQUISITES(msg_room_server_shutdown)
msg_room_server_shutdown::msg_room_server_shutdown() : room_message_base(msg_room_server_shutdown::get_type_string_crc()) {
}
void msg_room_server_shutdown::serialize(rapidjson::Document& obj) {
    room_message_base::serialize(obj);
}
bool msg_room_server_shutdown::deserialize(rapidjson::Document& obj) {
    if (!room_message_base::deserialize(obj)) {
        return false;
    }
    return true;
}


// MARK: - room_message_parser

room_message_parser::~room_message_parser() {
    records.clear();
}

template<typename T> void room_message_parser::register_message_type() {
    unsigned long crc = T::get_type_string_crc();
    std::map<unsigned long, type_room_message_create_cb>::iterator it = records.find(crc);
    if (it != records.end()) {
        DEBUG_WARN(LOG_LEVEL_2, __LOGTAG__, "room message type already registered - %s !!!", T::get_type_string().c_str());
        return;
    }
    records[crc] = &T::create;
    DEBUG_PRINT_IMPORTANT(__LOGTAG__, "room message type registered - %s : crc %ld !!!", T::get_type_string().c_str(), crc);
}

template<typename T> T* room_message_parser::parse(ssize_t len, uint8_t* buf) {
    unsigned long crc = T::get_type_string_crc();
    std::map<unsigned long, type_room_message_create_cb>::iterator it = records.find(crc);
    if (it == records.end()) {
        DEBUG_WARN(LOG_LEVEL_2, __LOGTAG__, "room message type not registered - %s !!!", T::get_type_string().c_str());
        return nullptr;
    }
    room_message_base* new_msg = records[crc]();
    T* msg = dynamic_cast<T*>(new_msg);
    if (msg == nullptr && new_msg != nullptr) {
        DEBUG_ASSERT(__LOGTAG__, msg!=nullptr, "room message dynamic_cast failed - %s !!!", T::get_type_string().c_str());
        GX_DELETE(new_msg); // Note :- dont use msg since it may be possible if dynamic_cast not succeeded (Edge cases)
        return nullptr;
    }
    rapidjson::Document obj;
    obj.Parse((char*)buf, len);
    if (msg->deserialize(obj)) {
        return msg;
    } else {
        GX_DELETE(msg);
        return nullptr;
    }
}
