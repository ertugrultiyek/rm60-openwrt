module("luci.controller.mobile_app", package.seeall)  --notice that new_tab is the name of the file new_tab.lua
--hlk-5g 改为 mobile_app
 function index()
	if not nixio.fs.access("/etc/config/mobile_app") then
    	return
    end
	local page

	 --page = node("admin", "hlk", "5G")
	 --page.target = view("hlk/mobile_app")
	 --page.title  = _("5G")
	 --page.order  = 30

	 page = entry({"admin", "hlk", "mobile_conf"}, cbi("hlk/mobile_conf_cbi"), _("Mobile Config"))
	 page.dependent = true
 
	 entry({"admin", "hlk", "mobile_conf", "module_status"}, call("module_status")).leaf = true	--调用 下面的lua 函数
	 entry({"admin", "hlk", "mobile_conf", "sim_status"}, call("sim_status")).leaf = true
end

function sim_status()
	simstatus = luci.sys.exec("at_trans show_sim_status")
	operator = luci.sys.exec("at_trans show_operator")
	rssi = luci.sys.exec("at_trans show_rssi")
	nettype = luci.sys.exec("at_trans show_nettype")

	--获取SIM 卡状态
	local connect_status = {
		simstatus = simstatus,
		operator = operator,
		rssi = rssi,
		nettype = nettype,
	}

	luci.http.prepare_content("application/json")
	luci.http.write_json(connect_status)
end

function module_status()
	module_name = "-"
	imei = "-"
	simstatus = "-"
	operator = "-"
	rssi = "-"
	nettype = "-"
	
	filenm,err=io.open("/tmp/mobile/module_name")
	if filenm  then
		module_name = luci.sys.exec("cat /tmp/mobile/module_name")
		filenm:close()
	end

	filenm,err=io.open("/tmp/mobile/sim_status")
	if filenm  then
		sim_status = luci.sys.exec("cat /tmp/mobile/sim_status")
		filenm:close()
	end

	filenm,err=io.open("/tmp/mobile/imei")
	if filenm  then
		imei = luci.sys.exec("cat /tmp/mobile/imei")
		filenm:close()
	end
	filenm,err=io.open("/tmp/mobile/rssi")
	if filenm  then
		rssi = luci.sys.exec("cat /tmp/mobile/rssi")
		filenm:close()
	end
	filenm,err=io.open("/tmp/mobile/operator")
	if filenm  then
		operator = luci.sys.exec("cat /tmp/mobile/operator")
		filenm:close()
	end
	filenm,err=io.open("/tmp/mobile/nettype")
	if filenm  then
		nettype = luci.sys.exec("cat /tmp/mobile/nettype")
		filenm:close()
	end

	--获取SIM 卡状态
	local moduleinfo = {
		modulename = module_name,
		imei = imei,
		simstatus = sim_status,
		operator = operator,
		rssi = rssi,
		nettype = nettype,
	}
	cmd = "logger module_name:"..module_name.." imei: "..imei.." simstatus: "..simstatus.." aaaaaaaaaaaaaaaaaaaa"
	--luci.sys.exec(cmd)

	luci.http.prepare_content("application/json")
	luci.http.write_json(moduleinfo)
end


