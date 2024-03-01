-- Copyright 2008 Steven Barth <steven@midlink.org>
-- Copyright 2008 Jo-Philipp Wich <jow@openwrt.org>
-- Licensed to the public under the Apache License 2.0.

m = Map("s2ncfg", translate("Serial to Network"))

s = m:section(TypedSection, "com", "")
s.anonymous = true

s:tab("general",  translate("General Settings"))

s:taboption("general", Value, "mode", translate("mode"))
s:taboption("general", Value, "remoteip", translate("remoteip"))
s:taboption("general", Value, "remoteport", translate("remoteport"))
s:taboption("general", Value, "CLport", translate("CLport"))
s:taboption("general", Value, "remotepro", translate("remotepro"))
s:taboption("general", Value, "timeout", translate("timeout"))
s:taboption("general", Value, "SMaxConn", translate("SMaxConn"))
s:taboption("general", Value, "SRemoteIpPort", translate("SRemoteIpPort"))
s:taboption("general", Value, "TCPRealTime", translate("TCPRealTime"))
s:taboption("general", Value, "tcp_client_check", translate("tcp_client_check"))
s:taboption("general", Value, "SRemoteIpPort", translate("SRemoteIpPort"))
s:taboption("general", Value, "uart", translate("uart"))
s:taboption("general", Value, "uartpacklen", translate("uartpacklen"))
s:taboption("general", Value, "uartpacktimeout", translate("uartpacktimeout"))
s:taboption("general", Value, "uartpackintertimeout", translate("uartpackintertimeout"))
s:taboption("general", Value, "DisName", translate("DisName"))

s = m:section(TypedSection, "net", "")
s.anonymous = true

s:tab("network",  translate("Network Settings"))

s:taboption("network", Value, "netmode", translate("netmode"),translate("Network " .."Mode"))
s:taboption("network", Value, "sta2_4", translate("sta2_4"))
s:taboption("network", Value, "sta5_8", translate("sta5_8"))
s:taboption("network", Value, "ap2_4", translate("ap2_4"))
s:taboption("network", Value, "ap5_8", translate("ap5_8"))
s:taboption("network", Value, "lanip", translate("lanip"))
s:taboption("network", Value, "dhcpd", translate("dhcpd"))
s:taboption("network", Value, "wanip", translate("wanip"))
s:taboption("network", Value, "waninterface", translate("waninterface"))
s:taboption("network", Value, "laninterface", translate("laninterface"))
s:taboption("network", Value, "eth_endis", translate("eth_endis"))
s:taboption("network", Value, "wifi_endis", translate("wifi_endis"))


return m
