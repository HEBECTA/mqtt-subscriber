include $(TOPDIR)/rules.mk

PKG_NAME:=subscriber
PKG_RELEASE:=1
PKG_VERSION:=1.0.0

include $(INCLUDE_DIR)/package.mk

define Package/subscriber
	DEPENDS:=+libmosquitto-ssl +libuci
	CATEGORY:=Base system
	TITLE:=subscriber
endef

define Package/subscriber/description
	MQTT subscriber
endef

define Package/subscriber/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_DIR) $(1)/etc/config
	$(INSTALL_DIR) $(1)/etc/init.d
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/subscriber $(1)/usr/bin
	$(INSTALL_BIN) ./files/subscriber.init $(1)/etc/init.d/subscriber
	$(INSTALL_CONF) ./files/subscriber.config $(1)/etc/config/subscriber
endef

$(eval $(call BuildPackage,subscriber))