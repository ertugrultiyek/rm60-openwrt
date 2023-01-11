-- Copyright 2016 David Thornley <david.thornley@touchstargroup.com>
-- Licensed to the public under the Apache License 2.0.

local map, section, net = ...

local device, apn, pincode, username, password
local auth, ipv6

--device = section:taboption("general", Value, "device", translate("Modem device"))
--device.rmempty = false

--local device_suggestions = nixio.fs.glob("/sys/class/net/*")

--if device_suggestions then
--	local node
--	for node in device_suggestions do
--		device:value(node)
--	end
--end

apn = section:taboption("general", Value, "apn", translate("APN"))

pincode = section:taboption("general", Value, "pincode", translate("PIN"))


username = section:taboption("general", Value, "username", translate("PAP/CHAP username"))


password = section:taboption("general", Value, "password", translate("PAP/CHAP password"))
password.password = true
