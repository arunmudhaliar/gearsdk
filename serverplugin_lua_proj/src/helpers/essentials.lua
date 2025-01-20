-- local openssl = require("openssl")

local essentials = {}

-- -- Function to get UTC time in readable string format and as a Unix timestamp
-- function essentials.get_time_utc_readable()
--     local utc_date = os.date("!*t")  -- Get UTC date
--     local utc_date_string = string.format("%04d-%02d-%02d %02d:%02d:%02d", utc_date.year, utc_date.month, utc_date.day, utc_date.hour, utc_date.min, utc_date.sec)
--     local utc_date_number = math.floor(os.time(utc_date))  -- Get Unix timestamp in seconds
--     return { utc_date_string = utc_date_string, utc_date_number = utc_date_number }
-- end

-- Function to extract IP and port from a string in the format "ip:port"
function essentials.extract_ip_and_port(input)
    local ip, port = input:match("^(%d+%.%d+%.%d+%.%d+):(%d+)$")
    if ip and port then
        return ip, math.floor(tonumber(port, 10))  -- Return IP and port as number
    end
    return nil  -- Return nil if input does not match the expected format
end

-- Function to calculate the SHA-256 hash of an input string
-- function essentials.sha256(input)
--     local hash = openssl.digest("sha256", input)  -- Calculate SHA-256 hash
--     return hash
-- end

-- -- Example usage
-- local time_info = essentials.get_time_utc_readable()
-- print("UTC Date String: ", time_info.utc_date_string)
-- print("UTC Date Number: ", time_info.utc_date_number)

-- local ip, port = essentials.extract_ip_and_port("192.168.1.1:8080")
-- if ip and port then
--     print("IP: ", ip)
--     print("Port: ", port)
-- else
--     print("Invalid IP and port format")
-- end

-- local sha_hash = essentials.sha256("hello world")
-- print("SHA-256 Hash: ", sha_hash)

return essentials
