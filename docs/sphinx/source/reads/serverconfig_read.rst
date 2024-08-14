.. _serverconfig_hpp:

serverconfig.hpp
=================

This document explains the `serverconfig` class and its associated `observer_serverconfig` class. These classes are used to manage and update configuration settings in your application.

Classes
--------

**observer_serverconfig**

Purpose: This is an abstract base class for observing changes in configuration. It defines a pure virtual method that must be implemented by any concrete observer class.

- Method:
  - `void configchanged(const qstring& path, const qstring& data)`: This method is called when a configuration change is detected. The `path` represents the configuration path, and `data` represents the new configuration data.

**serverconfig**

Purpose: This class handles loading, storing, and updating configuration settings from a file and a Zookeeper instance.

- Constructor:
  - `serverconfig(interface_qzookeeper* interface, observer_serverconfig* observer)`: Initializes the `serverconfig` instance with a Zookeeper interface and an observer for configuration changes.

- Destructor:
  - `~serverconfig()`: Cleans up resources and unregisters the Zookeeper callback.

- Public Methods:
  - `void clear()`: Clears all loaded configuration data.
  - `bool load(const fs::path& path, qzookeeper* qzk, const qstring& zk_root_folder)`: Loads configuration from a file and populates settings from a Zookeeper instance.
  - `int get_config(const qstring& key, const qstring& default_value, qstring& result)`: Retrieves a configuration value for a given key. If the key is not found, the default value is used.
  - `int get_int32(const qstring& key, const int32_t default_value)`: Retrieves an integer configuration value for a given key, returning the default value if the key is not found.
  - `qstring get_string(const qstring& key, const qstring& default_value)`: Retrieves a string configuration value for a given key, returning the default value if the key is not found.

- Private Methods:
  - `static void zk_value_change_listener(const qstring& path, const qstring& data, void* context)`: Static method used as a callback for Zookeeper value changes.
  - `bool iterate_and_load_keys(const qstring& buffer, qzookeeper* qzk, const qstring& zk_root_folder)`: Parses the configuration buffer and loads keys from Zookeeper.
  - `bool try_update_value(const qstring& path, const qstring& data)`: Updates the configuration value if the key exists.

Source Files
------------

**Header File: `serverconfig.hpp`**

This file declares the `serverconfig` and `observer_serverconfig` classes.

**Source File: `serverconfig.cpp`**

This file provides the implementation of the `serverconfig` class.

- Constructor:
  - `serverconfig(interface_qzookeeper* interface, observer_serverconfig* observer)`: Initializes the class with the provided Zookeeper interface and observer. Registers the `zk_value_change_listener` method as a callback for Zookeeper value changes.

- Destructor:
  - `~serverconfig()`: Calls `clear()` to remove configuration data and unregisters the callback from the Zookeeper interface.

- Methods:
    - `void clear()`: 
        - Empties the `configs` map, removing all configuration data.
    - `bool load(const fs::path& path, qzookeeper* qzk, const qstring& zk_root_folder)`:
        - Reads configuration data from a file using `qtextfile::get_content`.
        - If reading succeeds, calls `iterate_and_load_keys` to load configuration values from Zookeeper.
    - `int get_config(const qstring& key, const qstring& default_value, qstring& result)`:
        - Searches for the configuration key in the `configs` map.
        - If found, returns the associated value; otherwise, returns the default value.
    - `int get_int32(const qstring& key, const int32_t default_value)`:
        - Searches for the configuration key, and attempts to parse its value as an integer.
        - Returns the default value if parsing fails or the key is not found.
    - `qstring get_string(const qstring& key, const qstring& default_value)`:
        - Similar to `get_config`, but returns the value as a string directly.
    - `bool iterate_and_load_keys(const qstring& buffer, qzookeeper* qzk, const qstring& zk_root_folder)`:
        - Parses JSON data from `buffer` using RapidJSON.
        - Extracts configuration keys and values from the parsed JSON.
        - Retrieves configuration values from Zookeeper and populates the `configs` map.
    - `bool try_update_value(const qstring& path, const qstring& data)`:
        - Updates the value in the `configs` map if the key exists.
    - `void zk_value_change_listener(const qstring& path, const qstring& data, void* context)`:
        - Callback method triggered by Zookeeper when a value changes.
        - Calls `try_update_value` to update the internal configuration and notifies the observer of the change.

Summary
-------

- **`serverconfig`**: Manages configuration settings by loading them from a file and Zookeeper. It also supports dynamic updates through Zookeeper callbacks.
- **`observer_serverconfig`**: Defines an interface for classes that need to be notified of configuration changes.
- **Error Handling**: Includes error reporting for JSON parsing and Zookeeper interactions to ensure robustness.
