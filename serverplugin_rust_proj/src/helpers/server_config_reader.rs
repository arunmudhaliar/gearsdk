#![allow(non_camel_case_types)]  // Disable the warning for non-CamelCase types
use std::collections::HashMap;
use std::fs;

pub struct server_config_reader {
    config_map: HashMap<String, String>,
}

impl server_config_reader {
    // Static method to get the instance
    pub fn get_instance() -> &'static mut server_config_reader {
        static mut INSTANCE: Option<server_config_reader> = None;
        unsafe {
            INSTANCE.get_or_insert_with(|| server_config_reader {
                config_map: HashMap::new(),
            })
        }
    }

    // Method to load and parse the config file
    pub fn load_config(&mut self, file_path: &str) {
        let file_content = fs::read_to_string(file_path).unwrap_or_default();

        for line in file_content.lines() {
            let trimmed_line = line.trim();

            // Skip empty lines and comments
            if trimmed_line.is_empty() || trimmed_line.starts_with('#') {
                continue;
            }

            // Handle inline comments (e.g., key = value #comment)
            let key_value_part = trimmed_line.split('#').next().unwrap_or(trimmed_line);

            // Split key and value by '='
            let parts: Vec<&str> = key_value_part.split('=').map(|part| part.trim()).collect();
            if parts.len() == 2 {
                let key = parts[0].to_string();
                let value = self.strip_quotes(parts[1].to_string());
                self.config_map.insert(key, value);
            }
        }
    }

    // Method to get all key-value pairs as a HashMap
    pub fn get_all(&self) -> HashMap<String, String> {
        self.config_map.clone()
    }

    // Method to get a value by key
    pub fn get_value(&self, key: &str) -> String {
        self.config_map.get(key).cloned().unwrap_or_default()
    }

    pub fn get_value_else_default(&self, key: &str, default_value: &str) -> String {
        self.config_map.get(key).cloned().unwrap_or_else(|| default_value.to_string())
    }

    // Method to get a value as a number, with a default fallback
    pub fn get_value_as_number(&self, key: &str, default_value: u32) -> u32 {
        if let Ok(value) = self.get_value(key).parse::<u32>() {
            value
        } else {
            default_value
        }
    }

    // Helper method to strip quotes from a value
    fn strip_quotes(&self, value: String) -> String {
        value.trim_matches(|c| c == '"' || c == '\'' || c == '`').to_string()
    }
}
