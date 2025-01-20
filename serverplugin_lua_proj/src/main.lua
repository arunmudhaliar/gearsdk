-- local serversdk = require("src/helpers/serversdk")

-- local function main()
--     print("Welcome to Lua!")
--     serversdk.get_crc32("hello", 5)
-- end
-- main()

print("Lua version: " .. _VERSION)
local handle = io.popen("which luajit")
local result = handle:read("*a")
handle:close()
print("LuaJIT executable path: " .. result)

-- Add LuaJIT's lib path to package.cpath
-- package.cpath = package.cpath .. ";/usr/local/Cellar/luajit/2.1.1736781742/lib/lua/5.1/?.so" .. ";/usr/local/lib/lua/5.4/?.so"
package.cpath = package.cpath .. ";/Users/amudaliar/.luarocks/lib/lua/5.1/?.so"
-- Optionally, add LuaJIT's lua path to package.path if needed
-- package.path = package.path .. ";/usr/local/Cellar/luajit/2.1.1736781742/share/lua/5.1/?.lua" .. ";/usr/local/Cellar/luarocks/3.11.1/share/lua/5.4/?.lua"
package.path = package.path .. ";/Users/amudaliar/.luarocks/share/lua/5.1/?.lua"

local serversdk = require("src.helpers.serversdk")
local api_whoami = require("src.features.api_whoami")
local api_ping = require("src.features.api_ping")

if jit then
    print("LuaJIT detected")
else
    print("LuaJIT not available")
end

local routerserver = require("src.userserver.router")
local userserver = require("src.userserver.userserver")
local sdktypes = require("src.helpers.sdktypes")
-- local qh3server = require("userserver.userserver")
-- local api_user_get = require("features.user_get.api_user_get")
-- local api_whoami = require("features.user_get.api_whoami")
-- local api_ping = require("features.user_get.api_ping")
-- local server_config_reader = require("src.helpers.serverconfig-reader")
-- local platform = require("src/helpers/platform")

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
    -- self:start_userserver(native_router)
    -- self:start_userserver(native_router)
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
--     native_router = native_router or nil
    local userserver_instance = userserver.userserver:new()
    userserver_instance:register_api(api_whoami:new())
    userserver_instance:register_api(api_ping:new())
--     userserver_instance:register_api(api_user_get:new())
    userserver_instance:run(native_router)
    table.insert(self.userserver_instances, userserver_instance)
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

print(package.path)
print(package.cpath)

-- -- Create a new instance
-- local api_instance = api_whoami:new()

-- -- Get the path
-- print(api_instance:get_path())  -- Output: /whoami

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
