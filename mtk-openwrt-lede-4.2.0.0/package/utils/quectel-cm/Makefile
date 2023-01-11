#
# hua.shao@mediatek.com
#
# MTK Property Software.
#
#define Package/quectel
#DEPENDS += libpthread
#endef

include $(TOPDIR)/rules.mk

PKG_NAME:=quectel
PKG_RELEASE:=1

include $(INCLUDE_DIR)/package.mk

PKG_BUILD_DIR:=$(BUILD_DIR)/quectel

define Package/quectel
    SECTION:=utils
    CATEGORY:=Utilities
	TITLE:= Quectel CM
endef

define Package/quectel/description
	HLK Custom Application
endef

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	cp -rfd ./src/* $(PKG_BUILD_DIR)/
endef


define Package/quectel/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/quectel-CM $(1)/usr/bin/
	$(CP) ./files/* $(1)/
	$(INSTALL_DIR) $(1)/etc/config/
	$(INSTALL_CONF) ./files/hlk-5g.config $(1)/etc/config/hlk-5g
	$(INSTALL_CONF) ./files/mobile_app.config $(1)/etc/config/mobile_app
	$(INSTALL_CONF) ./files/mobile_probe $(1)/etc/
endef

$(eval $(call BuildPackage,quectel,+librt))
