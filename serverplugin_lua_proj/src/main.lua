print("Lua version: " .. _VERSION)
os.execute("which lua")
os.execute("which luajit")

package.cpath = package.cpath .. ";" .. os.getenv("HOME") .. "/.luarocks/lib/lua/5.1/?.so" .. ";" .. os.getenv("HOME") .. "/.asdf/installs/lua/5.1.5/luarocks/lib/lua/5.1/?.so"
package.path = package.path .. ";" .. os.getenv("HOME") ..  "/.luarocks/share/lua/5.1/?.lua" .. ";" .. os.getenv("HOME") .. "/.asdf/installs/lua/5.1.5/luarocks/share/lua/5.1/?.lua"

print(package.path)
print(package.cpath)
if jit then
    print("LuaJIT detected")
else
    print("LuaJIT not available")
end

local api_whoami = require("src.features.api_whoami")
local api_ping = require("src.features.api_ping")
local api_user_get = require("src.features.api_user_get")
local routerserver = require("src.userserver.router")
local userserver = require("src.userserver.userserver")
local sdktypes = require("src.helpers.sdktypes")

-- Server Application class
local server_app = {}
server_app.__index = server_app

-- Static variables
server_app.__LOGTAG__ = "server_app"
server_app.instance = nil

-- Constructor
function server_app:new()
    local instance = setmetatable({}, self)
    -- instance.custom_gameserver_instance = nil
    instance.router_instance = nil
    instance.userserver_instances = {}
    server_app.instance = instance
    return instance
end

-- Get singleton instance
function server_app:get_instance()
    return server_app.instance
end

-- Callback for when the router starts
function server_app:on_router_start_cb(native_router, thiz)
    sdktypes.debug_print(sdktypes.LOG_LEVEL_0, server_app.__LOGTAG__, "on_router_start_cb")
    thiz:start_userserver(native_router)
    -- thiz:start_userserver(native_router)
    -- thiz:start_userserver(native_router)
end

-- Start the router
function server_app:start_router()
    if self.router_instance ~= nil then
        sdktypes.debug_error(server_app.__LOGTAG__, "router_instance not null !!!")
        return
    end
    self.router_instance = routerserver.router:new(self.on_router_start_cb, self)
    self.router_instance:run()
end

-- -- Start the user server
function server_app:start_userserver(native_router)
    local userserver_instance = userserver.userserver:new()
    userserver_instance:register_api(api_whoami:new())
    userserver_instance:register_api(api_ping:new())
    userserver_instance:register_api(api_user_get:new())
    table.insert(self.userserver_instances, userserver_instance)
    userserver_instance:run(native_router)
end

-- Run the server application
function server_app:run()
    local success, err = pcall(function()
        self:start_router()
        -- self:start_userserver()
        -- self:start_gameserver()
    end)
    if not success then
        sdktypes.debug_error(server_app.__LOGTAG__, "Error starting server %s", err)
    end
end

-- Main execution
local app_instance = server_app:new()
app_instance:run()

-- Keep-alive ping
local function keep_alive()
    while true do
        print("Keep-alive ping...")
        os.execute("sleep " .. tonumber(5 * 60)) -- Every 5 minutes
    end
end

local co = coroutine.create(keep_alive)
coroutine.resume(co)
