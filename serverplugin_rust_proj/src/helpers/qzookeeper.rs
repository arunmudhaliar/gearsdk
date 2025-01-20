use std::{ffi::c_void, sync::{Arc}};
use zookeeper::{WatchedEvent, Watcher, ZkError, ZooKeeper};
use std::collections::HashMap;

// Define the type for value change callback function
// pub type type_qzk_value_changed = Box<dyn Fn(String, String, Arc<dyn std::any::Any>) + Send + Sync>;
pub type type_qzk_value_changed = fn(String, String, *mut c_void);
// pub type CallbackFn = Box<dyn Fn(String, String, Arc<dyn std::any::Any>) + Send + Sync>;

// Define a trait for the ZooKeeper client wrapper
pub trait interface_qzookeeper {
    fn register_value_change_callback(
        &self,
        callback: type_qzk_value_changed,
        context: *mut c_void,
    );

    fn unregister_value_change_callback(
        &self,
        callback: type_qzk_value_changed,
    );
}

/// Custom watcher to handle ZooKeeper events
struct qwatcher;

impl Watcher for qwatcher {
    fn handle(&self, event: WatchedEvent) {
        println!("Watcher event: {:?}", event);
    }
}

/// ZooKeeper wrapper class
pub struct qzookeeper {
    zk_client: Option<Arc<ZooKeeper>>,
    zk_connection_string: String,
    retry_attempts: usize,
    retry_interval: u64, // in seconds
    value_change_callbacks: std::cell::RefCell<HashMap<type_qzk_value_changed, *mut c_void>>,
}

impl qzookeeper {
    pub fn new(zk_connection_string: &str, retry_attempts: usize, retry_interval: u64) -> Self {
        qzookeeper {
            zk_client: None,
            zk_connection_string: zk_connection_string.to_string(),
            retry_attempts,
            retry_interval,
            value_change_callbacks: std::cell::RefCell::new(HashMap::new()),
        }
    }

    pub fn connect(&mut self) -> Result<(), ZkError> {
        let zk_client = ZooKeeper::connect(
            &self.zk_connection_string,
            std::time::Duration::from_secs(10),
            qwatcher,
        )?;
        self.zk_client = Some(Arc::new(zk_client));
        println!("Connected to ZooKeeper at {}", self.zk_connection_string);
        Ok(())
    }

    pub fn get_data(&self, path: &str) -> Result<Vec<u8>, ZkError> {
        let zk_client = self.zk_client.as_ref().ok_or(ZkError::ConnectionLoss)?;
        let watcher = qwatcher;
        let (data, _stat) = zk_client.get_data_w(path, watcher)?;
        println!("Data at {}: {:?}", path, String::from_utf8_lossy(&data));
        Ok(data)
    }

    // Trigger callbacks for a specific path
    pub fn trigger_callbacks(&self, path: String, data: String) {
        for (callback, context) in self.value_change_callbacks.borrow().iter() {
            callback(path.clone(), data.clone(), *context);
        }
    }
}

impl interface_qzookeeper for qzookeeper {
    fn register_value_change_callback(
        &self,
        callback: type_qzk_value_changed,
        context: *mut c_void,
    ) {
        // let mut mut_hash = &self.value_change_callbacks;
        // let mut m: HashMap<type_qzk_value_changed, *mut c_void> = &self.value_change_callbacks;
        // mut_hash.try_insert(callback, context);
        self.value_change_callbacks
        .borrow_mut()
        .insert(callback, context);
    }

    fn unregister_value_change_callback(
        &self,
        callback: type_qzk_value_changed,
    ) {
        self.value_change_callbacks
        .borrow_mut()
        .remove(&callback);
    }
}

impl interface_qzookeeper for Option<qzookeeper> {
    fn register_value_change_callback(
        &self,
        callback: type_qzk_value_changed,
        context: *mut c_void,
    ) {
        // // let mut mut_hash = &self.value_change_callbacks;
        // // let mut m: HashMap<type_qzk_value_changed, *mut c_void> = &self.value_change_callbacks;
        // // mut_hash.try_insert(callback, context);
        // self.value_change_callbacks
        // .borrow_mut()
        // .insert(callback, context);
        if let Some(inner) = self {
            inner.value_change_callbacks
                .borrow_mut()
                .insert(callback, context);
        } else {
            // Handle the `None` case if needed
            eprintln!("register_value_change_callback called on None");
        }
    }

    fn unregister_value_change_callback(
        &self,
        callback: type_qzk_value_changed,
    ) {
        if let Some(inner) = self {
            inner.value_change_callbacks
                .borrow_mut()
                .remove(&callback);
        } else {
            // Handle the `None` case if needed
            eprintln!("unregister_value_change_callback called on None");
        }
    }
}

/* 
#[tokio::main]
async fn main() {
    let zk = qzookeeper::new("localhost:2181".to_string(), 3, 5);

    zk.connect();
    
    // Register a callback function
    zk.register_value_change_callback("/example/path".to_string(), 
        Box::new(|path: String, data: String, context: Arc<dyn std::any::Any>| {
            println!("Callback triggered for path: {} with data: {}", path, data);
        }),
        Arc::new("context_data".to_string()) // Example context
    );

    // Simulate a value change that triggers the callback
    zk.trigger_callbacks("/example/path", "New data".to_string());

    // Unregister the callback
    zk.unregister_value_change_callback("/example/path".to_string(), 
        Box::new(|path: String, data: String, context: Arc<dyn std::any::Any>| {
            println!("Callback triggered for path: {} with data: {}", path, data);
        }),
        Arc::new("context_data".to_string()) // Example context
    );

    zk.get_data("/example/path");
    zk.close();
}
*/
