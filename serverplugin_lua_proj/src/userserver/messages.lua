local serversdk = require("src.helpers.serversdk")

local function define_class(base)
    local class = {}
    class.__index = class

    -- Inherit methods from the base class, if provided
    if base then
        setmetatable(class, { __index = base })
    end

    return class
end

-- Base class: message_base
local message_base = define_class()
function message_base.new()
    local self = setmetatable({}, message_base)
    return self
end

function message_base.get_type_string()
    return "message_base"
end

function message_base.get_type_string_crc()
    local type_string = message_base.get_type_string()
    local type_string_crc = serversdk.serverplugin.get_crc32(type_string, #type_string)
    return tonumber(type_string_crc)
end

function message_base:get_type_crc()
    return message_base.get_type_string_crc()
end

function message_base:get_type()
    return message_base.get_type_string()
end

-- Derived class: rq_msg_user_base
local rq_msg_user_base = define_class(message_base)
function rq_msg_user_base.new()
    local self = setmetatable({}, rq_msg_user_base)
    self.pid = ""
    self.token = ""
    return self
end

-- Derived class: rq_msg_user_get
local rq_msg_user_get = define_class(rq_msg_user_base)
function rq_msg_user_get.new()
    local self = setmetatable({}, rq_msg_user_get)
    self.device = rq_msg_user_get.device_struct.new()
    return self
end

function rq_msg_user_get.get_type_string()
    return "rq_msg_user_get"
end

function rq_msg_user_get.get_type_string_crc()
    local type_string = rq_msg_user_get.get_type_string()
    local type_string_crc = serversdk.serverplugin.get_crc32(type_string, #type_string)
    return tonumber(type_string_crc)
end

function rq_msg_user_get:get_type_crc()
    return rq_msg_user_get.get_type_string_crc()
end

function rq_msg_user_get:get_type()
    return rq_msg_user_get.get_type_string()
end

-- Namespace for device_struct
rq_msg_user_get.device_struct = define_class()
function rq_msg_user_get.device_struct.new()
    local self = setmetatable({}, rq_msg_user_get.device_struct)
    self.sys_name = ""
    self.node_name = ""
    self.release = ""
    self.arch = ""
    return self
end


-- Derived class: res_msg_user_base
local res_msg_user_base = define_class(message_base)
function res_msg_user_base.new()
    local self = setmetatable({}, res_msg_user_base)
    self.pid = ""         -- Default value for pid
    self.room_list = nil  -- Default value for room_list
    return self
end

-- Derived class: res_msg_user_get
local res_msg_user_get = define_class(res_msg_user_base)
function res_msg_user_get.new()
    local self = setmetatable({}, res_msg_user_get)
    self.last_login = ""   -- Default value for last_login
    self.user_name = ""    -- Default value for user_name
    self.token = ""        -- Default value for token
    self.gservers = nil    -- Default value for gservers
    return self
end

return {
    rq_msg_user_get = rq_msg_user_get,
    res_msg_user_get = res_msg_user_get
}
