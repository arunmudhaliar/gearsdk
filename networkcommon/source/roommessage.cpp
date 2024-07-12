//
//  Copyright 2024 homenet25
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
message_room_base::message_room_base(unsigned long type_string_crc) : message_base(), cached_type_string_crc(type_string_crc) {}
message_room_base::~message_room_base() {}

void message_room_base::serialize(rapidjson::Value& obj, rapidjson::Document::AllocatorType& allocator) const {
	obj.SetObject();

	// Create a JSON object to hold the data
	obj.AddMember("sig", Value().SetUint(signature), allocator);
	obj.AddMember("t_crc", Value().SetInt64(get_type_crc()), allocator);
}

bool message_room_base::deserialize(rapidjson::Value& obj) {
	if (obj.IsObject()) {
		if (obj.HasMember("sig") && obj["sig"].IsUint()) {
			signature = (unsigned short) obj["sig"].GetUint();
		} else {
			DEBUG_PRINT_IMPORTANT2(__LOGTAG__, "%s - message dont have sig", get_type().c_str());
		}
		if (obj.HasMember("t_crc") && obj["t_crc"].IsInt64()) {
			cached_type_string_crc = obj["t_crc"].GetInt64();
			DEBUG_ASSERT(__LOGTAG__, cached_type_string_crc == get_type_crc(), __FUNCTION__);
		} else {
			DEBUG_PRINT_IMPORTANT2(__LOGTAG__, "%s - message dont have t_crc", get_type().c_str());
		}
	} else {
		DEBUG_PRINT_IMPORTANT(__LOGTAG__, "%s - message dont have sig or t_crc", get_type().c_str());
	}
	return true;
}

int message_room_base::deserialize_header(ssize_t len, uint8_t* buf, unsigned short& sig, unsigned long& t_crc, rapidjson::Document& doc) {
	doc.Parse((char*) buf, len);
	if (doc.HasParseError()) {
		DEBUG_PRINT_ERROR(__LOGTAG__, "message_room_base::deserialize_header failed while parsing json string %.*s", len, buf);
		return -1;
	}
	if (doc.IsObject()) {
		if (doc.HasMember("sig") && doc["sig"].IsUint()) {
			sig = (unsigned short) doc["sig"].GetUint();
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
msg_room_match_request::msg_room_match_request() : message_room_base(msg_room_match_request::get_type_string_crc()) {}
void msg_room_match_request::serialize(rapidjson::Value& obj, rapidjson::Document::AllocatorType& allocator) const {
	message_room_base::serialize(obj, allocator);

	rapidjson::Value room_config_obj(rapidjson::kObjectType);
	room_config.serialize(room_config_obj, allocator);
	obj.AddMember("room_config", room_config_obj, allocator);
    obj.AddMember("prev_cid_hash_val", Value().SetUint(prev_cid_hash_val), allocator);
    obj.AddMember("room_id", Value().SetInt(room_id), allocator);
    obj.AddMember("pid", Value().SetString(pid.c_str(), (int)pid.length(), allocator), allocator);
}
bool msg_room_match_request::deserialize(rapidjson::Value& obj) {
	if (!message_room_base::deserialize(obj)) {
		return false;
	}

	if (!obj.IsObject())
		return false;

	// Access the nested objects and values
	if (obj.HasMember("room_config") && obj["room_config"].IsObject()) {
		Value& room_config_obj = obj["room_config"];
        if (!room_config.deserialize(room_config_obj)) {
            return false;
        }
	} else {
		return false;
	}
    if (obj.HasMember("pid") && obj["pid"].IsString()) {
        pid = obj["pid"].GetString();
    } else {
        return false;
    }
    
    if (obj.HasMember("prev_cid_hash_val") && obj["prev_cid_hash_val"].IsUint()) {
        prev_cid_hash_val = obj["prev_cid_hash_val"].GetUint();
    }
    if (obj.HasMember("room_id") && obj["room_id"].IsInt()) {
        room_id = obj["room_id"].GetInt();
    }
	return true;
}

// MARK: - msg_room_server_shutdown
DEFINE_MESSAGE_PRE_REQUISITES(msg_room_server_shutdown)
msg_room_server_shutdown::msg_room_server_shutdown() : message_room_base(msg_room_server_shutdown::get_type_string_crc()) {}
void msg_room_server_shutdown::serialize(rapidjson::Value& obj, rapidjson::Document::AllocatorType& allocator) const {
	message_room_base::serialize(obj, allocator);
}
bool msg_room_server_shutdown::deserialize(rapidjson::Value& obj) {
	message_room_base::deserialize(obj);
	return true;
}

// MARK: - msg_room_server_event_base
DEFINE_MESSAGE_PRE_REQUISITES(msg_room_server_event_base)
msg_room_server_event_base::msg_room_server_event_base(unsigned long type_string_crc) : message_room_base(type_string_crc) {}
void msg_room_server_event_base::serialize(rapidjson::Value& obj, rapidjson::Document::AllocatorType& allocator) const {
    message_room_base::serialize(obj, allocator);
    obj.AddMember("room_event", Value().SetString(room_event.c_str(), (unsigned int)room_event.length(), allocator), allocator);
}
bool msg_room_server_event_base::deserialize(rapidjson::Value& obj) {
    if (!message_room_base::deserialize(obj)) {
        return false;
    }
    // Access the nested objects and values
    if (obj.HasMember("room_event") && obj["room_event"].IsString()) {
        room_event = obj["room_event"].GetString();
    } else {
        return false;
    }
    return true;
}


// MARK: - room_player
void room_player::serialize(rapidjson::Value& obj, rapidjson::Document::AllocatorType& allocator) const {
    obj.AddMember("hash", Value().SetUint(hash), allocator);
    obj.AddMember("flag", Value().SetBool(flag), allocator);
    obj.AddMember("pid", Value().SetString(pid.c_str(), (int)pid.length(), allocator), allocator);
}
bool room_player::deserialize(rapidjson::Value& obj) {
    if (obj.HasMember("hash") && obj["hash"].IsUint()) {
        hash = obj["hash"].GetUint();
    } else {
        return false;
    }
    if (obj.HasMember("flag") && obj["flag"].IsBool()) {
        flag = obj["flag"].GetBool();
    } else {
        return false;
    }
    if (obj.HasMember("pid") && obj["pid"].IsString()) {
        pid = obj["pid"].GetString();
    } else {
        return false;
    }
    return true;
}

// MARK: - msg_room_server_event_player_add
DEFINE_MESSAGE_PRE_REQUISITES(msg_room_server_event_player_add)
msg_room_server_event_player_add::msg_room_server_event_player_add() : msg_room_server_event_base(msg_room_server_event_player_add::get_type_string_crc()) {
    room_event = "player-add";
}
msg_room_server_event_player_add::~msg_room_server_event_player_add() {
    for (auto* p : players) {
        GX_DELETE(p);
    }
    players.clear();
}
void msg_room_server_event_player_add::serialize(rapidjson::Value& obj, rapidjson::Document::AllocatorType& allocator) const {
    msg_room_server_event_base::serialize(obj, allocator);
    obj.AddMember("room_id", Value().SetInt(room_id), allocator);
    obj.AddMember("self", Value().SetUint(self), allocator);
    // Create an array of objects

    rapidjson::Value array(rapidjson::kArrayType);
    for (auto* p : players) {
        rapidjson::Value obj1(rapidjson::kObjectType);
        p->serialize(obj1, allocator);
        array.PushBack(obj1, allocator);
    }
    obj.AddMember("players", array, allocator);
}
    
bool msg_room_server_event_player_add::deserialize(rapidjson::Value& obj) {
    if (!msg_room_server_event_base::deserialize(obj)) {
        return false;
    }
    if (obj.HasMember("room_id") && obj["room_id"].IsInt()) {
        room_id = obj["room_id"].GetInt();
    } else {
        return false;
    }
    if (obj.HasMember("self") && obj["self"].IsUint()) {
        self = obj["self"].GetUint();
    }
    
    if (obj.HasMember("players") && obj["players"].IsObject()) {
        Value& players_obj = obj["players"];
        if (!players_obj.IsArray())
            return false;
        // Iterate over the array
        for (auto& elem : players_obj.GetArray()) {
            room_player* p = DEBUG_NEW room_player();
            if (!p->deserialize(elem)) {
                GX_DELETE(p);
                continue;
            }
            players.push_back(p);
        }
    }
    return true;
}

// MARK: - msg_room_server_event_player_remove
DEFINE_MESSAGE_PRE_REQUISITES(msg_room_server_event_player_remove)
msg_room_server_event_player_remove::msg_room_server_event_player_remove() : msg_room_server_event_base(msg_room_server_event_player_remove::get_type_string_crc()) {
    room_event = "player-remove";
}
msg_room_server_event_player_remove::~msg_room_server_event_player_remove() {
    for (auto* p : players) {
        GX_DELETE(p);
    }
    players.clear();
}
void msg_room_server_event_player_remove::serialize(rapidjson::Value& obj, rapidjson::Document::AllocatorType& allocator) const {
    msg_room_server_event_base::serialize(obj, allocator);
    obj.AddMember("room_id", Value().SetInt(room_id), allocator);
    obj.AddMember("self", Value().SetUint(self), allocator);
    // Create an array of objects
    rapidjson::Value array(rapidjson::kArrayType);
    for (auto* p : players) {
        rapidjson::Value obj1(rapidjson::kObjectType);
        p->serialize(obj1, allocator);
        array.PushBack(obj1, allocator);
    }
    obj.AddMember("players", array, allocator);
}
    
bool msg_room_server_event_player_remove::deserialize(rapidjson::Value& obj) {
    if (!msg_room_server_event_base::deserialize(obj)) {
        return false;
    }
    if (obj.HasMember("room_id") && obj["room_id"].IsInt()) {
        room_id = obj["room_id"].GetInt();
    } else {
        return false;
    }
    if (obj.HasMember("self") && obj["self"].IsUint()) {
        self = obj["self"].GetUint();
    }
    
    if (obj.HasMember("players") && obj["players"].IsObject()) {
        Value& players_obj = obj["players"];
        if (!players_obj.IsArray())
            return false;
        // Iterate over the array
        for (auto& elem : players_obj.GetArray()) {
            room_player* p = DEBUG_NEW room_player();
            if (!p->deserialize(elem)) {
                GX_DELETE(p);
                continue;
            }
            players.push_back(p);
        }
    }
    return true;
}

// MARK: - msg_room_server_event_start
DEFINE_MESSAGE_PRE_REQUISITES(msg_room_server_event_start)
msg_room_server_event_start::msg_room_server_event_start() : msg_room_server_event_base(msg_room_server_event_start::get_type_string_crc()) {
    room_event = "room-start";
}
void msg_room_server_event_start::serialize(rapidjson::Value& obj, rapidjson::Document::AllocatorType& allocator) const {
    msg_room_server_event_base::serialize(obj, allocator);
    obj.AddMember("room_id", Value().SetInt(room_id), allocator);
}
bool msg_room_server_event_start::deserialize(rapidjson::Value& obj) {
    if (!msg_room_server_event_base::deserialize(obj)) {
        return false;
    }
    if (obj.HasMember("room_id") && obj["room_id"].IsInt()) {
        room_id = obj["room_id"].GetInt();
    } else {
        return false;
    }
    return true;
}

// MARK: - msg_room_server_event_end
DEFINE_MESSAGE_PRE_REQUISITES(msg_room_server_event_end)
msg_room_server_event_end::msg_room_server_event_end() : msg_room_server_event_base(msg_room_server_event_end::get_type_string_crc()) {
    room_event = "room-end";
}
void msg_room_server_event_end::serialize(rapidjson::Value& obj, rapidjson::Document::AllocatorType& allocator) const {
    msg_room_server_event_base::serialize(obj, allocator);
    obj.AddMember("room_id", Value().SetInt(room_id), allocator);
}
bool msg_room_server_event_end::deserialize(rapidjson::Value& obj) {
    if (!msg_room_server_event_base::deserialize(obj)) {
        return false;
    }
    if (obj.HasMember("room_id") && obj["room_id"].IsInt()) {
        room_id = obj["room_id"].GetInt();
    } else {
        return false;
    }
    return true;
}

