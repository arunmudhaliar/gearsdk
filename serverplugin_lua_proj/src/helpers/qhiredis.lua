local ffi = require("ffi")

-- Define the C functions and types
ffi.cdef[[
    typedef struct qhiredis_t qhiredis_t;

    typedef void (*redis_hash_iterator_cb)(const char* field, const char* value, void* arg);
    typedef void (*redis_scan_iterator_cb)(const char* key, const char* field, const char* value, void* arg);

    qhiredis_t* qhiredis_create(const char* name, const char* redis_ip, uint16_t redis_port, const char* username, const char* password);
    void qhiredis_destroy(qhiredis_t* redis);
    int qhiredis_connect(qhiredis_t* redis, int unix_socket);
    void qhiredis_disconnect(qhiredis_t* redis);
    int qhiredis_set_value(qhiredis_t* redis, const char* key, const char* value);
    int qhiredis_set_value_exp(qhiredis_t* redis, const char* key, const char* value, int expiry_in_sec);
    int qhiredis_get_value(qhiredis_t* redis, const char* key, char* value, size_t value_len);
    int qhiredis_set_hash_value(qhiredis_t* redis, const char* hashkey, const char* field, const char* value);
    int qhiredis_get_hash_value(qhiredis_t* redis, const char* hashkey, const char* field, char* value, size_t value_len);
    int qhiredis_delete_hash_field(qhiredis_t* redis, const char* hashkey, const char* field);
    int qhiredis_incr(qhiredis_t* redis, const char* key, long long* value);
    int qhiredis_decr(qhiredis_t* redis, const char* key, long long* value);
    int qhiredis_incr_by(qhiredis_t* redis, const char* key, int delta, long long* value);
    int qhiredis_decr_by(qhiredis_t* redis, const char* key, int delta, long long* value);
    int qhiredis_expire_key(qhiredis_t* redis, const char* key, int expiry_in_sec);
    int qhiredis_delete_key(qhiredis_t* redis, const char* key);
    void qhiredis_iterate_hash(qhiredis_t* redis, const char* hashkey, void* arg, redis_hash_iterator_cb callback);
    void qhiredis_scan(qhiredis_t* redis, const char* prefix_key, void* arg, redis_scan_iterator_cb callback);
]]

-- Detect platform and load library
local lib_path
local lib_serverplugin_debug = "libserverplugin-debug"
local lib_serverplugin_release = "libserverplugin"
local lib_serverplugin = (os.getenv("SERVER_ENV") == "production") and lib_serverplugin_release or lib_serverplugin_debug
if ffi.os == "OSX" then
    lib_path = "../serverplugin/" .. lib_serverplugin .. ".dylib"
elseif ffi.os == "Linux" then
    lib_path = "../serverplugin/" .. lib_serverplugin .. ".so"
else
    error("Unsupported platform")
end
local sdklib = ffi.load(lib_path)

-- Define the Lua class
local qhiredis = {}
qhiredis.__index = qhiredis

-- Constructor
function qhiredis:new(name, redis_ip, redis_port, username, pass)
    -- local self = setmetatable({}, qhiredis)
    local obj = {}
    setmetatable(obj, self)
    self.__index = self

    obj.redis = sdklib.qhiredis_create(name, redis_ip, redis_port, username, pass)
    if obj.redis == nil then
        error("Failed to create qhiredis instance")
    end
    return obj
end

-- Destructor
function qhiredis:destroy()
    if self.redis ~= nil then
        sdklib.qhiredis_destroy(self.redis)
        self.redis = nil
    end
end

-- Connect to Redis
function qhiredis:connect(unix_socket)
    return sdklib.qhiredis_connect(self.redis, unix_socket)
end

-- Disconnect from Redis
function qhiredis:disconnect()
    sdklib.qhiredis_disconnect(self.redis)
end

-- Set value
function qhiredis:set_value(key, value)
    return sdklib.qhiredis_set_value(self.redis, key, value)
end

-- Set value with expiry
function qhiredis:set_value_exp(key, value, expiry_in_sec)
    return sdklib.qhiredis_set_value_exp(self.redis, key, value, expiry_in_sec)
end

-- Get value
function qhiredis:get_value(key)
    local value_buffer = ffi.new("char[?]", 1024)
    local result = sdklib.qhiredis_get_value(self.redis, key, value_buffer, 1024)
    if result == 0 then
        return ffi.string(value_buffer)
    else
        return nil, result
    end
end

-- Set hash value
function qhiredis:set_hash_value(hashkey, field, value)
    return sdklib.qhiredis_set_hash_value(self.redis, hashkey, field, value)
end

-- Get hash value
function qhiredis:get_hash_value(hashkey, field)
    local value_buffer = ffi.new("char[?]", 1024)
    local result = sdklib.qhiredis_get_hash_value(self.redis, hashkey, field, value_buffer, 1024)
    if result == 0 then
        return ffi.string(value_buffer)
    else
        return nil, result
    end
end

-- Delete hash field
function qhiredis:delete_hash_field(hashkey, field)
    return sdklib.qhiredis_delete_hash_field(self.redis, hashkey, field)
end

-- Increment a key
function qhiredis:incr(key)
    local value_ptr = ffi.new("long long[1]")
    local result = sdklib.qhiredis_incr(self.redis, key, value_ptr)
    if result == 0 then
        return tonumber(value_ptr[0])
    else
        return nil, result
    end
end

-- Decrement a key
function qhiredis:decr(key)
    local value_ptr = ffi.new("long long[1]")
    local result = sdklib.qhiredis_decr(self.redis, key, value_ptr)
    if result == 0 then
        return tonumber(value_ptr[0])
    else
        return nil, result
    end
end

-- Expire a key
function qhiredis:expire_key(key, expiry_in_sec)
    return sdklib.qhiredis_expire_key(self.redis, key, expiry_in_sec)
end

-- Delete a key
function qhiredis:delete_key(key)
    return sdklib.qhiredis_delete_key(self.redis, key)
end

-- Destructor for garbage collection
function qhiredis:__gc()
    self:destroy()
end

return qhiredis
