local function get_os()
	local raw_os_name, raw_arch_name = '', ''

	-- LuaJIT shortcut
	if jit and jit.os and jit.arch then
		raw_os_name = jit.os
		raw_arch_name = jit.arch
	else
		-- is popen supported?
		local popen_status, popen_result = pcall(io.popen, "")
		if popen_status then
			popen_result:close()
			-- Unix-based OS
			raw_os_name = io.popen('uname -s','r'):read('*l')
			raw_arch_name = io.popen('uname -m','r'):read('*l')
		else
			-- Windows
			local env_OS = os.getenv('OS')
			local env_ARCH = os.getenv('PROCESSOR_ARCHITECTURE')
			if env_OS and env_ARCH then
				raw_os_name, raw_arch_name = env_OS, env_ARCH
			end
		end
	end

	raw_os_name = (raw_os_name):lower()
	raw_arch_name = (raw_arch_name):lower()

	local os_patterns = {
		['windows'] = 'Windows',
		['linux'] = 'Linux',
		['mac'] = 'Mac',
		['darwin'] = 'Mac',
		['osx'] = 'Mac',
		['^mingw'] = 'Windows',
		['^cygwin'] = 'Windows',
		['bsd$'] = 'BSD',
		['SunOS'] = 'Solaris',
	}
	
	local arch_patterns = {
		['^x86$'] = 'x86',
		['i[%d]86'] = 'x86',
		['amd64'] = 'x86_64',
		['x86_64'] = 'x86_64',
		['x64'] = 'x86_64',
		['Power Macintosh'] = 'powerpc',
		['^arm'] = 'arm',
		['^mips'] = 'mips',
	}

	local os_name, arch_name = 'unknown', 'unknown'

	for pattern, name in pairs(os_patterns) do
		if raw_os_name:match(pattern) then
			os_name = name
			break
		end
	end
	for pattern, name in pairs(arch_patterns) do
		if raw_arch_name:match(pattern) then
			arch_name = name
			break
		end
	end
	return os_name, arch_name
end

if select(1, ...) ~= 'os_name' then
	print(("%q %q"):format(get_os()))
else
	return {
		getOS = get_os,
	}
end

local function detect_platform()
    local os_name, _ = get_os()
    if os_name == "Windows" then
        return "windows"
    elseif os_name == "Linux" then
        return "linux"
    elseif os_name == "Mac" then
        return "darwin"
    else
        return "unknown"
    end
end

local function get_current_dir()
    local path = debug.getinfo(1, "S").source:sub(2)  -- remove the "@" at the beginning
    return path:match("^(.*[/\\])")  -- extract the directory part
end

local function get_workspace_path()
    local path = os.getenv("PWD")  -- PWD holds the current working directory in Unix-like systems
    if not path then
        -- Fallback for Windows systems
        path = os.getenv("CD") or "."
    end
    return path
end

local function join_paths(base, relative)
    -- Concatenate paths with proper separator based on platform
    if package.config:sub(1,1) == '\\' then
        return base .. '\\' .. relative  -- For Windows
    else
        return base .. '/' .. relative  -- For Unix-like systems
    end
end

-- Return the functions to be used in other files
return {
    get_os = get_os
    , detect_platform = detect_platform
	, get_current_dir = get_current_dir
	, get_workspace_path = get_workspace_path
	, join_paths = join_paths
}