<img src="https://avatars.githubusercontent.com/u/53193414?s=200&v=4" alt="logo" width="200" height="200" align="right">

# Project ImmortalWrt — mt798x / 内核 6.6 分支

- 上游项目：[ImmortalWrt](https://github.com/immortalwrt/immortalwrt)
- 本仓库：[Potterli20/immortalwrt-mt798x-6.6-padavanonly](https://github.com/Potterli20/immortalwrt-mt798x-6.6-padavanonly)
- 默认分支：`openwrt-24.10-6.6`，基于 ImmortalWrt `openwrt-24.10`，内核 pin 在 **6.6.160**（见 `include/kernel-6.6`）

## Fork 关系（谁 fork 了谁）

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

本仓库配置的两个 remote：

| remote | 地址 |
|---|---|
| `origin` | `git@github.com:Potterli20/immortalwrt-mt798x-6.6-padavanonly` |
| `upstream` | `git@github.com:Yuzhii0718/immortalwrt-mt798x-6.6-padavanonly` |

## 本仓库的分支

| 分支 | 末端提交 | 相对 `openwrt-24.10-6.6` |
|---|---|---|
| `openwrt-24.10-6.6` | 2026-10-28 | 默认分支 |
| `mtk-switch` | 2026-05-01 mt753x 交换机脚本 | 已完全并入默认分支（独有 0） |
| `dev-an8811` | 2026-04-23 rg-x60-new 扩展布局 | 落后 372 / 独有 5 |
| `dual-system-dev` | 2026-04-18 cmcc_a10 双系统 mtd 布局 | 落后 403 / 独有 2 |
| `drop-dead-kernel-mirror` | 2026-07-01 移除失效内核镜像 | 落后 690 / 独有 745（长期分叉，未回收） |

数字按 `git rev-list --left-right --count` 于 2026-10-28 统计；末端提交只说明该分支最后一次改动，不代表完整用途。

## 构建配置来自 `defconfig/`

`defconfig/*.config` 是按芯片/机型整理的种子，Quickstart 第 5 步把它们拷成 `.config`。里面同时打开了
`CONFIG_TARGET_MULTI_PROFILE` 与 `CONFIG_TARGET_PER_DEVICE_ROOTFS`，一次构建会出多台机型的镜像。
内核与 mt_wifi 的可选特性（`CONFIG_MTK_*`）也在这里指定，不在仓库根目录跟踪 `.config`。

> 2026-10-28 修正：所有种子尾部有一段 `is not set` 转储，其中
> `# CONFIG_MTK_BAND_STEERING is not set` 会盖掉前面的 `CONFIG_MTK_BAND_STEERING=y`（kconfig 后出现的为准），
> 导致按种子构建的固件里 band steering 一直是关闭的。现已删除该行。

ImmortalWrt is a fork of [OpenWrt](https://openwrt.org), with more packages ported, more devices supported, default optimized profiles and localization modifications for mainland China users.<br/>
Compared to upstream, we allow to use (non-upstreamable) modifications/hacks to provide better feature/performance/support.

Default login address: http://192.168.6.1 or http://immortalwrt.lan, username: __root__, password: _none_.

## Download
Built firmware images are available for many architectures and come with a package selection to be used as WiFi home router. To quickly find a factory image usable to migrate from a vendor stock firmware to ImmortalWrt, try the *Firmware Selector*.

- [ImmortalWrt Firmware Selector](https://firmware-selector.immortalwrt.org/)

If your device is supported, please follow the **Info** link to see install instructions or consult the support resources listed below.

## Development
To build your own firmware you need a GNU/Linux, BSD or macOS system (case sensitive filesystem required). Cygwin is unsupported because of the lack of a case sensitive file system.<br/>

  ### Requirements
  To build with this project, Debian 11 is preferred. And you need use the CPU based on AMD64 architecture, with at least 4GB RAM and 25 GB available disk space. Make sure the __Internet__ is accessible.

  > 实测参考：本仓库一次 mediatek/filogic 构建后，工作树约 **41 GB**（`build_dir` 32 GB、`dl` 3.6 GB、
  > `staging_dir` 2.7 GB、`feeds` 1.1 GB、`.git` 555 MB、`bin` 468 MB），25 GB 的说法偏小；
  > 想省空间可以事后删 `build_dir`（下次会全量重编），`dl` 建议留着。

  The following tools are needed to compile ImmortalWrt, the package names vary between distributions.

  - Here is an example for Debian/Ubuntu users:<br/>
    - Method 1:
      <details>
        <summary>Setup dependencies via APT</summary>

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
    - Method 2:
      ```bash
      sudo bash -c 'bash <(curl -s https://build-scripts.immortalwrt.org/init_build_environment.sh)'
      ```

  Note:
  - Do everything as an unprivileged user, not root, without sudo.
  - Using CPUs based on other architectures should be fine to compile ImmortalWrt, but more hacks are needed - No warranty at all.
  - You must __not__ have spaces or non-ascii characters in PATH or in the work folders on the drive.
  - If you're using Windows Subsystem for Linux (or WSL), removing Windows folders from PATH is required, please see [Build system setup WSL](https://openwrt.org/docs/guide-developer/build-system/wsl) documentation.
  - Using macOS as the host build OS is __not__ recommended. No warranty at all. You can get tips from [Build system setup macOS](https://openwrt.org/docs/guide-developer/build-system/buildroot.exigence.macosx) documentation.
  - For more details, please see [Build system setup](https://openwrt.org/docs/guide-developer/build-system/install-buildsystem) documentation.

  ### Quickstart
  1. Clone this fork (需要别的上游时，把 URL 换成 `padavanonly/immortalwrt-mt798x-6.6` 或 `Yuzhii0718/immortalwrt-mt798x-6.6-padavanonly`，分支名同为 `openwrt-24.10-6.6`)：

     ```bash
     git clone -b openwrt-24.10-6.6 --single-branch \
       https://github.com/Potterli20/immortalwrt-mt798x-6.6-padavanonly immortalwrt-mt798x-6.6-padavanonly
     ```

  2. Run `cd immortalwrt-mt798x-6.6-padavanonly` to enter source directory.
  3. Run `./scripts/feeds update -a` to obtain all the latest package definitions defined in feeds.conf / feeds.conf.default
  4. Run `./scripts/feeds install -a` to install symlinks for all obtained packages into package/feeds/
  5. Copy the configuration file for your device from the `defconfig` directory to the project root directory and rename it `.config`
     
     ```bash
     # MT7981
     cp -f defconfig/mt7981-ax3000.config .config

     # MT7986
     cp -f defconfig/mt7986-ax6000.config .config
     ```
     
  6. Run `make menuconfig` to open the configuration menu, where you can customize the build configuration, such as choosing the target system, target profile, and packages to include in the firmware. After making your selections, save and exit the menu.
  7. Run `make download -j$(nproc)` to download all sources. This will download the GNU/Linux kernel, toolchain and all chosen applications for your target system. You can also run `make download -j$(nproc) V=s` to see the details of the downloading process.
  8. Run `make` to build your firmware. This will download all sources, build the cross-compile toolchain and then cross-compile the GNU/Linux kernel & all chosen applications for your target system.

  > If you build first time, recommend to run `make -j1 V=s` to see the details of the building process, which is helpful for debugging. After you get familiar with the build system, you can run `make -j$(nproc)` to speed up the building process.

  ### Related Repositories
  The main repository uses multiple sub-repositories to manage packages of different categories. All packages are installed via the OpenWrt package manager called opkg. If you're looking to develop the web interface or port packages to ImmortalWrt, please find the fitting repository below.
  - [LuCI Web Interface](https://github.com/immortalwrt/luci): Modern and modular interface to control the device via a web browser.
  - [ImmortalWrt Packages](https://github.com/immortalwrt/packages): Community repository of ported packages.
  - [OpenWrt Routing](https://github.com/openwrt/routing): Packages specifically focused on (mesh) routing.
  - [OpenWrt Video](https://github.com/openwrt/video): Packages specifically focused on display servers and clients (Xorg and Wayland).

## Support Information
For a list of supported devices see the [OpenWrt Hardware Database](https://openwrt.org/supported_devices)
  ### Documentation
  - [Quick Start Guide](https://openwrt.org/docs/guide-quick-start/start)
  - [User Guide](https://openwrt.org/docs/guide-user/start)
  - [Developer Documentation](https://openwrt.org/docs/guide-developer/start)
  - [Technical Reference](https://openwrt.org/docs/techref/start)

  ### Support Community
  - Support Chat: group [@ctcgfw_openwrt_discuss](https://t.me/ctcgfw_openwrt_discuss) on [Telegram](https://telegram.org/).
  - Support Chat: group [#immortalwrt](https://matrix.to/#/#immortalwrt:matrix.org) on [Matrix](https://matrix.org/).

## License
ImmortalWrt is licensed under [GPL-2.0-only](https://spdx.org/licenses/GPL-2.0-only.html).

## Acknowledgements
<table>
  <tr>
    <td><a href="https://dlercloud.com/"><img src="https://user-images.githubusercontent.com/22235437/111103249-f9ec6e00-8588-11eb-9bfc-67cc55574555.png" width="183" height="52" border="0" alt="Dler Cloud"></a></td>
    <td><a href="https://www.jetbrains.com/"><img src="https://resources.jetbrains.com/storage/products/company/brand/logos/jb_square.png" width="120" height="120" border="0" alt="JetBrains Black Box Logo logo"></a></td>
    <td><a href="https://sourceforge.net/"><img src="https://sourceforge.net/sflogo.php?type=17&group_id=3663829" alt="SourceForge" width=200></a></td>
  </tr>
</table>
