# SPDX-License-Identifier: GPL-2.0-or-later
#
# Reference skeleton for introducing a private kernel driver on the 25.12
# (6.12) branch. See driver-reuse-assessment.md section 4.2 for the eight
# steps this package demonstrates. Opt-in by default: never inflate the
# 16 MB NOR images.

include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/kernel.mk

PKG_NAME:=mtk-driver-template
PKG_RELEASE:=1
PKG_FILE_DEPENDS:=$(CURDIR)/src/

include $(INCLUDE_DIR)/package.mk

# Any symbol that changes the compiled module must be hashed into the
# configured stamp; otherwise editing Config.in does not trigger a rebuild.
PKG_CONFIG_DEPENDS:=CONFIG_MTK_DRIVER_TEMPLATE_PROBE

EXTRA_KCONFIG:= \
	CONFIG_MTK_DRIVER_TEMPLATE_PROBE=$(CONFIG_MTK_DRIVER_TEMPLATE_PROBE)

define KernelPackage/mtk-driver-template
  CATEGORY:=MTK Properties
  SUBMENU:=Drivers
  TITLE:=MediaTek private driver introduction template
  FILES:=$(PKG_BUILD_DIR)/mtk-driver-template.ko
  KCONFIG:=
  DEFAULT:=n
  DEPENDS:=@TARGET_mediatek
  AUTOLOAD:=$(call AutoLoad,90,mtk-driver-template)
endef

define KernelPackage/mtk-driver-template/description
  Minimal, opt-in reference module for the private-driver introduction
  checklist. It exposes a single read-only debugfs-free module parameter and
  proves that KernelPackage metadata, hidden kconfig gates, EXTRA_KCONFIG
  injection, external-module builds and apk packaging all work together.
endef

define KernelPackage/mtk-driver-template/config
	source "$(SOURCE)/Config.in"
endef

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	$(CP) ./src/. $(PKG_BUILD_DIR)/
endef

define Build/Compile
	$(MAKE) -C "$(LINUX_DIR)" \
		$(KERNEL_MAKE_FLAGS) \
		M="$(PKG_BUILD_DIR)" \
		$(EXTRA_KCONFIG) \
		modules
endef

$(eval $(call KernelPackage,mtk-driver-template))
