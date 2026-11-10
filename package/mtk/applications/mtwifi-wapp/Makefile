#
# This is free software, licensed under the GNU General Public License v2.
#
# Split out of package/mtk/applications/mtwifi-cfg on the 25.12 base, where the
# Lua mtwifi-cfg was replaced by mtwifi-cfg-ucode (which CONFLICTS:=mtwifi-cfg)
# and the wapp half of that package was dropped on the floor.  Keeping it as its
# own package means it no longer has to ride along with the Lua config stack.
#
# wapp / wappctrl / bs20 are prebuilt MTK aarch64 binaries (no upstream source
# in this tree), so no PKG_LICENSE is claimed here.

include $(TOPDIR)/rules.mk

PKG_NAME:=mtwifi-wapp
PKG_VERSION:=1
PKG_RELEASE:=1

include $(INCLUDE_DIR)/package.mk

define Package/mtwifi-wapp/Default
  SECTION:=MTK Properties
  CATEGORY:=MTK Properties
  SUBMENU:=Applications
endef

define Package/mtwifi-wapp
  $(call Package/mtwifi-wapp/Default)
  TITLE:=MTK MAP/1905 agent stack for mt798x (wapp, wappctrl, bs20)
  DEPENDS:=+libkvcutil +mtwifi-wapp-openssl11
endef

define Package/mtwifi-wapp/description
  1905.1 / MAP (Multi-AP) agent stack used by the MTK Wi-Fi drivers: bs20,
  wapp and wappctrl, plus the setbssid / steeringsta helpers and the /etc/map
  configuration.  Nothing starts on its own - /sbin/startwapp.sh reads
  wireless.<dev>.wapp for every mtwifi device and exits 0 unless one of them
  is set to 1, so installing this package changes nothing until that switch is
  turned on.  The hotplug hook fires on iface events and re-runs startwapp.sh.
endef

define Package/mtwifi-wapp-openssl11
  $(call Package/mtwifi-wapp/Default)
  TITLE:=OpenSSL 1.1 runtime libraries for wapp
endef

define Package/mtwifi-wapp-openssl11/description
  wapp and wappctrl are prebuilt against libssl.so.1.1 / libcrypto.so.1.1 and
  carry a DT_NEEDED for exactly those SONAMEs, and no source is available to
  rebuild them against OpenSSL 3.  This tree builds OpenSSL 3.5.9, whose
  libssl.so.3 / libcrypto.so.3 have different SONAMEs, so they cannot satisfy
  those binaries.

  This is end-of-life cryptography and it is deliberately quarantined in its
  own package: it only reaches an image when mtwifi-wapp is selected, so the
  exposure is visible in the package list instead of being smuggled in as a
  side effect of the Wi-Fi configuration package.
endef

define Build/Compile
endef

define Package/mtwifi-wapp/install
	$(INSTALL_DIR) $(1)/sbin $(1)/etc $(1)/etc/map $(1)/etc/hotplug.d/iface
	$(INSTALL_BIN) \
		./files/wapp \
		./files/wappctrl \
		./files/bs20 \
		./files/setbssid \
		./files/steeringsta \
		./files/startwapp.sh \
		$(1)/sbin/
	$(INSTALL_BIN) ./files/100-startwapp $(1)/etc/hotplug.d/iface/
	$(INSTALL_DATA) ./files/mapd_cfg $(1)/etc/map/
	$(INSTALL_DATA) ./files/1905d.cfg $(1)/etc/map/
	$(INSTALL_DATA) ./files/wapp_ap.conf $(1)/etc/
	$(INSTALL_DATA) ./files/wapp_ap_ra0.conf $(1)/etc/
endef

define Package/mtwifi-wapp-openssl11/install
	$(INSTALL_DIR) $(1)/usr/lib
	$(INSTALL_BIN) ./files/libs/libcrypto.so.1.1 $(1)/usr/lib/
	$(INSTALL_BIN) ./files/libs/libssl.so.1.1 $(1)/usr/lib/
endef

$(eval $(call BuildPackage,mtwifi-wapp))
$(eval $(call BuildPackage,mtwifi-wapp-openssl11))
