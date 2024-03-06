//
//  roommessage.hpp
//  qserver
//
//  Created by Arun A on 02/03/24.
//

#ifndef roommessage_hpp
#define roommessage_hpp

#include <rapidjson/rapidjson.h>
#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

#include "qstring.h"
#include "typex.h"
#include "essentials.hpp"
#include <map>

#undef __LOGTAG__
#define __LOGTAG__ "roommessage"

#define DECLARE_ROOM_MESSAGE_PRE_REQUISITES(classtype, msg_type_string) \
static const qstring get_type_string() { return msg_type_string; } \
static unsigned long get_type_string_crc() { \
    qstring type_string(classtype::get_type_string()); \
    unsigned long type_string_crc = crc32(0L, Z_NULL, 0); \
    type_string_crc = essentials::mod_crc32_z(type_string_crc, (const unsigned char*)type_string.c_str(), type_string.length()); \
    return type_string_crc; \
} \
static room_message_base* create() { \
    return new classtype(); \
}

#define DEFINE_ROOM_MESSAGE_PRE_REQUISITES(classtype) \
template void room_message_parser::register_message_type<classtype>(); \
template classtype* room_message_parser::parse<classtype>(ssize_t len, uint8_t* buf);

using namespace rapidjson;

// MARK: -
class room_message_base {
public:
    virtual ~room_message_base();
    // A method to deserialize from a RapidJSON document
    virtual bool deserialize(rapidjson::Document& obj);
    virtual void serialize(rapidjson::Document& obj);
    DECLARE_ROOM_MESSAGE_PRE_REQUISITES(room_message_base, "room_message_base")
    
    virtual unsigned long get_type_crc() {
        return room_message_base::get_type_string_crc();
    }
    
protected:
    room_message_base(unsigned long type_string_crc);
    
    unsigned short signature = 0x7A9B;
    unsigned long cached_type_string_crc = 0;
private:
    room_message_base(){};
};

// MARK: -
class room_message_parser {
public:
    ~room_message_parser();
    typedef room_message_base* (*type_room_message_create_cb)();
    template<typename T> void register_message_type();
    
    template<typename T> T* parse(ssize_t len, uint8_t* buf);
private:
    std::map<unsigned long, type_room_message_create_cb> records;
};

// MARK: -
class msg_room_config : public room_message_base {
public:
    msg_room_config();
    void serialize(rapidjson::Document& obj) override;
    bool deserialize(rapidjson::Document& obj) override;
    
    int min = 2;
    int max = 4;
    intx betx = 0;                 // Note: betx & rewardx are in fixed point values
    intx rewardx = FX_TWO;
    bool allow_after_start = false;
    DECLARE_ROOM_MESSAGE_PRE_REQUISITES(msg_room_config, "msg_room_config")
    
    unsigned long get_type_crc() override {
        return msg_room_config::get_type_string_crc();
    }
};

class msg_room_server_shutdown : public room_message_base {
public:
    msg_room_server_shutdown();
    void serialize(rapidjson::Document& obj) override;
    bool deserialize(rapidjson::Document& obj) override;
    
    DECLARE_ROOM_MESSAGE_PRE_REQUISITES(msg_room_server_shutdown, "msg_room_server_shutdown")
    
    unsigned long get_type_crc() override {
        return msg_room_server_shutdown::get_type_string_crc();
    }
};

#endif /* roommessage_hpp */
