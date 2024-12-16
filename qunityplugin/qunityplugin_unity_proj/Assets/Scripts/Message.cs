using System;

[Serializable]
public class message_base {
    public message_base() {
    }

    public static string get_type_string() {
        return "message_base";
    }
    public static ulong get_type_string_crc() {
        string type_string = message_base.get_type_string();
        ulong type_string_crc = qunitysdk.get_crc32(type_string, type_string.Length);
        //type_string_crc = essentials::mod_crc32_z(type_string_crc, (const unsigned char*)type_string.c_str(), type_string.length());
        return type_string_crc;
    }

    public virtual ulong get_type_crc() {
        return message_base.get_type_string_crc();
    }
    public virtual string get_type() {
        return message_base.get_type_string();
    }
}

[Serializable]
public class rq_msg_user_base : message_base {
    public string pid;
    public string token;
    public rq_msg_user_base() : base() {
    }
}

[Serializable]
public class rq_msg_user_get : rq_msg_user_base {
    public rq_msg_user_get() : base() {
        device = new device_struct();
    }

    [Serializable]
    public struct device_struct {
        public string sys_name;
        public string node_name;
        public string release;
        public string arch;
        public string duid;
        public string locale;
    }
    public device_struct device;

    public static new string get_type_string() {
        return "rq_msg_user_get";
    }
    public static new ulong get_type_string_crc() {
        string type_string = rq_msg_user_get.get_type_string();
        ulong type_string_crc = qunitysdk.get_crc32(type_string, type_string.Length);
        //type_string_crc = essentials::mod_crc32_z(type_string_crc, (const unsigned char*)type_string.c_str(), type_string.length());
        return type_string_crc;
    }

    public override ulong get_type_crc() {
        return rq_msg_user_get.get_type_string_crc();
    }
    public override string get_type() {
        return rq_msg_user_get.get_type_string();
    }
}

class FX {
    public static int Q_FACTOR = 12;
    public static int FX_ONE = (1 << Q_FACTOR);
    public static int FX_TWO = (FX_ONE + FX_ONE);
}

[Serializable]
public class msg_room_config : message_base {
   public msg_room_config() : base() {
    }
    public static new string get_type_string() {
        return "msg_room_config";
    }
    public static new ulong get_type_string_crc() {
        string type_string = msg_room_config.get_type_string();
        ulong type_string_crc = qunitysdk.get_crc32(type_string, type_string.Length);
        //type_string_crc = essentials::mod_crc32_z(type_string_crc, (const unsigned char*)type_string.c_str(), type_string.length());
        return type_string_crc;
    }

    public override ulong get_type_crc() {
        return msg_room_config.get_type_string_crc();
    }
    public override string get_type() {
        return msg_room_config.get_type_string();
    }
    public int min = 2;
    public int max = 4;
    public int betx = 0;  // Note: betx & rewardx are in fixed point values
    public int rewardx = FX.FX_TWO;
    public bool allow_after_start = false;
};