#![allow(non_camel_case_types)]  // Disable the warning for non-CamelCase types
use std::error::Error;
use std::sync::Arc;
use std::{env, ffi::c_void};
use std::path::PathBuf;
// use async_std::task;
use helpers::serversdk::debug_error;
use tokio::sync::Mutex;
// use signal_hook::consts::signal::*;
// use signal_hook::iterator::Signals;
use std::process;
use tokio::signal::unix::{signal, SignalKind};
use once_cell::sync::Lazy;

mod helpers {
    pub mod essentials;
    pub mod serversdk;
    pub mod server_config_reader;
    pub mod qmongo;
    pub mod qhiredis;
    pub mod qzookeeper;
    pub mod serverconfig;
}
mod userserver {
    pub mod router;
    pub mod qh3userserver;
}
use helpers::{serverconfig, server_config_reader, serversdk, qmongo::qmongo, qhiredis::qhiredis};
use userserver::{router::router, qh3userserver::qh3userserver};

fn print_current_path() -> Option<PathBuf> {
    match env::current_dir() {
        Ok(path) => {
            println!("Current path: {}", path.display());
            Some(path)
        }
        Err(e) => {
            println!("Error getting current path: {}", e);
            None
        }
    }
}

pub struct server_app {
    __LOGTAG__: &'static str,
    router_instance: Option<router>,
    userserver_instances: Option<Vec<qh3userserver>>, // Optional vector of userserver instances
}

impl server_app {
    pub fn new() -> Self {
        Self {
            __LOGTAG__: "server_app",
            router_instance: None,
            userserver_instances: Some(Vec::new())
        }
    }

    pub fn get_instance() -> &'static mut Self {
        static mut INSTANCE: Option<server_app> = None;
        unsafe {
            INSTANCE.get_or_insert_with(server_app::new)
        }
    }
    // pub fn get_instance() -> Arc<Mutex<Self>> {
    //     static INSTANCE: Lazy<Arc<Mutex<server_app>>> = Lazy::new(|| {
    //         Arc::new(Mutex::new(server_app::new()))
    //     });

    //     INSTANCE.clone()
    // }


    fn get_raw_pointer(&self) -> *mut c_void {
        self as *const server_app as *mut c_void
    }

    pub fn on_router_start_cb(_native_router: *mut c_void, arg: *mut c_void) {
        println!("Router started callback executed.");

        let server_app_ref: *mut server_app = arg as *mut server_app;
        let server_app_instance = unsafe { &mut *server_app_ref };
        server_app_instance.start_userserver(_native_router);
        // server_app_instance.start_userserver(_native_router);
        // server_app_instance.start_userserver(_native_router);
    }

    fn start_router(&mut self) {
        if self.router_instance.is_some() {
            debug_error(self.__LOGTAG__, "router_instance not null !!!");
            return;
        }

        let router = router::new(Self::on_router_start_cb, self.get_raw_pointer());
        self.router_instance = Some(router);
        self.router_instance.as_ref().unwrap().run();
    }

    #[tokio::main]
    async fn start_userserver(&mut self, _native_router: *mut c_void) {
        println!("start_userserver.");
        let mut userserver_instance = qh3userserver::new();
        userserver_instance.run(_native_router).await;
        if let Some(ref mut instances) = self.userserver_instances {
            instances.push(userserver_instance);
        }
    }

    pub fn run(&mut self) {
        self.start_router();
    }
}



#[tokio::main]
async fn main() -> Result<(), Box<dyn Error>> {
    // Initialize Config and Database Connection
    unsafe {
        let config_reader = server_config_reader::server_config_reader::get_instance();
        config_reader.load_config("./serversonfig.rel.inf");

        /*
        let app_name = "app_name".to_string();
        let db_name = "gsdk_mongodb".to_string();
        let uri_string = config_reader.get_value("router_mongodb_uri");

        let mut qmongo = qmongo::new(app_name, db_name, uri_string);
        qmongo.connect().await?;

        let redis_address = config_reader.get_value("router_redis_uri");
        match essentials::extract_ip_and_port(&redis_address) {
            Some((ip, port)) => {
                println!("Valid: IP = {}, Port = {}", ip, port);
                let hiredis_label = "hiredis".to_string();
                let gsdkuser_label = "gsdkuser".to_string();
                let pass_label = "Fr0gmoon123".to_string();
                let mut qhiredis = qhiredis::new(hiredis_label, ip, port, Some(gsdkuser_label), Some(pass_label));
                qhiredis.connect_redis().await;
            },
            None => println!("Invalid: {}", redis_address),
        }
        */

        serversdk::setup_signal_handler();
        serversdk::pre_init_serverplugin_sdk();
        let app = server_app::get_instance();
        app.run();
    }

    // Signal Handling
    let mut sigint = signal(SignalKind::interrupt())?;
    let mut sigterm = signal(SignalKind::terminate())?;

    println!("Program is running... Press Ctrl+C to stop.");

    tokio::select! {
        _ = sigint.recv() => {
            println!("Received SIGINT, shutting down...");
        }
        _ = sigterm.recv() => {
            println!("Received SIGTERM, shutting down...");
        }
    }

    println!("Shutting down gracefully...");
    process::exit(0);
}


/* 
fn main() -> Result<(), Box<dyn Error>> {


        task::block_on(async {
            unsafe {
                let config_reader = server_config_reader::server_config_reader::get_instance();
                config_reader.load_config("./serversonfig.rel.inf");
            let app_name = "app_name".to_string();
            let db_name = "gsdk_mongodb".to_string();
            let uri_string = config_reader.get_value("router_mongodb_uri");
        
            let mut qmongo = qmongo::new(app_name, db_name, uri_string);
        
            qmongo.connect().await;

            // Handle Ctrl+C
            println!("Program is running... Press Ctrl+C to stop.");
            tokio::signal::ctrl_c().await;
            println!("Ctrl+C detected, shutting down...");
            process::exit(0);

            serversdk::setup_signal_handler();
            serversdk::pre_init_serverplugin_sdk();
            let app = server_app::get_instance();
            app.run();
        };
        });
    Ok(())
}
*/