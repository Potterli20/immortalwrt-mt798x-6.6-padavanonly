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

# libwebsockets 的 -DISABLE_WERROR=ON 少一个 D，cmake 里一直是 DISABLE_WERROR:BOOL=OFF
lw=feeds/packages/libs/libwebsockets/Makefile
if [ -f "$lw" ] && grep -q -- '-DISABLE_WERROR=ON' "$lw"; then
	sed -i 's/-DISABLE_WERROR=ON/-DDISABLE_WERROR=ON/' "$lw"
	echo "修正 $lw 的 DISABLE_WERROR"
fi

# 下面几处是 feed 的 Makefile 依赖了本树（24.10 + 内核 6.6）里不存在的包名，
# make 时每次都会刷 "WARNING: Makefile '...' has a dependency on '...', which does not exist"。

# natcap 还写着 21.02 时代的虚拟名 libustream-ssl，现在只有
# libustream-{openssl,wolfssl,mbedtls}，本树选的是 openssl。
natcap=feeds/x/natcap/Makefile
if [ -f "$natcap" ] && grep -qF -- '+libustream-ssl' "$natcap"; then
	sed -i 's/+libustream-ssl/+libustream-openssl/' "$natcap"
	echo "修正 $natcap 的 libustream 依赖"
fi

# flowtable 依赖的 kmod-nf-flow-netlink 只在 MTK SDK 内核里有，本树 6.6 没有对应补丁
# （filogic/config-6.6 里那行 CONFIG_NF_FLOW_TABLE_NETLINK 也是 not set）。
flowtable=feeds/mtk_openwrt_feed/feed/app/flowtable/Makefile
if [ -f "$flowtable" ] && grep -qF -- '+kmod-nf-flow-netlink' "$flowtable"; then
	sed -i 's/ +kmod-nf-flow-netlink//' "$flowtable"
	echo "去掉 $flowtable 的 kmod-nf-flow-netlink 依赖"
fi

# morselinkeval 依赖的 luci-lib-chartjs-4 由本树的 package/luci-lib-chartjs-4 提供，
# feed 里那行依赖名不用动。

# gst1-plugins-bad 把 videoparsersbad 子包注释掉了，包名 gst1-mod-videoparsersbad 跟着消失
# （morse 的 aws-kvs 依赖它），h264parse/h265parse 这些也压根没编出来。
# 放开注释，并补上它真正链接的 libgst1codecparsers / libgst1video。
gst=feeds/packages/multimedia/gst1-plugins-bad/Makefile
if [ -f "$gst" ] && grep -qF -- '#$(eval $(call GstBuildPlugin,videoparsersbad' "$gst"; then
	sed -i 's|^#$(eval $(call GstBuildPlugin,videoparsersbad,videoparsersbad support,,,))|$(eval $(call GstBuildPlugin,videoparsersbad,videoparsersbad support,codecparsers video,,))|' "$gst"
	echo "放开 $gst 的 gst1-mod-videoparsersbad 子包"
fi
