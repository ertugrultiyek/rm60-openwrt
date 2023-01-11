--使用/etc/config/hlk-5g的配置文件作为 映射数据模型Model
--hlk-5g 改为 mobile_app  既/etc/config/mobile_app 海凌科移动拨号应用
m = Map("mobile_app", translate(""), translate("")) -- cbi_file is the config file in /etc/config

--m2 = Map("network", "")
--创建一个TAB
m:section(SimpleSection).template  = "mobile_module"    --模组型号 和IMEI显示 页面 显示的是 mobile_module.htm 的页面

--m:section(SimpleSection).template  = "sim_status"

--[[
s = m:section(SimpleSection, "Mobile Dial Config", translate(""), translate("Mobile Dial Config"))  -- info is the section called info in cbi_file
s.addremove = false

a = s:option(Flag, "enable_mobile_dial", translate("Enable Dial"), translate("Enable Mobile Dial function"))
a.rmempty  = false

function a.write(self, section, value)
    --获取network中协议配置为 rm500q的节点
    hlk_mobile_dial_config = 0
    
    rc = luci.sys.call("cat /etc/config/network |grep \"option proto \'rm500q\'\" 2>&1 > /dev/null")
    if rc == 0 then
        --5G配置已存在
        hlk_mobile_dial_config = 1
        luci.sys.call("logger Mobile Dial config hasbeen exist")
    end

    if value == "1" then
        if hlk_mobile_dial_config == 1 then
            rc1 = luci.sys.call("ifconfig -a|grep \"rmnet_usb0.1\" 2>&1 > /dev/null")
            rc2 = luci.sys.call("ifconfig -a|grep \"wwan0\" 2>&1 > /dev/null")
            rc3 = luci.sys.call("ifconfig -a|grep \"usb0\" 2>&1 > /dev/null")
            if rc1 == 0 then
                luci.sys.call("logger RMNET RM500Q-GL Has been Started")
            elseif rc2 == 0 then
                --判断是移远的RM500Q 还是广和通的模块
                luci.sys.call("logger WWAN  5G Has been Started")
            elseif rc3 == 0 then
                luci.sys.call("logger WWAN  RG500U-CN Has been Started")
            else
                luci.sys.call("logger 5G Starting")
            end
            --获取network中的5Gex接口名称
            interface_5g = luci.sys.exec("uci show network|grep rm500q|awk -F'\.' '{print $2}'")
            luci.sys.call("ifup "..interface_5g)
        else
            --5G配置不存在 需要添加默认5G配置
            luci.sys.call("Add 5G Config")
            rc = luci.sys.call("uci set network.5G=interface")
            rc = luci.sys.call("uci set network.5G.proto=rm500q")
            rc = luci.sys.call("uci set network.5G.device=/dev/cdc-wdm0")
            --添加防火墙域
            old_network_zone = luci.sys.exec("uci show firewall.@zone[1].network|awk -F\"'\" '{print $2}'")
            
            old_network_zone1 = string.gsub(old_network_zone, "\n", "")

            luci.sys.call("logger old network zone:"..old_network_zone)
            luci.sys.call("logger old network zone1:"..old_network_zone1)
            rc = luci.sys.call("uci del firewall.@zone[1].network")
            --luci.sys.call("logger ".."uci set firewall.@zone[1].network='"..old_network_zone.." 5G'")
            rc = luci.sys.call("uci set firewall.@zone[1].network='"..old_network_zone1.." 5G'")
            rc = luci.sys.call("uci commit")
            luci.sys.call("ifup 5G")
        end
    else
        if hlk_mobile_dial_config == 1 then
            rc1 = luci.sys.call("ifconfig |grep \"rmnet_usb0.1\" 2>&1 > /dev/null")
            rc2 = luci.sys.call("ifconfig |grep \"wwan0\" 2>&1 > /dev/null")
            if rc1 == 0 then
                luci.sys.call("logger 5G Down")
                interface_5g = luci.sys.exec("uci show network|grep rm500q|awk -F'\.' '{print $2}'")
                luci.sys.call("ifdown "..interface_5g)
            elseif rc2 == 0 then
                luci.sys.call("logger 5G Down")
                interface_5g = luci.sys.exec("uci show network|grep rm500q|awk -F'\.' '{print $2}'")
                luci.sys.call("ifdown "..interface_5g)
            end
        end
    end

    return Flag.write(self, section, value)
end
    

a = s:option(Value, 'apn', translate("APN"), translate("Access Point Name") )
a.default = ''
a = s:option(Value, 'user', translate('USER'), translate("Apn User Name"))
a.default = ''
a = s:option(Value, 'pwd', translate('PWD'), translate("Apn Password"))
a.default = ''
a = s:option(Value, 'pin', translate('PIN'), translate("Personal Identification Number"))
a.default = ''
]]
return m

