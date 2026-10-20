# 
# Copyright (C) 2015 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#
include $(TOPDIR)/rules.mk

PKG_NAME:=patchelf
PKG_VERSION:=0.19.2

PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION).tar.bz2
PKG_SOURCE_URL:=https://github.com/NixOS/patchelf/releases/download/$(PKG_VERSION)
PKG_HASH:=d4ad9a4e5c689e09119ce2f30a94b0e8b4f98c78590123ea21665e34c6928801

HOST_BUILD_PARALLEL:=1
HOST_FIXUP:=autoreconf

include $(INCLUDE_DIR)/host-build.mk

define Host/Install
	$(INSTALL_DIR) $(STAGING_DIR_HOST)/bin
	$(INSTALL_BIN) $(HOST_BUILD_DIR)/src/patchelf $(STAGING_DIR_HOST)/bin/patchelf
endef

define Host/Clean
	rm -rf $(STAGING_DIR_HOST)/bin/patchelf
endef

$(eval $(call HostBuild))
