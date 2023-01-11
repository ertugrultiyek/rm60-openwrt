#
# Copyright (C) 2008 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/kernel.mk

PKG_NAME:=quectel_wwan_q
PKG_RELEASE:=1

include $(INCLUDE_DIR)/package.mk

define KernelPackage/quectel_wwan_q
  SUBMENU:=Quectel USB 5G Support
  TITLE:=Quectel USB 5G
  FILES:=$(PKG_BUILD_DIR)/qmi_wwan_q.ko
  AUTOLOAD:=$(call AutoLoad,98,qmi_wwan_q)
  DEPENDS+=kmod-usb-wdm
  DEPENDS+=kmod-usb-net
  KCONFIG:=
endef

define KernelPackage/quectel_wwan_q/description
 Kernel module for register a quectel rm500G 5g module
endef

EXTRA_KCONFIG:= \
	CONFIG_QUECTEL_WWAN_Q=m

EXTRA_CFLAGS:= \
	$(patsubst CONFIG_%, -DCONFIG_%=1, $(patsubst %=m,%,$(filter %=m,$(EXTRA_KCONFIG)))) \
	$(patsubst CONFIG_%, -DCONFIG_%=1, $(patsubst %=y,%,$(filter %=y,$(EXTRA_KCONFIG)))) \

MAKE_OPTS:= \
	ARCH="$(LINUX_KARCH)" \
	CROSS_COMPILE="$(TARGET_CROSS)" \
	SUBDIRS="$(PKG_BUILD_DIR)" \
	EXTRA_CFLAGS="$(EXTRA_CFLAGS)" \
	$(EXTRA_KCONFIG)

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	$(CP) ./src/* $(PKG_BUILD_DIR)/
endef

define Build/Compile
	$(MAKE) -C "$(LINUX_DIR)" \
		$(MAKE_OPTS) \
		modules
endef

$(eval $(call KernelPackage,quectel_wwan_q,+kmod-usb-wdm,+kmod-usb-net))
