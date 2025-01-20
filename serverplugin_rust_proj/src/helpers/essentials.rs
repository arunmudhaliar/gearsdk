#![allow(non_camel_case_types)]  // Disable the warning for non-CamelCase types
use std::{ffi::{CStr, CString}, time::{SystemTime, UNIX_EPOCH}};
use sha2::{Sha256, Digest};
use regex::Regex;

pub struct essentials;

impl essentials {
    // // Helper function to convert String to *const c_char
    // pub fn string_to_c_char(input: &str) -> *const std::os::raw::c_char {
    //     // Convert Rust string to CString
    //     let c_string = CString::new(input).expect("CString::new failed");
        
    //     // Return a pointer to the C-style string (*const c_char)
    //     c_string.as_ptr()
    // }

    // pub fn to_c_string_unchecked(input: &str) -> *const std::os::raw::c_char {
    //     // CString::new(input).expect("Failed to convert to CString").as_ptr()
    //     let c_string = Box::new(CString::new(input).expect("Failed to convert to CString"));
    //     Box::into_raw(c_string) // Return the raw pointer
    // }

    // pub fn c_string_to_string(c_str_ptr: *const std::os::raw::c_char ) -> String {
    //     // Safety: Dereferencing a raw pointer is unsafe, but we trust the pointer to be valid here
    //     unsafe {
    //         if c_str_ptr.is_null() {
    //             return String::new(); // Return an empty string if the pointer is null
    //         }
    
    //         // Convert *const c_char to CStr, then to String
    //         CStr::from_ptr(c_str_ptr)
    //             .to_string_lossy() // Converts CStr to a Cow<str> (lossy, handles invalid UTF-8)
    //             .into_owned() // Converts Cow<str> to a String
    //     }
    // }

    // Get current UTC time in a readable string and as a UNIX timestamp
    pub fn get_time_utc_readable() -> (String, u64) {
        let utc_date = SystemTime::now();
        let utc_date_string = utc_date
            .duration_since(UNIX_EPOCH)
            .map(|duration| {
                let naive = chrono::NaiveDateTime::from_timestamp(duration.as_secs() as i64, 0);
                naive.format("%+").to_string() // Format the UTC date as string
            })
            .unwrap_or_default();

        let utc_date_number = utc_date
            .duration_since(UNIX_EPOCH)
            .map(|duration| duration.as_secs())
            .unwrap_or_default();

        (utc_date_string, utc_date_number)
    }

    // Extract IP and port from the input string
    pub fn extract_ip_and_port(input: &str) -> Option<(String, u16)> {
        let regex = Regex::new(r"^([0-9]+\.[0-9]+\.[0-9]+\.[0-9]+):(\d+)$").unwrap();
        if let Some(captures) = regex.captures(input) {
            let ip = captures.get(1).map(|m| m.as_str().to_string()).unwrap();
            let port = captures.get(2).map(|m| m.as_str().parse::<u16>().unwrap()).unwrap();
            Some((ip, port))
        } else {
            None
        }
    }

    // Compute the sha256 hash of the input string
    pub fn sha256(input: &str) -> String {
        let mut hasher = Sha256::new();
        hasher.update(input);
        let result = hasher.finalize();
        format!("{:x}", result) // Return the hash as a hexadecimal string
    }
}
