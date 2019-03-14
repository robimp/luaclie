require("luaclie.luaclie_c")

luaclie = {}

luaclie.COLOR            = {}
luaclie.COLOR.DEFAULT    = "\27[0m"
luaclie.COLOR.GRAY       = "\27[2m"
luaclie.COLOR.GRAY0      = "\27[30;1m"
luaclie.COLOR.GRAY1      = "\27[90;2m"
luaclie.COLOR.GRAY2      = "\27[30;3m"
luaclie.COLOR.RED        = "\27[31;1m"
luaclie.COLOR.RED0       = "\27[91;2m"
luaclie.COLOR.RED1       = "\27[31;3m"
luaclie.COLOR.RED2       = "\27[31;2m"
luaclie.COLOR.GREEN      = "\27[32;1m"
luaclie.COLOR.GREEN0     = "\27[92;2m"
luaclie.COLOR.GREEN1     = "\27[32;3m"
luaclie.COLOR.GREEN2     = "\27[32;2m"
luaclie.COLOR.YELLOW     = "\27[33;1m"
luaclie.COLOR.YELLOW0    = "\27[93;2m"
luaclie.COLOR.YELLOW1    = "\27[33;3m"
luaclie.COLOR.YELLOW2    = "\27[33;2m"
luaclie.COLOR.BLUE       = "\27[34;1m"
luaclie.COLOR.BLUE0      = "\27[94;2m"
luaclie.COLOR.BLUE1      = "\27[34;3m"
luaclie.COLOR.BLUE2      = "\27[34;2m"
luaclie.COLOR.MAGENTA    = "\27[35;1m"
luaclie.COLOR.MAGENTA0   = "\27[95;2m"
luaclie.COLOR.MAGENTA1   = "\27[35;3m"
luaclie.COLOR.MAGENTA2   = "\27[35;2m"
luaclie.COLOR.CYAN       = "\27[36;1m"
luaclie.COLOR.CYAN0      = "\27[96;2m"
luaclie.COLOR.CYAN1      = "\27[36;3m"
luaclie.COLOR.CYAN2      = "\27[36;2m"
luaclie.COLOR.BOLD       = "\27[1m"
luaclie.COLOR.BG         = {}
luaclie.COLOR.BG.GRAY    = "\27[47m"
luaclie.COLOR.BG.RED     = "\27[41m"
luaclie.COLOR.BG.GREEN   = "\27[42m"
luaclie.COLOR.BG.YELLOW  = "\27[43m"
luaclie.COLOR.BG.BLUE    = "\27[44m"
luaclie.COLOR.BG.MAGENTA = "\27[45m"
luaclie.COLOR.BG.CYAN    = "\27[46m"

--D: Table Miner
--P: table_path : Table path string
--R: keys       : Table with all keys on table
luaclie.tableminer = function (table_path)
	local current_table = {}
	local keys          = {}
	local key_name
	local table_name

	if table_path == nil then
		table_path = ""
	end

	current_table = _ENV

	for table_name in string.gmatch(table_path, "[^%.^:]+[%.:]") do
		if type(current_table) ~= "table" then
			return {}
		end
		current_table = current_table[string.match(table_name, "[^%.^:]+")]
	end

	if type(current_table) ~= "table" then
		return {}
	end

	for key in pairs(current_table) do
		key_name = key

		if type(current_table[key]) == "function" then
			key_name = key_name .. "("
		elseif type(current_table[key]) == "table" then
			key_name = key_name .. "."
		end

		table.insert(keys, key_name)
	end

	return keys
end

--D: Function Reference
--P: text : Input text with command line
luaclie.funcref = function (text)
	local current_table  = _ENV
	local table_name
	local last_func_name = nil
	local comment_table  = {}
	local func_info
	local maxlen         = 0
	local currlen

	print()

	-- Get function name with table path
	for func_name in string.gmatch(text, "[%a%d%.:_]+%(") do
		last_func_name = func_name
	end

	if last_func_name == nil then
		print("no function found in \"" .. text .. "\"")
		return
	end

	-- Travel thought tables until function
	for func_name in string.gmatch(last_func_name, "[^%.^:^%(]+[%.:%(]") do
		if current_table == nil then
			print("table \"" .. table_name .. "\" is a nil value")
			return
		end
		table_name    = string.match(func_name, "[^%.^:^%(]+")
		current_table = current_table[table_name]
	end

	-- Error: no table
	if current_table == nil then
		print("function \"" .. table_name .. "\" is a nil value")
		return
	end

	func_info = debug.getinfo(current_table)
	
	-- Error: defined in user interface
	if func_info["short_src"] == "stdin" then
		print("function defined in user interface, it has no description")
		return
	elseif func_info["short_src"] == "[C]" then
		print("function defined in C code, it has no description")
		return
	end

	local file = io.open(func_info["short_src"])
	
	-- Error: no such file
	if file == nil then
		print("error: no such file: " .. func_info["short_src"])
		return
	end

	-- Search comments above function
	local counter = func_info["linedefined"]-1;
	for line in file:lines() do
		if string.match(line, "^[%s\t]*%-%-") ~= nil then
			table.insert(comment_table, string.match(line, "^[%s\t]*(%-%-.*)"))
		else 
			comment_table = {}
		end
		counter = counter-1
		if counter == 0 then
			break
		end
	end

	-- Get maximum string length from comment
	for _, line in pairs(comment_table) do
		currlen = string.len(line)
		if currlen > maxlen then
			maxlen = currlen
		end
	end
	if maxlen >= 80 then
		maxlen = 80
	end

	-- Print separator
	luaclie.printf("\27[30;4;1m")
	for i = 1, maxlen + 18 do
		luaclie.printf("-")
	end
	luaclie.printf("\27[0m\n")
	
	-- Print help text
	print("\27[33;1mFunction:\27[0m ")
	print("        " .. last_func_name)
	print("\27[33;1mReference:\27[0m")
	for _, line in pairs(comment_table) do
		print("        " .. line)
	end
	
	-- Print separator
	luaclie.printf("\27[30;4;1m")
	for i = 1, maxlen + 18 do
		luaclie.printf("-")
	end
	luaclie.printf("\27[0m\n")
end

--D: C-like printf
luaclie.printf = function (string, ...)
	io.write(string.format(string, ...))
end

--D: printf with color
luaclie.printc = function (color, string, ...)
	
	luaclie.printf(color)
	luaclie.printf(string, ...)
	luaclie.printf(luaclie.COLOR.DEFAULT)
end

--D: Clear screen
luaclie.cls = function ()
	os.execute("clear")
end

return luaclie
