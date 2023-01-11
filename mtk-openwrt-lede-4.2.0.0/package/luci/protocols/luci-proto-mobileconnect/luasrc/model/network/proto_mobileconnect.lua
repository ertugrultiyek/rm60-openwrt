-- Copyright 2016 David Thornley <david.thornley@touchstargroup.com>
-- Licensed to the public under the Apache License 2.0.

local netmod = luci.model.network
local interface = luci.model.network.interface
local proto = netmod:register_protocol("mobileconnect")

function proto.get_i18n(self)
	return luci.i18n.translate("Mobile Connect")
end

function proto.ifname(self)
	local base = netmod._M.protocol
	local ifname = base.ifname(self) -- call base class "protocol.ifname(self)"

	if ifname == nil then
		ifname = "MobileConnect-" .. self.sid
	end
	return ifname
end

function proto.get_interface(self)
	return interface(self:ifname(), self)
end

function proto.opkg_package(self)
	return "quectel"
end

function proto.is_installed(self)
	return nixio.fs.access("/lib/netifd/proto/mobileconnect.sh")
end

function proto.is_floating(self)
	return true
end

function proto.is_virtual(self)
	return true
end

function proto.get_interfaces(self)
	return nil
end

function proto.contains_interface(self, ifc)
	 return (netmod:ifnameof(ifc) == self:ifname())
end
