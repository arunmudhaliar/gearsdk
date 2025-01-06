import { serversdk } from "../helpers/serversdk";

export class message_base {
    constructor() {}

    public static get_type_string(): string {
        return "message_base";
    }

    public static get_type_string_crc(): bigint {
        const type_string = message_base.get_type_string();
        const type_string_crc = serversdk.serverplugin.get_crc32(type_string, type_string.length);
        // type_string_crc = essentials.mod_crc32_z(type_string_crc, type_string, type_string.length);
        return BigInt(type_string_crc);
    }

    public get_type_crc(): bigint {
        return message_base.get_type_string_crc();
    }

    public get_type(): string {
        return message_base.get_type_string();
    }
}

export class rq_msg_user_base extends message_base {
    public pid: string;
    public token: string;

    constructor() {
        super();
        this.pid = "";
        this.token = "";
    }
}

export class rq_msg_user_get extends rq_msg_user_base {
    public device: rq_msg_user_get.device_struct;

    constructor() {
        super();
        this.device = new rq_msg_user_get.device_struct();
    }

    public static get_type_string(): string {
        return "rq_msg_user_get";
    }

    public static get_type_string_crc(): bigint {
        const type_string = rq_msg_user_get.get_type_string();
        const type_string_crc = serversdk.serverplugin.get_crc32(type_string, type_string.length);
        // type_string_crc = essentials.mod_crc32_z(type_string_crc, type_string, type_string.length);
        return BigInt(type_string_crc);
    }

    public override get_type_crc(): bigint {
        return rq_msg_user_get.get_type_string_crc();
    }

    public override get_type(): string {
        return rq_msg_user_get.get_type_string();
    }
}

export namespace rq_msg_user_get {
    export class device_struct {
        public sys_name: string;
        public node_name: string;
        public release: string;
        public arch: string;

        constructor() {
            this.sys_name = "";
            this.node_name = "";
            this.release = "";
            this.arch = "";
        }
    }
}

export class rq_msg_ping {
    public msg: string;
    constructor() {
        this.msg = "";
    }
}

export class FX {
    public static Q_FACTOR: number = 12;
    public static FX_ONE: number = 1 << FX.Q_FACTOR;
    public static FX_TWO: number = FX.FX_ONE + FX.FX_ONE;
}

export class msg_room_config extends message_base {
    public min: number;
    public max: number;
    public betx: number;
    public rewardx: number;
    public allow_after_start: boolean;

    constructor() {
        super();
        this.min = 2;
        this.max = 4;
        this.betx = 0; // Note: betx & rewardx are in fixed-point values
        this.rewardx = FX.FX_TWO;
        this.allow_after_start = false;
    }

    public static get_type_string(): string {
        return "msg_room_config";
    }

    public static get_type_string_crc(): bigint {
        const type_string = msg_room_config.get_type_string();
        const type_string_crc = serversdk.serverplugin.get_crc32(type_string, type_string.length);
        // type_string_crc = essentials.mod_crc32_z(type_string_crc, type_string, type_string.length);
        return BigInt(type_string_crc);
    }

    public override get_type_crc(): bigint {
        return msg_room_config.get_type_string_crc();
    }

    public override get_type(): string {
        return msg_room_config.get_type_string();
    }
}

export class msg_room_config_list extends message_base {
    public configs: msg_room_config[];
    constructor() {
        super();
        this.configs = [];
    }
}

export class res_msg_user_base extends message_base {
    public pid: string;
	public room_list : msg_room_config_list | null;
    constructor() {
        super();
        this.pid='';
        this.room_list = null;
    }
}

// export class res_msg_gservers extends message_base {
//     public gservers : Map<string, string[]>;
//     constructor() {
//         super();
//         this.gservers = new Map<string, string[]>();
//     }
// }
// export class res_msg_gservers_serialisable extends message_base {
//     public gservers : any;
//     constructor(gservers : res_msg_gservers) {
//         super();
//         this.gservers = Object.fromEntries(gservers.gservers);
//     }
// }

export class res_msg_user_get extends res_msg_user_base {
    public last_login: string;
    public user_name: string;
    public token: string;
    public gservers: any;
    constructor() {
        super();
        this.last_login = '';
        this.user_name = '';
        this.token = '';
        this.gservers = null;
    }
 };