local sdktypes = require("src.helpers.sdktypes")
local serversdk = require("src.helpers.serversdk")
local cjson = require("cjson")
local ffi = require("ffi")

local serverconfig = {}
serverconfig.__index = serverconfig

-- Observer for configuration changes
local observer_serverconfig = {}
function observer_serverconfig:configchanged(path, data)
    error("configchanged must be implemented by the subclass")
end

-- Serverconfig class
function serverconfig:new(zk_interface, observer)
    local obj = {}
    setmetatable(obj, self)
    self.__index = self
    obj.__LOGTAG__ = "serverconfig"
    obj.zk_interface = zk_interface
    obj.config_change_observer = observer
    obj.configs = {}
    obj.value_change_callback_handler = nil
    if obj.zk_interface then
        obj.value_change_callback_handler = obj.zk_interface:register_value_change_callback(serverconfig.zk_value_change_listener, serversdk.serverplugin.add_lua_object_to_registry(obj))
    end
    return obj
    -- local self = setmetatable({}, serverconfig)
    -- self.__LOGTAG__ = "serverconfig"
    -- self.zk_interface = zk_interface
    -- self.config_change_observer = observer
    -- self.configs = {}

    -- if self.zk_interface then
    --     self.zk_interface:register_value_change_callback(serverconfig.zk_value_change_listener, self)
    -- end

    -- return self
end

function serverconfig:clear()
    self.configs = {}
end

function serverconfig:load(path, qzk, zk_root_folder)
    local file = io.open(path, "r")
    if not file then
        print(string.format("Couldn't read zk config - %s", path))
        return false
    end

    local buffer = file:read("*a")
    file:close()
    return self:iterate_and_load_keys(buffer, qzk, zk_root_folder)
end

function serverconfig:get_config(key, default_value)
    return self.configs[key] or default_value
end

function serverconfig:get_int32(key, default_value)
    local value = self.configs[key]
    if not value then
        print(string.format("get_int32: zk config not found for key %s. Setting default value of %d!", key, default_value))
        return default_value
    end

    local parsed_value = tonumber(value)
    if not parsed_value then
        print(string.format("Unable to parse %s value - %s. Setting default value of %d!", key, value, default_value))
        return default_value
    end

    return parsed_value
end

function serverconfig:get_string(key, default_value)
    return self.configs[key] or default_value
end

function serverconfig:iterate_and_load_keys(buffer, qzk, zk_root_folder)
    local success, parsed_data = pcall(function() return cjson.decode(buffer) end)
    if not success then
        print(string.format("JSON parse error: %s", parsed_data))
        return false
    end

    local config_keys = {}

    for key, value in pairs(parsed_data) do
        sdktypes.debug_print(sdktypes.LOG_LEVEL_0, self.__LOGTAG__, key)
        if type(value) ~= "table" then
            print(string.format("Root key (%s) must be a table!", key))
            return false
        end

        config_keys[key] = value
    end

    local count = 0
    for root_key, values in pairs(config_keys) do
        for _, key in ipairs(values) do
            local zk_key = string.format("%s/%s/%s", zk_root_folder, root_key, key)
            local data, err = qzk:get_data(zk_key, "{}")
            if err then
                print(string.format("Node does not exist or failed to fetch for key %s: %s", zk_key, err))
            else
                local mod_zk_key = string.format("%s/%s", root_key, key)
                self.configs[mod_zk_key] = data
                count = count + 1
            end
        end
    end

    sdktypes.debug_print(sdktypes.LOG_LEVEL_0, self.__LOGTAG__, string.format("%d keys loaded", count))
    return true
end

function serverconfig:try_update_value(path, data)
    local array = {}
    -- Split path by '/'
    for part in path:gmatch("[^/]+") do
        table.insert(array, part)
    end

    local mod_zk_key = path
    if #array > 1 then
        -- Replace the first part of the path (if present)
        mod_zk_key = path:gsub("^/[^/]+/", "")
    end

    if self.configs[mod_zk_key] then
        self.configs[mod_zk_key] = data
        return true
    end
    return false
end

function serverconfig.zk_value_change_listener(path, data, context)
    local thiz = serversdk.serverplugin.get_from_registry(context)
    local lua_path = ffi.string(path)
    local lua_data = ffi.string(data)
    if thiz:try_update_value(lua_path, lua_data) then
        sdktypes.debug_print(sdktypes.LOG_LEVEL_0, thiz.__LOGTAG__, string.format("Config updated: %s", lua_path))
        if thiz.config_change_observer and thiz.config_change_observer.configchanged then
            thiz.config_change_observer:configchanged(lua_path, lua_data)
        end
    else
        print(string.format("Config not found for: %s", lua_path))
    end
end

-- Destructor for garbage collection
function serverconfig:__gc()
    if self.zk_interface then
        self.zk_interface:unregister_value_change_callback(serverconfig.zk_value_change_listener, self.value_change_callback_handler)
    end
end

return {
    serverconfig = serverconfig,
    observer_serverconfig = observer_serverconfig
}
