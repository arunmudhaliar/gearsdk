using System;

[Serializable]
public class message_room_base : message_base {
    public ulong t_crc = 0;
    public ushort sig = 0x7A9B;
    private message_room_base() : base() {
    }
    protected message_room_base( ulong typeStringCrc ) : base() {
        t_crc = typeStringCrc;
    }

    public static new string get_type_string() {
        return "message_room_base";
    }
    public static new ulong get_type_string_crc() {
        string type_string = message_room_base.get_type_string();
        ulong type_string_crc = qunitysdk.get_crc32(type_string, type_string.Length);
        //type_string_crc = essentials::mod_crc32_z(type_string_crc, (const unsigned char*)type_string.c_str(), type_string.length());
        return type_string_crc;
    }

    public override ulong get_type_crc() {
        return message_room_base.get_type_string_crc();
    }
    public override string get_type() {
        return message_room_base.get_type_string();
    }
}

class msg_room_match_request : message_room_base {
   public msg_room_match_request() : base(msg_room_match_request.get_type_string_crc()) {
    }

    public static new string get_type_string() {
        return "msg_room_match_request";
    }
    public static new ulong get_type_string_crc() {
        string type_string = msg_room_match_request.get_type_string();
        ulong type_string_crc = qunitysdk.get_crc32(type_string, type_string.Length);
        //type_string_crc = essentials::mod_crc32_z(type_string_crc, (const unsigned char*)type_string.c_str(), type_string.length());
        return type_string_crc;
    }

    public override ulong get_type_crc() {
        return msg_room_match_request.get_type_string_crc();
    }
    public override string get_type() {
        return msg_room_match_request.get_type_string();
    }
    public msg_room_config room_config;
};
