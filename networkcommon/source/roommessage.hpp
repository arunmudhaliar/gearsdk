//
//  roommessage.hpp
//  networkcommon
//
//  Created by Arun A on 24/03/24.
//

#ifndef roommessage_hpp
#define roommessage_hpp

#include "message.hpp"

class message_room_base : public message_base {
   private:
	message_room_base();

   public:
	message_room_base(unsigned long type_string_crc);
	virtual ~message_room_base();
	void serialize(rapidjson::Value& obj, rapidjson::Document::AllocatorType& allocator) const override;
	bool deserialize(rapidjson::Value& obj) override;
	DECLARE_MESSAGE_PRE_REQUISITES(message_room_base, "message_room_base")
	unsigned short signature = 0x7A9B;
	unsigned long cached_type_string_crc = 0;
	static int deserialize_header(ssize_t len, uint8_t* buf, unsigned short& sig, unsigned long& t_crc, rapidjson::Document& doc);
};

class msg_room_match_request : public message_room_base {
   public:
	msg_room_match_request();
	void serialize(rapidjson::Value& obj, rapidjson::Document::AllocatorType& allocator) const override;
	bool deserialize(rapidjson::Value& obj) override;
	DECLARE_MESSAGE_PRE_REQUISITES(msg_room_match_request, "msg_room_match_request")
	msg_room_config room_config;
};

class msg_room_server_shutdown : public message_room_base {
   public:
	msg_room_server_shutdown();
	void serialize(rapidjson::Value& obj, rapidjson::Document::AllocatorType& allocator) const override;
	bool deserialize(rapidjson::Value& obj) override;
	DECLARE_MESSAGE_PRE_REQUISITES(msg_room_server_shutdown, "msg_room_server_shutdown")
};

#endif /* roommessage_hpp */
