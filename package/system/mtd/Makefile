#
# Copyright (C) 2006-2012 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/kernel.mk

PKG_NAME:=mtd
PKG_RELEASE:=26

PKG_BUILD_DIR := $(KERNEL_BUILD_DIR)/$(PKG_NAME)
STAMP_PREPARED := $(STAMP_PREPARED)_$(call confvar,CONFIG_MTD_REDBOOT_PARTS)

PKG_LICENSE:=GPL-2.0+
PKG_LICENSE_FILES:=

PKG_FLAGS:=nonshared
PKG_BUILD_FLAGS:=lto

include $(INCLUDE_DIR)/package.mk

define Package/mtd
  SECTION:=utils
  CATEGORY:=Base system
  DEPENDS:=+libubox
  TITLE:=Update utility for trx firmware images
endef

define Package/mtd/description
 This package contains an utility useful to upgrade from other firmware or 
 older OpenWrt releases.
endef

target=$(firstword $(subst -, ,$(BOARD)))

MAKE_FLAGS += TARGET="$(target)"
TARGET_CFLAGS += -Dtarget_$(target)=1 -Wall
# src/md5.h declares old-style empty prototypes (void MD5_Update ();). C23
# (GCC 15 default -std=gnu23) treats "()" as "no parameters", so the calls in
# md5.c fail to compile. Pin C17 to keep the legacy K&R semantics.
TARGET_CFLAGS += -std=gnu17

ifdef CONFIG_MTD_REDBOOT_PARTS
  MAKE_FLAGS += FIS_SUPPORT=1
  TARGET_CFLAGS += -DFIS_SUPPORT=1
endif

define Package/mtd/install
	$(INSTALL_DIR) $(1)/sbin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/mtd $(1)/sbin/
endef

$(eval $(call BuildPackage,mtd))
