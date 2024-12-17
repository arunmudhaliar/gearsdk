//
//  Copyright 2024 homenet25
//  message.hpp
//  qserver
//
//  Created by Arun A on 02/03/24.
//

#ifndef message_hpp
#define message_hpp

#include "essentials.hpp"
#include "qstring.hpp"
#include "typex.h"

#include <map>
#include <rapidjson/document.h>
#include <rapidjson/rapidjson.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

#undef __LOGTAG__
#define __LOGTAG__ "message"

#define DECLARE_MESSAGE_PRE_REQUISITES(classtype, msg_type_string)                                                                    \
	static qstring get_type_string() {                                                                                                \
		return msg_type_string;                                                                                                       \
	}                                                                                                                                 \
	static unsigned long get_type_string_crc() {                                                                                      \
		qstring type_string(classtype::get_type_string());                                                                            \
		unsigned long type_string_crc = crc32(0L, Z_NULL, 0);                                                                         \
		type_string_crc = essentials::mod_crc32_z(type_string_crc, (const unsigned char*) type_string.c_str(), type_string.length()); \
		return type_string_crc;                                                                                                       \
	}                                                                                                                                 \
	static message_base* create() {                                                                                                   \
		return new classtype();                                                                                                       \
	}                                                                                                                                 \
	void get_json_string(qstring& result) const {                                                                                     \
		Document doc;                                                                                                                 \
		serialize(doc, doc.GetAllocator());                                                                                           \
		essentials::get_json_string(doc, result);                                                                                     \
	}                                                                                                                                 \
	qstring get_type() override {                                                                                                     \
		return msg_type_string;                                                                                                       \
	}                                                                                                                                 \
	unsigned long get_type_crc() const override {                                                                                     \
		return classtype::get_type_string_crc();                                                                                      \
	}

#define DEFINE_MESSAGE_PRE_REQUISITES(classtype)                      \
	template void message_parser::register_message_type<classtype>(); \
	template classtype* message_parser::parse<classtype>(ssize_t len, uint8_t * buf);

using namespace rapidjson;

// MARK: -
class message_base {
   protected:
	message_base() {};

   public:
	virtual ~message_base();
	// A method to deserialize from a RapidJSON document
	virtual bool deserialize(rapidjson::Value& obj);
	virtual void serialize(rapidjson::Value& obj, rapidjson::Document::AllocatorType& allocator) const;
	static const qstring get_type_string() { return "message_base"; }
	static unsigned long get_type_string_crc() {
		qstring type_string(message_base::get_type_string());
		unsigned long type_string_crc = crc32(0L, Z_NULL, 0);
		type_string_crc = essentials::mod_crc32_z(type_string_crc, (const unsigned char*) type_string.c_str(), type_string.length());
		return type_string_crc;
	}
	static message_base* create() { return new message_base(); }
	void get_json_string(qstring& result) {
		Document doc;
		serialize(doc, doc.GetAllocator());
		essentials::get_json_string(doc, result);
	}
	virtual unsigned long get_type_crc() const { return message_base::get_type_string_crc(); }
	virtual qstring get_type() { return "message_base"; }
};

// MARK: -
class message_parser {
   public:
	~message_parser();
	template <typename T>
	T* parse(ssize_t len, uint8_t* buf);

   private:
	template <typename T>
	void register_message_type();
	typedef message_base* (*type_room_message_create_cb)();
	std::map<unsigned long, type_room_message_create_cb> records;
};

// MARK: -
class msg_room_config : public message_base {
   public:
	msg_room_config();
	void serialize(rapidjson::Value& obj, rapidjson::Document::AllocatorType& allocator) const override;
	bool deserialize(rapidjson::Value& obj) override;
	DECLARE_MESSAGE_PRE_REQUISITES(msg_room_config, "msg_room_config")
	int min = 2;
	int max = 4;
	intx betx = 0;	// Note: betx & rewardx are in fixed point values
	intx rewardx = FX_TWO;
	bool allow_after_start = false;
};

class msg_room_config_list : public message_base {
   public:
	msg_room_config_list();
	msg_room_config_list(const msg_room_config_list& list);
	virtual ~msg_room_config_list();
	void serialize(rapidjson::Value& obj, rapidjson::Document::AllocatorType& allocator) const override;
	bool deserialize(rapidjson::Value& obj) override;
	DECLARE_MESSAGE_PRE_REQUISITES(msg_room_config_list, "msg_room_config_list")
	std::vector<msg_room_config*> configs;

   private:
	void destroy_all();
};

// MARK: -
class rq_msg_user_base : public message_base {
   public:
	rq_msg_user_base();
	virtual ~rq_msg_user_base();
	void serialize(rapidjson::Value& obj, rapidjson::Document::AllocatorType& allocator) const override;
	bool deserialize(rapidjson::Value& obj) override;
	DECLARE_MESSAGE_PRE_REQUISITES(rq_msg_user_base, "rq_msg_user_base")
	qstring pid;
	qstring token;
};

// MARK: -
class rq_msg_user_get : public rq_msg_user_base {
   public:
	rq_msg_user_get();
	virtual ~rq_msg_user_get();
	void serialize(rapidjson::Value& obj, rapidjson::Document::AllocatorType& allocator) const override;
	bool deserialize(rapidjson::Value& obj) override;
	DECLARE_MESSAGE_PRE_REQUISITES(rq_msg_user_get, "rq_msg_user_get")
	qstring sys_name;
	qstring node_name;
	qstring release;
	qstring arch;
	qstring duid;
	qstring locale;
};

// MARK: -
class res_msg_user_base : public message_base {
   public:
	res_msg_user_base();
	virtual ~res_msg_user_base();
	void serialize(rapidjson::Value& obj, rapidjson::Document::AllocatorType& allocator) const override;
	bool deserialize(rapidjson::Value& obj) override;
	DECLARE_MESSAGE_PRE_REQUISITES(res_msg_user_base, "res_msg_user_base")
	qstring pid;
	msg_room_config_list* room_list = nullptr;
};

// MARK: -
class res_msg_gservers : public message_base {
   public:
	res_msg_gservers();
	void serialize(rapidjson::Value& obj, rapidjson::Document::AllocatorType& allocator) const override;
	bool deserialize(rapidjson::Value& obj) override;
	DECLARE_MESSAGE_PRE_REQUISITES(res_msg_gservers, "res_msg_gservers")
	std::map<qstring, std::vector<qstring>> gservers;
};

// MARK: -
class res_msg_user_get : public res_msg_user_base {
   public:
	res_msg_user_get();
	void serialize(rapidjson::Value& obj, rapidjson::Document::AllocatorType& allocator) const override;
	bool deserialize(rapidjson::Value& obj) override;
	DECLARE_MESSAGE_PRE_REQUISITES(res_msg_user_get, "res_msg_user_get")
	qstring last_login;
	qstring user_name;
	qstring token;
	res_msg_gservers gservers;
};

#endif /* message_hpp */
