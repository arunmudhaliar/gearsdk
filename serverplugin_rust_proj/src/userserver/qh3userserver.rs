#![allow(non_camel_case_types)]  use std::path::PathBuf;
// Disable the warning for non-CamelCase types
use std::{env, ffi};
use std::ffi::{c_int, c_void, CString};
use std::ptr;
use std::sync::{Arc, Mutex};
use bson::raw::Error;
use serde::ser;

use crate::helpers::essentials::essentials;
use crate::helpers::qhiredis::qhiredis;
use crate::helpers::qmongo::qmongo;
use crate::helpers::qzookeeper::{interface_qzookeeper, qzookeeper};
use crate::helpers::server_config_reader::server_config_reader;
use crate::helpers::serverconfig::{observer_serverconfig, serverconfig};
use crate::helpers::serversdk::{debug_error, debug_print, routerconfig, type_on_router_error, type_on_router_pre_start, type_on_router_start, type_on_router_stop, LOG_LEVEL_0};

pub struct qh3userserver {
    router_config: routerconfig,
    qmongo: Option<qmongo>,
    qhiredis: Option<qhiredis>,
    qzookeeper: Option<qzookeeper>,
    zkconfig: Option<serverconfig>
}

impl qh3userserver {
    const LOGTAG: &'static str = "userserver";

    pub fn new() -> Self {
        let config_reader = server_config_reader::get_instance();
        qh3userserver {
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
            qmongo: None,
            qhiredis: None,
            qzookeeper: None,
            zkconfig: None
        }
    }

    fn get_raw_pointer(&self) -> *mut c_void {
        self as *const qh3userserver as *mut c_void
    }

    pub async fn run(&mut self, _native_router: *mut c_void) {
        let router_address = CString::new(self.router_config.router_address.clone()).expect("CString::new failed for router_address");
        let mongodb_uri = CString::new(self.router_config.mongodb_uri.clone()).expect("CString::new failed for router_address");
        let redis_address = CString::new(self.router_config.redis_address.clone()).expect("CString::new failed for redis_address");
        let zk_uri = CString::new(self.router_config.zk_uri.clone()).expect("CString::new failed for zk_uri");
        let root_dir = CString::new(self.router_config.root_dir.clone()).expect("CString::new failed for root_dir");
        let app_id = CString::new(self.router_config.app_id.clone()).expect("CString::new failed for app_id");


        let app_name = "app_name".to_string();
        let db_name = "gsdk_mongodb".to_string();
        self.qmongo = Some(qmongo::new(app_name, db_name, self.router_config.mongodb_uri.clone()));
        if let Some(qmongo_instance) = self.qmongo.as_mut() {
            qmongo_instance.connect().await;
        }

        match essentials::extract_ip_and_port(&self.router_config.redis_address) {
            Some((ip, port)) => {
                println!("Valid: IP = {}, Port = {}", ip, port);
                let hiredis_label = "hiredis".to_string();
                let gsdkuser_label = "gsdkuser".to_string();
                let pass_label = "Fr0gmoon123".to_string();
                self.qhiredis = Some(qhiredis::new(hiredis_label, ip, port, Some(gsdkuser_label), Some(pass_label)));
                // self.qhiredis.unwrap().connect_redis().await;
                if let Some(qmongo_qhiredis) = self.qhiredis.as_mut() {
                    qmongo_qhiredis.connect_redis().await;
                }
            },
            None => println!("Invalid: {}", self.router_config.redis_address),
        }
        
        self.qzookeeper = Some(qzookeeper::new(self.router_config.zk_uri.as_str(), 3, 10));
        if let Some(qzookeeper_instance) = self.qzookeeper.as_mut() {
            qzookeeper_instance.connect();
        }

        if self.qzookeeper.is_none() {
            println!("Invalid: self.qzookeeper");
            return;
        }

        // // load server config
        self.zkconfig = Some(serverconfig::new(Some(Arc::new(self.qzookeeper.take().unwrap()) as Arc<dyn interface_qzookeeper>),
         None));

         if self.qzookeeper.is_none() {
            println!("Invalid: self.qzookeeper");
            return;
        }
        
         let config_path = PathBuf::from(&self.router_config.root_dir)
            .join("configs")
            .join("dev")
            .join("runtime-config.json");

         if let Some(zkconfig_instance) = self.zkconfig.as_mut() {

            if self.qzookeeper.is_none() {
                println!("Invalid: self.qzookeeper");
                return;
            }

            if let Some(ref zk) = self.qzookeeper {
                zkconfig_instance.load(
                    config_path.to_str().expect("Invalid UTF-8 path"),
                    zk,
                    "/qh3server",
                ).map_err(|e| {
                    eprintln!("Failed to load zkconfig: {}", e);
                    e
                });
            }
            // zkconfig_instance.load(config_path.to_str().expect("Invalid UTF-8 path"), self.qzookeeper, "/qh3server");
        }

        // println!("MongoDB URI: {}", mongodb_uri);
        unsafe { 
            // serversdk::spawn_qh3server(
            //     _native_router,
            //     router_address.as_ptr(),
            //     mongodb_uri.as_ptr(),
            //     redis_address.as_ptr(),
            //     zk_uri.as_ptr(),
            //     root_dir.as_ptr(),
            //     self.router_config.command_port,
            //     self.router_config.router_port_return,
            //     app_id.as_ptr(),
            //     ptr::null(),
            //     ptr::null(),
            //     ptr::null(),
            //     ptr::null(),
            //     ptr::null()
            // );
        };
    }
}

impl observer_serverconfig for qh3userserver {
    fn configchanged(
        &self,
        path: String, data: String,
    ) {

    }
}

impl observer_serverconfig for Option<&qh3userserver> {
    fn configchanged(
        &self,
        path: String, data: String,
    ) {

    }
}

impl observer_serverconfig for Option<&mut qh3userserver> {
    fn configchanged(
        &self,
        path: String, data: String,
    ) {

    }
}