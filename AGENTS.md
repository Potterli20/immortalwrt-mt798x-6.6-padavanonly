# AGENTS.md — immortalwrt-mt798x-6.6-padavanonly

## 上游与参考源

- 当前分支 `openwrt-24.10-6.6`；`origin` = Potterli20/immortalwrt-mt798x-6.6-padavanonly，`upstream` = Yuzhii0718/immortalwrt-mt798x-6.6-padavanonly。
- rebase / 补丁移植参考：<https://github.com/chasey-dev/immortalwrt-mt798x-rebase>
  - ImmortalWrt + MTK OpenWrt feeds 补丁导入。默认分支 `25.12`；对照本分支优先看 `rework-24.10`，其余有 `openwrt-25.12`、`25.12-dev-wifi7`、`rework-21.02`。
- 内核更新来源：<https://github.com/unifreq/linux-6.6.y>（分支 `main`，2026-10-20 核对为 6.6.159）。
- 内核补丁来源：<https://gitlab.com/xanmod/linux>（XanMod 补丁）。

## 当前内核接线

- `target/linux/ramips/Makefile` 设 `KERNEL_PATCHVER:=6.6`。
- `include/kernel-6.6` pin `LINUX_VERSION-6.6 = .159` + `LINUX_KERNEL_HASH-6.6.159`，即从 kernel.org 取主线稳定树；`.config` 中 `CONFIG_KERNEL_GIT_CLONE_URI=""`。
- 若要改用 unifreq 的树，走 `CONFIG_KERNEL_GIT_CLONE_URI` / `CONFIG_KERNEL_GIT_REF`（版本名生成逻辑见 `include/kernel-version.mk`），同时核对 `include/kernel-6.6` 里的 hash 与 `target/linux/generic` 补丁是否重复。

## 备注

- 构建机是 gcc 15（默认 C23）+ cmake 4，相关修复已进仓库（见 `git log` 里 `package:` / `tools:` / `feeds-patches:` 系列提交），后续 rebase 时保留这些改动。
