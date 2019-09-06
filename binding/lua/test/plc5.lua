local plctag = require 'plctag'

local path = "protocol=ab_eip&gateway=10.206.1.28&cpu=PLC5&elem_size=4&elem_count=5&name=F8:10&debug=1"
local elem_count = 5
local elem_size = 4
local data_timeout = 5000


local function test_plc5()
	local tag = plctag.create(path)

	while plctag.Status.PENDING == plctag.status(tag) do
		plctag.sleep(100)
		io.write('.')
	end
	local rc = plctag.status(tag) 

	if rc ~= plctag.Status.OK then
		print(string.format('Error setting up tag internal state. %s', plctag.decode_error(rc)))
		return
	end

	-- get the data
	rc = plctag.read(tag, data_timeout)
	if rc ~= plctag.Status.OK then
		print(string.format('ERROR: Unable to read the data! Got error code %d: %s', rc, plctag.decode_err(rc)))
		return
	end

	-- print out the data
	for i = 0, elem_count - 1 do
		print(string.format('data[%d]=%f', i, plctag.get_float32(tag, i * elem_size)))
	end

	-- now test a write
	for i = 0, elem_count do
		local val = plctag.get_float32(tag, i * elem_size)
		val = val + 1.5
		print(string.format('Setting element %d to %f', i, val))
		plctag.set_float32(tag, i * elem_size, val)
	end

	rc = plctag.write(tag, data_timeout)

	if rc ~= plctag.Status.OK then
		print(string.format('ERROR: Unable to write the data! Got error code %d: %s', rc, plctag.decode_err(rc)))
		return
	end

	-- Get the data again
	rc = plctag.read(tag, data_timeout)
	if rc ~= plctag.Status.OK then
		print(string.format('ERROR: Unable to read the data! Got error code %d: %s', rc, plctag.decode_err(rc)))
		return
	end

	-- print out the data
	for i = 0, elem_count - 1 do
		print(string.format('data[%d]=%f', i, plctag.get_float32(tag, i * elem_size)))
	end

	-- we are done
	plctab.destroy(tag)
end

test_plc5()
