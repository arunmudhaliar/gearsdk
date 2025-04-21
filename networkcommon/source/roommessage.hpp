//
//  Copyright 2024 homenet25
//  roommessage.hpp
//  networkcommon
//
//  Created by Arun A on 24/03/24.
//

#ifndef roommessage_hpp
#define roommessage_hpp

#include "message.hpp"

class message_room_base : public message_base {
   protected:
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
	static int deserialize_first_hi_message(ssize_t len, uint8_t* buf, rapidjson::Document& doc);
};

class msg_room_match_request : public message_room_base {
   public:
	msg_room_match_request();
	void serialize(rapidjson::Value& obj, rapidjson::Document::AllocatorType& allocator) const override;
	bool deserialize(rapidjson::Value& obj) override;
	DECLARE_MESSAGE_PRE_REQUISITES(msg_room_match_request, "msg_room_match_request")
	msg_room_config room_config;
	unsigned prev_cid_hash_val = 0;	 // used for reconnection
	int room_id = -1;				 // used for reconnection
	qstring pid;
};

class msg_room_server_shutdown : public message_room_base {
   public:
	msg_room_server_shutdown();
	void serialize(rapidjson::Value& obj, rapidjson::Document::AllocatorType& allocator) const override;
	bool deserialize(rapidjson::Value& obj) override;
	DECLARE_MESSAGE_PRE_REQUISITES(msg_room_server_shutdown, "msg_room_server_shutdown")
};

class room_player {
   public:
	void serialize(rapidjson::Value& obj, rapidjson::Document::AllocatorType& allocator) const;
	bool deserialize(rapidjson::Value& obj);
	unsigned hash = 0;
	bool flag = false;
	qstring pid;
};

class msg_room_server_event_base : public message_room_base {
   protected:
	msg_room_server_event_base() {}

   public:
	msg_room_server_event_base(unsigned long type_string_crc);
	virtual ~msg_room_server_event_base() {}
	void serialize(rapidjson::Value& obj, rapidjson::Document::AllocatorType& allocator) const override;
	bool deserialize(rapidjson::Value& obj) override;
	DECLARE_MESSAGE_PRE_REQUISITES(msg_room_server_event_base, "msg_room_server_event_base")
	int room_id = -1;
	qstring room_event;
};

class msg_room_server_event_player_add : public msg_room_server_event_base {
   public:
	msg_room_server_event_player_add();
	virtual ~msg_room_server_event_player_add();
	void serialize(rapidjson::Value& obj, rapidjson::Document::AllocatorType& allocator) const override;
	bool deserialize(rapidjson::Value& obj) override;
	DECLARE_MESSAGE_PRE_REQUISITES(msg_room_server_event_player_add, "msg_room_server_event_player_add")
	//	int room_id = -1;
	std::vector<room_player*> players;
	unsigned self = 0;
};

class msg_room_server_event_player_remove : public msg_room_server_event_base {
   public:
	msg_room_server_event_player_remove();
	virtual ~msg_room_server_event_player_remove();
	void serialize(rapidjson::Value& obj, rapidjson::Document::AllocatorType& allocator) const override;
	bool deserialize(rapidjson::Value& obj) override;
	DECLARE_MESSAGE_PRE_REQUISITES(msg_room_server_event_player_remove, "msg_room_server_event_player_remove")

	std::vector<room_player*> players;
	unsigned self = 0;
};

class msg_room_server_event_start : public msg_room_server_event_base {
   public:
	msg_room_server_event_start();
	void serialize(rapidjson::Value& obj, rapidjson::Document::AllocatorType& allocator) const override;
	bool deserialize(rapidjson::Value& obj) override;
	DECLARE_MESSAGE_PRE_REQUISITES(msg_room_server_event_start, "msg_room_server_event_start")
	//	int room_id = -1;
};

class msg_room_server_event_end : public msg_room_server_event_base {
   public:
	msg_room_server_event_end();
	void serialize(rapidjson::Value& obj, rapidjson::Document::AllocatorType& allocator) const override;
	bool deserialize(rapidjson::Value& obj) override;
	DECLARE_MESSAGE_PRE_REQUISITES(msg_room_server_event_end, "msg_room_server_event_end")
	//	int room_id = -1;
};

#endif /* roommessage_hpp */
