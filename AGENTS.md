# AGENTS.md — immortalwrt-mt798x-6.6-padavanonly（25.12 基座）

## 基座与分支

- 当前分支 `openwrt-25.12-6.12`，基于 <https://github.com/chasey-dev/immortalwrt-mt798x-rebase> 的 `25.12` 分支（ImmortalWrt 25.12 + MTK OpenWrt feeds）。
- 旧树保留在分支 `openwrt-24.10-6.6`，工作树 `/home/potter-lgy/wt-2410`，仅作对照与回退，不要删除或直接在上面开发。
- 内核 6.12.112（`include/kernel-6.12` / `target/linux/mediatek/patches-6.12`），U-Boot 2025.10（`package/boot/uboot-mediatek`）。
- 本仓库所有自有资产（自定义包、profile/dts、补丁、种子、feed 补丁、板级适配）均已迁到 25.12；目录名里的 `6.6` 是历史名称，不代表内核版本。

## 无线栈（mt_wifi 变体）

- 基座是 mt_wifi 闭源驱动变体：`package/mtk/` 全套（`mt_wifi` 7.6.7.3、`mtwifi-cfg` / `mtwifi-cfg-ucode`、`warp`、`conninfra`、`l1parser`、`luci-app-turboacc-mtk`、`hnat-detect`、`mtk-smp`），整树没有 `package/kernel/mt76`。
- 25.12 的配置栈从 lua `mtwifi-cfg` 切到 `mtwifi-cfg-ucode` + `iwinfo-ucode`：
  - `mtwifi-cfg-ucode` 有 `CONFLICTS:=mtwifi-cfg`；
  - `iwinfo-ucode` 有 `DEPENDS:=+ucode-mod-iwinfo @!PACKAGE_iwinfo`。
- 因此种子 defconfig 必须选 `CONFIG_PACKAGE_mtwifi-cfg-ucode=y` + `CONFIG_PACKAGE_iwinfo-ucode=y` + `CONFIG_PACKAGE_luci-app-mtwifi-cfg=y`，**不能**再选 24.10 的 `CONFIG_PACKAGE_mtwifi-cfg=y`，否则 luci app 的依赖 `depends on !PACKAGE_iwinfo` 不成立，`luci-app-mtwifi-cfg` 会被静默丢弃。

## 目录/资产布局

- `defconfig/`：13 份种子（mt7975/7981/7986 各档 + nowifi + dae 变体）。种子是 `./scripts/config/conf --savedefconfig` 的精简结果，设备选择写成 `CONFIG_TARGET_DEVICE_mediatek_filogic_DEVICE_<dev>=y`，`make defconfig` 会补全默认值。
- `target/linux/mediatek/dts/`：69 个 24.10 独有 dts/dtsi 已迁入，include 改为 25.12 的 `mt7981b.dtsi`，`&{/chosen}` 写法，基座 dts 补 `spi_nand:` / `partitions:` / `ubi:` 标签；叶子 dts 必须逐个过 `cpp` + `dtc`。
- `target/linux/mediatek/image/filogic.mk` / `filogic-ext.mk`：254 个 `define Device/`，含全部自有设备。
- `package/`：`brutal`、`fancontrol`、`luci-app-fancontrol`、`luci-lib-chartjs-4`，以及 `package/mtk/applications/` 下 8 个自有应用（5g-modem、Airpi-gpio-fan、luci-app-Airpifanctrl、luci-app-mwan3helper-chinaroute、luci-app-openfi、luci-app-wrtbwmon、ndisc、wrtbwmon）。
- `package/boot/uboot-mediatek/patches/`：507~510 为 imou_lc-hx3001、tplink_wma301、zbtlink_zbt-z8103ax-c、ruijie_ew-6000gx-pro 四个自建 U-Boot 目标；503 是 ruijie rg-x60-new-ubi。
- `feeds.conf.default` + `scripts/apply-feed-patches.sh` + `feeds-patches/`：feed 补丁脚本是幂等的，`./scripts/feeds update -a` / `install -a` 之后要重跑。

## 内核与补丁

- 内核补丁目录是 `patches-6.12`。当前只有两条自有迁移补丁：`999-eth-57-mtk_eth_soc-add-mdio-reset-delay.patch`、`9999-12-mediatek-usb-net-cellular-modem-quirks.patch`；其余 24.10 时代补丁未迁（25.12 的 MTK feeds 已覆盖大部分）。
- `target/linux/mediatek/patches-6.12` 的补丁不要照抄 24.10 版本，锚点会偏移，按 6.12 重新导出。
- `package/system/rpcd/Makefile` 的 `BuildPlugin` 仍保留 `DEPENDS+=+rpcd $(2)`（上游写裸 `rpcd`，会引入 kconfig 递归依赖，改 select 后消失）。

## U-Boot 2025.10 适配（自建目标必看）

2025.10 相对 2024.10 的板级 dts 有三处硬差异：

1. pinctrl 标签由 `pinctrl:` 改名 `pio:`——板级 dts 里引用要写 `&pio`，不能写 `&pinctrl` / `&gpio`。
2. pin 子节点（`spi_flash_pins`、`uart1_pins`、`pwm_pins` 等）不再由 SoC dtsi 提供，必须在板 dts 的 `&pio { }` 块里自行定义（参考 `mt7981-rfb.dts`）。
3. `CONFIG_BOARD_INIT` 默认 y，`board_r.c` 无条件引用板级 `board_init()`；自建 defconfig 必须显式写 `# CONFIG_BOARD_INIT is not set`，否则链接报 `undefined reference to board_init`。

补丁写法：defconfig 段是在 `+CONFIG_BOARD_LATE_INIT=y` 之前插入 `+# CONFIG_BOARD_INIT is not set`，改完要用脚本重算 hunk 头 `@@ -0,0 +1,N @@`（507~509 为 108 行，510 为 131 行）。

U-Boot 制品名：`<device>-bl31-uboot.fip`、`<device>-preloader.bin`；验证用 `strings *.fip | grep "U-Boot 2025.10"`。

## 板级 base-files 适配

- `target/linux/mediatek/filogic/base-files/etc/board.d/{01_leds,02_network,03_gpio_switches}`：把 24.10 的 `case` 条目按"未覆盖才插入"的规则合到 25.12 同名 `case` 的 `*)` 默认分支之前；合并后必须 `sh -n` 且用脚本复核旧树板名全部被覆盖。
- `target/linux/mediatek/filogic/base-files/lib/upgrade/platform.sh`：按旧树分组补板名——
  - fit 组（`fit_do_upgrade`）：`mediatek,7981r128`、`ruijie,ew-6000gx-pro-ubootmod`、`ruijie,rg-x60-new-ubootmod`、`tplink,wma301-ubootmod`、`zbtlink,zbt-z8103ax-c-ubootmod`、`zbtlink,zbt-z8103ax-ubi`；
  - emmc 组（`emmc_do_upgrade`）：`bt,r320`、`bt,rb300`、`cmcc,rax3000m-emmc`、`huasifei,wh3000-pro`、`philips,hy3000`、`sn,r1`、`supergateway,s20*`、`umi,uax3000e`（`platform_copy_config` 同步补）；
  - `honor,fur-602*`、`newland,nl-wr9103` 不单列，落默认分支即可——`nand.sh` 默认 `CI_UBIPART=ubi` / `CI_KERNPART=kernel` / `CI_ROOTPART=rootfs`，与旧树显式写法等价；
  - `imou,lc-hx3001` 沿用 25.12 上游（`fit_check_image` + 默认 `nand_do_upgrade`，`nand_upgrade_fit` 能解 .itb）。
- 改完 `platform.sh` 只跑 `sh -n` 不够，最好再 `make package/base-files/compile V=s` 确认打包无异常。

## 种子与构建

```sh
# 用某份种子生成 .config
cp defconfig/mt7981-ax3000.config .config && make defconfig

# 全量构建（日志写到仓库内，避免 /tmp 被清）
make -j6 world V=s > .buildlogs/full.log 2>&1; echo "BUILD_EXIT=$?" >> .buildlogs/full.log
```

- `make defconfig` 硬编码 `--defconfig=.config`，种子直接当 `.config` 输入即可。
- 设备清单核对看 `tmp/.targetinfo`，格式是 `Target-Profile: DEVICE_<name>` / `Target-Profile-Packages:`，不是 `CONFIG_TARGET_DEVICE_...=y`。
- 只编单个包必须从顶层 `make package/xxx/compile V=s`，`make -C package/xxx` 无效（只打印 `Entering directory`）。
- 只编单个设备镜像：`make target/linux/image V=s` 前先把该设备选中，或直接用种子。

25.12 实测基线（2026-11-08，`mt7981-ax3000.config`，96 设备）：`make -j6 world` 约 55 分钟、`BUILD_EXIT=0`；产物含 `kmod-mt_wifi-6.12.112.7.6.7.3-r1.apk`、`kmod-warp`、`kmod-conninfra`，rootfs 内 `mt_wifi.ko` / `conninfra.ko` / `mtk_warp*.ko` 齐全，`iwinfo-ucode` / `mtwifi-cfg-ucode` / `luci-app-mtwifi-cfg` 均已安装。

## 已知噪声与坑

- ~~构建日志里 `install-bin-trusted-firmware-a-*` 的 `Error 1 (ignored)`~~：已修复（`include/package-bin.mk` 的 `-rmdir` 改 `rmdir ... || true`）。另两处噪声也一并修了：`mt_wifi` 的 `Module.symvers: No such file`（`include/kernel.mk` 的 `collect_module_symvers` 加缺文件守卫 + `mt_wifi` 包设 `PKG_EXTMOD_SUBDIRS:=mt_wifi_ap`）、`./tools/printinitialenv: Exec format error`（`uboot-tools` 新增 `015-envtools-skip-initial-env.patch`，envtools 不再依赖交叉编译的 `u-boot-initial-env`）。
- 日志里多数 `error` 匹配其实是 `-Winline` 编译告警或 DTC 的 `Failed prerequisite` 警告，判失败要匹配 `make.*\*\*\*` / `Error [12]` / `undefined reference`。
- `mt_wifi` 的 984 条闭源代码风格告警已在 `package/mtk/drivers/mt_wifi/Makefile` 用 `MT_WIFI_KCFLAGS` 统一压掉（含 fortify 的 20 处柔性尾数组 `-Wattribute-warning`）；`patches-7673/042-use-kernel-MIN-macro.patch` 顺带删掉 3 处重复的 `MIN` 定义（`rrm*.c` 那份写成了取最大值，已改回内核 `MIN`）。完整重编后 `warning:` 计数为 0。
- `warp` 同属闭源驱动，45 条风格告警（`missing-prototypes` / `unused-label` / `unused-function` / `old-style-declaration` / `discarded-qualifiers`）在 `package/mtk/drivers/warp/Makefile` 用 `WARP_KCFLAGS` 同样压掉，`mtk_warp.ko` 重编后 `warning:` 计数 0。
- ~~`libtool: warning: '<x>.la' has not been installed in '/usr/lib'`~~：已修复。根因是 OpenWrt 用 prefix `/usr` 构建、却装进 `staging_dir`，libtool 安装模式的这三条提示（另两条是 `relinking '<x>'` 和 `remember to run 'libtool --finish /usr/lib'`）在暂存安装模型下永远无法照做。`tools/libtool/patches/210-quiet-staging-install-advisories.patch` 只删 `func_warning` 调用，`finalize=false` 与后续 `--finish` 执行全部保留。全量构建 530 条 → 0；判据是对比 `staging_dir/host/share/libtool/build-aux/ltmain.sh` 里 `has not been installed in` 的命中数（打了补丁的树为 0，未打的 `~/mt798x-6.16` 为 1），并重编 `libnl` 端到端复量。
- ~~`"terminfo.tmp", line NNNN: limiting value of 'colors'/'pairs' from 0x... to 0x7fff`~~：已修复，10 条 → 0。ncurses 6.4 的 `terminfo.src` 给 `xterm+direct{,2}`、`xterm+indirect` 写了 `colors#0x1000000`、给 256 色条目写了 `pairs#0x10000`，而 terminfo 数值字段是 16 位，`tic` 编译时一律钳到 `0x7fff`。`package/libs/ncurses/patches/901-terminfo-clamp-colors-pairs.patch` 把钳位后的值直接写进源文件，与未打补丁那棵树编译出的 **2851 个 terminfo 文件逐字节相同**。注意 hunk 行号必须以**已应用 `100-urxvt`(+166 行)、`101-kbs`(+2 行)、`900-terminfo` 之后**的 `terminfo.src` 为基准，否则最后一个 hunk 会偏移 169 行。
- ~~openssh 在 OpenSSL 3.0 下的 `[-Wdeprecated-declarations]`~~：已修复，197 条 → 0。全部来自 `openssh-without-pam` 这一个 variant（`ssh-rsa.c`/`ssh-ecdsa.c`/`ssh-pkcs11.c`/`sshkey.c`/`umac.c` 等 20 个文件、55 个已弃用 API，`EC_KEY_*`/`RSA_*`/`DH_*`/`AES_encrypt`），上游未迁 EVP。`scripts/apply-feed-patches.sh` 给 `feeds/packages/net/openssh/Makefile` 的 `--with-cflags-after=` 追加 `-Wno-deprecated-declarations`。**重编前必须 `make package/feeds/packages/openssh/clean`**：`include/package.mk` 的 `STAMP_CONFIGURED` 只哈希 `PKG_CONFIG_DEPENDS`，而 openssh 没有声明它，改 `CONFIGURE_ARGS` 不会让 `.configured_*` 失效。
- ~~`chown: changing ownership of '.../ipkg-install/var/run/rdnssd': Operation not permitted` 连带 `Makefile:609: install-data-hook Error 1 (ignored)`~~：已修复。`ndisc6` 的 `rdnssd/Makefile.am` 无条件 `chown -- nobody $(DESTDIR)$(myrundir)`；非 root 构建 + DESTDIR 暂存必然 EPERM，而暂存根里的属主本无意义（镜像打包阶段由包元数据决定，且本仓库的 ndisc6 只装 `/usr/sbin/rdnssd`、不发货该目录）。`package/mtk/applications/ndisc/patches/100-rdnssd-staged-install-no-chown.patch` 把 chown 限定为「非 DESTDIR 且以 root 安装」，保留上游原生 `make install` 行为；该包无 `PKG_FIXUP`，故同时打 `Makefile.am` 和 tarball 里已生成的 `Makefile.in`。
- ~~`wavlink_wl-wn586x3b` 每次构建报 4 条 `cp: cannot stat '...-initramfs-kernel.bin'` + `Makefile:48 Error 1 (ignored)`~~：已修复。它是全树唯一 `KERNEL_INITRAMFS_SUFFIX := .itb` 却在 `IMAGES` 里列 `initramfs-kernel.bin` 的设备——它的 initramfs 产物是 `-initramfs.itb`（本来就已正确产出并进 `bin/targets/`），`-squashfs-initramfs-kernel.bin` 这条链根本不会被生成。已从 `target/linux/mediatek/image/filogic.mk` 的 `IMAGES` 去掉该条目；兄弟设备 `wl-wn536ax6-a` / `wl-wn551x3` 都用默认 `IMAGES`。
- **cudy RE3000 v1 / cudy WR3000 v1 / yuncore AX835 / wavlink WL-WN573HX3 已从 `defconfig/mt7981-ax3000.config` 去掉**（种子 96 → 92 设备）。这四个是 16 MB NOR 机型，`IMAGE_SIZE` 与 dts 分区一致（`0xf10000`=15424k、`0xe00000`=14336k，均已核对，不是配置写错），而 mt_wifi 变体的 `mt_wifi.ko` 未 strip 就有 12,109,112 字节、加约 4.7 MB 内核后镜像达 23,592,964 字节，`check-size` 必然失败、镜像不产出并连带 `sha256sum: No such file`。这是本变体的物理上限，不是可修的缺陷；要覆盖这些机型得用不含 mt_wifi 的精简配置。
- `linux-atm` 的告警已按逐条改源码方式**基本清零**：包内串行 `make -j1 .../compile V=s` 从 343 条 `warning:` 降到 **1** 条。手段全部是改代码，没有任何 `-Wno-*`：`Makefile` 把内核 uapi 头的 `-I` 改 `-isystem`（151 条 `_IOC` 系列 `redefined` → 0，已用真交叉编译器验证头文件解析序列与预处理 token 流逐字节相同、目标文件同尺寸）；`patches/702~709` 修 6 处真缺陷、12 处 `%d` 配 64 位、11 处 K&R 转 ANSI、`hediag.c` 加 `const`（一处声明消 83 条）、45 处 `AsnOcts` 字面量加 `(char *)`、删 71 个 snacc 样板死局部、4 个 flex 词法器加 `%option nounput noinput`、`atmd.h` 四个链表宏重排（证明 348 token 一致）、34 条 `pointer-sign` 全部改声明/原型、autotools 现代化（`configure.in`→`configure.ac` 等，20 条 → 0）、`src/extra/Makefile.am` 的双目标模式规则拆成 3 条显式规则（3 条 peer-target 警告 → 0，固件产物逐字节相同）。**只剩 1 条**：`atmf_uni.c:245` 的 `tmp` set-but-not-used——它是 `Q_REMOVE(r,i)` 用来承接表头赋值的左值，删不掉；而该赋值结果被丢弃是否算真缺陷需要另行判断，不在无测试的 ILMI 路径上盲改。
- 仍保留的构建噪声（均已定位到根因，非压制）：openssh 的 `Makefile:430: check-config Error 126 (ignored)`（上游 `check-config` 会在宿主机执行刚交叉编译出的 `sshd -t`，必然 Exec format error）；`autoreconf`/`aclocal` 偶发的 `#include_next is a GCC extension`（来自 `staging_dir/host/include/byteswap.h`，属 binutils 自身构建）；openssh 的 `chacha_private.h:53/54` 2 条 initializer-string 截断 NUL 提示；`Makefile:769: ignoring prerequisites on suffix rule definition` 1 条。
- 判"构建是否真的干净"不能只看 `BUILD_EXIT=0`：本轮曾因列号换算漏了制表符按 8 列展开、且把行尾引号误判成字面量起点，把 `(char *) ` 插进了 `"ILMI"` 内部变成 `"ILMI(char *) "`，而编译照样成功。凡涉及字节级改动，改完要逐条比对字符串字面量集合是否变化。
- `make defconfig` 可能仍报 6 条 feed 递归依赖（`dnsdist`、`dnsdist-full`、`knot-resolver`、`snort3`、`openappid`、`luci-app-weechat`，由 `luajit2` 提供虚拟 `luajit`）：上游 feed 问题，不影响配置生成与构建。
- `.gitignore` 会忽略 `/.config`、`/.config.old`；构建日志请放 `.buildlogs/`（本仓库约定，不入库）。历史上 `/*.patch` 的忽略规则只作用于根目录，`target/linux/**/patches-*` 下新增补丁正常入库。
- `bin/targets/mediatek/filogic/packages/` 可能残留 24.10 时代的 `*_6.6.*.ipk`（旧构建未清 `bin/`），不影响 apk 产物；要干净目录需清空 `bin/` 重打包。
- `package/libs/openssl` 现同步到上游 **3.5.9-r2**（KTLS 开关、`-Wl,--as-needed`、`Build/Configure` 里 `CC="gcc"` 探测交叉编译器）。版本事实：3.5.9 既是 OpenSSL 项目 **3.5 LTS 分支的最新补丁版**，也是 **openwrt/openwrt master 自己在用的版本**；更新的 3.6.5 / 4.0.3 / 4.1.0 全部非 LTS 且上游未采用，真要跳得先重排本包 9 个补丁与 `openssl-module.mk` 并验证所有链接 libssl 的消费者——所以"更新 openssl"在这棵树上的正确含义是同步到上游 `-r2`，不是跳大版本。
  - 有意**没有**跟上游的两处：上游把 `OPENSSL_OPTIMIZE_SPEED` 的默认从 `y` 收窄成 `y if x86_64||i386||riscv64`，而种子不钉这个符号，跟着改会让所有从种子新建的配置在 aarch64 上静默关掉速度优化（改的是加密性能权衡，不该顺手带）；上游的 `$(ATOMIC64_DEPENDS)` 需要 `HAS_ATOMIC64` 这一 per-arch 特性，本 25.12 基座完全没定义，故该行保留按架构列表的旧写法。
- `AC_PROG_LEX` 的参数别写 `yywrap`：那会让 autoconf 去链接一个只**声明**不定义 `yywrap` 的 conftest（`programs.m4:45`），交叉构建下必然失败并走到 `programs.m4:109-110` 的 `required lex library not found; giving up on flex`，把 `LEX` 置成 `:`；后果是 `ylwrap` 不再重新生成词法器、编译的是 tarball 里那个没有 `yywrap()` 定义的旧 `.c`，最后 `qgen` 链接失败——一个参数错误伪装成了链接错误。linux-atm 的正确写法是 `AC_PROG_LEX(noyywrap)`（每个词法器自带 `yywrap()`、`-lfl` 已被 200-no_libfl 摘净）。
- `tools/` 里 `elfutils` 已升到 0.197，配套 `gnulib` 用 2026-07-04 快照（`libdwfl_stacktrace` 子目录与 0.197 必需），旧版 gnulib 补丁（640~901 等）已删除。
