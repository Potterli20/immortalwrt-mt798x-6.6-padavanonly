include $(TOPDIR)/rules.mk

PKG_NAME:=brutalctl
PKG_VERSION:=2.0.0
PKG_RELEASE:=1

# Shares the download cache with the kmod-brutal package (same source tarball)
PKG_SOURCE:=tcp-brutal-$(PKG_VERSION).tar.gz
PKG_SOURCE_URL:=https://codeload.github.com/apernet/tcp-brutal/tar.gz/v$(PKG_VERSION)?
PKG_HASH:=10d2906364e081ef5e09da9048363ca7c8cf83c5b21688e166a77c39302f7f1b
PKG_LICENSE:=GPL-3.0
PKG_LICENSE_FILES:=LICENSE
PKG_MAINTAINER:=Potterli20 <Potterli20@users.noreply.github.com>

# The tarball top-level dir is named after the upstream repo, not this package
PKG_BUILD_DIR:=$(BUILD_DIR)/tcp-brutal-$(PKG_VERSION)

include $(INCLUDE_DIR)/package.mk

define Package/brutalctl
	SECTION:=net
	CATEGORY:=Network
	TITLE:=CLI to manage TCP Brutal destination rules
	URL:=https://github.com/apernet/tcp-brutal
	DEPENDS:=+kmod-brutal +ip-full
endef

define Package/brutalctl/description
	brutalctl manages TCP Brutal destination rules: all TCP connections
	to a destination prefix share one rate, without application support.
	On add it also installs the route selecting the brutal congestion
	control for the prefix (proto 233), del/flush remove it.

	Examples:
	  brutalctl add 203.0.113.5/32 100
	  brutalctl list
	  brutalctl del 203.0.113.5/32
	  brutalctl flush
endef

define Build/Configure
endef

define Build/Compile
	$(TARGET_CC) $(TARGET_CFLAGS) $(TARGET_LDFLAGS) -std=c99 -Wall -Wextra -o \
		$(PKG_BUILD_DIR)/brutalctl $(PKG_BUILD_DIR)/tools/brutalctl.c
endef

define Package/brutalctl/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/brutalctl $(1)/usr/bin/brutalctl
endef

$(eval $(call BuildPackage,brutalctl))
