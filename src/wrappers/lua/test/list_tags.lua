
local plctag = require 'plctag'

for i, v in ipairs(arg) do
	print(i, v)
end

if #arg < 2 then
	print('Usage list_tags <PLC IP> <PLC path>\nExample: list_tags 10.1.2.3 1,0\n')
	os.exit(1)
end

local ip = arg[1]
local path = arg[2]

local tag_string = string.format("protocol=ab-eip&gateway=%s&path=%s&cpu=lgx&name=@tags", ip, path);

print("Using tag string:", tag_string)

local timeout_ms = 5000

local tag = plctag.create(tag_string, timeout_ms)

if tag < 0 then
	print("Unable to open tag!", plctag.decode_error(tag))
	os.exit(1)
end

local rc = plctag.read(tag, timeout_ms)
if rc ~= plctag.Status.OK then
	print("Unable to read tag!", plctag.decode_error(tag))
	os.exit(1)
end

local offset = 0
local index = 0

while offset < plctag.get_size(tag) do
	local id = plc_tag_get_uint32(tag, offset)
	offset = offset + 4
	local tag_type = plc_tag_get_uint16(tag, offset)
	offset = offset + 2
	local len = plc_tag_get_uint16(tag, offset)
	offset = offset + 2
	local dims_1 = plc_tag_get_uint32(tag, offset)
	offset = offset + 4
	local dims_2 = plc_tag_get_uint32(tag, offset)
	offset = offset + 4
	local dims_3 = plc_tag_get_uint32(tag, offset)
	offset = offset + 4

	local name_len = plc_tag_get_uint16(tag, offset)
	offset = offset + 2

	local tag_name = {}
	for i = 0, name_len do
		tag_name[#tag_name] = string.char(plc_tag_get_int8(tag, offset))
		offset = offset + 1
	end

	print(index, tag_name, id, tag_type, len, dims_1, dims_2, dims_3)
	index = index + 1
end

plctag.destroy(tag)
