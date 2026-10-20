#
# Copyright (C) 2006-2015 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#
include $(TOPDIR)/rules.mk

PKG_NAME:=automake
PKG_CPE_ID:=cpe:/a:gnu:automake
PKG_VERSION:=1.19
PKG_API_VERSION:=$(word 2,$(subst ., ,$(PKG_VERSION)))

PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION).tar.gz
PKG_SOURCE_URL:=@GNU/automake
PKG_HASH:=79e8b1f7a967e87ce8a9ded76bee7f793d0ce1886ab2002feb1b0510f578b75a

HOST_BUILD_PARALLEL:=1

include $(INCLUDE_DIR)/host-build.mk

HOST_CONFIGURE_ARGS += \
	--disable-silent-rules

HOST_CONFIGURE_VARS += \
	PERL="/usr/bin/env perl"

define Host/Configure
	(cd $(HOST_BUILD_DIR); mkdir -p .git; $(AM_TOOL_PATHS) STAGING_DIR_HOST="" ./bootstrap)
	$(call Host/Configure/Default)
endef

define Host/Install
	# remove old automake resources to avoid version conflicts
	$(call Host/Uninstall)
	$(call Host/Compile/Default,install)
	mv $(STAGING_DIR_HOST)/bin/aclocal $(STAGING_DIR_HOST)/bin/aclocal.real
	$(INSTALL_BIN) ./files/aclocal $(STAGING_DIR_HOST)/bin
	( \
		api=$(PKG_API_VERSION); \
		while [ "$$$$api" -ge 11 ]; do \
			ln -sf aclocal "$(STAGING_DIR_HOST)/bin/aclocal-1.$$$$api"; \
			api=$$$$(($$$$api - 1)); \
		done; \
	)
endef

define Host/Uninstall
	-$(call Host/Compile/Default,uninstall)
	rm -rf $(STAGING_DIR_HOST)/share/aclocal-[0-9]*
	rm -rf $(STAGING_DIR_HOST)/share/automake-[0-9]*
endef

$(eval $(call HostBuild))
