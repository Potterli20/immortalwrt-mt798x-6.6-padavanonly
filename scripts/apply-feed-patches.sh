#!/bin/sh
# 把本仓库自带的 feed 修复放回 feed 检出目录。
# feeds/ 不被 git 跟踪，跑过 ./scripts/feeds update -a 之后需要再跑一次本脚本。

cd "$(dirname "$0")/.." || exit 1

find feeds-patches -name '*.patch' -type f | while read -r f; do
	target=feeds/${f#feeds-patches/}
	dir=${target%/*}
	if [ ! -d "$dir" ]; then
		echo "跳过 $target（对应 feed 包未安装）"
	elif [ -f "$target" ]; then
		echo "已存在 $target"
	else
		cp "$f" "$target" && echo "补入 $target"
	fi
done

# 下面几处是 feed 的 Makefile 依赖了本树（25.12 + 内核 6.12）里不存在的包名，
# make 时每次都会刷 "WARNING: Makefile '...' has a dependency on '...', which does not exist"。

# natcap 还写着 21.02 时代的虚拟名 libustream-ssl，现在只有
# libustream-{openssl,wolfssl,mbedtls}，本树选的是 openssl。
natcap=feeds/x/natcap/Makefile
if [ -f "$natcap" ] && grep -qF -- '+libustream-ssl' "$natcap"; then
	sed -i 's/+libustream-ssl/+libustream-openssl/' "$natcap"
	echo "修正 $natcap 的 libustream 依赖"
fi

# flowtable 依赖的 kmod-nf-flow-netlink 只在 MTK SDK 内核里有，本树 6.12 没有对应补丁
# （filogic/config-6.12 里那行 CONFIG_NF_FLOW_TABLE_NETLINK 也是 not set）。
flowtable=feeds/mtk_openwrt_feed/feed/app/flowtable/Makefile
if [ -f "$flowtable" ] && grep -qF -- '+kmod-nf-flow-netlink' "$flowtable"; then
	sed -i 's/ +kmod-nf-flow-netlink//' "$flowtable"
	echo "去掉 $flowtable 的 kmod-nf-flow-netlink 依赖"
fi

# morselinkeval 依赖的 luci-lib-chartjs-4 由本树的 package/luci-lib-chartjs-4 提供，
# feed 里那行依赖名不用动。

# gst1-plugins-bad 把 videoparsersbad 子包写上了 codecparsers，但它真正链接的还有
# libgst1video（h264parse/h265parse 用 GstVideoDecoder），漏了会在链接期找不到符号。
gst=feeds/packages/multimedia/gst1-plugins-bad/Makefile
if [ -f "$gst" ] && grep -qF -- 'GstBuildPlugin,videoparsersbad,videoparsersbad support,codecparsers,,' "$gst"; then
	sed -i 's|videoparsersbad support,codecparsers,)|videoparsersbad support,codecparsers video,)|' "$gst"
	echo "修正 $gst 的 videoparsersbad 链接依赖"
fi

# libwebsockets 的 -DISABLE_WERROR=ON 少一个 D，cmake 里一直是 DISABLE_WERROR:BOOL=OFF
lw=feeds/packages/libs/libwebsockets/Makefile
if [ -f "$lw" ] && grep -q -- '-DISABLE_WERROR=ON' "$lw"; then
	sed -i 's/-DISABLE_WERROR=ON/-DDISABLE_WERROR=ON/' "$lw"
	echo "修正 $lw 的 DISABLE_WERROR"
fi

# 下面是 make menuconfig 的 "recursive dependency detected"，成因两类：同一个包名被两个
# feed 定义，或者 PROVIDES/CONFLICTS 与 select/depends 两条边互相指。

# p910nd-xwrt 同时写了 PROVIDES:=p910nd 和 CONFLICTS:=p910nd：PROVIDES 让 kconfig 生成
# select PACKAGE_p910nd，CONFLICTS 又生成 depends on (PACKAGE_p910nd != y)，自己成环。
# 保留 CONFLICTS（两份都装会抢 /usr/sbin/p910nd），撤掉 PROVIDES——此后 +p910nd 只解析到
# packages 那份，想用 x-wrt 变体就显式装 p910nd-xwrt。
p910=feeds/x/p910nd-xwrt/Makefile
if [ -f "$p910" ] && grep -qF -- 'PROVIDES:=p910nd' "$p910"; then
	sed -i '/^[[:space:]]*PROVIDES:=p910nd$/d' "$p910"
	echo "去掉 $p910 的 PROVIDES"
fi

# openvlm 的 +kmod-usb-audio 让它 depends on kmod-sound-core，而 +alsa-utils 经 alsa-lib
# 又把 sound-core select 回来，同一个符号两条边成环。sound kmod 是硬需求，alsa-utils
# 只是 amixer/aplay 这类命令行工具，去掉它。
openvlm=feeds/openmanet/utils/openvlm/Makefile
if [ -f "$openvlm" ] && grep -qF -- '+alsa-utils' "$openvlm"; then
	sed -i 's/ +alsa-utils//' "$openvlm"
	echo "去掉 $openvlm 的 alsa-utils 依赖"
fi

# 两个 feed 撞同名包，同一个 PACKAGE_ 符号被定义两遍，kconfig 直接报自环。
# kmod-oaf 留 packages 那份：open-app-filter 一个 Makefile 同时出 kmod-oaf 和 appfilter 守护进程，
# 217heidai 的 oaf 只出 kmod-oaf——撤掉它才不会把 luci-app-oaf / luci-app-appfilter 的
# +appfilter 依赖一起撤走（kmod 和守护进程同源，配套也一致）。
if [ -L package/feeds/packages/open-app-filter ] && [ -L package/feeds/217heidai_Package/oaf ]; then
	rm -f package/feeds/217heidai_Package/oaf
	echo "撤掉重名包 package/feeds/217heidai_Package/oaf（保留 packages 的 kmod-oaf+appfilter）"
fi

# quectel-cm 留 packages 那份：本树的 qmi_wwan/cdc_mbim 模组补丁是照主线驱动打的，
# x-wrt 那份依赖 kmod-qmi-wwan-q，luci-proto-quectel 也是对着上游包写的。
if [ -L package/feeds/packages/quectel-cm ] && [ -L package/feeds/x/quectel-qconnect-manager ]; then
	rm -f package/feeds/x/quectel-qconnect-manager
	echo "撤掉重名包 package/feeds/x/quectel-qconnect-manager（保留 packages 的 quectel-cm）"
fi
