#!/usr/bin/env bash
# SPDX-FileCopyrightText: 2026 testerxydw
# SPDX-License-Identifier: GPL-3.0-or-later
#
# 一键打包脚本：依赖检查 + 编译 + 打包 + 生成发布产物（多架构）。
# 基于 debian/ 标准 dpkg-buildpackage，产物含 deb + 裸二进制 + checksum。
#
# 用法:
#   ./scripts/package.sh              # 本机架构
#   ./scripts/package.sh amd64 arm64  # 指定架构（arm64 需 docker + QEMU）
#
# 产物目录: build/dist/<arch>/
#   convert-search-plugin_<ver>_<arch>.deb
#   convert-search-plugin_<arch>            (裸二进制)
#   SHA256SUMS_<arch>.txt

set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"

VERSION="$(dpkg-parsechangelog -S version 2>/dev/null || echo '1.0.0')"
DIST_DIR="$ROOT/build/dist"
JOBS="$(nproc)"

log() { echo "[package] $*"; }
err() { echo "[package][ERR] $*" >&2; }

check_deps() {
    local missing=()
    for c in cmake dpkg-buildpackage dpkg-parsechangelog; do
        command -v "$c" >/dev/null 2>&1 || missing+=("$c")
    done
    if [[ ${#missing[@]} -gt 0 ]]; then
        err "缺少依赖: ${missing[*]}"
        err "安装: sudo apt-get install -y cmake dpkg-dev debhelper"
        return 1
    fi
    return 0
}

# 用 dpkg-buildpackage 在给定源码目录构建，输出 deb 到 $DIST_DIR/$arch
build_with_dpkg() {
    local arch="$1" src="$2"
    log "dpkg-buildpackage ($arch)"
    (
        cd "$src"
        # -us -uc: 不签名; -b: 仅二进制; -a<arch>: 目标架构
        dpkg-buildpackage -us -uc -b -a"$arch" >/dev/null 2>&1 || \
        dpkg-buildpackage -us -uc -b >/dev/null 2>&1
    )
    # deb 生成在源码目录上级（../）
    local deb
    deb="$(ls -1 "$src"/../convert-search-plugin_*_${arch}.deb 2>/dev/null | head -1)"
    [[ -z "$deb" ]] && deb="$(ls -1 "$ROOT"/../convert-search-plugin_*_${arch}.deb 2>/dev/null | head -1)"
    [[ -z "$deb" ]] && { err "未找到生成的 deb ($arch)"; return 1; }
    mkdir -p "$DIST_DIR/$arch"
    cp "$deb" "$DIST_DIR/$arch/"
    # 裸二进制：从 deb 解包提取
    dpkg-deb -x "$deb" "$DIST_DIR/$arch/extract" >/dev/null 2>&1 || true
    local bin
    bin="$(find "$DIST_DIR/$arch/extract" -name convert-search-plugin -type f 2>/dev/null | head -1)"
    if [[ -n "$bin" ]]; then
        cp "$bin" "$DIST_DIR/$arch/convert-search-plugin_${arch}"
        rm -rf "$DIST_DIR/$arch/extract"
    fi
    ( cd "$DIST_DIR/$arch" && sha256sum "$(basename "$deb")" \
        "convert-search-plugin_${arch}" > "SHA256SUMS_${arch}.txt" 2>/dev/null || true )
    log "产物: $DIST_DIR/$arch/$(basename "$deb")"
}

build_arch() {
    local arch="$1"
    log "构建架构: $arch"
    if [[ "$arch" == "$(uname -m)" || "$arch" == "amd64" && "$(uname -m)" == "x86_64" ]]; then
        build_with_dpkg "$arch" "$ROOT"
    elif [[ "$arch" == "arm64" || "$arch" == "aarch64" ]]; then
        if ! command -v docker >/dev/null 2>&1; then
            err "arm64 需 docker + QEMU; 本机缺失，请在 GitHub Actions 构建。"
            return 1
        fi
        log "arm64 容器内 dpkg-buildpackage"
        docker run --rm --platform linux/arm64 \
            -v "$ROOT":/src -w /src ubuntu:24.04 bash -c '
            set -e
            apt-get update
            apt-get install -y --no-install-recommends \
                cmake dpkg-dev debhelper extra-cmake-modules \
                qt6-base-dev qt6-base-dev-tools pkg-config build-essential
            # 修正 control 架构字段以便 dpkg-buildpackage -aarm64 通过
            sed -i "s/^Architecture: any/Architecture: arm64/" debian/control
            dpkg-buildpackage -us -uc -b -aarm64
            '
        # deb 在容器内生成于 /src/.. (即挂载的 ROOT/..), 拷回 DIST
        local deb
        deb="$(ls -1 "$ROOT"/../convert-search-plugin_*_arm64.deb 2>/dev/null | head -1)"
        [[ -z "$deb" ]] && { err "容器内未生成 arm64 deb"; return 1; }
        mkdir -p "$DIST_DIR/arm64"
        cp "$deb" "$DIST_DIR/arm64/"
        dpkg-deb -x "$deb" "$DIST_DIR/arm64/extract" >/dev/null 2>&1 || true
        local bin
        bin="$(find "$DIST_DIR/arm64/extract" -name convert-search-plugin -type f 2>/dev/null | head -1)"
        [[ -n "$bin" ]] && { cp "$bin" "$DIST_DIR/arm64/convert-search-plugin_arm64"; rm -rf "$DIST_DIR/arm64/extract"; }
        ( cd "$DIST_DIR/arm64" && sha256sum "$(basename "$deb")" "convert-search-plugin_arm64" > SHA256SUMS_arm64.txt 2>/dev/null || true )
        log "产物: $DIST_DIR/arm64/$(basename "$deb")"
    else
        err "不支持架构: $arch (仅 amd64/arm64)"
        return 1
    fi
}

main() {
    check_deps || exit 1
    local arches=("$@")
    if [[ ${#arches[@]} -eq 0 ]]; then
        arches=("amd64"); [[ "$(uname -m)" == "aarch64" ]] && arches=("arm64")
    fi
    for a in "${arches[@]}"; do
        build_arch "$a" || err "架构 $a 构建失败"
    done
    # 汇总 checksum
    cat "$DIST_DIR"/*/SHA256SUMS_*.txt > "$DIST_DIR/SHA256SUMS.txt" 2>/dev/null || true
    log "完成. 产物: $DIST_DIR"
}

main "$@"
