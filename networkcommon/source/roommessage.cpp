//
//  roommessage.cpp
//  networkcommon
//
//  Created by Arun A on 24/03/24.
//
#include "roommessage.hpp"

// MARK: - message_room_base
DEFINE_MESSAGE_PRE_REQUISITES(message_room_base)
message_room_base::message_room_base() : message_base() {
    DEBUG_ASSERT(__LOGTAG__, false, "This line of code must not be executed !!!");
}
message_room_base::message_room_base(unsigned long type_string_crc) : message_base(), cached_type_string_crc(type_string_crc) {
}
message_room_base::~message_room_base() {
}

void message_room_base::serialize(rapidjson::Value& obj, rapidjson::Document::AllocatorType& allocator) const {
    obj.SetObject();

    // Create a JSON object to hold the data
    obj.AddMember("sig", Value().SetUint(signature), allocator);
    obj.AddMember("t_crc", Value().SetInt64(get_type_crc()), allocator);
}

bool message_room_base::deserialize(rapidjson::Value& obj) {
    if (obj.IsObject()) {
        if (obj.HasMember("sig") && obj["sig"].IsUint()) {
            signature = (unsigned short)obj["sig"].GetUint();
        } else {
            DEBUG_PRINT_IMPORTANT2(__LOGTAG__, "%s - message dont have sig", get_type().c_str());
        }
        if (obj.HasMember("t_crc") && obj["t_crc"].IsInt64()) {
            cached_type_string_crc = obj["t_crc"].GetInt64();
            DEBUG_ASSERT(__LOGTAG__, cached_type_string_crc==get_type_crc(), __FUNCTION__);
        } else {
            DEBUG_PRINT_IMPORTANT2(__LOGTAG__, "%s - message dont have t_crc", get_type().c_str());
        }
    } else {
        DEBUG_PRINT_IMPORTANT(__LOGTAG__, "%s - message dont have sig or t_crc", get_type().c_str());
    }
    return true;
}

int message_room_base::deserialize_header(ssize_t len, uint8_t* buf, unsigned short& sig, unsigned long& t_crc, rapidjson::Document& doc) {
    doc.Parse((char*)buf, len);
    if (doc.HasParseError()) {
        DEBUG_PRINT_ERROR(__LOGTAG__, "message_room_base::deserialize_header failed while parsing json string %.*s", len, buf);
        return -1;
    }
    if (doc.IsObject()) {
        if (doc.HasMember("sig") && doc["sig"].IsUint()) {
            sig = (unsigned short)doc["sig"].GetUint();
        } else {
            DEBUG_PRINT_IMPORTANT2(__LOGTAG__, "room message dont have sig - %.*s", len, buf);
        }
        if (doc.HasMember("t_crc") && doc["t_crc"].IsInt64()) {
            t_crc = doc["t_crc"].GetInt64();
        } else {
            DEBUG_PRINT_IMPORTANT2(__LOGTAG__, "room message dont have t_crc - %.*s", len, buf);
            return -3;
        }
    } else {
        DEBUG_PRINT_IMPORTANT(__LOGTAG__, "room message dont have sig or t_crc - %.*s", len, buf);
        return -2;
    }
    return 0;
}

// MARK: - msg_room_match_request
DEFINE_MESSAGE_PRE_REQUISITES(msg_room_match_request)
msg_room_match_request::msg_room_match_request() : message_room_base(msg_room_match_request::get_type_string_crc()) {
}
void msg_room_match_request::serialize(rapidjson::Value& obj, rapidjson::Document::AllocatorType& allocator) const {
    message_room_base::serialize(obj, allocator);
    
    rapidjson::Value room_config_obj(rapidjson::kObjectType);
    room_config.serialize(room_config_obj, allocator);
    obj.AddMember("room_config", room_config_obj, allocator);
    
}
bool msg_room_match_request::deserialize(rapidjson::Value& obj) {
    if (!message_room_base::deserialize(obj)) {
        return false;
    }
    
    if (!obj.IsObject()) return false;

    // Access the nested objects and values
    if (obj.HasMember("room_config") && obj["room_config"].IsObject()) {
        Value& room_config_obj = obj["room_config"];
        return room_config.deserialize(room_config_obj);
    } else {
        return false;
    }
    return false;
}

// MARK: - msg_room_server_shutdown
DEFINE_MESSAGE_PRE_REQUISITES(msg_room_server_shutdown)
msg_room_server_shutdown::msg_room_server_shutdown() : message_room_base(msg_room_server_shutdown::get_type_string_crc()) {
}
void msg_room_server_shutdown::serialize(rapidjson::Value& obj, rapidjson::Document::AllocatorType& allocator) const {
    message_room_base::serialize(obj, allocator);
}
bool msg_room_server_shutdown::deserialize(rapidjson::Value& obj) {
    message_room_base::deserialize(obj);
    return true;
}
