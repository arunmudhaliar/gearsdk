//
//  message.cpp
//  qserver
//
//  Created by Arun A on 02/03/24.
//

#include "message.hpp"

// MARK: - message_base
message_base::~message_base() {}

void message_base::serialize(rapidjson::Value& obj, rapidjson::Document::AllocatorType& allocator) const {
	UNUSED(allocator);
	obj.SetObject();
}

bool message_base::deserialize(rapidjson::Value& obj) {
	UNUSED(obj);
	return true;
}

// MARK: - msg_room_config
DEFINE_MESSAGE_PRE_REQUISITES(msg_room_config)
msg_room_config::msg_room_config() : message_base() {}

bool msg_room_config::deserialize(rapidjson::Value& obj) {
	message_base::deserialize(obj);

	if (!obj.IsObject())
		return false;

	// Check and assign "min"
	if (obj.HasMember("min") && obj["min"].IsInt()) {
		min = obj["min"].GetInt();
	} else
		return false;

	// Check and assign "max"
	if (obj.HasMember("max") && obj["max"].IsInt()) {
		max = obj["max"].GetInt();
	} else
		return false;

	// Check and assign "betx"
	if (obj.HasMember("betx") && obj["betx"].IsInt()) {
		betx = obj["betx"].GetInt();
	} else
		return false;

	// Check and assign "rewardx"
	if (obj.HasMember("rewardx") && obj["rewardx"].IsInt()) {
		rewardx = obj["rewardx"].GetInt();
	} else
		return false;

	// Check and assign "allow_after_start"
	if (obj.HasMember("allow_after_start") && obj["allow_after_start"].IsBool()) {
		allow_after_start = obj["allow_after_start"].GetBool();
	} else
		return false;

	return true;
}

void msg_room_config::serialize(rapidjson::Value& obj, rapidjson::Document::AllocatorType& allocator) const {
	message_base::serialize(obj, allocator);
	//    obj.SetObject();

	// Create a JSON object to hold the data
	obj.AddMember("min", Value().SetInt(min), allocator);
	obj.AddMember("max", Value().SetInt(max), allocator);
	obj.AddMember("betx", Value().SetInt(betx), allocator);
	obj.AddMember("rewardx", Value().SetInt(rewardx), allocator);
	obj.AddMember("allow_after_start", Value().SetBool(allow_after_start), allocator);
}

// MARK: - msg_room_config_list
DEFINE_MESSAGE_PRE_REQUISITES(msg_room_config_list)
msg_room_config_list::msg_room_config_list() : message_base() {}

msg_room_config_list::msg_room_config_list(const msg_room_config_list& list) : message_base() {
	qstring result;
	list.get_json_string(result);
	rapidjson::Document obj;
	obj.Parse((char*) result.c_str(), result.length());
	if (obj.HasParseError()) {
		DEBUG_PRINT_ERROR(__LOGTAG__, "msg_room_config_list::msg_room_config_list(copy constructor) failed while parsing json string %.*s", result.length(), (char*) result.c_str());
		return;
	}
	deserialize(obj);
}

msg_room_config_list::~msg_room_config_list() {
	destroy_all();
}

void msg_room_config_list::destroy_all() {
	for (auto* config : configs) {
		GX_DELETE(config);
	}
	configs.clear();
}
bool msg_room_config_list::deserialize(rapidjson::Value& obj) {
	destroy_all();

	message_base::deserialize(obj);

	if (!obj.IsArray())
		return false;
	// Iterate over the array
	for (auto& elem : obj.GetArray()) {
		msg_room_config* new_room_config = (msg_room_config*) msg_room_config::create();
		if (!new_room_config->deserialize(elem)) {
			GX_DELETE(new_room_config);
			continue;
		}
		configs.push_back(new_room_config);
	}
	return true;
}

void msg_room_config_list::serialize(rapidjson::Value& obj, rapidjson::Document::AllocatorType& allocator) const {
	message_base::serialize(obj, allocator);
	obj.SetArray();
	// Create an array of objects
	rapidjson::Value array(rapidjson::kArrayType);
	for (auto* config : configs) {
		rapidjson::Value obj1(rapidjson::kObjectType);
		obj1.AddMember("min", Value().SetInt(config->min), allocator);
		obj1.AddMember("max", Value().SetInt(config->max), allocator);
		obj1.AddMember("betx", Value().SetInt(config->betx), allocator);
		obj1.AddMember("rewardx", Value().SetInt(config->rewardx), allocator);
		obj1.AddMember("allow_after_start", Value().SetBool(config->allow_after_start), allocator);
		array.PushBack(obj1, allocator);
	}
	obj.Swap(array);
}

// MARK: - rq_msg_user_base
DEFINE_MESSAGE_PRE_REQUISITES(rq_msg_user_base)
rq_msg_user_base::rq_msg_user_base() : message_base() {}

rq_msg_user_base::~rq_msg_user_base() {}
bool rq_msg_user_base::deserialize(rapidjson::Value& obj) {
	if (!message_base::deserialize(obj)) {
		return false;
	}

	if (!obj.IsObject())
		return false;
	if (obj.HasMember("pid") && obj["pid"].IsString()) {
		pid = obj["pid"].GetString();
	}
	if (obj.HasMember("token") && obj["token"].IsString()) {
		token = obj["token"].GetString();
	}

	return true;
}

void rq_msg_user_base::serialize(rapidjson::Value& obj, rapidjson::Document::AllocatorType& allocator) const {
	message_base::serialize(obj, allocator);
	//    obj.SetObject();

	// Create a JSON object to hold the data
	obj.AddMember("pid", Value().SetString(pid.c_str(), (uint32_t) pid.length()), allocator);
	obj.AddMember("token", Value().SetString(token.c_str(), (uint32_t) token.length()), allocator);
}

// MARK: - rq_msg_user_get
DEFINE_MESSAGE_PRE_REQUISITES(rq_msg_user_get)
rq_msg_user_get::rq_msg_user_get() : rq_msg_user_base() {}

rq_msg_user_get::~rq_msg_user_get() {}
bool rq_msg_user_get::deserialize(rapidjson::Value& obj) {
	if (!rq_msg_user_base::deserialize(obj)) {
		return false;
	}

	if (!obj.IsObject())
		return false;

	// Access the nested objects and values
	if (obj.HasMember("details") && obj["details"].IsObject()) {
		const Value& details = obj["details"];
		if (details.HasMember("sys_name") && details["sys_name"].IsString()) {
			sys_name = details["sys_name"].GetString();
		} else {
			return false;
		}
		if (details.HasMember("node_name") && details["node_name"].IsString()) {
			node_name = details["node_name"].GetString();
		} else {
			return false;
		}
		if (details.HasMember("release") && details["release"].IsString()) {
			release = details["release"].GetString();
		} else {
			return false;
		}
		if (details.HasMember("arch") && details["arch"].IsString()) {
			arch = details["arch"].GetString();
		} else {
			return false;
		}
	} else {
		return false;
	}

	return true;
}

void rq_msg_user_get::serialize(rapidjson::Value& obj, rapidjson::Document::AllocatorType& allocator) const {
	rq_msg_user_base::serialize(obj, allocator);
	//    obj.SetObject();

	// Create the "details" object
	rapidjson::Value details(rapidjson::kObjectType);

	// Add values to the "details" object
	details.AddMember("sys_name", Value().SetString(sys_name.c_str(), (uint32_t) sys_name.length()), allocator);
	details.AddMember("node_name", Value().SetString(node_name.c_str(), (uint32_t) node_name.length()), allocator);
	details.AddMember("release", Value().SetString(release.c_str(), (uint32_t) release.length()), allocator);
	details.AddMember("arch", Value().SetString(arch.c_str(), (uint32_t) arch.length()), allocator);

	// Add the "details" object to the main document
	obj.AddMember("details", details, allocator);
}

// MARK: - res_msg_user_base
DEFINE_MESSAGE_PRE_REQUISITES(res_msg_user_base)
res_msg_user_base::res_msg_user_base() : message_base() {}
res_msg_user_base::~res_msg_user_base() {
	GX_DELETE(room_list);
}

bool res_msg_user_base::deserialize(rapidjson::Value& obj) {
	message_base::deserialize(obj);

	if (!obj.IsObject())
		return false;

	if (obj.HasMember("pid") && obj["pid"].IsString()) {
		pid = obj["pid"].GetString();
	} else {
		return false;
	}

	if (obj.HasMember("room_list") && obj["room_list"].IsObject()) {
		GX_DELETE(room_list);
		room_list = (msg_room_config_list*) msg_room_config_list::create();
		Value& room_list_obj = obj["room_list"];
		room_list->deserialize(room_list_obj);
	}

	return true;
}

void res_msg_user_base::serialize(rapidjson::Value& obj, rapidjson::Document::AllocatorType& allocator) const {
	message_base::serialize(obj, allocator);
	obj.AddMember("pid", Value().SetString(pid.c_str(), (uint32_t) pid.length()), allocator);

	// room config
	if (room_list != nullptr) {
		rapidjson::Value room_list_obj(rapidjson::kObjectType);
		room_list->serialize(room_list_obj, allocator);
		obj.AddMember("room_list", room_list_obj, allocator);
	}
}

// MARK: - res_msg_user_get
DEFINE_MESSAGE_PRE_REQUISITES(res_msg_user_get)
res_msg_user_get::res_msg_user_get() : res_msg_user_base() {}

bool res_msg_user_get::deserialize(rapidjson::Value& obj) {
	if (!res_msg_user_base::deserialize(obj)) {
		return false;
	}
	if (!obj.IsObject())
		return false;

	if (obj.HasMember("last_login") && obj["last_login"].IsString()) {
		last_login = obj["last_login"].GetString();
	}
	if (obj.HasMember("user_name") && obj["user_name"].IsString()) {
		user_name = obj["user_name"].GetString();
	}
	if (obj.HasMember("token") && obj["token"].IsString()) {
		token = obj["token"].GetString();
	}
	return true;
}

void res_msg_user_get::serialize(rapidjson::Value& obj, rapidjson::Document::AllocatorType& allocator) const {
	res_msg_user_base::serialize(obj, allocator);

	// Create a JSON object to hold the data
	obj.AddMember("last_login", Value().SetString(last_login.c_str(), (uint32_t) last_login.length()), allocator);
	obj.AddMember("user_name", Value().SetString(user_name.c_str(), (uint32_t) user_name.length()), allocator);
	obj.AddMember("token", Value().SetString(token.c_str(), (uint32_t) token.length()), allocator);
}

// MARK: - message_parser
message_parser::~message_parser() {
	records.clear();
}

template <typename T>
void message_parser::register_message_type() {
	unsigned long crc = T::get_type_string_crc();
	std::map<unsigned long, type_room_message_create_cb>::iterator it = records.find(crc);
	if (it != records.end()) {
		DEBUG_WARN(LOG_LEVEL_2, __LOGTAG__, "message type already registered - %s !!!", T::get_type_string().c_str());
		return;
	}
	records[crc] = &T::create;
	DEBUG_PRINT_IMPORTANT(__LOGTAG__, "message type registered - %s : crc %ld !!!", T::get_type_string().c_str(), crc);
}

template <typename T>
T* message_parser::parse(ssize_t len, uint8_t* buf) {
	unsigned long crc = T::get_type_string_crc();
	std::map<unsigned long, type_room_message_create_cb>::iterator it = records.find(crc);
	if (it == records.end()) {
		DEBUG_WARN(LOG_LEVEL_2, __LOGTAG__, "message type not registered - %s. registering.. !!!", T::get_type_string().c_str());
        register_message_type<T>();
	}
	message_base* new_msg = records[crc]();
	T* msg = dynamic_cast<T*>(new_msg);
	if (msg == nullptr && new_msg != nullptr) {
		DEBUG_ASSERT(__LOGTAG__, msg != nullptr, "message dynamic_cast failed - %s !!!", T::get_type_string().c_str());
		GX_DELETE(new_msg);	 // Note :- dont use msg since it may be possible if dynamic_cast not succeeded (Edge cases)
		return nullptr;
	}
	rapidjson::Document obj;
	obj.Parse((char*) buf, len);
	if (obj.HasParseError()) {
		GX_DELETE(new_msg);
		DEBUG_PRINT_ERROR(__LOGTAG__, "message_parser::parse failed while parsing json string %.*s", len, buf);
		return nullptr;
	}
	if (msg->deserialize(obj)) {
		return msg;
	} else {
		GX_DELETE(msg);
		return nullptr;
	}
}

#include "roommessage.cpp"
