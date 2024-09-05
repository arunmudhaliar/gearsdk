.. _essentials_read:

essentials.hpp
==============

This file defines essential utilities and classes for network operations, including mutex handling, time utilities, memory information, file system operations, and more.

Includes
--------

The file includes several headers for common utilities and external libraries:

- `../../common/qstring.hpp`
- `../../common/sdktypes.hpp`
- `../../common/timer.hpp`
- `qtimer.hpp`
- `<map>`
- `<netdb.h>`
- `<netinet/in.h>`
- `<string>`
- `<vector>`
- `<rapidjson/document.h>`
- `<rapidjson/rapidjson.h>`
- `<rapidjson/stringbuffer.h>`
- `<rapidjson/writer.h>`
- `<time.h>`
- `<zlib.h>`

Macros
------

- `__LOGTAG__`: Defines the log tag for this file.
- `EV_START_RECORD(timestamp_)`: Starts recording an event timestamp.
- `EV_PRINT_IF_ELAPSED(timestamp_, tag, formatted_msg, warn_after_ms)`: Prints if the elapsed time exceeds a warning threshold.
- `EV_PRINT_IF_ELAPSED_AND_CLEAR(timestamp_, tag, formatted_msg, warn_after_ms)`: Prints if the elapsed time exceeds a warning threshold and clears the timestamp.

Classes
-------

qmutexcondition
---------------

Handles condition variables for mutexes.

- `qmutexcondition()`: Constructor.
- `~qmutexcondition()`: Destructor.
- `int init(const std::string& name)`: Initializes the condition variable.
- `int signal(const char* msg = nullptr)`: Signals the condition variable.
- `int broadcast(const char* msg = nullptr)`: Broadcasts the condition variable.
- `int conditionWait(qmutex& qmutex, const char* msg)`: Waits on the condition variable.

qmutex
------

Handles mutex operations.

- `qmutex()`: Constructor.
- `~qmutex()`: Destructor.
- `int init(const std::string& name)`: Initializes the mutex.
- `int try_lock(const char* lockedBy, const char* msg = nullptr)`: Tries to lock the mutex.
- `int unlock(const char* msg = nullptr)`: Unlocks the mutex.
- `pthread_mutex_t* getMutexInternal()`: Returns the internal mutex.
- `void conditional_wait(const char* waiting_at)`: Waits conditionally.
- `void block(const char* blockedBy = nullptr)`: Blocks the mutex.
- `void unblock(const char* unblockedBy)`: Unblocks the mutex.

essentials
----------

Provides various utility functions.

- `int init_essentials()`: Initializes essential utilities.
- `static int32_t resolve_cmd_line_args(...)`: Resolves command line arguments.
- `static time_t get_time_local()`: Gets the local time.
- `static qstring get_time_local_tostring()`: Gets the local time as a string.
- `static time_t get_time_utc()`: Gets the UTC time.
- `static qstring get_time_utc_string()`: Gets the UTC time as a string.
- `static qstring get_time_utc_readable()`: Gets the readable UTC time.
- `static qstring get_time_utc_postgresql_format()`: Gets the UTC time in PostgreSQL format.
- `static qstring get_sysname()`: Gets the system name.
- `static qstring get_device_name()`: Gets the device name.
- `static qstring get_device_arch()`: Gets the device architecture.
- `static qstring get_device_release_str()`: Gets the device release string.
- `static int get_memory_info(...)`: Gets memory information.
- `static long long get_total_ram()`: Gets the total RAM.
- `static long long get_used_mem()`: Gets the used memory.
- `static int get_process_used_mem()`: Gets the memory used by the process.
- `static int get_all_child_folders(...)`: Gets all child folders.
- `static int get_all_files(...)`: Gets all files with a specific extension.
- `static unsigned long get_crc(const uint8_t* buffer, ssize_t len)`: Gets the CRC of a buffer.
- `static int get_addr_storage(...)`: Gets the address storage.
- `static int update_port(...)`: Updates the port.
- `static uLong mod_crc32_z(...)`: Modifies the CRC32.

conn_io_req_res
---------------

Handles connection I/O request and response.

- `struct header`: Represents a header.
- `struct payload`: Represents a payload.
- `static conn_io_req_res* create()`: Creates a new instance.
- `const payload& get_payload() const`: Gets the payload.
- `const payload& set_payload(const qstring& payload_)`: Sets the payload.
- `const payload& append_to_payload(const uint8_t* str, int len)`: Appends to the payload.
- `void clear_payload()`: Clears the payload.
- `header* add_or_get_header(const qstring& name_, const qstring& value_)`: Adds or gets a header.
- `header* get_header(const qstring& name_) const`: Gets a header.
- `bool is_postrequest() const`: Checks if it is a POST request.
- `bool validate()`: Validates the request/response.
- `bool has_crc_header()`: Checks if it has a CRC header.

qaddress
--------

Handles address information.

- `qaddress()`: Constructor.
- `qaddress(const qstring& ip_, uint16_t port_)`: Constructor with IP and port.
- `qaddress(const qstring& ip_, const qstring& port_)`: Constructor with IP and port as strings.
- `qaddress(struct sockaddr& addr)`: Constructor with sockaddr.
- `int set(struct sockaddr& addr)`: Sets the address.
- `int set(const qstring& ip_, const qstring& port_)`: Sets the IP and port.
- `int serialise(qstring& buf)`: Serializes the address.
- `int deserialise(const uint8_t* buf, ssize_t len)`: Deserializes the address.

Attributes
----------

- `uint16_t port`: Port number.
- `qstring ip`: IP address.