#![allow(non_camel_case_types)]  // Disable the warning for non-CamelCase types
use std::{env, ffi};
use std::ffi::{c_int, c_void, CString};
use std::ptr;
use std::sync::{Arc, Mutex};
use crate::helpers::essentials::essentials;
use crate::helpers::server_config_reader::server_config_reader;
// use ffi::Callback;
// use crate::helpers::{debug_error, debug_print, LOG_LEVEL_0, LOG_LEVEL_4};
use crate::helpers::serversdk::{self, debug_error, debug_print, routerconfig, type_on_router_error, type_on_router_pre_start, type_on_router_start, type_on_router_stop, LOG_LEVEL_0};

pub struct router {
    router_config: routerconfig,
    // on_router_start_cb: Arc<dyn Fn(*mut c_void) + Send + Sync>,
    on_router_start_cb: fn(router: *mut c_void, arg: *mut c_void),
    arg: *mut c_void
}

impl router {
    const LOGTAG: &'static str = "router";

    pub fn new(on_router_start_cb: fn(router: *mut c_void, arg: *mut c_void), arg: *mut c_void) -> Self {
        let config_reader = server_config_reader::get_instance();
        
        router {
            router_config: routerconfig {
                router_address: config_reader.get_value("router_address"),
                mongodb_uri: config_reader.get_value("router_mongodb_uri"),
                redis_address: config_reader.get_value("router_redis_uri"),
                zk_uri: config_reader.get_value("router_zk_uri"),
                root_dir: env::current_dir().unwrap_or_default().to_string_lossy().into_owned(),
                command_port: config_reader.get_value_as_number("command_port", 4010) as u16,
                router_port_return: config_reader.get_value_as_number("router_port_return", 4005) as u16,
                app_id: config_reader.get_value("app_id"),
            },
            on_router_start_cb,
            arg,
        }
    }

    // router events
    unsafe extern fn on_router_pre_start(native_router: *mut c_void, user_arg: *mut c_void) {
        debug_print(LOG_LEVEL_0, Self::LOGTAG, "on_router_pre_start");
    }

    unsafe extern fn on_router_start(native_router: *mut c_void, user_arg: *mut c_void) {
        debug_print(LOG_LEVEL_0, Self::LOGTAG, "on_router_start");
        let router_ref: *mut router = user_arg as *mut router;
        let router_instance = &mut *router_ref;
        (router_instance.on_router_start_cb)(native_router, router_instance.arg);
    }

    unsafe extern fn on_router_stop(native_router: *mut c_void, user_arg: *mut c_void) {
        debug_print(LOG_LEVEL_0, Self::LOGTAG, "on_router_stop");
    }

    unsafe extern fn on_router_error(native_router: *mut c_void, user_arg: *mut c_void, error_code: i32) {
        debug_error(Self::LOGTAG, &format!("on_router_error: {}", error_code));
    }

    fn get_raw_pointer(&self) -> *mut c_void {
        self as *const router as *mut c_void
    }

    pub fn run(&self) {
        // let message = "Hello from Rust!";
        // let c_message = CString::new(message).expect("Failed to convert to CString");
        // let c_ptr = essentials::to_c_string_unchecked(&self.router_config.router_address);

        let router_address = CString::new(self.router_config.router_address.clone()).expect("CString::new failed for router_address");
        let mongodb_uri = CString::new(self.router_config.mongodb_uri.clone()).expect("CString::new failed for router_address");
        let redis_address = CString::new(self.router_config.redis_address.clone()).expect("CString::new failed for redis_address");
        let zk_uri = CString::new(self.router_config.zk_uri.clone()).expect("CString::new failed for zk_uri");
        let root_dir = CString::new(self.router_config.root_dir.clone()).expect("CString::new failed for root_dir");
        let app_id = CString::new(self.router_config.app_id.clone()).expect("CString::new failed for app_id");

        // println!("MongoDB URI: {}", mongodb_uri);
        unsafe { 
            serversdk::spawn_qh3router(
            router_address.as_ptr(),
            mongodb_uri.as_ptr(),
            redis_address.as_ptr(),
            zk_uri.as_ptr(),
            root_dir.as_ptr(),
            self.router_config.command_port,
            self.router_config.router_port_return,
            app_id.as_ptr(),
            router::on_router_pre_start,
            router::on_router_start,
            router::on_router_stop,
            router::on_router_error,
            self.get_raw_pointer()
            );
        };
    }
}
