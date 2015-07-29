require("luaclie_c")

luaclie = {}

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
		if string.match(line, "^%-%-") ~= nil then
			table.insert(comment_table, line)
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

--D: Clear screen
luaclie.cls = function ()
	os.execute("clear")
end