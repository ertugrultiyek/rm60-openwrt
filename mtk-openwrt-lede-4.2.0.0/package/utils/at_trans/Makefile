#
# hua.shao@mediatek.com
#
# MTK Property Software.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=at_trans
PKG_RELEASE:=1

include $(INCLUDE_DIR)/package.mk

PKG_BUILD_DIR:=$(BUILD_DIR)/at_trans

define Package/at_trans
	SECTION:= HLK Custom
	CATEGORY:=Utilities
	TITLE:= HiLink Ser2Net
	DEPENDS:=+libpcre
endef

define Package/at_trans/description
	HLK Custom Application
endef

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	cp -rfd ./src/* $(PKG_BUILD_DIR)/
endef


define Package/at_trans/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_DIR) $(1)/etc
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/at_trans $(1)/usr/bin/
endef

$(eval $(call BuildPackage,at_trans))
