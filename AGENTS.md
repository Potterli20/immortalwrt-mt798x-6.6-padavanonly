# AGENTS.md — immortalwrt-mt798x-6.6-padavanonly

## 上游与参考源

- 当前分支 `openwrt-24.10-6.6`；`origin` = Potterli20/immortalwrt-mt798x-6.6-padavanonly，`upstream` = Yuzhii0718/immortalwrt-mt798x-6.6-padavanonly，本地另配了 remote `fat4` = fat4-alt/immortalwrt-mt798x-6.6。
- rebase / 补丁移植参考：<https://github.com/chasey-dev/immortalwrt-mt798x-rebase>
  - ImmortalWrt + MTK OpenWrt feeds 补丁导入。默认分支 `25.12`；对照本分支优先看 `rework-24.10`，其余有 `openwrt-25.12`、`25.12-dev-wifi7`、`rework-21.02`。
- 内核更新来源：<https://github.com/unifreq/linux-6.6.y>（分支 `main`，2026-11-07 核对为 6.6.162，commit `e35bbc24107c`，当天 05:52 UTC 推送）。
- 内核补丁来源：<https://gitlab.com/xanmod/linux>（XanMod 补丁）。
- 蒲公英（Oraybox）官方固件集合：<https://github.com/web1n/oraybox-firmwares>
  - 移植参考用：`stable/` 与 `develop/` 下按型号+硬件版本+版本号命名，`develop/` 里较新的带 `sysupgrade` 字样（看着是 OpenWrt 衍生固件）；可下载单个 bin 解包提取分区表、dtb、kernel config、MAC/uboot 环境。本树现有蒲公英支持只有 ramips 的 `oraybox,x1`（mt76x8）与 `oraybox,x3a`（mt7621），`target/linux/mediatek` 下无 oraybox profile。
  - 只作外部参考，不要把固件并入源码树：仓库约 1.2 GB、无 LFS、全是二进制、未声明 license（2026-10-21 核对，最后推送 2024-12-15）。

## 当前内核接线

- `target/linux/ramips/Makefile` 与 `target/linux/mediatek/Makefile` 都设 `KERNEL_PATCHVER:=6.6`。
- `include/kernel-6.6` pin `LINUX_VERSION-6.6 = .162` + `LINUX_KERNEL_HASH-6.6.162`，即从 kernel.org 取主线稳定树；`.config` 中 `CONFIG_KERNEL_GIT_CLONE_URI=""`。bump 小版本要同时改这两行，再跑 `make target/linux/prepare`（挪开内核构建目录更准，看 0 FAILED / 0 fuzz）和 `make target/linux/compile`，失效的补丁用 quilt 在 `target/linux/mediatek/patches-6.6`（323 条）和 `target/linux/generic/{backport,hack,pending}-6.6` 里重挂锚点。
- 2026-11-07 bump `.161` → `.162`：unifreq `main` 当天 05:52 UTC 推 `e35bbc24107c`，kernel.org 同步上架。6.6.162 只有 3 个上游提交，全在 Xen 后端——xen-blkback 读 I/O 统计的 UAF（XSA-528 / CVE-2026-100082）、xen-netback 把 `xenvif_mcast_addr_list_free()` 挪到同步点之后（XSA-527 / CVE-2026-100081）、xen-netback 释放统计前补 `synchronize_net()`（XSA-526 / CVE-2026-100080）。本树生成的 `.config` 是 `# CONFIG_XEN is not set`，三处都不进编译，这次 bump 纯属版本跟随。
- bump 的复验手法与读数（可复用）：`make target/linux/prepare V=s` exit 0、959 条补丁、`Hunk #N FAILED` 0 条；18 处 fuzz / 334 处 offset 是 .161 就有的，不是 bump 引入——基线不用改 pin，直接命令行覆盖 `make target/linux/prepare V=s LINUX_VERSION-6.6=.161 LINUX_KERNEL_HASH-6.6.161=<旧 hash>`（`include/kernel-version.mk:50` 按版本名动态取 hash，命令行变量压过文件赋值），两边逐补丁逐 hunk 文本完全一致，上面那批重导出的 MTK 补丁（`999-pcie-03`~`06`、`999-wdt-01`、`999-cpufreq-01`~`05`、`999-2766`）都是 0 fuzz / 0 offset。判失败别只 `grep -i failed`：`670-ipv6-...-failed-policy.patch` 与 `671-...-_POLICY_FAILED-...patch` 的文件名会假命中，要匹配 `Hunk #\d+ FAILED`。另注意 prepare 会 `rm -rf` 整个 `build_dir/target-*/linux-mediatek_filogic`，旧版本构建目录不保留，所以下面两条 2026-11-06 记录里的 `linux-6.6.161` 路径现在要读作 `linux-6.6.162`。tarball 直连 kernel.org 被限速到 ~200 KB/s，换 `https://mirrors.ustc.edu.cn/kernel.org/linux/kernel/v6.x/` 实测 13 MB/s，hash 仍按 kernel.org 的 `sha256sums.asc` 校验。`make target/linux/compile -j6 V=s` 同样过了：exit 0、0 error、6 条 warning（都在 netfilter / `mtk_eth_soc` / `mtk_eth_dbg` / `pci_mediatek_rbus` 这些本次未变的文件里）、`include/config/kernel.release` = `6.6.162`、`arch/arm64/boot/Image` 19.9 MB、墙钟 540 s；`drivers/block/xen-blkback` 与 `drivers/net/xen-netback` 下 0 个 `.o`，实证那三处修复不进编译。
- `target/linux/mediatek/patches-6.6` 于 2026-11-06 跟 `mediatek/mtk-openwrt-feeds`（分支 `main`，`24.10/files/target/linux/mediatek/patches-6.6` 共 31 条，MTK 对该目录最后改动 2026-01-06）对过一次账，并补进缺的：`999-pcie-04`、`999-pcie-05`、`999-wdt-01`，cpufreq 换到 MTK 2025-07-23 重构后的 `999-cpufreq-01`~`05`（本地原 `-02` 改名 `-03`，本地原 `999-pcie-04` 改名 `-06`），`999-pcie-03` 同时跟到新版（WiFi 复位块移到 PERST 之前、极性改成 assert→delay→deassert；`wifi-reset-gpios` 没写进任何 dts，所以对现有 profile 无行为变化）。MTK 原文件锚点是 6.6.102 前后，直接拷会 fuzz，全部按 6.6.161 重新导出过；复现方式是取 `dl/linux-6.6.161.tar.xz` 里的单文件，按文件名顺序逐补丁打快照再 `diff -u` 重导。仍未取：`999-2733-add-netlink-support-for-dsa`（mt7530 netlink）、`999-xHCI-01`~`03`、`999-clk-01`/`999-clk-02(usb-pipe)`（mt7988 专属）、`999-2702-crypto-avoid-rcu-stall`（`CONFIG_CRYPTO_DEV_SAFEXCEL` 未开）；`999-5000`~`5106` 那批 hnat 修复挂不上，因为本树 `files-6.6` 的 `mtk_hnat` 是另一代际（无 `foe_entry.data[]`/`NETSYS_V3` 分支）。
- 注意 `.gitignore` 第 46 行的 `*.patch` 会吞掉所有新建补丁（现有那些都是历史上 `-f` 加进去的），往 `target/linux/*/patches-*` 里放新文件必须 `git add -f`，rebase 后核对用 `git ls-files target/linux/mediatek/patches-6.6 | wc -l`（当前 323）。
- 2026-11-06 又跟 `mediatek/mtk-openwrt-feeds` 的 `autobuild/unified/` 对过一次账——那才是 MTK 的完整配方：`filogic/24.10`（mt_wifi 变体，内核补丁 141 条 + `patches-base` 38 + `patches-feeds` 5）与 `filogic/mac80211/24.10`（上游 mt76 变体，内核补丁 44 + `patches-base` 10），`autobuild.sh` 按 `platform[-wifi[-sku[-variant]]]` 解析目录后依次套 `patches-base`/`patches-feeds`/`files/`/内核补丁；本树混用了两条线（`010-01` rbus 属 mt_wifi，`999-30xx` flow-offload 属 mac80211）。已补进 an8801sb 全套：`files-6.6/drivers/net/phy/an8801.c`+`.h`、`999-2766`（Kconfig/Makefile 各加一条），配套的 `KernelPackage/phy-airoha-an8801sb`（`netdevices.mk:2045`）和 `filogic/config-6.6` 的 `# CONFIG_AIROHA_AN8801_PHY is not set` 从此有效；两个 `mt7987-netsys-eth{0,2}-an8801sb.dtso` 本树早有，只差 MTK 后加的版权注释，未动。同时把本地 `999-2709`~`2717` 整体 +1 平移到 `2710`~`2718`，与 MTK 编号一一对应，`2709` 留空位。
- 三条「看着小其实代际耦合」的坑，别再单条硬塞：`999-2709-fix-panic-issue-with-napi_enable` 有 2 个 hunk 落在本树 `mtk_open`/SER 区域（约 5741/5794 行）打不上；`999-2609/2610 pcs link_poll` 要基于 MTK 现行 `999-2601`~`2608` 那一代，且 `2610` 打上后本树自己的 `999-2788-pcs-mtk-usxgmii-avoid-link-up-reset` 就挂了；`999-2136` 的新版是在 MTK 旧版之上再叠一层（`while (cnt_period > max_cnt_period)`、`bw_mode` 阈值分支），得连 `999-2135`/`806-v6.9-pwm-mediatek-add-support-for-MT7988` 一起重做。重做方式与验证：把 `dl/linux-6.6.161.tar.xz` 全量解到临时目录，按 `backport→pending→hack→patches-6.6` 顺序用 `patch -f -p1`（跟 `scripts/patch-kernel.sh` 一致）重放，逐条对触及文件打快照再 `diff -u` 重导；单个厂商驱动可用「拷贝 `build_dir/.../linux-6.6.161/.config` + 追加 `CONFIG_xxx=m` + `olddefconfig` + `make drivers/.../foo.o`」离线验证（an8801.c 在 6.6.161 + gcc 15.2 下 0 告警编出 `.o`）。
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
