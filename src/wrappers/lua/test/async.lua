local plctag = require 'plctag'

--local path_fmt = "protocol=ab_eip&gateway=10.206.1.27&path=1,0&cpu=LGX&elem_size=4&elem_count=1&name=pcomm_test_dint_array[%d]"
local path_fmt = "protocol=ab_eip&gateway=127.0.0.1&path=1,0&cpu=LGX&elem_size=4&elem_count=1&name=pcomm_test_dint_array[%d]"
local num_tags = 150
local data_timeout = 5000

local function test_async()
	local tags = {}

	for i = 0, num_tags - 1 do
		local path = string.format(path_fmt, i)
		print(string.format('create tag %d: %s', i, path))
		local tag = plctag.create(path, 5000)
		if not tag then
			print(string.format("Error: cloud not create tag %d", i))
			return
		end

		tags[#tags + 1] = tag

		local rc = plctag.status(tag) 
		print(string.format("%d: %s", rc, plctag.decode_error(rc)))
	end

	print("Sleeping to let the connect complete.")
	plctag.sleep(1000)

	for _, tag in ipairs(tags) do
		local status = plctag.status(tag)
		if status ~= plctag.Status.OK then
			print(string.format('Error: status tag error %d: %s', status, plctag.decode_error(status)))
		end
	end

	for _, tag in ipairs(tags) do
		local rc = plctag.read(tag, 0)
		if rc ~= plctag.Status.OK and rc ~= plctag.Status.PENDING then
			print(string.format('ERROR: Unable to read the data! Got error code %d, %s', rc, plctag.decode_error(rc)))
			return
		end
	end

	print("Sleeping to let the reads completed")
	plctag.sleep(2000)

	for i, tag in ipairs(tags) do
		local rc = plctag.status(tag)
		if rc == plctag.Status.PENDING then
			print(string.format("Tag %d is still pending", i))
		elseif rc ~= plctag.Status.OK then
			print(string.format("Tag %d status error! %d: %s", i, rc, plctag.decode_error(rc)))
		else
			print(string.format("Tag %d data[0]=%d", i, plctag.get_int32(tag, 0)))
		end
	end

	for _, tag in ipairs(tags) do
		plctag.destroy(tag)
	end

end

test_async()

