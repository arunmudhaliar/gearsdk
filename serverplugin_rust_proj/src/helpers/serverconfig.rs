use std::{fs, sync::Arc};
use std::collections::HashMap;

use super::qzookeeper::{self, interface_qzookeeper};

pub trait observer_serverconfig {
    fn configchanged(
        &self,
        path: String, data: String,
    );
}

pub struct serverconfig {
    zk_interface: Option<Arc<dyn interface_qzookeeper>>,
    config_change_observer: Option<Arc<dyn observer_serverconfig>>,
    configs: HashMap<String, String>,
}

impl serverconfig {
    pub fn new(zk_interface: Option<Arc<dyn interface_qzookeeper>>, observer: Option<Arc<dyn observer_serverconfig>>) -> Self {
        let mut config = serverconfig {
            zk_interface,
            config_change_observer: observer,
            configs: HashMap::new(),
        };
        
        if let Some(zk) = &config.zk_interface {
            zk.register_value_change_callback(serverconfig::zk_value_change_listener, &config as *const _ as *mut std::ffi::c_void);
        }

        config
    }

    pub fn clear(&mut self) {
        self.configs.clear();
    }

    pub fn load(&mut self, path: &str, zk: &qzookeeper::qzookeeper, zk_root_folder: &str) -> Result<bool, String> {
        let buffer = fs::read_to_string(path).map_err(|e| e.to_string())?;
        self.iterate_and_load_keys(&buffer, zk, zk_root_folder)
    }

    pub fn get_config(&self, key: &str, default_value: &str) -> String {
        self.configs.get(key).unwrap_or(&default_value.to_string()).to_string()
    }

    pub fn get_int32(&self, key: &str, default_value: i32) -> i32 {
        self.configs.get(key).and_then(|v| v.parse::<i32>().ok()).unwrap_or(default_value)
    }

    pub fn get_string(&self, key: &str, default_value: &str) -> String {
        self.configs.get(key).unwrap_or(&default_value.to_string()).to_string()
    }

    fn iterate_and_load_keys(&mut self, buffer: &str, zk: &qzookeeper::qzookeeper, zk_root_folder: &str) -> Result<bool, String> {
        let config_keys: HashMap<String, Vec<String>> = serde_json::from_str(buffer).map_err(|e| e.to_string())?;

        let mut load_promises = vec![];

        for (root_key, values) in config_keys.iter() {
            for key in values {
                let zk_key = format!("{}/{}", zk_root_folder, root_key);
                load_promises.push(zk.get_data(&zk_key));
            }
        }

        for result in load_promises {
            if let Err(e) = result {
                return Err(format!("Error loading zk data: {}", e));
            }
        }

        Ok(true)
    }

    pub fn try_update_value(&mut self, path: &str, data: &str) -> bool {
        if self.configs.contains_key(path) {
            self.configs.insert(path.to_string(), data.to_string());
            true
        } else {
            false
        }
    }

    fn zk_value_change_listener(path: String, data: String, context: *mut std::ffi::c_void) {
        let config: &mut serverconfig = unsafe { &mut *(context as *mut serverconfig) };
        if config.try_update_value(&path, &data) {
            println!("Config updated: {}", path);
            if let Some(observer) = &config.config_change_observer {
                observer.configchanged(path, data);
            }
        } else {
            println!("Config not found for: {}", path);
        }
    }
}

impl observer_serverconfig for serverconfig {
    fn configchanged(
        &self,
        path: String, data: String,
    ) {

    }
}