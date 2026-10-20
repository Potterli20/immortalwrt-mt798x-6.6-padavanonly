#
# Copyright (C) 2006-2015 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#
include $(TOPDIR)/rules.mk

PKG_NAME:=autoconf
PKG_VERSION:=2.73

PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION).tar.xz
PKG_SOURCE_URL:=@GNU/autoconf
PKG_HASH:=9fd672b1c8425fac2fa67fa0477b990987268b90ff36d5f016dae57be0d6b52e

HOST_BUILD_PARALLEL:=1

include $(INCLUDE_DIR)/host-build.mk

HOST_CONFIGURE_VARS += \
	EMACS="no" \
	PERL="/usr/bin/env perl"

define Host/Compile
	-$(call Host/Compile/Default,--touch install-man1) # disable building of man pages
	$(call Host/Compile/Default)
endef

define Host/Uninstall
	-$(call Host/Compile/Default,uninstall)
endef

$(eval $(call HostBuild))
