using System;
using System.Collections.Generic;
using System.Text.Json;
using UnityEngine;

[Serializable]
public abstract class message_room_base : message_base {
    public ulong t_crc { get; set; } = 0;
    public ushort sig { get; set; } = 0x7A9B;
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
        return type_string_crc;
    }

    public override ulong get_type_crc() {
        return message_room_base.get_type_string_crc();
    }
    public override string get_type() {
        return message_room_base.get_type_string();
    }
    
    public static int deserialize_header(
        ulong len,
        byte[] buf,
        out string jsonString,
        out ushort sig,
        out ulong t_crc,
        out JsonDocument doc)
    {
        sig = 0;
        t_crc = 0;
        doc = null;
        jsonString = "";
        try
        {
            jsonString = System.Text.Encoding.UTF8.GetString(buf, 0, (int)len);
            doc = JsonDocument.Parse(jsonString);

            var root = doc.RootElement;

            if (root.ValueKind == JsonValueKind.Object)
            {
                if (root.TryGetProperty("sig", out JsonElement sigElement) && sigElement.ValueKind == JsonValueKind.Number)
                {
                    sig = (ushort)sigElement.GetUInt16();
                }
                else
                {
                    Debug.LogWarning($"Room message doesn't have sig {jsonString}");
                }

                if (root.TryGetProperty("t_crc", out JsonElement crcElement) && crcElement.ValueKind == JsonValueKind.Number)
                {
                    t_crc = crcElement.GetUInt64();
                }
                else
                {
                    Debug.LogWarning($"Room message doesn't have t_crc {jsonString}");
                    return -3;
                }
            }
            else
            {
                Debug.LogWarning($"Room message is not an object or missing fields {jsonString}");
                return -2;
            }

            return 0;
        }
        catch (JsonException)
        {
            Debug.LogError($"DeserializeHeader failed while parsing JSON {jsonString}");
            return -1;
        }
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
    public msg_room_config room_config { get; set; }
    public uint prev_cid_hash_val { get; set; } = 0;     // used for reconnection
    public int room_id { get; set; } = -1;               // used for reconnection
    public string pid { get; set; }
};

[Serializable]
public class room_player {
    public long hash { get; set; }
    public bool flag { get; set; }
    public string pid { get; set; }
}

[Serializable]
public abstract class msg_room_server_event : message_room_base {
    public msg_room_server_event(ulong typeStringCrc) : base(typeStringCrc) {
    }
    public static new string get_type_string() {
        return "msg_room_server_event";
    }
    public static new ulong get_type_string_crc() {
        string type_string = msg_room_server_event.get_type_string();
        ulong type_string_crc = qunitysdk.get_crc32(type_string, type_string.Length);
        return type_string_crc;
    }

    public override ulong get_type_crc() {
        return msg_room_server_event.get_type_string_crc();
    }
    public override string get_type() {
        return msg_room_server_event.get_type_string();
    }
    public string room_event { get; set; }
    public int room_id { get; set; }
}

[Serializable]
public class msg_room_server_event_player_add : msg_room_server_event {
    public msg_room_server_event_player_add() : base(msg_room_server_event_player_add.get_type_string_crc()) {
        room_event = "player-add";
    }
    public static new string get_type_string() {
        return "msg_room_server_event_player_add";
    }
    public static new ulong get_type_string_crc() {
        string type_string = msg_room_server_event_player_add.get_type_string();
        ulong type_string_crc = qunitysdk.get_crc32(type_string, type_string.Length);
        return type_string_crc;
    }

    public override ulong get_type_crc() {
        return msg_room_server_event_player_add.get_type_string_crc();
    }
    public override string get_type() {
        return msg_room_server_event_player_add.get_type_string();
    }
    public List<room_player> players{ get; set; }
    public long self { get; set; }
}

[Serializable]
public class msg_room_server_event_player_remove : msg_room_server_event {
    public msg_room_server_event_player_remove() : base(msg_room_server_event_player_remove.get_type_string_crc()) {
        room_event = "player-remove";
    }
    public static new string get_type_string() {
        return "msg_room_server_event_player_remove";
    }
    public static new ulong get_type_string_crc() {
        string type_string = msg_room_server_event_player_remove.get_type_string();
        ulong type_string_crc = qunitysdk.get_crc32(type_string, type_string.Length);
        return type_string_crc;
    }

    public override ulong get_type_crc() {
        return msg_room_server_event_player_remove.get_type_string_crc();
    }
    public override string get_type() {
        return msg_room_server_event_player_remove.get_type_string();
    }

    public List<room_player> players { get; set; }
    public long self { get; set; }
}

[Serializable]
public class msg_room_server_event_start : msg_room_server_event {
    public msg_room_server_event_start() : base(msg_room_server_event_start.get_type_string_crc()) {
        room_event = "room-start";
    }
    public static new string get_type_string() {
        return "msg_room_server_event_start";
    }
    public static new ulong get_type_string_crc() {
        string type_string = msg_room_server_event_start.get_type_string();
        ulong type_string_crc = qunitysdk.get_crc32(type_string, type_string.Length);
        return type_string_crc;
    }

    public override ulong get_type_crc() {
        return msg_room_server_event_start.get_type_string_crc();
    }
    public override string get_type() {
        return msg_room_server_event_start.get_type_string();
    }
}

[Serializable]
public class msg_room_server_event_end : msg_room_server_event {
    public msg_room_server_event_end() : base(msg_room_server_event_end.get_type_string_crc()) {
        room_event = "rroom-end";
    }
    public static new string get_type_string() {
        return "msg_room_server_event_end";
    }
    public static new ulong get_type_string_crc() {
        string type_string = msg_room_server_event_end.get_type_string();
        ulong type_string_crc = qunitysdk.get_crc32(type_string, type_string.Length);
        return type_string_crc;
    }

    public override ulong get_type_crc() {
        return msg_room_server_event_end.get_type_string_crc();
    }
    public override string get_type() {
        return msg_room_server_event_end.get_type_string();
    }
}