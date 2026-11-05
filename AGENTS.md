# AGENTS.md — immortalwrt-mt798x-6.6-padavanonly

## 上游与参考源

- 当前分支 `openwrt-24.10-6.6`；`origin` = Potterli20/immortalwrt-mt798x-6.6-padavanonly，`upstream` = Yuzhii0718/immortalwrt-mt798x-6.6-padavanonly，本地另配了 remote `fat4` = fat4-alt/immortalwrt-mt798x-6.6。
- rebase / 补丁移植参考：<https://github.com/chasey-dev/immortalwrt-mt798x-rebase>
  - ImmortalWrt + MTK OpenWrt feeds 补丁导入。默认分支 `25.12`；对照本分支优先看 `rework-24.10`，其余有 `openwrt-25.12`、`25.12-dev-wifi7`、`rework-21.02`。
- 内核更新来源：<https://github.com/unifreq/linux-6.6.y>（分支 `main`，2026-10-20 核对为 6.6.159）。
- 内核补丁来源：<https://gitlab.com/xanmod/linux>（XanMod 补丁）。
- 蒲公英（Oraybox）官方固件集合：<https://github.com/web1n/oraybox-firmwares>
  - 移植参考用：`stable/` 与 `develop/` 下按型号+硬件版本+版本号命名，`develop/` 里较新的带 `sysupgrade` 字样（看着是 OpenWrt 衍生固件）；可下载单个 bin 解包提取分区表、dtb、kernel config、MAC/uboot 环境。本树现有蒲公英支持只有 ramips 的 `oraybox,x1`（mt76x8）与 `oraybox,x3a`（mt7621），`target/linux/mediatek` 下无 oraybox profile。
  - 只作外部参考，不要把固件并入源码树：仓库约 1.2 GB、无 LFS、全是二进制、未声明 license（2026-10-21 核对，最后推送 2024-12-15）。

## 当前内核接线

- `target/linux/ramips/Makefile` 与 `target/linux/mediatek/Makefile` 都设 `KERNEL_PATCHVER:=6.6`。
- `include/kernel-6.6` pin `LINUX_VERSION-6.6 = .161` + `LINUX_KERNEL_HASH-6.6.161`，即从 kernel.org 取主线稳定树；`.config` 中 `CONFIG_KERNEL_GIT_CLONE_URI=""`。bump 小版本要同时改这两行，再跑 `make target/linux/prepare`（挪开内核构建目录更准，看 0 FAILED / 0 fuzz）和 `make target/linux/compile`，失效的补丁用 quilt 在 `target/linux/mediatek/patches-6.6`（316 条）和 `target/linux/generic/{backport,hack,pending}-6.6` 里重挂锚点。
- 若要改用 unifreq 的树，走 `CONFIG_KERNEL_GIT_CLONE_URI` / `CONFIG_KERNEL_GIT_REF`（版本名生成逻辑见 `include/kernel-version.mk`），同时核对 `include/kernel-6.6` 里的 hash 与 `target/linux/generic` 补丁是否重复。

## 依赖名本地改动

- `make` 刷的 `WARNING: Makefile '...' has a dependency on '...', which does not exist` 已经清零，复验方式是重扫包索引后跑 `./scripts/package-metadata.pl mk tmp/.packageinfo`（或直接 `make prepare-tmpinfo`），rebase 之后要再确认一次。
- 树内三处：`package/utils/busybox` 去掉 `+USE_GLIBC:libcrypt-compat`（openwrt main 和 immortalwrt master 都查不到这个包，本树还是 musl）；`package/firmware/linux-firmware/mediatek.mk` 去掉 `+LINUX_6_18:kmod-phy-mediatek-2p5g`、`aeonsemi.mk` 去掉 `+LINUX_6_18:kmod-phy-aeonsemi-as21xxx`（本树只有 6.6，`LINUX_6_18` 符号和这两个驱动都不存在，将来 bump 到 6.18 再按上游恢复）。`aeonsemi.mk` 顺手把 `as21x1x_fw.bin` 的安装目录改成 `/lib/firmware/aeonsemi`（原来平铺在 `/lib/firmware`，驱动按子路径找不着，上游 master 也是错的）。
- feed 侧改动全部写进 `scripts/apply-feed-patches.sh`（幂等，`./scripts/feeds update -a` 之后重跑）：natcap `libustream-ssl` → `libustream-openssl`；mtk flowtable 去掉 `kmod-nf-flow-netlink`（只有 MTK SDK 内核有那补丁）；gst1-plugins-bad 放开被注释的 `videoparsersbad` 子包并补上真正链接的 `codecparsers video`，`gst1-mod-videoparsersbad` 这个包名随之恢复（morse 的 aws-kvs 依赖它）。
- `package/luci-lib-chartjs-4` 是本树补的 Chart.js 4.5.0，走 luci.mk、bundle 直接进仓库（同上游 luci-lib-chartjs 打包 `nlbw.chart.min.js` 的做法，所以 `LUCI_MINIFY_JS:=0`），再靠 `resources/preload/chartjs4.js` 注入全局 `Chart`——morselinkeval 的 JS 只认全局 `Chart`、自己不会 loadScript。这样 morse feed 那行 `+luci-lib-chartjs-4` 依赖不用改。`.gitignore` 里的 `/package/*` 会吞掉新包目录（fancontrol 那批当年是靠 `-f` 加进去的），已补 `!/package/luci-lib-chartjs-4/`。

## kconfig 递归依赖

- `make menuconfig` 原先报 5 条 `recursive dependency detected`，现已清零。复验命令：
  `cp .config /tmp/k.conf && KCONFIG_CONFIG=/tmp/k.conf ./scripts/config/conf --olddefconfig Config.in`
  （直接喂 `tmp/.config-package.in` 会虚报——少了 Config.in 的上下文，实测 95 条假环）。
- 树内一处偏离：`package/system/rpcd/Makefile` 的 `BuildPlugin` 把 `DEPENDS+=rpcd $(2)` 改成
  `DEPENDS+=+rpcd $(2)`。上游一直写裸 `rpcd`，生成的 kconfig 是 `depends on PACKAGE_rpcd`，
  而 luci-base → rpcd-mod-luci → rpcd 那条链是 `select`，于是任何「主题/应用依赖 luci-lua-runtime
  又依赖 morse-mode(+rpcd-mod-ucode)」的组合都成环。改成 select 后环消失，代价是装任意
  rpcd 模块会自动带上 rpcd 本身。
- 同名包撞车两处，撤掉重复的那侧（`./scripts/feeds install -a` 之后重跑
  `scripts/apply-feed-patches.sh` 会再撤一次）：`kmod-oaf` 由 `packages/open-app-filter`
  提供（同一份 Makefile 还出 `appfilter` 守护进程，217heidai 的 `oaf` 只出 ko，撤它会连带把
  `luci-app-oaf`/`luci-app-appfilter` 的 `+appfilter` 依赖撤掉，所以留 packages 那份）；
  `quectel-cm` 由 `packages` 提供（本树的 qmi_wwan/cdc_mbim 模组补丁照主线驱动打，
  x-wrt 那份依赖 `kmod-qmi-wwan-q`，luci-proto-quectel 也是对着上游包写的）。
- feed 侧另两条也在同一个脚本里：`x/p910nd-xwrt` 删 `PROVIDES:=p910nd`（跟同一行的
  `CONFLICTS:=p910nd` 正好一条 select 一条 depends 成环，保留 CONFLICTS，`+p910nd` 从此只解析到
  packages 那份）；`openmanet/utils/openvlm` 删 `+alsa-utils`（`+kmod-usb-audio` 已让它
  depends on kmod-sound-core，alsa-utils 经 alsa-lib 又把 sound-core select 回来）。

## 备注

- 构建机是 gcc 15（默认 C23）+ cmake 4，相关修复已进仓库（见 `git log` 里 `package:` / `tools:` / `feeds-patches:` 系列提交），后续 rebase 时保留这些改动。
