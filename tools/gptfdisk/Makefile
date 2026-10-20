include $(TOPDIR)/rules.mk

PKG_NAME:=gptfdisk
PKG_VERSION:=1.0.10

PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION).tar.gz
PKG_SOURCE_URL:=@SF/gptfdisk/$(PKG_VERSION)
PKG_HASH:=2abed61bc6d2b9ec498973c0440b8b804b7a72d7144069b5a9209b2ad693a282

HOST_BUILD_PARALLEL := 1

include $(INCLUDE_DIR)/host-build.mk

HOST_CPPFLAGS += \
	-I$(STAGING_DIR_HOST)/include/e2fsprogs

define Host/Compile
	$(call Host/Compile/Default,sgdisk)
endef

define Host/Install
        $(INSTALL_BIN) $(HOST_BUILD_DIR)/sgdisk $(STAGING_DIR_HOST)/bin/
endef

define Host/Clean
        rm -f $(STAGING_DIR_HOST)/bin/sgdisk
endef

$(eval $(call HostBuild))
