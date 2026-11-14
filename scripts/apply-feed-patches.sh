#!/bin/sh
# 把本仓库自带的 feed 修复放回 feed 检出目录。
# feeds/ 不被 git 跟踪，跑过 ./scripts/feeds update -a 之后需要再跑一次本脚本。

cd "$(dirname "$0")/.." || exit 1

# feeds install 按目录 mtime 缓存 tmp/info/.packageinfo-* 分片；sed 改 Makefile
# 不会更新目录 mtime，DEFAULT 之类的元数据会一直沿用旧分片。改动下面这些
# feed 包时删掉对应分片，让下一次 install/defconfig 重新扫描（重复删无害）。
rm -f tmp/info/.packageinfo-feeds_mtk_openwrt_feed_crypto-eip

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

# openssh 10.6 在 OpenSSL 3.0 下还在用一批被标记 deprecated 的旧 API
# （umac.c 的 HMAC_*、ssh-pkcs11/ecdsa/rsa 的 EC_KEY/RSA_* 等），编译期刷
# "Since OpenSSL 3.0 [-Wdeprecated-declarations]"。代码是上游的，等上游改用
# EVP 接口；这里通过 configure 的 cflags-after 把告警压掉。
ssh=feeds/packages/net/openssh/Makefile
if [ -f "$ssh" ] && grep -qF -- '--with-cflags-after=-fzero-call-used-regs=skip' "$ssh"; then
	sed -i 's|--with-cflags-after=-fzero-call-used-regs=skip|--with-cflags-after="-fzero-call-used-regs=skip -Wno-deprecated-declarations"|' "$ssh"
	echo "给 $ssh 追加 -Wno-deprecated-declarations"
fi

# crypto-eip 的 adapter_addrpair.h 头文件保护宏拼错（ADDDRPAIR 多一个 D），
# gcc 15 起 -Wall 含 -Wheader-guard，配合包内写死的 -Wall -Werror，
# adapter_pcl_generic.c 会直接编译失败（gcc 14 及以前不报）。
eip_hdr=feeds/mtk_openwrt_feed/feed/kernel/crypto-eip/src/ddk/inc/crypto-eip/ddk/slad/adapter_addrpair.h
if [ -f "$eip_hdr" ] && grep -qF -- '#ifndef ADAPTER_ADDDRPAIR_H_' "$eip_hdr"; then
	sed -i 's/#ifndef ADAPTER_ADDDRPAIR_H_/#ifndef ADAPTER_ADDRPAIR_H_/' "$eip_hdr"
	echo "修正 $eip_hdr 的头文件保护宏"
fi

# crypto-eip 顶层 KernelPackage 没有 FILES，只是 KCONFIG/依赖容器；真正的
# crypto-eip-inline.ko 由 CRYPTO_OFFLOAD_INLINE select 出子包，而该 choice
# 只允许 MT7988 / mt7988a-rfb / mt7988d-rfb 设备。本树 13 份种子全是 filogic
# 子目标且未选这些设备，所以 feed 的 DEFAULT:=y 会在每次 make defconfig 后
# 悄悄选中一个编不出任何 .ko 的 kmod-crypto-eip：既把 CRYPTO_HW /
# INET_ESP_OFFLOAD / XFRM_USER 等内核选项拉进 6.12 配置，又白跑一次
# crypto-eip/compile，产出的 apk 里只有 .list 没有模块。
# 按私有驱动引入标准「不默认开启」，改为纯 opt-in；要真正启用 EIP197 时
# 需先选 MT7988 设备并显式打开 CRYPTO_OFFLOAD_INLINE（见
# driver-reuse-assessment.md 4.3）。
eip_mk=feeds/mtk_openwrt_feed/feed/kernel/crypto-eip/crypto-eip.mk
if [ -f "$eip_mk" ] && grep -qF -- '  DEFAULT:=y' "$eip_mk"; then
	sed -i 's/^  DEFAULT:=y$/  DEFAULT:=n/' "$eip_mk"
	echo "把 $eip_mk 的 kmod-crypto-eip 改为 opt-in（DEFAULT:=n）"
fi

# CRYPTO_OFFLOAD_INLINE 的 depends 只写了
# TARGET_DEVICE_mediatek_filogic_DEVICE_mediatek_mt7988{a,d}-rfb 与
# TARGET_mediatek_mt7988；但本树 25.12 的 profile choice 生成的是
# TARGET_mediatek_filogic_DEVICE_mediatek_mt7988a-rfb（少一个 DEVICE_）。
# 用真实 conf 求解验证过：只留旧 depends 时即使选中 MT7988 设备，整个
# choice 仍不可达、CRYPTO_OFFLOAD_INLINE 连同 inline 子包一起被丢弃。
# 追加本树符号（保留原有 depends，不改动上游语义），让 EIP197 inline 可启用。
eip_cfg=feeds/mtk_openwrt_feed/feed/kernel/crypto-eip/Config.in
old_dep='depends on TARGET_mediatek_mt7988 || TARGET_DEVICE_mediatek_filogic_DEVICE_mediatek_mt7988a-rfb || TARGET_DEVICE_mediatek_filogic_DEVICE_mediatek_mt7988d-rfb'
new_dep="$old_dep || TARGET_mediatek_filogic_DEVICE_mediatek_mt7988a-rfb || TARGET_mediatek_filogic_DEVICE_mediatek_mt7988d-rfb"
if [ -f "$eip_cfg" ] && grep -qF -- "$old_dep" "$eip_cfg" && ! grep -qF -- "$new_dep" "$eip_cfg"; then
	# 用 # 作分隔符：depends 串本身含 |，不能用 s|old|new|。
	sed -i "s#${old_dep}#${new_dep}#" "$eip_cfg"
	echo "给 $eip_cfg 的 CRYPTO_OFFLOAD_INLINE 补上 25.12 设备符号"
fi

# feed 的 src/init.c 在 xfrm offload init/deinit 里给内核侧函数指针
# mtk_crypto_set_dec_secpath 赋值；但本树 6.12 的 mtk_hnat/hnat.h 只提供
# mtk_crypto_offloadable，没有这个 hook，内核侧也没有任何消费者（全树
# grep 0 命中）。这是 feed 比 25.12 私有 ABI 新导致的 -Werror 编译失败。
# 用 #if 0 包住这两处悬空赋值，保留原文便于回溯；不伪造一个没人调用
# 的 hook，也不改动 HNAT/XFRM 实际转发逻辑。
eip_init=feeds/mtk_openwrt_feed/feed/kernel/crypto-eip/src/init.c
if [ -f "$eip_init" ] && grep -qF -- 'mtk_crypto_set_dec_secpath = NULL;' "$eip_init" &&
   ! grep -qF -- '25.12: secpath hook not provided' "$eip_init"; then
	awk '
		/^[[:space:]]*mtk_crypto_set_dec_secpath =/ {
			print "\t/* 25.12: secpath hook not provided */"
			print "#if 0"
			print $0
			print "#endif"
			next
		}
		{ print }
	' "$eip_init" > "$eip_init.tmp" && mv "$eip_init.tmp" "$eip_init"
	echo "排除 $eip_init 里 6.12 未提供的 secpath hook"
fi

# feed 的 xfrm-offload.c 把 skb_hnat_cdrt(skb) 当左值直接赋值；本树 6.12 的
# mtk_hnat/nf_hnat_mtk.h 在 !CONFIG_MEDIATEK_NETSYS_V3 时把它定义成 (0)
# （NETSYS_V3 才在 hnat_desc 里有 cdrt 位域），直接赋值触发
# "lvalue required as left operand of assignment"。6.12 提供了统一 setter
# skb_hnat_set_cdrt(skb, cdrt)：V3 写位域，非 V3 为空操作，正是这里需要的
# 语义。相邻的 skb_hnat_magic_tag(skb) 在三种 hnat_desc 里都是真实位域，
# 保持原样。
eip_xfrm=feeds/mtk_openwrt_feed/feed/kernel/crypto-eip/src/xfrm-offload.c
if [ -f "$eip_xfrm" ] && grep -qF -- 'skb_hnat_cdrt(skb) = xfrm_params->cdrt->idx;' "$eip_xfrm"; then
	sed -i 's/skb_hnat_cdrt(skb) = xfrm_params->cdrt->idx;/skb_hnat_set_cdrt(skb, xfrm_params->cdrt->idx);/' "$eip_xfrm"
	echo "把 $eip_xfrm 的 cdrt 直写改为 6.12 skb_hnat_set_cdrt()"
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
