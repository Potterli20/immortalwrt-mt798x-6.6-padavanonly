#!/bin/sh
# 把本仓库自带的 feed 修复放回 feed 检出目录。
# feeds/ 不被 git 跟踪，跑过 ./scripts/feeds update -a 之后需要再跑一次本脚本。

cd "$(dirname "$0")/.." || exit 1

find feeds-patches -name '*.patch' -type f | while read -r f; do
	target=feeds/${f#feeds-patches/}
	dir=${target%/*}
	if [ ! -d "$dir" ]; then
		echo "跳过 $target（对应 feed 包未安装）"
	elif [ -f "$target" ]; then
		echo "已存在 $target"
	else
		cp "$f" "$target" && echo "补入 $target"
	fi
done

# libwebsockets 的 -DISABLE_WERROR=ON 少一个 D，cmake 里一直是 DISABLE_WERROR:BOOL=OFF
lw=feeds/packages/libs/libwebsockets/Makefile
if [ -f "$lw" ] && grep -q -- '-DISABLE_WERROR=ON' "$lw"; then
	sed -i 's/-DISABLE_WERROR=ON/-DDISABLE_WERROR=ON/' "$lw"
	echo "修正 $lw 的 DISABLE_WERROR"
fi
