#
# morse feed 的 luci-app-morselinkeval 依赖 luci-lib-chartjs-4，luci 24.10 自带的
# luci-lib-chartjs 只有 Chart.js 2，包名对不上，这里在本树里补一个 4.x。
# bundle 直接随仓库放（同上游 luci-lib-chartjs 打包 nlbw.chart.min.js 的做法），
# 免得每次编译都要去外网拉，也躲开 feed 里那个不存在的依赖名。
#

include $(TOPDIR)/rules.mk

LUCI_TITLE:=Chart.js 4 javascript library
LUCI_DEPENDS:=+luci-base
LUCI_PKGARCH:=all
# bundle 已经是 minify 过的，再过一遍 jsmin 只会引入风险
LUCI_MINIFY_JS:=0
LUCI_MINIFY_CSS:=0
LUCI_MINIFY_LUA:=0

PKG_NAME:=luci-lib-chartjs-4
PKG_VERSION:=4.5.0
PKG_RELEASE:=1

PKG_LICENSE:=MIT
PKG_LICENSE_FILES:=LICENSE.md

include $(TOPDIR)/feeds/luci/luci.mk

# call BuildPackage - OpenWrt buildroot signature
