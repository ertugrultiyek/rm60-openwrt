#!/bin/sh

[ -n "$INCLUDE_ONLY" ] || {
	. /lib/functions.sh
	. ../netifd-proto.sh
	init_proto "$@"
}

#/bin/sh ./mobileconnect.sh mobileconnect setup 4G {"proto":"mobileconnect"}

proto_mobileconnect_init_config() {
	available=1
	no_device=1
	proto_config_add_string "device:device"
	proto_config_add_string apn
	proto_config_add_string auth
	proto_config_add_string username
	proto_config_add_string password
	proto_config_add_string pincode
	proto_config_add_defaults
}

proto_mobileconnect_setup() {
	local interface="$1"
	local device apn auth username password pincode
	local timeout mtu $PROTO_DEFAULT_OPTIONS
	local ip4table ip6table
	local ifname

	json_get_vars device apn auth username password pincode
	json_get_vars ip4table
	json_get_vars mtu $PROTO_DEFAULT_OPTIONS

	logger "proto_mobileconnect_setup" $1 $2 $3 $ctl_device

	[ -n "$ctl_device" ] && device=$ctl_device

	#[ -n "$device" ] || {
	#	logger "No control device specified"
	#	proto_notify_error "$interface" NO_DEVICE
	#	proto_set_available "$interface" 0
	#	return 1
	#}
	#logger "$device"

	#获取ifname

	#判断模组型号
	#1. 是否是移远模组
	local has_quectel_model=`lsusb|grep "2c7c">/dev/null;echo $?`
	local has_fibcom_model=`lsusb|grep "2cb7">/dev/null;echo $?`
	local module_type=""

	#判断 有无设备插入时生成USB pid文件

	if [ $has_quectel_model -eq 0 ];then
		#判断模组型号
		module_type=`lsusb |grep 2c7c|awk '{print $6}'|awk -F":" '{print $2}'`
		if [ $module_type = "0900" ];then
			module_type="RM500U"
			ifname="usb0"
			logger "device is RM500U ifname is usb0" 
		elif [ $module_type = "0800" ];then
			module_type="RM500Q"
			ifname="wwan0"
			logger "device is RM500Q ifname is wwan0_1" 
			#switch card 
			echo -e "AT+QUIMSLOT=2" > /dev/ttyUSB2
			at_trans show_imei
			at_trans show_rssi
			#get imei and rssi
		elif [ $module_type = "0801" ];then
                        module_type="RM520N"
                        ifname="wwan0"
                        logger "device is RM520N ifname is wwan0_1"
                        #switch card 
                        echo -e "AT+QUIMSLOT=2" > /dev/ttyUSB2
                        at_trans show_imei
                        at_trans show_rssi
                        #get imei and rssi

        elif [ $module_type = "0125" ];then
            module_type="EC20"
            ifname="wwan0"
            logger "device is EC20 ifname is wwan0"
        elif [ $module_type = "6026" ];then
            module_type="EC200T"
            ifname="wwan0"
            logger "device is EC200T ifname is wwan0"
		fi
		#判断是否是广和通模组
		#lsusb|grep 2c7c
	elif [ $has_fibcom_model -eq 0 ];then
		#判断模组型号
		module_type=`lsusb |grep 2cb7|awk '{print $6}'|awk -F":" '{print $2}'`
		if [ $module_type = "0a05" ];then
			module_type="FM650"
			ifname="usb0"
			logger "device is FM650 ifname is usb0" 
		fi
	else
		module_type=""
	fi

	if [ $module_type == "RM500U" ];then
		logger "device is RM500U"
	elif [ $module_type = "RM500Q" ];then
		logger "device is RM500Q"
	elif [ $module_type = "RM520N" ];then
		logger "device is RM520N"
	elif [ $module_type = "FM650" ];then
		logger "device is FM650"
    elif [ $module_type = "EC20" ];then
        logger "device is EC20"
    elif [ $module_type = "EC200T" ];then
        logger "device is EC200T"
	else
		logger "No control device specified"
		proto_notify_error "$interface" NO_DEVICE
		proto_set_available "$interface" 0
		return 1
	fi


	[ -n "$mtu" ] && {
		logger "Setting MTU to $mtu"
		#/sbin/ip link set dev $ifname.1 mtu $mtu
		/sbin/ip link set dev ${ifname} mtu $mtu
	}

	#查询 是否USB设备是否存在
	logger "Waiting for SIM initialization"

	logger "Waiting for network registration"

	logger "Starting network $interface"

	if [ $module_type = "RM500Q" or $module_type = "RM520N" ];then
		logger "Setting up $ifname"_1
		proto_init_update "${ifname}"_1 1
	else
		logger "Setting up $ifname"
		proto_init_update "${ifname}" 1
	fi
	proto_set_keep 1
	proto_add_data
	proto_close_data
	proto_send_update "$interface"

	local zone="$(fw3 -q network "$interface" 2>/dev/null)"

	#get apn user password
	apn=""
	user=""
	psswd=""
	pincode=""

	#apn = `uci get network.@${1}.apn`

	local has_apn=0
	local has_pin=0
	local has_user=0
	local has_psswd=0

	uci get network.${1}.apn > /dev/null 2>&1

	if [ $? -eq 0 ];then
		has_apn=1
		apn=`uci get network.${1}.apn`
	fi

	uci get network.${1}.pincode > /dev/null 2>&1
	if [ $? -eq 0 ];then
		has_pin=1
		
		pincode=`uci get network.${1}.pincode`
	fi

	uci get network.${1}.username > /dev/null 2>&1
	if [ $? -eq 0 ];then
		has_user=1
		user=`uci get network.${1}.username`
	fi

	uci get network.${1}.password > /dev/null 2>&1
	if [ $? -eq 0 ];then
		has_psswd=1
		psswd=`uci get network.${1}.password`
	fi

	logger "#####apn:$apn pincode:$pincode user:$user psswd:$psswd"

	if [ $has_quectel_model -eq 0 ];then
		if [ $has_apn -eq 1 ];then
			if [ $has_pin -eq 1 ];then
				quectel-CM -i $ifname -s $apn $user $psswd -p $pincode
			else
				quectel-CM -i $ifname -s $apn $user $psswd
			fi
		else
			if [ $has_pin -eq 1 ];then
				quectel-CM -i $ifname -p $pincode
			else
				quectel-CM -i $ifname
			fi
		fi
	elif [ $has_fibcom_model -eq 0 ];then
		logger "start fibcom lte_dial"
		ifconfig $ifname 0.0.0.0
		#lte_dial

		if [ $has_apn -eq 1 ];then
			if [ $has_pin -eq 1 ];then
				cmd="lte_dial  -s $apn $user $psswd -p $pincode"
				logger "cmd: $cmd"
				lte_dial -s $apn $user $psswd -p $pincode
			else
				cmd="lte_dial -s $apn $user $psswd"
				logger "cmd: $cmd"

				lte_dial  -s $apn $user $psswd
			fi
		else
			if [ $has_pin -eq 1 ];then
				cmd="lte_dial -p $pincode"
				logger "cmd: $cmd"

				lte_dial -p $pincode
			else
				lte_dial
			fi
		fi
		logger "fibcom lte_dial abnormal stop"
	fi
}

mobileconnect_wds_stop() {
	killall quectel-CM
	killall lte_dial
	#判断 mobile_app 参数是否打开
	#uci set mobile_app.@mobile_app[-1].enable_mobile_dial='0'
	#uci commit
}

proto_mobileconnect_teardown() {
	local interface="$1"

	logger "Stopping network $interface"

	json_load "$(ubus call network.interface.$interface status)"
	json_select data

	mobileconnect_wds_stop

	proto_init_update "*" 0
	proto_send_update "$interface"
}

[ -n "$INCLUDE_ONLY" ] || {
	add_protocol mobileconnect
}
