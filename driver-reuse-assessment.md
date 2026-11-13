# openwrt-25.12-6.12 私有驱动代码资源梳理、评估与复用方案

> 生成日期：2026-11-13　基线分支：`openwrt-25.12-6.12`（HEAD `22ab70e78e`）
> 内核 `6.12.112`、U-Boot `2025.10`、配置栈 `mtwifi-cfg-ucode + iwinfo-ucode`
> 评估范围：`/home/potter-lgy/immortalwrt-mt798x-6.6-padavanonly`、`/home/potter-lgy/wt-medyma`（`dev/medyma-npu` @ `e285fae271`）、本机 7 棵同源树、项目记忆与 git 历史
> 方法：全树只读盘点 + 构建产物/日志复核 + git 历史与官方规则核对。除本报告外未改动任何代码。

---

## 0. 结论摘要（先看这里）

**不存在可以从 MedyMa 6.6 分支直接搬到 `openwrt-25.12-6.12` 的私有驱动代码。** 三条硬证据：

1. `git merge-base openwrt-25.12-6.12 dev/medyma-npu` 返回 **rc=1，无共同祖先**；两树 root 分别为 `1bcf20d189` 与 `de7e78bcf0`，默认 diff 为 **14,419 个文件**（`--no-renames` 15,380 / `renameLimit=100000` 13,249）。
2. 目标分支是 `6.12.112 + 上游 mt_wifi 7.6.7.3`；来源分支是 `6.6.159 + 私有 mt_hwifi/mtk_npu/mtk_hnat`。内核模块按 `LINUX_VERSION`+`LINUX_VERMAGIC` 绑定（[include/kernel.mk:220](file:///home/potter-lgy/immortalwrt-mt798x-6.6-padavanonly/include/kernel.mk#L220)），6.6 的 `.ko` 无法在 6.12 加载。
3. 本轮在 MedyMa 树上修的 4 个文件（`mtk_npu/Config.in`、`mt_wifi_cmn/patches/00{1,2}`、`scripts/tests/mtk-npu-sync-selftest.py`）在 25.12 上**全部不存在**；`MTK_NPU_*`、`hnat_tunnel_register`、`mt_wifi_cmn` 等符号在 25.12 的 git 跟踪树里命中 **0**。

但「无直接复用」≠「无候选资源」。25.12 工作树里存在 MediaTek 官方 feed 的 `pce / mtk_npu / crypto-eip` 三包副本（非 git 跟踪、13 份种子均未选），其中 `crypto-eip` 已在 6.12.112 上完成过一次编译——但其顶层包是无 `FILES` 的 KCONFIG 容器，且 `DEFAULT:=y` 会在所有 mediatek 目标上静默选中它（后续已修）。因此本报告按用户要求第 4 条，落在「分类 B」：**给出符合 25.12 规范的私有驱动引入标准与标准化开发指导**，同时对唯一有构建实证的 `crypto-eip` 给出条件化集成流程。

**不建议执行 `git merge dev/medyma-npu`**：会把 1.4 万文件的 6.6 私有树灌进主线。`dev/medyma-npu` 已作为独立分支推到 `origin`（`e285fae271`），与主线各司其职。

---

## 1. 现有私有驱动资产全景（分类 A：已在本分支、生产验证成熟）

### 1.1 四块核心闭源驱动（git 跟踪、有 6.12 产物、入选种子）

| 包 | PKG_VERSION | 源码/哈希 | 6.12.112 产物 | 种子选中 |
|---|---|---|---|---|
| `kmod-mt_wifi` | 7.6.7.3 | `mt79xx_20250408-705eb4.tar.xz`（`b029d7b4…`） | `kmod-mt_wifi-6.12.112.7.6.7.3-r1.apk`（3,571,039 B） | 11/13 |
| `kmod-warp` | 20250408 | `warp_20250408-5f71ec.tar.xz`（`0954055e…`） | `kmod-warp-6.12.112.20250408-r1.apk`（149,435 B） | 8/13 |
| `kmod-conninfra` | 20231229 | `mt79xx_conninfra_20231229-f2fa25.tar.xz`（`672929de…`） | `kmod-conninfra-6.12.112.20231229-r1.apk`（95,447 B） | 6/13 |
| `kmod-mediatek_hnat` | 内核内建 | [files-6.12/.../mtk_hnat/Makefile](file:///home/potter-lgy/immortalwrt-mt798x-6.6-padavanonly/target/linux/mediatek/files-6.12/drivers/net/ethernet/mediatek/mtk_hnat/Makefile) 产出 `mtkhnat.ko` | `kmod-mediatek_hnat-6.12.112-r1.apk`（39,071 B） | 6/13 |

- 定义位置：[package/mtk/drivers/](file:///home/potter-lgy/immortalwrt-mt798x-6.6-padavanonly/package/mtk/drivers/) 4 个顶层包；`kmod-mediatek_hnat` 实为 [package/kernel/linux/modules/netdevices.mk:2363](file:///home/potter-lgy/immortalwrt-mt798x-6.6-padavanonly/package/kernel/linux/modules/netdevices.mk#L2363) 的内核内建包装，不是外部包。
- `CONFIG_NET_MEDIATEK_HNAT` 由 [999-eth-91-...patch](file:///home/potter-lgy/immortalwrt-mt798x-6.6-padavanonly/target/linux/mediatek/patches-6.12/999-eth-91-mtk_eth_soc-add-mtkhnat-driver-support.patch) 添加，[filogic/config-6.12:338](file:///home/potter-lgy/immortalwrt-mt798x-6.6-padavanonly/target/linux/mediatek/filogic/config-6.12#L338) 默认未设；项目记忆要求必须 `=m` 不可 `=y`（否则模块文件缺失）。
- **生产验证时长**：2026-11-08 基线全量构建 `BUILD_EXIT=0`（[.buildlogs/full_ax3000.log](file:///home/potter-lgy/immortalwrt-mt798x-6.6-padavanonly/.buildlogs/full_ax3000.log)，93,929 行，约 55 分钟，96 设备），四个 apk 现存于 bin/，最近一次重编 2026-11-13 09:06。到 2026-11-13 已有约 5 天构建实证；AGENTS.md:88 记录 rootfs 内 `mt_wifi.ko / conninfra.ko / mtk_warp*.ko` 齐全。**注意：这是构建验证，不是实机长稳运行记录**——本机无任何 uptime/流量压测证据，报告不虚报「生产环境运行验证时长」。
- 稳定性记录：AGENTS.md:94-95 记 `mt_wifi` 984 条 / `warp` 45 条闭源风格告警已用 `MT_WIFI_KCFLAGS` / `WARP_KCFLAGS` 归零；`Module.symvers: No such file` 已由 `collect_module_symvers` 守卫 + `PKG_EXTMOD_SUBDIRS:=mt_wifi_ap` 修复。

### 1.2 支撑资产

- 内核补丁 **549** 个（`target/linux/mediatek/patches-6.12/`），分组 `999-dts` 109 / `999-eth` 58 / `999-ppe` 45 / `999-hnat` 11 / `999-wed` 21 等；本仓库自有迁移补丁仅 2 个（`999-eth-57…`、`9999-12…`）。`files-6.12/` 45 文件（11 个 mtk_hnat、7 个 dts/dtso、14 个 xHCI…）。
- `package/mtk/` git 跟踪 **1,677** 文件，27 个顶层包；另有 `brutal/tcp-brutal.ko`、`Airpi-gpio-fan.ko`、`5g-modem`（7 个 ko）、`mtwifi-wapp`（5 个预编译 ELF + 隔离的 `libssl.so.1.1`）。
- 13 份种子、[filogic.mk](file:///home/potter-lgy/immortalwrt-mt798x-6.6-padavanonly/target/linux/mediatek/image/filogic.mk) 254 个 `define Device/`、69 个 dts、U-Boot 补丁 507~510。

### 1.3 分类 A 的复用结论

这四块**不是待引入资源，而是本分支的既有基线**，无需任何集成动作。任何新私有驱动都必须以它们为兼容基准（同一工具链、同一 `mtwifi-cfg-ucode` 栈、同一 `KernelPackage` 元数据），且不得破坏已验证的 `BUILD_EXIT=0`。

---

## 2. 候选资源筛选与逐项可行性分析

### 2.1 候选池（按「业务场景匹配度 / 验证时长 / 稳定性记录」筛）

| # | 候选 | 来源 | 场景匹配 | 构建实证 | 稳定性记录 | 结论 |
|---|---|---|---|---|---|---|
| C1 | `mt_wifi / warp / conninfra / mediatek_hnat` | 本分支 git 跟踪 | 高（13 种子主力） | 6.12.112 `BUILD_EXIT=0` + 4 apk | 5 天基线、告警清零 | **已是基线，无需引入** |
| C2 | `crypto-eip`（feed `r1`） | `feeds/mtk_openwrt_feed/feed/kernel/crypto-eip`，443 文件 | **低**（见 2.2） | 6.12 编过顶层容器，inline 子包从未启用 | feed 补丁修过 gcc15 头保护宏与 `DEFAULT:=y` | **条件化候选，需先修门控** |
| C3 | `pce` / `mtk_npu`（feed `r1`） | 同 feed，17 / 148 文件 | 低（种子 0 选中） | 无 6.12 产物（仅残留 `*_6.6.161*.ipk`） | 无 | **不引入** |
| C4 | MedyMa 9 个私有包 | `wt-medyma` | 不兼容 | 6.6.159 `BUILD_EXIT=0`（full5/full6） | 本轮 3 提交 + 4 自检 | **禁止跨树搬运** |
| C5 | MedyMa 21 个自有内核补丁「思路」 | `patches-6.6/` | 部分通用 | 6.6 专属锚点 | 无 6.12 复核 | **只作线索，按 6.12 重导出** |

### 2.2 C2 `crypto-eip`：唯一需要专项分析的反直觉案例

**已核实的事实链**（这一节是本报告最有价值的发现）：

- `feeds.conf.default:13` 挂 `src-git mtk_openwrt_feed https://github.com/mediatek/mtk-openwrt-feeds`；三包在 `feeds/` 下真实存在，但 `git ls-files feeds/mtk_openwrt_feed/` = **0**（feed 安装产物，非仓库资产，重新 `feeds update/install` 会被覆盖）。
- 当前 `.config:7539` 有 `CONFIG_PACKAGE_kmod-crypto-eip=y`（13 份种子**均未显式选**），基线日志 `.buildlogs/full_ax3000.log:18246` 有 `package/feeds/mtk_openwrt_feed/crypto-eip/compile#14.98#1.73#18.81`。
- 产物 `kmod-crypto-eip-6.12.112-r1.apk` 仅 **778 B**，`apk adbdump` 显示只有 `lib/apk/packages/kmod-crypto-eip.list`、没有 `.ko`。
- **（2026-11-13 后续复核，修正初稿判断）这不是 `KernelPackage` 的「静默降级空包」**：[crypto-eip.mk:38-63](file:///home/potter-lgy/immortalwrt-mt798x-6.6-padavanonly/feeds/mtk_openwrt_feed/feed/kernel/crypto-eip/crypto-eip.mk#L38) 的顶层 `KernelPackage/crypto-eip` **本来就没有 `FILES`**，它只是 `KCONFIG` + 依赖容器；真正的 `crypto-eip-inline.ko` 由 [crypto-eip.mk:76-92](file:///home/potter-lgy/immortalwrt-mt798x-6.6-padavanonly/feeds/mtk_openwrt_feed/feed/kernel/crypto-eip/crypto-eip.mk#L76) 的 `kmod-crypto-eip-inline` 子包持有 `FILES`。顶层 778 B 包是**正常形态**。
- 真正的问题是 **`DEFAULT:=y`**（crypto-eip.mk:42）：`package-metadata.pl:309/320` 会为它生成 `default y if DEFAULT_kmod-crypto-eip` 与无条件 `default y`，于是 13 份种子虽 0 份显式选择，`make defconfig` 仍在所有 mediatek 目标上静默选中该包，把 `CRYPTO_HW / CRYPTO_AUTHENC / INET_ESP_OFFLOAD / INET6_ESP_OFFLOAD / XFRM_USER` 拉进内核 `.config`，并白跑一次 `crypto-eip/compile#29.34#4.10#48.98`，而 inline 子包永远不会被 select。
- inline 未启用的原因：[crypto-eip.mk:14-30](file:///home/potter-lgy/immortalwrt-mt798x-6.6-padavanonly/feeds/mtk_openwrt_feed/feed/kernel/crypto-eip/crypto-eip.mk#L14) 只在 `CONFIG_CRYPTO_OFFLOAD_INLINE=y` 时注入 `CONFIG_MTK_CRYPTO_EIP_INLINE=m`；而 [Config.in:10-12](file:///home/potter-lgy/immortalwrt-mt798x-6.6-padavanonly/feeds/mtk_openwrt_feed/feed/kernel/crypto-eip/Config.in#L10) 的 `CRYPTO_OFFLOAD_INLINE` 只 `depends on TARGET_mediatek_mt7988 || …mt7988a-rfb || …mt7988d-rfb`。本分支 13 份种子全是 `CONFIG_TARGET_SUBTARGET="filogic"` 且未选这些设备（设备符号名在 `.config:514` 与 depends 完全一致，只是未选中），所以该 choice 不可达。
- 已在 `scripts/apply-feed-patches.sh` 幂等把 `DEFAULT:=y` 改为 `n`，并清掉 `tmp/info/.packageinfo-feeds_mtk_openwrt_feed_crypto-eip` 分片缓存（`feeds install` 按目录 mtime 缓存，sed 不改目录 mtime 会导致修改不生效）。干净种子复核：`Default: n` → `.config` 为 `# CONFIG_PACKAGE_kmod-crypto-eip is not set`。
- 兼容性补丁已存在：[scripts/apply-feed-patches.sh:66-73](file:///home/potter-lgy/immortalwrt-mt798x-6.6-padavanonly/scripts/apply-feed-patches.sh#L66) 修 `adapter_addrpair.h` 的 `ADAPTER_ADDDRPAIR_H_` 拼写（gcc 15 的 `-Wheader-guard` + 包内 `-Wall -Werror` 会编译失败，gcc 14 不报）；当前文件已是 `ADAPTER_ADDRPAIR_H_`，说明补丁已生效。
- 残留 `kmod-crypto-eip_6.6.161-r1.ipk` / `kmod-pce_6.6.161-r1.ipk` 属 24.10 时代旧构建未清 `bin/`（AGENTS.md:116），**不是 6.12 可用产物**。

**四项专项判定**：

| 维度 | 结论 |
|---|---|
| 6.12 接口兼容性 | 源码级部分兼容（feed 补丁已修 gcc15 头保护宏）；但 EIP197 inline 依赖 `<mtk_hnat/...>` 私有头与 `kmod-pce`，[crypto-eip.mk:27-28](file:///home/potter-lgy/immortalwrt-mt798x-6.6-padavanonly/feeds/mtk_openwrt_feed/feed/kernel/crypto-eip/crypto-eip.mk#L27) 的 `-I$(KERNEL_BUILD_DIR)/pce/inc/` 要求 pce 先编；且 `CRYPTO_OFFLOAD_INLINE_FLOWBLOCK` 与 `kmod-mediatek_hnat` 互斥（Config.in:45）。**未做 6.12 端到端链接验证** |
| 依赖冲突风险 | 高：与 `kmod-mediatek_hnat`（6/13 种子已选）在 flowblock 路径冲突；`CONFLICTS:=kmod-crypto-hw-safexcel`；`kmod-crypto-eip-inline` 还 select 5 个 ddk 子包与 `strongswan` |
| 性能表现 | **无可引用数据**。无任何实机 IPsec/crypto 吞吐测试，不能据 14.98 秒编译时间推断性能 |
| 维护状态 | 上游 MediaTek feed 持续维护（本仓库用 `apply-feed-patches.sh` 做幂等适配）；但本分支**从未启用**、13 种子 0 选中、无运行时记录，成熟度 = 「编译过空包」 |

**判定：C2 属「有源码候选但无成熟运行验证」，不满足第 1 条「生产环境运行验证时长 / 稳定性记录」的筛选门槛，不纳入可直接复用清单。**

### 2.3 C4 MedyMa 私有树：不可移植的具体技术依据

- 9 个包：`crypto-eip`(r3) / `mt_hwifi`(r8) / `mtk_npu`(r13) / `mt_wifi7`(r9) / `mt_wifi_cmn`(r2) / `mt_wifi_osal`(r8) / `pce`(r4) / `warp`(r4) / `wifi-profile`(r1)；含 2 个 `tops-*.img` 固件、281 个 `.bin`、1047+1047 份 vendored 源码。
- 私有 ABI，非上游 uapi：`#include <mtk_hnat/...>` 11 文件 16 行、`#include <pce/...>` 28 文件 78 行；`FOE_MAGIC_WED` 23 次、`tops_entry` 54 次、`cdrt` 395 次；`CONFIG_MTK_PPP_TUNNEL_INFO` 7 文件 40 次（由 6.6 补丁新增的私有 Kconfig，25.12 命中 0）；`mediatek,npu` DTS 节点在 `mt7988a-bpi-r4-npu.dtsi:16`（`npu@9100000`）。
- 21 个 `9999*` 自有补丁改 netfilter / mtk_eth / ppe / qdma / npu / ppp / dts / eip；AGENTS.md:47-48 明确「不要照抄 24.10 版本，锚点会偏移，按 6.12 重新导出」。
- 本轮 3 个提交（`0fa7a92ef6` 隐藏门控 `default y if`、`6af5a04bca` 自检脚本对齐 `flow_prepare`、`e285fae271` 删冗余补丁）只对该 6.6 树有意义；经 full5/full6 `BUILD_EXIT=0` 与 `npu.ko`（nf_hnat+mcast 共 8 个 `.o`）实证。25.12 无对应消费方。
- 项目记忆既定结论（project_memory.md:42-45，2026-11-13 12:05）：私有驱动层与 6.12 不兼容、通用安全包已被上游覆盖、HQoS/TOPS/conntrack 补丁为 6.6 专属，均无需移植。

### 2.4 C3 `pce / mtk_npu` feed 版与 MedyMa 版的差异（避免误用本轮修复）

feed 版 `mtk_npu/Config.in:68` 只有 `config MTK_NPU_HNAT`（无 `default y if`、无 `MTK_NPU_24_10_*_BACKEND`），`Makefile` 也没有 `Build/Compile` 的 `$(error)` 守卫——**是另一个更上游的版本**。本轮 `0fa7a92ef6` 的 4 行修复**不能** cherry-pick 到 feed 版。用户若将来启用 feed `kmod-npu`，需重新做 kconfig 求解验证，不能复用结论。

---

## 3. 历史复用案例、最佳实践与踩坑（本地可核证据）

### 3.1 已成功的跨分支迁移（可做模板）

主提交 `5164491bd0`（2026-11-08，`tree: migrate to ImmortalWrt 25.12 (kernel 6.12) mt_wifi base`），`git show --name-status` 实测新增 **1,293 文件**：69 dts/dtsi、7 份 defconfig（现共 13）、4 个 U-Boot 补丁（507~510）；body 明列 13 种子 / 49 profile / 自定义包 / feed 补丁脚本 / base-files 板名与升级分组。

| 类别 | 可直接搬 | 必须重做 |
|---|---|---|
| 种子 / `Device/` / 包目录 / board.d case / 升级分组 | ✅ 但配置栈要换 `mtwifi-cfg-ucode` + `iwinfo-ucode`（AGENTS.md:14-17）；包要比 `Package/*` 定义而非目录（AGENTS.md:9） | |
| DTS | | include 改 `mt7981b.dtsi`、`&{/chosen}`、补 `spi_nand:`/`partitions:`/`ubi:`，逐叶 `cpp`+`dtc`（AGENTS.md:39） |
| U-Boot 2025.10 | | `&pio`（非 `&pinctrl`）、板级 pin 子节点、`# CONFIG_BOARD_INIT is not set`、hunk 头重算 108/131 行（AGENTS.md:55-59） |
| 内核补丁 | | 按 6.12 重新导出（AGENTS.md:48） |
| `mtwifi-wapp` | | 24.10 的两个提交随基座替换丢失，以独立包重迁（AGENTS.md:9） |

### 3.2 官方/构建系统最佳实践（本地源码行号）

- [include/kernel.mk:147-173](file:///home/potter-lgy/immortalwrt-mt798x-6.6-padavanonly/include/kernel.mk#L147)：`PKG_EXTMOD_SUBDIRS`、`collect_module_symvers`（`[ -f …/Module.symvers ] || continue` 守卫）、`$(PKG_NAME).symvers`。
- [include/kernel.mk:175-195](file:///home/potter-lgy/immortalwrt-mt798x-6.6-padavanonly/include/kernel.mk#L175)：`KernelPackage/Defaults` 的 `FILES` / `AUTOLOAD` / `MODPARAMS` / `ModuleAutoLoad`；[207-226](file:///home/potter-lgy/immortalwrt-mt798x-6.6-padavanonly/include/kernel.mk#L207) `KernelPackage` 生成 `kmod-$(1)`，`:220` 绑定 `kernel (=$(LINUX_VERSION)~$(LINUX_VERMAGIC)-r$(LINUX_RELEASE))`——**这是跨内核版本不可二进制复用的机制根因**；[249-269](file:///home/potter-lgy/immortalwrt-mt798x-6.6-padavanonly/include/kernel.mk#L249) `KCONFIG` 非 m/y 生成空包 + WARNING、`FILES` 存在性校验。
- [scripts/package-metadata.pl:181-213](file:///home/potter-lgy/immortalwrt-mt798x-6.6-padavanonly/scripts/package-metadata.pl#L181)（`mconf_depends`）：`+` → `select`，`@` 阻止 `PACKAGE_` 前缀，故 **`+@SYM` 输出裸 `select SYM`**（能拉起无 prompt 的 hidden bool），纯 `@SYM` 输出 `depends on SYM`。这是 AGENTS.md:49（`BuildPlugin DEPENDS` 改 select 断递归环）与本轮 kconfig 门控修复的语义来源。
- [include/package.mk:126](file:///home/potter-lgy/immortalwrt-mt798x-6.6-padavanonly/include/package.mk#L126) `STAMP_CONFIGURED` 只哈希 `PKG_CONFIG_DEPENDS`；改 `CONFIGURE_ARGS` 不使 `.configured_*` 失效（AGENTS.md:98）。
- 判失败必须匹配 `make.*\*\*\*` / `Error [12]` / `undefined reference`（AGENTS.md:93）；`BUILD_EXIT=0` 不等于字节正确（AGENTS.md:113 曾把 `(char *) ` 插进 `"ILMI"` 仍编译通过）。

### 3.3 与「引入私有驱动」最相关的 6 条踩坑

1. **16 MB NOR 物理上限**（AGENTS.md:101）：`mt_wifi.ko` 未 strip 12,109,112 B + ~4.7 MB 内核 = 23,592,964 B > `0xf10000`/`0xe00000`，`check-size` 必失败；4 机型已移出 `mt7981-ax3000.config`（96→92，提交 `560a8c4549`）。**任何新增内核模块都必须先做镜像体积预算**。
2. **Module.symvers 缺失**（AGENTS.md:92）：`collect_module_symvers` 加守卫 + `mt_wifi` 设 `PKG_EXTMOD_SUBDIRS:=mt_wifi_ap`；新外部模块必须声明 `KBUILD_EXTRA_SYMBOLS` / `PKG_SYMVERS_DIR`。
3. **kconfig 门控**（本轮实证）：hidden bool 无 `default`/`select` 会被 `make defconfig` 求解成 n，defconfig 里手钉 `CONFIG_X=y` 会被忽略；`+@SYM` 走 `select` 才能拉起。新包每个隐藏门控都要用真实 `scripts/config/conf` 全场景求解。
4. **补丁与 vendored 源码重复**（本轮实证）：`11e6bc6a52` 把修改写进 `src/` 却漏删 `patches/00{1,2}`，导致 `Hunk #1 FAILED` 与 `redefinition of mt_ktime_get_real_ts64`。引入包时必须核对补丁集与 src 的一致性。
5. **STAMP_CONFIGURED / 工具链陈旧告警**（AGENTS.md:98/112）：改配置不重编会有假通过；gcc 14.3→15.2 后 111/322 个 build_dir 仍引用旧头，判告警必须单包 `{clean,compile}`。
6. **DESTDIR/chown、产物名、libtool 告警**（AGENTS.md:96/99/100）：非 root 暂存安装 EPERM、`initramfs.itb` vs `.bin` 产物名不一致、libtool 暂存安装提示——新包打包脚本要在非 root 构建下验证。

### 3.4 本机同源树（佐证内核版本矩阵，均不可二进制复用）

| 树 | 内核 |
|---|---|
| 本仓库 / `mt798x-6.16` | 6.12.112 |
| `immortalwrt` master | 6.12.108（另有 6.18） |
| `openwrt-ipq` | 6.12.107 |
| `wt-2410`（24.10 对照） | 6.6.162 |
| `immwrt-upstream-merge` | 6.6.161 |
| `wt-medyma`（MedyMa） | 6.6.159 |
| `tcp-brutal-work` SDK | 6.6.144 |

---

## 4. 分类处理结果

按用户要求第 4 条，评估结论落在 **分类 B：无满足「可直接复用」门槛的私有驱动资源**（C1 是既有基线不算「引入」，C2 未通过成熟度门槛，C3/C4 不可移植）。因此输出下列**私有驱动引入标准与标准化开发指导**；同时对 C2 附加一段条件化集成流程，供将来真的需要 EIP197 时使用。

### 4.1 引入标准（准入清单，逐条必须满足）

1. **架构兼容**：必须基于 6.12.112 重新编译，禁止复用 6.6/6.12.10x 的 `.ko` 或预编译 ELF；核对 `vermagic`、`Module.symvers`、`KBUILD_EXTRA_SYMBOLS`。
2. **依赖体系**：不得与 4 块核心闭源驱动冲突；`KCONFIG` 新增符号须 `=m`（HNAT 特例，AGENTS.md / project_memory.md:11）；不得破坏 `mtwifi-cfg-ucode + iwinfo-ucode` 栈（种子三件套）。
3. **包规范**：遵循 `KernelPackage/*` 的 `FILES`/`AUTOLOAD`/`DEPENDS`/`KCONFIG`；私有符号若要 hidden 门控，必须在 `Config.in` 写 `default y if <backend>` 或由 `select` 拉起，并用 `scripts/config/conf` 全场景求解。
4. **补丁规范**：`target/linux/mediatek/patches-6.12/` 的补丁按 6.12 重新导出，hunk 锚点不得照抄 24.10/6.6；若包自带 vendored `src/`，核对补丁集不重复。
5. **feed 适配**：`feeds/` 下文件非 git 跟踪，任何修改必须固化到幂等的 `scripts/apply-feed-patches.sh`，并在 `feeds update/install` 后重跑。
6. **体积预算**：先算 `.ko` 未 strip 大小 + 内核增量，对照 16 MB NOR 机型 `IMAGE_SIZE`；超限则必须 opt-in 且从相关种子排除。
7. **告警与构建**：单包 `{clean,compile}` 后 `warning:` 清零或归因；`EXTRA_CFLAGS` 用源码修正而非 `-Wno-*`；全量 `make -j6 world V=s` 达 `BUILD_EXIT=0` 且日志无 `make.*\*\*\*`。
8. **测试**：新增宿主自检放 `scripts/tests/`（本分支目前**没有**该目录，需新建并保持 `python3 -m py_compile` 通过）；产出真实 `.ko` 并核对 `apk` 内非空；无 `FILES` 的容器包要显式标注，避免与真正含 `.ko` 的子包混淆。
9. **安全**：预编译二进制须隔离 EOL 依赖（参考 `mtwifi-wapp-openssl11` 的 `.so.1.1` 隔离）；不引入未审计的固件 blob；不默认开启。
10. **提交**：每个功能模块独立提交、信息写明「为什么」，种子变更同步 13 份并复跑 `make defconfig`。

### 4.2 标准化开发流程（新私有驱动落地 8 步）

1. **立项**：写明业务场景、目标子目标（filogic / filogic_a73）、依赖矩阵、镜像体积预算；确认不是 6.6/6.12.10x 产物复用。
2. **取源**：优先 MediaTek 官方 feed（`feeds.conf.default`）或 vendored tarball（带 `PKG_HASH` + `PKG_MIRROR_HASH`）；记录上游 commit。
3. **包化**：`package/mtk/drivers/<name>/{Makefile,*.mk,Config.in,src/}`；`KernelPackage` 定义完整；隐藏门控加 `default y if`；声明 `PKG_CONFIG_DEPENDS` 覆盖所有影响编译的符号。
4. **门控验证**：用 `scripts/config/conf --defconfig=...` 对「全关 / 单开 / 共存 / 互斥」四类场景求解，断言门控值；再 `make defconfig` 复核。
5. **单包编译**：`make package/mtk/drivers/<name>/{clean,compile} V=s`；核对 `.ko` 产物、backend `.o` 累加、`warning:` 计数。
6. **打包校验**：解 `apk` 确认含 `.ko`（用 `apk adbdump` 查文件清单；`kmod-crypto-eip` 顶层是容器包，需校验的是 `kmod-crypto-eip-inline`）；`readelf -d`/`nm -u` 核对未解析符号都有 provider。
7. **全量构建**：`make -j6 world V=s > .buildlogs/full.log`；匹配致命行；核对镜像 `check-size` 与 `sha256sums`。
8. **实机验收**：目前**全项目缺失**这一步。须补：模块加载/卸载、长稳 uptime、吞吐与 CPU 占用、与 HNAT/flowtable 共存、热插拔/异常恢复；无此数据不得宣称「生产成熟」。

### 4.3 C2 `crypto-eip` 条件化集成流程（仅在确需 EIP197 inline IPsec 时执行）

前提：目标必须是 `TARGET_mediatek_mt7988` 或 `mt7988a-rfb`/`mt7988d-rfb`（当前 13 种子全是 `filogic`，**默认不满足**），且先修 `Config.in:12` 的 depends 或新增对应设备种子。

| 步 | 动作 | 验收点 |
|---|---|---|
| 1 | 确认 `CONFIG_CRYPTO_OFFLOAD_INLINE=y` 且 `CRYPTO_XFRM_OFFLOAD_MTK_PCE=y`；`CRYPTO_OFFLOAD_INLINE_FLOWBLOCK=n`（与 `kmod-mediatek_hnat` 互斥） | `make defconfig` 后符号为真 |
| 2 | 先编 `kmod-pce`（`-I$(KERNEL_BUILD_DIR)/pce/inc/` 依赖），再编 `crypto-eip` | `pce.ko` 在位、`crypto-eip-inline.ko` 生成 |
| 3 | 重跑 `scripts/apply-feed-patches.sh` 修 gcc15 头保护宏 | 头为 `ADAPTER_ADDRPAIR_H_`，`-Wall -Werror` 通过 |
| 4 | 核对 `EXTRA_KCONFIG` 注入 `CONFIG_MTK_CRYPTO_EIP_INLINE=m` | 编译命令行含该符号 |
| 5 | 打包校验：`apk adbdump` 清单里必须有 `.ko`；容器包单独标注 | 排除真正编不出模块的包 |
| 6 | `CONFLICTS:=kmod-crypto-hw-safexcel` 与 ddk/strongswan 依赖链复核 | 无 kconfig 递归环（AGENTS.md:49/114） |
| 7 | 单包 → 全量 `BUILD_EXIT=0` → 实机 IPsec 吞吐/长稳 | 有实测数据才可升为「成熟」 |

**版本适配要点**：feed `r1` 与 MedyMa `r3` 不是同一版本（后者有 `Build/Compile` 的 `$(error)` 锁与 CAPWAP/DTLS 裁剪），不要混用两份 `Config.in`；`MTK_TOPS_CAPWAP_DTLS` / `TOPS_TNL_NUM` 在 25.12 无 `Config.in` 定义，保持关闭。

---

## 5. 复用优先级排序与风险清单

| 优先级 | 对象 | 动作 | 依据 | 风险 |
|---|---|---|---|---|
| **P0** | 四块核心闭源驱动（C1） | **保持不动**，仅作回归基线 | 6.12.112 构建实证、13 种子主力 | 任何改动都可能破坏 `BUILD_EXIT=0`；16 MB NOR 体积上限 |
| **P1** | `crypto-eip`（C2） | **已修 `DEFAULT:=y` 与分片缓存**（本次落地）；真正启用仍需 MT7988 设备 + inline 子包 | 唯一有 6.12 编译记录 | 与 `kmod-mediatek_hnat` 冲突；ddk/strongswan 依赖链；无实机数据 |
| **P2** | `pce` / `mtk_npu` feed 版（C3） | **不引入**；仅保留 feed 副本供对照 | 13 种子 0 选中、无 6.12 产物 | feed 非 git 跟踪、`feeds install` 会覆盖；与本轮 MedyMa 修复版本不同，勿 cherry-pick |
| **P3** | MedyMa 21 个自有补丁「思路」（C5） | 只作线索索引，按 6.12 锚点重新导出 | AGENTS.md:48、project_memory.md:45 | 锚点偏移；netfilter/conntrack/mtk_eth 在 6.12 结构已变；需逐条复核 |
| **P4** | MedyMa 9 个私有包（C4） | **明确排除**（独立分支 `origin/dev/medyma-npu` 保留） | merge-base rc=1、14419 文件差异、私有头/DTS/固件 | 强 merge 会摧毁主线；vermagic 不兼容；私有 ABI 无 6.12 provider |

### 潜在风险点明细

1. **架构风险（最高）**：两树无共同祖先；`git merge` 会引入 1.4 万文件冲突。禁止。
2. **ABI 风险**：`kernel.mk:220` 的 vermagic 绑定；`<mtk_hnat/...>`/`<pce/...>` 私有头在 25.12 无实现；`CONFIG_MTK_PPP_TUNNEL_INFO` 等私有 Kconfig 命中 0。
3. **依赖冲突**：`CRYPTO_OFFLOAD_INLINE_FLOWBLOCK` vs `kmod-mediatek_hnat`；`mtwifi-cfg` vs `mtwifi-cfg-ucode`；`luajit2` 引发的 6 条 feed 递归依赖（AGENTS.md:114，上游噪声）。
4. **容器包/空包风险**：无 `FILES` 的顶层 `KernelPackage` 是合法容器（如 `kmod-crypto-eip`），但 `DEFAULT:=y` 会把它静默铺到所有目标并拉起内核选项；`KCONFIG` 不满足也会生成空包 + WARNING。构建日志「成功」不等于模块可用，所有含 `.ko` 的子包必须 `apk adbdump` 校验。
5. **体积风险**：16 MB NOR 机型物理上限，新增 `.ko` 前必须预算。
6. **feed 易失风险**：`feeds/` 不入库，改动必须固化到 `apply-feed-patches.sh` 且幂等。
7. **验证缺口风险（全局）**：本机所有「稳定性」证据都来自构建日志与产物，无实机长稳/吞吐/uptime 数据。任何「生产成熟」结论都需补实机验收后才能成立。
8. **维护状态风险**：MedyMa 私包为单人分支、feed 三包为本仓库 0 选中；二者都无本分支的维护迭代记录，不能按「活跃维护」计。

---

## 6. 分阶段实施建议

**阶段 0（已达成，无需动作）**：确认 C1 基线 `BUILD_EXIT=0`、4 个 apk 在位、`dev/medyma-npu` 已推 `origin` 且与主线 `0/0` 同步。本报告即为阶段 0 的交付物。

**阶段 1（近期，低风险）**：
- 把「4.1 引入标准」的 10 条沉淀为 `AGENTS.md` 的一节或 `docs/` 检查单（本次仅产出报告，未改 AGENTS.md，待确认后再落）；
- `crypto-eip` 已落地：`apply-feed-patches.sh` 幂等改 `DEFAULT:=n`、清分片缓存；新增 `scripts/tests/check-kmod-package.sh` 做 `.ko` 清单校验；干净种子复核为 `not set`。
- 清理 `bin/targets/.../packages/` 里 24.10 时代 `*_6.6.*.ipk` 残留，避免误读为 6.12 产物。

**阶段 2（按需，中风险，仅在确需 EIP197 时）**：按 4.3 的 7 步执行；先补 `mt7988` 目标种子与 `CRYPTO_OFFLOAD_INLINE` depends，再单包→全量→实机。未通过第 7 步不得宣称成熟。

**阶段 3（研究，高风险，不承诺落地）**：若将来需要 `mtk_npu/hnat` 类能力，以 C5 的 21 个补丁为线索，按 6.12 锚点逐条重导出，每条都做「单包编译 + 宿主自检 + 实机」三重验证；禁止整包搬运。

**阶段 4（能力建设）**：补实机验收能力（长稳 uptime、吞吐/CPU、模块加载卸载、热插拔异常恢复），把当前「构建验证」升级为「运行验证」，之后才谈得上按第 1 条维度筛选「成熟驱动模块」。

---

## 附录：本报告引用的关键证据路径

- 基线：[AGENTS.md](file:///home/potter-lgy/immortalwrt-mt798x-6.6-padavanonly/AGENTS.md)、[.buildlogs/full_ax3000.log](file:///home/potter-lgy/immortalwrt-mt798x-6.6-padavanonly/.buildlogs/full_ax3000.log)、[defconfig/](file:///home/potter-lgy/immortalwrt-mt798x-6.6-padavanonly/defconfig/)、[feeds.conf.default](file:///home/potter-lgy/immortalwrt-mt798x-6.6-padavanonly/feeds.conf.default)
- 构建系统：[include/kernel.mk](file:///home/potter-lgy/immortalwrt-mt798x-6.6-padavanonly/include/kernel.mk)、[include/package.mk](file:///home/potter-lgy/immortalwrt-mt798x-6.6-padavanonly/include/package.mk)、[scripts/package-metadata.pl](file:///home/potter-lgy/immortalwrt-mt798x-6.6-padavanonly/scripts/package-metadata.pl)、[scripts/apply-feed-patches.sh](file:///home/potter-lgy/immortalwrt-mt798x-6.6-padavanonly/scripts/apply-feed-patches.sh)
- feed 候选：[crypto-eip/Config.in](file:///home/potter-lgy/immortalwrt-mt798x-6.6-padavanonly/feeds/mtk_openwrt_feed/feed/kernel/crypto-eip/Config.in)、[crypto-eip.mk](file:///home/potter-lgy/immortalwrt-mt798x-6.6-padavanonly/feeds/mtk_openwrt_feed/feed/kernel/crypto-eip/crypto-eip.mk)、[mtk_npu/Config.in](file:///home/potter-lgy/immortalwrt-mt798x-6.6-padavanonly/feeds/mtk_openwrt_feed/feed/kernel/mtk_npu/Config.in)
- 对照树：[wt-medyma/.buildlogs/full5.log](file:///home/potter-lgy/wt-medyma/.buildlogs/full5.log)、[full6-coexist.log](file:///home/potter-lgy/wt-medyma/.buildlogs/full6-coexist.log)、[mtk_npu/Config.in](file:///home/potter-lgy/wt-medyma/package/mtk/drivers/mtk_npu/Config.in)
- 既定结论：`project_memory.md:42-45`、`20261113/topics.md:2-3`（2026-11-13 12:05）
