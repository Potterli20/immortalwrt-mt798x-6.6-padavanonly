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
- `make defconfig` 可能仍报 6 条 feed 递归依赖（`dnsdist`、`dnsdist-full`、`knot-resolver`、`snort3`、`openappid`、`luci-app-weechat`，由 `luajit2` 提供虚拟 `luajit`）：上游 feed 问题，不影响配置生成与构建。
- `.gitignore` 会忽略 `/.config`、`/.config.old`；构建日志请放 `.buildlogs/`（本仓库约定，不入库）。历史上 `/*.patch` 的忽略规则只作用于根目录，`target/linux/**/patches-*` 下新增补丁正常入库。
- `bin/targets/mediatek/filogic/packages/` 可能残留 24.10 时代的 `*_6.6.*.ipk`（旧构建未清 `bin/`），不影响 apk 产物；要干净目录需清空 `bin/` 重打包。
- `tools/` 里 `elfutils` 已升到 0.197，配套 `gnulib` 用 2026-07-04 快照（`libdwfl_stacktrace` 子目录与 0.197 必需），旧版 gnulib 补丁（640~901 等）已删除。
