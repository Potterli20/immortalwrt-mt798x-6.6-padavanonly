include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/kernel.mk

PKG_NAME:=tcp-brutal
PKG_VERSION:=2.0.0
PKG_RELEASE:=2

PKG_SOURCE:=tcp-brutal-$(PKG_VERSION).tar.gz
PKG_SOURCE_URL:=https://codeload.github.com/apernet/tcp-brutal/tar.gz/v$(PKG_VERSION)?
PKG_HASH:=10d2906364e081ef5e09da9048363ca7c8cf83c5b21688e166a77c39302f7f1b
PKG_LICENSE:=GPL-3.0
PKG_LICENSE_FILES:=LICENSE
PKG_MAINTAINER:=Potterli20 <Potterli20@users.noreply.github.com>

include $(INCLUDE_DIR)/package.mk
include $(INCLUDE_DIR)/kernel-defaults.mk

define KernelPackage/brutal
	SECTION:=kernel
	SUBMENU:=Network Support
	TITLE:=TCP Brutal Kernel module
	URL:=https://github.com/apernet/tcp-brutal
	DEPENDS:=@(LINUX_5_10||LINUX_5_15||LINUX_6_1||LINUX_6_6||LINUX_6_12)
	FILES:=$(PKG_BUILD_DIR)/brutal.$(LINUX_KMOD_SUFFIX)
	AUTOLOAD:=$(call AutoLoad,30,brutal)
endef

define KernelPackage/brutal/description
	Hysteria's congestion control algorithm ported to kernel mode TCP.

	Since v2 no application support is needed: add a rate rule for a
	destination with brutalctl and every TCP connection to it uses
	Brutal. The v1 TCP_BRUTAL_PARAMS sockopt API (used by e.g.
	Hysteria2) is still accepted. Requires Linux 5.10 or later.
endef

define Build/Configure
endef

define Build/Compile
	$(KERNEL_MAKE) M="$(PKG_BUILD_DIR)" modules
endef

$(eval $(call KernelPackage,brutal))
