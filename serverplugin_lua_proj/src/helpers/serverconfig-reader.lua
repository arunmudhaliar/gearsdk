local platform = require("src.helpers.platform")
local server_config_reader = {}
server_config_reader.__index = server_config_reader

-- Singleton instance
local instance = nil

-- Constructor
function server_config_reader:new()
    local obj = {
        config_map = {} -- Table to store key-value pairs
    }
    setmetatable(obj, self)
    return obj
end

-- Static method to get the instance
function server_config_reader:get_instance()
    if not instance then
        instance = server_config_reader:new()
        instance:initialize_config();
    end
    return instance
end

-- Method to load and parse the config file
function server_config_reader:load_config(file_path)
    local file = io.open(file_path, "r")
    if not file then
        error("Could not open config file: " .. file_path)
    end

    for line in file:lines() do
        line = line:match("^%s*(.-)%s*$") -- Trim whitespace

        -- Skip empty lines and comments
        if line ~= "" and not line:match("^#") then
            -- Handle inline comments (e.g., key = value #comment)
            local key_value_part = line:match("^(.-)#") or line
            key_value_part = key_value_part:match("^%s*(.-)%s*$") -- Trim again

            -- Split key and value by '='
            local key, raw_value = key_value_part:match("^(.-)%s*=%s*(.-)$")

            if key and raw_value then
                -- Remove surrounding quotes (if any) from the value
                local value = self:strip_quotes(raw_value)
                self.config_map[key] = value
            end
        end
    end

    file:close()
end

-- Method to get all key-value pairs
function server_config_reader:get_all()
    return self.config_map
end

-- Method to get a value by key
function server_config_reader:get_value(key)
    return self.config_map[key] or ""
end

-- Method to get a value with a default fallback
function server_config_reader:get_value_else_default(key, default_value)
    return self.config_map[key] or default_value
end

-- Method to get a value as a number, with a default fallback
function server_config_reader:get_value_as_number(key, default_value)
    local value = self:get_value(key)
    if value ~= "" then
        local parsed_number = tonumber(value)
        if parsed_number then
            return parsed_number
        end
    end
    return default_value
end

-- Helper method to strip quotes from a value
function server_config_reader:strip_quotes(value)
    return value:gsub("^['\"`]", ""):gsub("['\"`]$", "")
end

-- Initialize configuration
function server_config_reader:initialize_config()
    local __dirname = platform.get_workspace_path();
    if os.getenv("SERVER_ENV") == "production" then
        print("Running in production mode")
        self:load_config(platform.join_paths(__dirname, './serverconfig.rel.inf'))
    else
        print("Running in development mode")
        self:load_config(platform.join_paths(__dirname, './serverconfig.rel.inf'))
    end
end

return server_config_reader
