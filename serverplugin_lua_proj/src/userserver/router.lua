local serversdk = require("src.helpers.serversdk")
local sdktypes = require("src.helpers.sdktypes")
local serverconfig_reader = require("src.helpers.serverconfig-reader")
local ffi = require("ffi")
ffi.cdef[[
    typedef struct qh3router qh3router;
    typedef void (*type_on_router_pre_start)(qh3router* router, void* user_arg);
    typedef void (*type_on_router_start)(qh3router* router, void* user_arg);
    typedef void (*type_on_router_stop)(qh3router* router, void* user_arg);
    typedef void (*type_on_router_error)(qh3router* router, void* user_arg, int error_code);
]]

local server = {}
server.router = {}
server.router.__LOGTAG__ = "router"

-- Router configuration
server.router.router_config = {
    router_address = serverconfig_reader:get_instance():get_value("router_address"),
    mongodb_uri = serverconfig_reader:get_instance():get_value("router_mongodb_uri"),
    redis_address = serverconfig_reader:get_instance():get_value("router_redis_uri"),
    zk_uri = serverconfig_reader:get_instance():get_value("router_zk_uri"),
    root_dir = os.getenv("PWD") or ".", -- Get the current working directory
    command_port = serverconfig_reader:get_instance():get_value_as_number("command_port", 4010),
    router_port_return = serverconfig_reader:get_instance():get_value_as_number("router_port_return", 4005),
    app_id = serverconfig_reader:get_instance():get_value("app_id"),
}

-- Constructor
function server.router:new(on_router_start_cb, server_app_obj)
    local obj = {}
    setmetatable(obj, self)
    self.__index = self
    obj.on_router_start_cb = on_router_start_cb
    obj.server_app_obj = server_app_obj
    return obj
end

-- Callback: on_router_pre_start
function server.router:on_router_pre_start(native_router, user_arg)
    sdktypes.debug_print(sdktypes.LOG_LEVEL_0, server.router.__LOGTAG__, "on_router_pre_start")
end

server.router.on_pre_start_cb = ffi.cast("type_on_router_pre_start", function(native_router, user_arg)
    print("native_router:", native_router, "user_arg:", user_arg)
end)

-- Callback: on_router_start
server.router.on_router_start = ffi.cast("type_on_router_start", function(native_router, user_arg)
    sdktypes.debug_print(sdktypes.LOG_LEVEL_0, server.router.__LOGTAG__, "on_router_start")
    local thiz = serversdk.serverplugin.get_from_registry(user_arg)
    -- Execute the start callback asynchronously
    if thiz.on_router_start_cb then
        -- Simulate asynchronous behavior
        local success, err = pcall(function()
            thiz:on_router_start_cb(native_router, thiz.server_app_obj)
        end)
        if not success then
            sdktypes.debug_error(server.router.__LOGTAG__, "Error in on_router_start callback: " .. tostring(err))
        end
    end
end)

-- Callback: on_router_stop
server.router.on_router_stop = ffi.cast("type_on_router_stop", function(native_router, user_arg)
    sdktypes.debug_print(sdktypes.LOG_LEVEL_0, server.router.__LOGTAG__ "on_router_stop")
end)

-- Callback: on_router_error
server.router.on_router_error = ffi.cast("type_on_router_error", function(native_router, user_arg, error_code)
    sdktypes.debug_error(server.router.__LOGTAG__, "on_router_error: " .. tostring(error_code))
end)

-- Run function to start the router
function server.router:run()
    local config = self.router_config
    return serversdk.serverplugin.spawn_qh3router(
        config.router_address,
        config.mongodb_uri,
        config.redis_address,
        config.zk_uri,
        config.root_dir,
        config.command_port,
        config.router_port_return,
        config.app_id,
        self.on_pre_start_cb,
        self.on_router_start,
        self.on_router_stop,
        self.on_router_error,
        self
    )
end

-- Simulate destructor using the __gc metamethod
function server.router:__gc()
    print(self.name .. " object is being garbage collected.")
end

return server
