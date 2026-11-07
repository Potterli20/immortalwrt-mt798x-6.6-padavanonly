<img src="https://avatars.githubusercontent.com/u/53193414?s=200&v=4" alt="logo" width="200" height="200" align="right">

# ImmortalWrt mt798x（padavanonly 线）· 内核 6.6

ImmortalWrt 是 [OpenWrt](https://openwrt.org) 的分支：移植了更多软件包、支持更多机型、默认给出优化过的 profile，并针对中国大陆用户做了本地化。与上游相比，本线允许使用无法上游化的改动 / hack，换取更好的特性、性能与机型支持。

| | |
|---|---|
| 本仓库 | [Potterli20/immortalwrt-mt798x-6.6-padavanonly](https://github.com/Potterli20/immortalwrt-mt798x-6.6-padavanonly) |
| 默认分支 | `openwrt-24.10-6.6`（基于 ImmortalWrt `openwrt-24.10`） |
| 内核 | pin 在 **6.6.162**，取 kernel.org 主线稳定树，版本与 hash 见 `include/kernel-6.6` |
| Wi-Fi 驱动 | MTK `mt_wifi` 7.6.7.3（Padavan 同源，配套 `luci-app-mtwifi-cfg`）；种子都走这条路，wpad / mac80211 是关闭的 |
| 机型主线 | `target/linux/mediatek/filogic`，156 个机型条目（mt7981 × 101、mt7986 × 48、mt7988 × 7） |
| 出厂地址 | <http://192.168.6.1>，用户 `root`，密码为空（锐捷 `ruijie,rg-x60*` / `ew-6000gx-pro*` 是 192.168.110.1） |

> 表内信息核对于 2026-11-07。维护性的坑与约定集中写在 [AGENTS.md](AGENTS.md)，改构建脚本前请先看。

## 目录

- [本线特性](#本线特性)
- [Fork 关系](#fork-关系)
- [分支](#分支)
- [构建](#构建)
- [日常维护](#日常维护)
- [获取镜像](#获取镜像)
- [相关仓库](#相关仓库)
- [支持信息](#支持信息)

## 本线特性

相对 ImmortalWrt `openwrt-24.10`，这条线的主要差异：

- **内核跟随更紧**：pin 在 kernel.org 主线稳定树（当前 6.6.162），并逐条跟进
  `mediatek/mtk-openwrt-feeds` 的内核补丁，`patches-6.6` 现有 323 条（含 an8801sb PHY、PCIe、cpufreq、WDT 等）。
- **无线走 MTK 原厂驱动**：`mt_wifi` 7.6.7.3 + `luci-app-mtwifi-cfg`，wpad / mac80211 关闭；
  WHNAT、band steering、mtwifi 专有参数都在 LuCI 里可直接调。
- **机型覆盖面大**：filogic 下 156 个机型条目，含同板不同 flash / 内存 / 射频前端的变体。
- **开箱可用**：`defconfig/` 提供 11 份种子（多 profile 一次出多台机型），feed 侧问题由
  `scripts/apply-feed-patches.sh` 就地修好，`make` 不再刷依赖告警与 kconfig 递归依赖。
- **允许不可上游的 hack**：这条线接受为特性、性能与机型支持而做的非上游改动，代价是 rebase 时需要按
  AGENTS.md 逐项复验。

## Fork 关系

谁 fork 了谁：

```
immortalwrt/immortalwrt
└── padavanonly/immortalwrt-mt798x-6.6           本线路源头（曾名 immortalwrt-mt798x-24.10，旧链接由 GitHub 重定向）
    ├── Yuzhii0718/immortalwrt-mt798x-6.6-padavanonly      本仓库的 upstream remote
    │   ├── Potterli20/immortalwrt-mt798x-6.6-padavanonly  ← 本仓库（origin remote）
    │   ├── huniangitb/immortalwrt-mt798x-6.6-padavanonly  private/q30p：JCG Q30 Pro 私人构建线
    │   ├── GTX680/immortalwrt-mt798x-6.6-padavanonly      autocore/tempinfo 等零散修复
    │   └── biuyoudeaad/immortalwrt-mt798x-6.6-padavanonly JCG Q30 Pro / Cudy TR3000 马甲板支持
    ├── 561410590/immortalwrt-mt798x-6.6          mtwifi-sync-*：mt_wifi 驱动同步线（当前基线比本仓库旧）
    └── MedyMa/immortalwrt-mt798x-6.6            mt798x-mt799x-6.6-mtwifi：mt7988 + Wi-Fi 7（BE14/mt_hwifi/NPU）
```

同步用的 remote：

| remote | 地址 |
|---|---|
| `origin` | `git@github.com:Potterli20/immortalwrt-mt798x-6.6-padavanonly` |
| `upstream` | `git@github.com:Yuzhii0718/immortalwrt-mt798x-6.6-padavanonly` |
| `fat4` | `git@github.com:fat4-alt/immortalwrt-mt798x-6.6.git` |

## 分支

| 分支 | 末端提交 | 相对 `openwrt-24.10-6.6` |
|---|---|---|
| `openwrt-24.10-6.6` | 2026-11-07 | 默认分支 |
| `mtk-switch` | 2026-05-01 mt753x 交换机脚本语法修复 | 已完全并入默认分支（独有 0） |
| `dev-an8811` | 2026-04-23 rg-x60-new 扩展布局 | 落后 402 / 独有 5 |
| `dual-system-dev` | 2026-04-18 cmcc_a10 双系统 mtd 布局 | 落后 433 / 独有 2 |
| `drop-dead-kernel-mirror` | 2026-07-01 移除失效内核镜像 | 落后 720 / 独有 745（长期分叉，未回收） |

数字按 `git rev-list --left-right --count openwrt-24.10-6.6...upstream/<分支>` 于 2026-11-07 统计；末端提交只说明该分支最后一次改动，不代表完整用途。日常只用默认分支即可，后三条是待回收或历史分支。

## 构建

构建需要 GNU/Linux、BSD 或 macOS（大小写敏感的文件系统）。Cygwin 不支持，因为文件系统大小写不敏感。

### 环境要求

推荐 Debian 11+，AMD64 CPU，至少 4 GB 内存，**25 GB 以上可用磁盘**。网络必须可访问。

> 实测参考：本仓库一次 mediatek/filogic 构建后工作树约 **41 GB**（`build_dir` 32 GB、`dl` 3.6 GB、
> `staging_dir` 2.7 GB、`feeds` 1.1 GB、`.git` 555 MB、`bin` 468 MB），25 GB 的说法偏小。
> 想省空间可以事后删 `build_dir`（下次会全量重编），`dl` 建议留着。

以下工具是必需的，包名因发行版而异。Debian/Ubuntu 示例：

<details>
<summary>方式一：APT 装依赖</summary>

```bash
sudo apt update -y
sudo apt full-upgrade -y
sudo apt install -y ack antlr3 asciidoc autoconf automake autopoint binutils bison build-essential \
  bzip2 ccache clang cmake cpio curl device-tree-compiler ecj fastjar flex gawk gettext gcc-multilib \
  g++-multilib git gnutls-dev gperf haveged help2man intltool lib32gcc-s1 libc6-dev-i386 libelf-dev \
  libglib2.0-dev libgmp3-dev libltdl-dev libmpc-dev libmpfr-dev libncurses-dev libpython3-dev \
  libreadline-dev libssl-dev libtool libyaml-dev libz-dev lld llvm lrzsz mkisofs msmtp nano \
  ninja-build p7zip p7zip-full patch pkgconf python3 python3-pip python3-ply python3-docutils \
  python3-pyelftools qemu-utils re2c rsync scons squashfs-tools subversion swig texinfo uglifyjs \
  upx-ucl unzip vim wget xmlto xxd zlib1g-dev zstd
```

</details>

<details>
<summary>方式二：用 ImmortalWrt 官方初始化脚本</summary>

```bash
sudo bash -c 'bash <(curl -s https://build-scripts.immortalwrt.org/init_build_environment.sh)'
```

</details>

注意：

- 全程用普通用户身份编译，不要用 root，也不要 sudo。
- 其他架构的 CPU 也能编，但需要更多 hack，不提供任何保证。
- 路径和工作目录里**不能**有空格或非 ASCII 字符。
- WSL 用户必须把 Windows 的目录从 PATH 里摘掉，见 [Build system setup WSL](https://openwrt.org/docs/guide-developer/build-system/wsl)。
- 不推荐用 macOS 做宿主系统，不提供任何保证，可参考 [Build system setup macOS](https://openwrt.org/docs/guide-developer/build-system/buildroot.exigence.macosx)。
- 更多细节见 [Build system setup](https://openwrt.org/docs/guide-developer/build-system/install-buildsystem)。

### Docker 编译容器（可选）

不想污染宿主环境可以用 `docker/` 下的容器：

```bash
./docker/build.sh                 # 构建镜像，默认 tag immortalwrt-sdk:latest
./docker/run.sh                   # 以仓库根为工作目录进入容器
```

`run.sh` 把源码树 bind-mount 到**同一个绝对路径**，这是必须的：`build_dir` / `staging_dir` 里记录的
toolchain 路径是硬编码的，路径一变就触发全量重编。镜像基于 Debian trixie，装的就是上面那份依赖清单，
按 trixie 做了调整（`gnutls-dev` → `libgnutls28-dev`、去掉已移除的 `python3-distutils` 与 x86 multilib），
编译用户是 uid 1000 的非 root `openwrt`，与宿主对齐以免 bind-mount 产物权限错乱。

### Quickstart

1. 克隆本仓库（需要别的上游时，把 URL 换成 `padavanonly/immortalwrt-mt798x-6.6` 或
   `Yuzhii0718/immortalwrt-mt798x-6.6-padavanonly`，分支名同为 `openwrt-24.10-6.6`）：

   ```bash
   git clone -b openwrt-24.10-6.6 --single-branch \
     https://github.com/Potterli20/immortalwrt-mt798x-6.6-padavanonly immortalwrt-mt798x-6.6-padavanonly
   cd immortalwrt-mt798x-6.6-padavanonly
   ```

2. 拉取 feed：

   ```bash
   ./scripts/feeds update -a
   ./scripts/feeds install -a
   ```

   `feeds.conf.default` 里除了 ImmortalWrt 主线的 packages / luci / routing / telephony，还挂了
   `mediatek/mtk-openwrt-feeds`、`x-wrt/com.x-wrt`、`morse`、`openmanet`、`217heidai/OpenWrt-Packages` 等第三方源。

3. **补回 feed 侧修复**（`feeds/` 不被 git 跟踪，每次 `feeds update -a` 之后都要重跑一次）：

   ```bash
   ./scripts/apply-feed-patches.sh
   ```

   脚本是幂等的，负责把只存在于 feed 的问题就地修好：feed 依赖了本树没有的包名（会让 `make` 刷一屏
   `WARNING: ... has a dependency on '...', which does not exist`）、kconfig 递归依赖、
   `libwebsockets` 那个少写一个 `D` 的 `-DISABLE_WERROR=ON`、同名包撞车等。跳过和修正都会逐行打印。

4. 拷一份机型种子配置当 `.config`（见下表）：

   ```bash
   cp -f defconfig/mt7981-ax3000.config .config     # MT7981 / AX3000
   cp -f defconfig/mt7986-ax6000.config .config     # MT7986 / AX6000
   ```

5. `make menuconfig` 里确认 target system、target profile 和要打包的软件包，保存退出。

6. `make download -j$(nproc)` 预取内核、toolchain 与所有选中的源码（调试用 `V=s`）。

7. `make` 开始构建，产物在 `bin/targets/mediatek/`。

> 首次构建建议 `make -j1 V=s`，出错时能看到完整上下文；熟悉之后再 `make -j$(nproc)` 提速。

### `defconfig/` 种子

除 `mt7986-ax4200-bpir3_mini.config` 外，种子都同时打开了 `CONFIG_TARGET_MULTI_PROFILE` 与
`CONFIG_TARGET_PER_DEVICE_ROOTFS`，一次构建会出多台机型的镜像；mini 那份是单 profile
（`DEVICE_BPI-R3MINI-EMMC`）的完整转储。内核与 mt_wifi 的可选特性（`CONFIG_MTK_*`）也都在种子里指定。
仓库根目录的 `.config` 不入库。

| 文件 | 用途 |
|---|---|
| `mt7981-ax3000.config` | MT7981 / AX3000 主力种子 |
| `mt7981-ax3000_dae.config` | 同上，追加 XDP/BPF 内核与 kmod 选项（DAE 加速路径需要） |
| `mt7981-nowifi.config` | MT7981，不含无线驱动与无线软件包的精简种子 |
| `mt7986-ax6000.config` | MT7986 / AX6000 主力种子 |
| `mt7986-ax6000_dae.config` | 同上 + DAE 选项 |
| `mt7986-ax4200.config` | MT7986 / AX4200 |
| `mt7986-ax7800.config` | MT7986 / AX7800 |
| `mt7986-ax4200-bpir3_mini.config` | BananaPi R3 mini，**完整 `.config` 转储**（8000+ 行，单 profile，不是精简种子） |
| `mt7975-ipailna-high-power.config` | iPAiLNA 高功率前端机型（芯片仍是 MT7986，文件名沿用射频前端命名），用 `MT7986_DEFAULT_FIRMWARE` 固件变体 |
| `mt7975-ipailna-high-power_dae.config` | 同上 + DAE 选项 |
| `mt7986-nowifi.config` | MT7986 无无线精简种子 |

种子之间的差异主要是 `CONFIG_TARGET_DEVICE_mediatek_filogic_DEVICE_*` 机型列表、mt_wifi 固件变体和
DAE 选项，其余部分基本同源；改机型时建议直接 diff 两个种子，别整文件替换。

> 2026-10-28 修正：种子尾部原本有一段 `is not set` 转储，其中
> `# CONFIG_MTK_BAND_STEERING is not set` 会盖掉前面的 `CONFIG_MTK_BAND_STEERING=y`（kconfig 后出现的为准），
> 导致按种子构建的固件里 band steering 一直是关闭的。现已删除该行，`nowifi` / `bpir3_mini` 两份没有这个开关。

## 日常维护

同步上游或改构建脚本之后，这几件事最容易漏：

- `feeds/` 不在版本控制里：每次 `./scripts/feeds update -a && ./scripts/feeds install -a` 之后都要重跑
  `./scripts/apply-feed-patches.sh`，否则依赖告警、kconfig 递归依赖、同名包撞车会重新出现。脚本幂等，可以随便重复跑。
- 内核小版本 bump 要同时改 `include/kernel-6.6` 的 `LINUX_VERSION-6.6` 与 `LINUX_KERNEL_HASH-6.6.*` 两行，
  再 `make target/linux/prepare V=s` 确认失败数为 0。判失败请匹配 `Hunk #N FAILED`，
  不要 `grep -i failed`——有两个补丁的文件名里本身带 `failed`，会假命中。
- 往 `target/linux/*/patches-*/` 加新补丁必须 `git add -f`：`.gitignore` 里的 `*.patch` 会把它们静默吞掉。
  同理，`/package/*` 也会吞掉新包目录（`luci-lib-chartjs-4` 就是这样补进去的），新包记得确认 `git status` 里有它。
- `.config` 不入库。改了机型或特性，请把差异同步回 `defconfig/` 对应种子，而不是只留本地。

## 获取镜像

本仓库**没有** Release 固件，官方 [ImmortalWrt Firmware Selector](https://firmware-selector.immortalwrt.org/)
也只覆盖上游机型、不含这里的补丁，所以请自行编译。选择器仍然可以当作上游机型的安装说明参考。

## 相关仓库

主仓库用多个子仓库分类管理软件包，都通过 OpenWrt 的 opkg 安装。要做 Web 界面或移植软件包，找对应的仓库：

- [LuCI Web Interface](https://github.com/immortalwrt/luci)：现代、模块化的浏览器管理界面。
- [ImmortalWrt Packages](https://github.com/immortalwrt/packages)：社区移植软件包仓库。
- [OpenWrt Routing](https://github.com/openwrt/routing)：偏（mesh）路由的软件包。
- [OpenWrt Video](https://github.com/openwrt/video)：显示服务端与客户端（Xorg / Wayland）。

## 支持信息

本仓库实际支持的机型以 `target/linux/mediatek/image/filogic.mk` 里的 `TARGET_DEVICES +=` 为准（当前 156 条）；
通用机型资料见 [OpenWrt Hardware Database](https://openwrt.org/supported_devices)。

文档：

- [Quick Start Guide](https://openwrt.org/docs/guide-quick-start/start)
- [User Guide](https://openwrt.org/docs/guide-user/start)
- [Developer Documentation](https://openwrt.org/docs/guide-developer/start)
- [Technical Reference](https://openwrt.org/docs/techref/start)

支持社区：

- Telegram：[@ctcgfw_openwrt_discuss](https://t.me/ctcgfw_openwrt_discuss)
- Matrix：[#immortalwrt](https://matrix.to/#/#immortalwrt:matrix.org)

问题反馈优先用仓库的 [bug-report 模板](../../issues/new?labels=bug&template=bug-report.yml)。

## License

ImmortalWrt 以 [GPL-2.0-only](https://spdx.org/licenses/GPL-2.0-only.html) 授权。

## Acknowledgements

<table>
  <tr>
    <td><a href="https://dlercloud.com/"><img src="https://user-images.githubusercontent.com/22235437/111103249-f9ec6e00-8588-11eb-9bfc-67cc55574555.png" width="183" height="52" border="0" alt="Dler Cloud"></a></td>
    <td><a href="https://www.jetbrains.com/"><img src="https://resources.jetbrains.com/storage/products/company/brand/logos/jb_square.png" width="120" height="120" border="0" alt="JetBrains Black Box Logo logo"></a></td>
    <td><a href="https://sourceforge.net/"><img src="https://sourceforge.net/sflogo.php?type=17&group_id=3663829" alt="SourceForge" width=200></a></td>
  </tr>
</table>
