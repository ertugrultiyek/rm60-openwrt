
PKG_CHECK_FORMAT_SECURITY:=0

include $(TOPDIR)/rules.mk
 
PKG_NAME:=hser2net
PKG_RELEASE:=1
PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)

include $(INCLUDE_DIR)/package.mk

define Package/hser2net
	SECTION:=utils
	CATEGORY:=Utilities
  	DEPENDS:= +uci +libuci
	TITLE:=hser2net -- prints a snarky message
endef

TARGET_CFLAGS += -I$(STAGING_DIR)/usr/include
TARGET_LDFLAGS += -L$(STAGING_DIR)/usr/lib


 
define Package/hser2net/description
    It's my first package demo.
endef
 
define Build/Prepare
	echo "Here is Package/Prepare"
	mkdir -p $(PKG_BUILD_DIR)
	$(CP) ./ser2net/* $(PKG_BUILD_DIR)/
endef
define Package/hser2net/install
	echo "Here is Package/install"
	$(INSTALL_DIR) $(1)/etc/config
	$(INSTALL_CONF) ./files/s2ncfg.conf $(1)/etc/config/s2ncfg
	$(INSTALL_DIR) $(1)/etc/init.d
	$(INSTALL_BIN) ./files/s2ncfg.init $(1)/etc/init.d/s2ncfg
	$(INSTALL_DIR) $(1)/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/hser2net $(1)/bin/
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/readme $(1)/bin/
	$(INSTALL_BIN) ./files/hlk_firewall $(1)/etc/
endef
    
$(eval $(call BuildPackage,hser2net,+libuci))
