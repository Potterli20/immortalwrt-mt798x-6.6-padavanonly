#!/usr/bin/env bash
# 以整套源码树为工作目录进入编译容器。
# 关键：把仓库根 bind-mount 到【相同绝对路径】，否则 build_dir/staging_dir 里
#      硬编码的 toolchain 绝对路径失效，会触发全量重编。
set -euo pipefail
DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO="$(cd "$DIR/.." && pwd)"          # 整套源码树 = 仓库根
IMG="${IMG:-immortalwrt-sdk:latest}"

docker run -it --rm \
  --user "$(id -u):$(id -g)" \
  --hostname openwrt-build \
  -v "$REPO:$REPO" \
  -w "$REPO" \
  -e HOME=/home/openwrt \
  -e LANG=C.UTF-8 \
  -v immortalwrt-home:/home/openwrt \
  "$IMG" "${@:-bash}"
