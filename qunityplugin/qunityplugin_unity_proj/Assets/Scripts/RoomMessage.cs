using System;
using System.Collections.Generic;

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

[Serializable]
public class msg_room_server_shutdown : message_room_base {
    public msg_room_server_shutdown() : base(msg_room_server_shutdown.get_type_string_crc()) {
    }

    public static new string get_type_string() {
        return "msg_room_server_shutdown";
    }
    public static new ulong get_type_string_crc() {
        string type_string = msg_room_server_shutdown.get_type_string();
        ulong type_string_crc = qunitysdk.get_crc32(type_string, type_string.Length);
        //type_string_crc = essentials::mod_crc32_z(type_string_crc, (const unsigned char*)type_string.c_str(), type_string.length());
        return type_string_crc;
    }

    public override ulong get_type_crc() {
        return msg_room_server_shutdown.get_type_string_crc();
    }
    public override string get_type() {
        return msg_room_server_shutdown.get_type_string();
    }
}

[Serializable]
public class msg_room_match_request : message_room_base {
   public msg_room_match_request( string pid_) : base(msg_room_match_request.get_type_string_crc()) {
        this.pid = pid_;
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
    public uint prev_cid_hash_val = 0;     // used for reconnection
    public int room_id = -1;               // used for reconnection
    public string pid;
};

[Serializable]
public class room_player {
    public long hash;
    public bool flag;
    public string pid;
}

[Serializable]
public class msg_room_server_event : message_room_base {
    public msg_room_server_event() : base(msg_room_server_event.get_type_string_crc()) {
    }
    public static new string get_type_string() {
        return "msg_room_server_event";
    }
    public static new ulong get_type_string_crc() {
        string type_string = msg_room_server_event.get_type_string();
        ulong type_string_crc = qunitysdk.get_crc32(type_string, type_string.Length);
        //type_string_crc = essentials::mod_crc32_z(type_string_crc, (const unsigned char*)type_string.c_str(), type_string.length());
        return type_string_crc;
    }

    public override ulong get_type_crc() {
        return msg_room_server_event.get_type_string_crc();
    }
    public override string get_type() {
        return msg_room_server_event.get_type_string();
    }
    public string room_event;
    public int room_id;
    public List<room_player> players;
    public long self;
}

