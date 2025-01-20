

local sdktypes = {}

-- Log levels
sdktypes.LOG_LEVEL = 2
sdktypes.LOG_LEVEL_0 = 0
sdktypes.LOG_LEVEL_1 = 1
sdktypes.LOG_LEVEL_2 = 2
sdktypes.LOG_LEVEL_3 = 3
sdktypes.LOG_LEVEL_4 = 4
sdktypes.LOG_LEVEL_5 = 5
sdktypes.LOG_LEVEL_6 = 6

-- Exit codes
sdktypes.EXIT_SUCCESS = 0
sdktypes.EXIT_FAILURE = 1

-- Debugging functions
function sdktypes.debug_print(level, logtag, message, ...)
    if level > sdktypes.LOG_LEVEL then
        return
    end
    print(string.format("[%s] - %s", logtag, message), ...)
end

function sdktypes.debug_error(logtag, message, ...)
    print(string.format("[ERROR] [%s] - %s", logtag, message), ...)
end

function sdktypes.debug_warn(logtag, message, ...)
    print(string.format("[WARN] [%s] - %s", logtag, message), ...)
end

return sdktypes;