#!/bin/bash

OPENWRT_BIN=lede-ramips-mt7621-mt7621-rfb-ax-nor-squashfs-sysupgrade.bin

cd ../
git_ver=`git log|awk '{if (NR==1){print $2}}'`

#GD11 RM60
product=$1
method=$2

rm -rf package/base-files/files/etc/config/
if [ $product = "GD11" ];then
	#更新响应配置文件
	mkdir package/base-files/files/etc/config/
	cp -f product/GD11/dts/* target/linux/ramips/dts/
	cp -f product/GD11/config/default_config .config
	cp -f product/GD11/config/network package/base-files/files/etc/config/
	cp -f product/GD11/config/config-4.4 target/linux/ramips/mt7621/config-4.4
	cp -f product/GD11/hlk_version package/base-files/files/etc/
	cp -f product/GD11/root-ramips/etc/init.d/boot package/base-files/files/etc/init.d/
	cp -f product/GD11/root-ramips/etc/rc.local package/base-files/files/etc/rc.local
	cp -f product/GD11/root-ramips/etc/config/* package/base-files/files/etc/config/
elif [ $product = "RM60" ];then
	cp -f product/RM60/dts/* target/linux/ramips/dts/
	cp -f product/RM60/config/default_config .config
	cp -f product/RM60/config/config-4.4 target/linux/ramips/mt7621/config-4.4
	cp -f product/RM60/hlk_version package/base-files/files/etc/
	cp -f product/RM60/root-ramips/etc/init.d/boot package/base-files/files/etc/init.d/
	cp -f product/RM60/root-ramips/etc/config/* package/base-files/files/etc/config/
else
	echo "Unknown Product"
	exit -1
fi
VER=`cat package/base-files/files/etc/hlk_version`

if [ $method = "all" ];then
	make V=99
	if [ $? -eq 0 ];then
		cd ./bin/targets/ramips/mt7621/
		rm -f RM60*
		cp $OPENWRT_BIN ${VER}_`date +"%Y%m%d%H%M%S"`_${git_ver}.bin
	fi
elif [ $method = "menuconfig" ];then
	make menuconfig
    if [ $? -eq 0 ];then
        if [ $product = "GD11" ];then
            #更新响应配置文件
            cp -f .config product/GD11/config/default_config
        elif [ $product = "RM60" ];then
			cp -f .config product/RM60/config/default_config
        fi
    fi

elif [ $method = "kernel_menuconfig" ];then
	make kernel_menuconfig
	if [ $? -eq 0 ];then
		if [ $product = "GD11" ];then
    		#更新响应配置文件
		    cp -f target/linux/ramips/mt7621/config-4.4 product/GD11/config/config-4.4
		elif [ $product = "RM60" ];then
		    cp -f target/linux/ramips/mt7621/config-4.4 product/RM60/config/config-4.4
		fi
	fi
fi

