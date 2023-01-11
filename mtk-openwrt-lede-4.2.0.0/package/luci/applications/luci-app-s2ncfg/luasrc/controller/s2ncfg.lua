-- Copyright 2008 Steven Barth <steven@midlink.org>
-- Copyright 2008 Jo-Philipp Wich <jow@openwrt.org>
-- Licensed to the public under the Apache License 2.0.

module("luci.controller.s2ncfg", package.seeall)

function index()
	if not nixio.fs.access("/etc/config/s2ncfg") then
		return
	end

	local page

	page = entry({"admin", "services", "s2ncfg"}, cbi("s2ncfg"), _("Serial to Network"))
	page.dependent = true
end
