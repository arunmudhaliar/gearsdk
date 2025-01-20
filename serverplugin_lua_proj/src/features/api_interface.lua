local interface_api = {}
interface_api.__index = interface_api

local ffi = require("ffi")
ffi.cdef[[
    typedef const char* (*type_on_api_parse)(
        void* server,
        int userserver_registry_id,
        uint8_t* cid,
        uint16_t cid_len,
        const char* path,
        const char* buffer,
        unsigned long len,
        const char* headers_buffer,
        unsigned long headers_buffer_size
    );
]]

function interface_api:get_path()
    error("get_path must be implemented")
end

function interface_api:get_post_cb()
    error("get_post_cb must be implemented")
end

-- -- Function to check if an object implements the interface
-- local function check_interface_implementation(obj, interface)
--     for method, ttype in pairs(interface) do
--         if method == '__index' then
--             -- Skip the '__index' method
--             goto continue
--         end
--         if type(obj[method]) ~= "function" then
--             error(string.format("Method '%s' must be implemented in the subclass", method))
--         end
--         ::continue::  -- Label for the 'goto'
--     end
-- end

-- Expose the interface and the check function
return {
    interface_api = interface_api,
    -- check_interface_implementation = check_interface_implementation
}
