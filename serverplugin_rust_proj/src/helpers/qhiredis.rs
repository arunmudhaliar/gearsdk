#![allow(non_camel_case_types)]  // Disable the warning for non-CamelCase types
use redis::{AsyncCommands, Client, RedisError};
use std::sync::Arc;
use tokio::sync::Mutex;
use log::{debug, error, info, warn};

use crate::helpers::serversdk::{debug_print, LOG_LEVEL_0};

type RedisHashIteratorCallback = fn(field: String, value: String, arg: Option<&str>);
type RedisScanCallback = fn(key: String, field: String, value: String, arg: Option<&str>);

#[derive(Clone)]
pub struct qhiredis {
    name: String,
    redis_ip: String,
    redis_port: u16,
    redis_username: Option<String>,
    redis_password: Option<String>,
    redis_client: Arc<Mutex<Option<redis::aio::Connection>>>,
}

impl qhiredis {
    pub fn new(
        name: String,
        redis_ip: String,
        redis_port: u16,
        redis_username: Option<String>,
        redis_password: Option<String>,
    ) -> Self {
        Self {
            name,
            redis_ip,
            redis_port,
            redis_username,
            redis_password,
            redis_client: Arc::new(Mutex::new(None)),
        }
    }

    pub async fn connect_redis(&self) -> Result<(), RedisError> {
        let mut client_lock = self.redis_client.lock().await;

        if client_lock.is_some() {
            // debug_print(LOG_LEVEL_0, "qhiredis", message);
            println!(
                "[{}] Already connected to Redis at {}:{}",
                self.name, self.redis_ip, self.redis_port
            );
            return Ok(());
        }

        let connection_string = if let (Some(username), Some(password)) =
            (&self.redis_username, &self.redis_password)
        {
            format!(
                "redis://{}:{}@{}:{}",
                username, password, self.redis_ip, self.redis_port
            )
        } else {
            format!("redis://{}:{}", self.redis_ip, self.redis_port)
        };

        let client = Client::open(connection_string)?;
        let connection = client.get_async_connection().await?;

        *client_lock = Some(connection);
        println!(
            "[{}] Connected to Redis at {}:{}",
            self.name, self.redis_ip, self.redis_port
        );

        Ok(())
    }

    pub async fn disconnect_redis(&self) {
        let mut client_lock = self.redis_client.lock().await;

        if client_lock.is_some() {
            println!("[{}] Redis connection closed", self.name);
            *client_lock = None;
        }
    }

    pub async fn retry_connection(&self) -> Result<(), RedisError> {
        self.disconnect_redis().await;
        self.connect_redis().await
    }

    pub async fn set_value(&self, key: &str, value: &str, expiry_in_sec: Option<u64>) -> Result<(), RedisError> {
        let mut client_lock = self.redis_client.lock().await;

        if let Some(client) = client_lock.as_mut() {
            if let Some(expiry) = expiry_in_sec {
                client.set_ex(key, value, expiry).await?;
            } else {
                client.set(key, value).await?;
            }
            Ok(())
        } else {
            Err(RedisError::from((redis::ErrorKind::IoError, "Redis client not connected")))
        }
    }

    pub async fn get_value(&self, key: &str) -> Result<Option<String>, RedisError> {
        let mut client_lock = self.redis_client.lock().await;

        if let Some(client) = client_lock.as_mut() {
            let result: Option<String> = client.get(key).await?;
            Ok(result)
        } else {
            Err(RedisError::from((redis::ErrorKind::IoError, "Redis client not connected")))
        }
    }

    pub async fn set_hash_value(&self, hash_key: &str, field: &str, value: &str) -> Result<(), RedisError> {
        let mut client_lock = self.redis_client.lock().await;

        if let Some(client) = client_lock.as_mut() {
            client.hset(hash_key, field, value).await?;
            Ok(())
        } else {
            Err(RedisError::from((redis::ErrorKind::IoError, "Redis client not connected")))
        }
    }

    pub async fn get_hash_value(&self, hash_key: &str, field: &str) -> Result<Option<String>, RedisError> {
        let mut client_lock = self.redis_client.lock().await;

        if let Some(client) = client_lock.as_mut() {
            let result: Option<String> = client.hget(hash_key, field).await?;
            Ok(result)
        } else {
            Err(RedisError::from((redis::ErrorKind::IoError, "Redis client not connected")))
        }
    }

    pub async fn incr(&self, key: &str) -> Result<i64, RedisError> {
        let mut client_lock = self.redis_client.lock().await;

        if let Some(client) = client_lock.as_mut() {
            let result: i64 = client.incr(key, 1).await?;
            Ok(result)
        } else {
            Err(RedisError::from((redis::ErrorKind::IoError, "Redis client not connected")))
        }
    }

    pub async fn decr_by(&self, key: &str, decrement: i64) -> Result<i64, RedisError> {
        let mut client_lock = self.redis_client.lock().await;

        if let Some(client) = client_lock.as_mut() {
            let result: i64 = client.decr(key, decrement).await?;
            Ok(result)
        } else {
            Err(RedisError::from((redis::ErrorKind::IoError, "Redis client not connected")))
        }
    }
    
    // pub async fn iterate_hash(
    //     &self,
    //     hash_key: &str,
    //     callback: RedisHashIteratorCallback,
    //     arg: Option<&str>,
    // ) -> Result<(), RedisError> {
    //     let mut client_lock = self.redis_client.lock().await;
    
    //     if let Some(client) = client_lock.as_mut() {
    //         let mut cursor: usize = 0;
    //         loop {
    //             // Using hscan which returns AsyncIter
    //             let mut iter = client.hscan(hash_key).await?;
                
    //             // Iterate over the returned AsyncIter
    //             while let Some((field, value)) = iter.next().await {
    //                 callback(field, value, arg);
    //             }
    
    //             // If cursor is 0, it means the iteration is complete
    //             if cursor == 0 {
    //                 break;
    //             }
    //             cursor = iter.cursor();
    //         }
    //         Ok(())
    //     } else {
    //         Err(RedisError::from((redis::ErrorKind::IoError, "Redis client not connected")))
    //     }
    // }
    
    // pub async fn iterate_hash(
    //     &self,
    //     hash_key: &str,
    //     callback: RedisHashIteratorCallback,
    //     arg: Option<&str>,
    // ) -> Result<(), RedisError> {
    //     let mut client_lock = self.redis_client.lock().await;

    //     if let Some(client) = client_lock.as_mut() {
    //         let mut cursor: usize = 0;
    //         loop {
    //             let (new_cursor, results): (usize, Vec<(String, String)>) =
    //                 client.hscan(hash_key, cursor).await;
    //             for (field, value) in results {
    //                 callback(field, value, arg);
    //             }
    //             if new_cursor == 0 {
    //                 break;
    //             }
    //             cursor = new_cursor;
    //         }
    //         Ok(())
    //     } else {
    //         Err(RedisError::from((redis::ErrorKind::IoError, "Redis client not connected")))
    //     }
    // }
}
