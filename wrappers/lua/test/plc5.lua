local plctag = require 'plctag'

local path = "protocol=ab_eip&gateway=10.206.1.28&cpu=PLC5&elem_size=4&elem_count=5&name=F8:10&debug=1"
local tag = plctag.create(path)

while plctag.Status.PENDING == plctag.status(tag) do
	plctag.sleep(100)
	io.write('.')
end

if plctag.status(tag) ~= plctag.Status.OK then
	print('Error setting up tag internal state. '..plctag.decode_error(plctag.status(tag)))
end

