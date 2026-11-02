# ImmortalWrt mt798x (mediatek/filogic) 整套源码树编译环境
# 镜像只提供编译依赖；源码树在运行时以相同绝对路径 bind-mount 进来，
# 从而复用已有的 build_dir / staging_dir / dl 直接续编。
FROM debian:trixie

ARG DEBIAN_FRONTEND=noninteractive

# 基础 + locale（OpenWrt 需要 UTF-8）
RUN apt-get update && apt-get install -y --no-install-recommends \
      ca-certificates gnupg locales git \
    && sed -i 's/^# *\(en_US.UTF-8\)/\1/' /etc/locale.gen \
    && echo 'C.UTF-8 UTF-8' >> /etc/locale.gen \
    && locale-gen \
    && rm -rf /var/lib/apt/lists/*

# 编译依赖：照 README 清单，按 trixie / aarch64 修正
#   gnutls-dev -> libgnutls28-dev
#   去掉 python3-distutils（trixie 已移除）
#   去掉 x86 multilib（aarch64 原生编译无用）
RUN apt-get update && apt-get install -y --no-install-recommends \
      ack antlr3 asciidoc autoconf automake autopoint binutils bison build-essential \
      bzip2 ccache clang cmake cpio curl device-tree-compiler ecj fastjar flex gawk gettext \
      libgnutls28-dev gperf haveged help2man intltool libelf-dev libglib2.0-dev libgmp3-dev \
      libltdl-dev libmpc-dev libmpfr-dev libncurses-dev libpython3-dev libreadline-dev \
      libssl-dev libtool libyaml-dev libz-dev lld llvm lrzsz mkisofs msmtp nano ninja-build \
      p7zip p7zip-full patch pkgconf python3 python3-pip python3-setuptools python3-ply \
      python3-docutils python3-pyelftools qemu-utils re2c rsync scons squashfs-tools subversion \
      swig texinfo uglifyjs upx-ucl unzip vim wget xmlto xxd zlib1g-dev zstd \
    && rm -rf /var/lib/apt/lists/*

ENV LANG=C.UTF-8 LC_ALL=C.UTF-8

# 非 root 编译用户；uid 与 host(potter-lgy 1000:1000) 对齐，bind-mount 产物权限不破。
# gid 由 run 脚本用 --user $(id -u):$(id -g) 数字注入覆盖，镜像内不强制建 gid=1000 组。
RUN useradd -m -u 1000 -s /bin/bash openwrt

USER openwrt
ENV HOME=/home/openwrt
WORKDIR /home/openwrt

CMD ["bash"]
