#!/usr/bin/env bash
# 构建 ImmortalWrt 编译镜像（context 只含本 docker/ 目录，秒级）
set -euo pipefail
DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
IMG="${IMG:-immortalwrt-sdk:latest}"
docker build -t "$IMG" -f "$DIR/Dockerfile" "$DIR"
echo "OK -> $IMG"
