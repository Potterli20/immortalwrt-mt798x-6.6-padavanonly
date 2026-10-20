include $(TOPDIR)/rules.mk

PKG_NAME:=popt
PKG_VERSION:=1.19

PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION).tar.gz
PKG_SOURCE_URL:=https://sources.buildroot.net/popt/ \
		http://rpm5.org/files/popt
PKG_HASH:=c25a4838fc8e4c1c8aacb8bd620edb3084a3d63bf8987fdad3ca2758c63240f9

HOST_FIXUP:=autoreconf

include $(INCLUDE_DIR)/host-build.mk

HOST_CONFIGURE_ARGS += \
	--disable-nls \
	--disable-rpath \
	--enable-shared=no \
	--without-libiconv-prefix \
	--without-libintl-prefix

$(eval $(call HostBuild))
