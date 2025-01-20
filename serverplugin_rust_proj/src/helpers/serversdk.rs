use std::ffi::CString;
use std::ptr;
use std::os::raw::{c_char, c_int, c_void};
use std::env;

use super::server_config_reader::server_config_reader;


pub const LOG_LEVEL_0: i32 = 0;
pub const LOG_LEVEL_4: i32 = 4;

pub fn debug_print(level: i32, tag: &str, message: &str) {
    println!("[{}] {}: {}", level, tag, message);
}

pub fn debug_error(tag: &str, message: &str) {
    println!("[{}]: {}", tag, message);
}

pub struct routerconfig {
    pub router_address: String,
    pub mongodb_uri: String,
    pub redis_address: String,
    pub zk_uri: String,
    pub root_dir: String,
    pub command_port: u16,
    pub router_port_return: u16,
    pub app_id: String,
}

impl routerconfig {
    pub fn new() -> Self {
        let config_reader = server_config_reader::get_instance();
        
        routerconfig {
            router_address: config_reader.get_value("router_address"),
            mongodb_uri: config_reader.get_value("router_mongodb_uri"),
            redis_address: config_reader.get_value("router_redis_uri"),
            zk_uri: config_reader.get_value("router_zk_uri"),
            root_dir: env::current_dir().unwrap_or_default().to_string_lossy().into_owned(),
            command_port: config_reader.get_value_as_number("command_port", 4010) as u16,
            router_port_return: config_reader.get_value_as_number("router_port_return", 4005) as u16,
            app_id: config_reader.get_value("app_id"),
        }
    }
}


// Router events
// type TypeOnRouterPreStart = extern "C" fn(router: *mut c_void);
pub type type_on_router_pre_start = unsafe extern "C" fn(router: *mut c_void, user_arg: *mut c_void);
pub type type_on_router_start = unsafe extern "C" fn(router: *mut c_void, user_arg: *mut c_void);
pub type type_on_router_stop = unsafe extern "C" fn(router: *mut c_void, user_arg: *mut c_void);
pub type type_on_router_error = unsafe extern "C" fn(router: *mut c_void, user_arg: *mut c_void, error_code: i32);

// qh3server events
pub type type_on_server_pre_start = Option<unsafe extern "C" fn(server: *mut c_void)>;
pub type type_on_server_start = Option<unsafe extern "C" fn(router: *mut c_void, ip: *const c_char, port: c_int)>;
pub type type_on_server_stop = Option<unsafe extern "C" fn(server: *mut c_void)>;
pub type type_on_server_error = Option<unsafe extern "C" fn(server: *mut c_void, error_code: c_int)>;
pub type type_on_server_parse = Option<unsafe extern "C" fn(server: *mut c_void, cid: *const c_void, cid_len: c_int, path: *const c_char, buffer: *const c_char, len: c_int, headers: *const c_char, header_buffer_size: c_int)>;

// qserver events
pub type type_on_qserver_pre_start = Option<unsafe extern "C" fn(server: *mut c_void)>;
pub type type_on_qserver_start = Option<unsafe extern "C" fn(router: *mut c_void, ip: *const c_char, port: c_int)>;
pub type type_on_qserver_stop = Option<unsafe extern "C" fn(server: *mut c_void)>;
pub type type_on_qserver_error = Option<unsafe extern "C" fn(server: *mut c_void, error_code: c_int)>;

// Room events
pub type type_on_room_event_create = Option<unsafe extern "C" fn(native_server: *mut c_void, room: c_int)>;
pub type type_on_room_event_start = Option<unsafe extern "C" fn(native_server: *mut c_void, room: c_int)>;
pub type type_on_room_event_player_added = Option<unsafe extern "C" fn(native_server: *mut c_void, room: c_int, pid: *const c_char, cid_hash: c_int)>;
pub type type_on_room_event_message = Option<unsafe extern "C" fn(native_server: *mut c_void, room: c_int, pid: *const c_char, cid_hash: c_int, message: *const c_char)>;
pub type type_on_room_event_player_removed = Option<unsafe extern "C" fn(native_server: *mut c_void, room: c_int, pid: *const c_char, cid_hash: c_int)>;
pub type type_on_room_event_end = Option<unsafe extern "C" fn(native_server: *mut c_void, room: c_int)>;
pub type type_on_room_event_countdown_to_start = Option<unsafe extern "C" fn(native_server: *mut c_void, room: c_int, count: c_int, max_count: c_int)>;
pub type type_on_room_event_countdown_cancelled = Option<unsafe extern "C" fn(native_server: *mut c_void, room: c_int)>;

// Define the function signatures

#[link(name = "serverplugin")]
extern "C" {
    pub(crate) fn setup_signal_handler();
    pub(crate) fn pre_init_serverplugin_sdk();
    pub(crate) fn spawn_qh3router(
        router_address: *const std::os::raw::c_char,
        mongodb_uri: *const std::os::raw::c_char,
        redis_address: *const std::os::raw::c_char,
        zk_uri: *const std::os::raw::c_char,
        root_dir: *const std::os::raw::c_char,
        command_port: u16,
        router_port_return: u16,
        app_id: *const std::os::raw::c_char,
        pre_start_cb: type_on_router_pre_start,
        start_cb: type_on_router_start,
        stop_cb: type_on_router_stop,
        error_cb: type_on_router_error,
        user_arg: *mut c_void
    );
    pub(crate) fn spawn_qh3server(
        native_router: *mut c_void,
        server_address: *const c_char,
        mongodb_uri: *const c_char,
        redis_address: *const c_char,
        zk_uri: *const c_char,
        root_dir: *const c_char,
        command_port: c_int,
        router_port_return: *mut c_int,
        app_id: *const c_char,
        pre_start_cb: type_on_server_pre_start,
        start_cb: type_on_server_start,
        stop_cb: type_on_server_stop,
        error_cb: type_on_server_error,
        parse_cb: type_on_server_parse
    );
    pub(crate) fn spawn_qserver(
        server_address: *const c_char,
        redis_address: *const c_char,
        zk_uri: *const c_char,
        root_dir: *const c_char,
        app_id: *const c_char,
        pre_start_cb: type_on_qserver_pre_start,
        start_cb: type_on_qserver_start,
        stop_cb: type_on_qserver_stop,
        error_cb: type_on_qserver_error,
        room_event_create: type_on_room_event_create,
        room_event_start: type_on_room_event_start,
        room_event_player_added: type_on_room_event_player_added,
        room_event_message: type_on_room_event_message,
        room_event_player_removed: type_on_room_event_player_removed,
        room_event_end: type_on_room_event_end,
        room_event_countdown_to_start: type_on_room_event_countdown_to_start,
        room_event_countdown_cancelled: type_on_room_event_countdown_cancelled
    ) -> c_int;
    pub(crate) fn get_crc32(str: *const c_char, len: c_int) -> c_int;
    pub(crate) fn mod_crc32(adler: c_int, buf: *const c_char, len: c_int) -> c_int;
    pub(crate) fn qh3server_try_send_response(
        native_server: *mut c_void,
        cid: *const c_void,
        cid_len: c_int,
        payload: *const c_char,
        len: c_int,
        user_data: *const c_char,
        user_data_len: c_int
    );
    pub(crate) fn get_live_connection_count(native_server: *mut c_void) -> c_int;
    pub(crate) fn get_device_public_ip() -> *const c_char;
    pub(crate) fn qh3server_logfile(
        native_server: *mut c_void,
        lvl: c_int,
        log_type: c_int,
        tag: *const c_char,
        pid: *const c_char,
        roomid: *const c_char,
        message: *const c_char
    ) -> c_int;
    pub(crate) fn qh3server_stats_count(
        native_server: *mut c_void,
        counter: *const c_char,
        count_val: c_int,
        session: *const c_char,
        pid: *const c_char,
        version: *const c_char,
        epic: *const c_char,
        myth: *const c_char,
        legend: *const c_char,
        story: *const c_char,
        message: *const c_char
    ) -> c_int;
    pub(crate) fn qserver_logfile(
        native_server: *mut c_void,
        lvl: c_int,
        log_type: c_int,
        tag: *const c_char,
        pid: *const c_char,
        roomid: *const c_char,
        message: *const c_char
    ) -> c_int;
    pub(crate) fn qserver_stats_count(
        native_server: *mut c_void,
        counter: *const c_char,
        count_val: c_int,
        session: *const c_char,
        pid: *const c_char,
        version: *const c_char,
        epic: *const c_char,
        myth: *const c_char,
        legend: *const c_char,
        story: *const c_char,
        message: *const c_char
    ) -> c_int;
}
