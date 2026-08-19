#!/usr/bin/env bash
# SPDX-FileCopyrightText: 2026 testerxydw
# SPDX-License-Identifier: GPL-3.0-or-later
#
# 发布标记脚本：将当前提交标记为发布版本，并推送到远程。
# 与 package.sh 配合：package.sh 每次打包自动自增末位（记录测试次数），
# 本脚本用于把某个版本号正式标记为发布版本（打 tag + 推送）。
#
# 用法:
#   ./scripts/release-tag.sh                       # 发布当前打包版本（自动读取 changelog 顶部版本）
#   ./scripts/release-tag.sh "发布说明"             # 同上，带发布说明（第一个参数被当作说明，前提是等于 changelog 版本）
#   ./scripts/release-tag.sh 1.2.0                 # 手填版本号（必须与 changelog 顶部一致，否则报错）
#
# 保证「发布版本号 == 打包版本号」：
#   - 无参数时自动从 debian/changelog 顶部取版本（即 package.sh 最近一次打包的版本）。
#   - 传参数时强制校验与 changelog 顶部一致，不一致直接报错退出。
#
# 约定：
#   - tag 名 = v + 版本号（如 v1.2.0）
#   - 不会修改任何源码/版本号，只做 git tag + push

set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"

log() { echo "[release-tag] $*"; }
err() { echo "[release-tag][ERR] $*" >&2; }

# 版本号来源：
#   - 不传参数：自动取 debian/changelog 顶部版本（即 package.sh 最近一次打包版本），
#     保证发布号 == 打包版本号，二者天然一致。
#   - 传参数：手填版本号，强制与 changelog 顶部一致，否则报错退出。
CHANGELOG_VER="$(dpkg-parsechangelog -S version 2>/dev/null || echo '')"
if [[ -z "$CHANGELOG_VER" ]]; then
    err "无法读取 debian/changelog 版本号，请先执行 ./scripts/package.sh"
    exit 1
fi

if [[ $# -ge 1 ]]; then
    VER="$1"
    NOTE="${2:-Release $VER}"
    # 手填版本必须与打包版本一致，避免发布号与产物对不上
    if [[ "$VER" != "$CHANGELOG_VER" ]]; then
        err "传入版本 $VER 与打包版本 $CHANGELOG_VER 不一致"
        err "请使用 ./scripts/release-tag.sh（不带参数）以发布当前打包版本，"
        err "或先将 changelog 同步为 $VER（执行 ./scripts/package.sh）。"
        exit 1
    fi
else
    # 无参数：直接采用当前打包版本，说明默认 Release <版本>
    VER="$CHANGELOG_VER"
    NOTE="Release $VER"
fi

# 版本号格式校验：数字.数字.数字（允许末位带 -rc1 之类的后缀）
if [[ ! "$VER" =~ ^[0-9]+\.[0-9]+\.[0-9]+([-+].[0-9A-Za-z.~]*)?$ ]]; then
    err "版本号格式不合法: $VER （应为 x.y.z，如 1.2.0）"
    exit 1
fi

TAG="v${VER}"

# 工作区需干净（避免把未提交改动一起标记）
if [[ -n "$(git status --porcelain)" ]]; then
    err "工作区有未提交改动，请先 commit 或 stash 再打 tag："
    git status --porcelain | head
    exit 1
fi

# 已存在则报错，避免覆盖
if git rev-parse "$TAG" >/dev/null 2>&1; then
    err "tag 已存在: $TAG"
    exit 1
fi

# 打 annotated tag（含发布说明），便于追溯
git tag -a "$TAG" -m "$NOTE"
log "已打 tag: $TAG"

# 推送 tag 到 origin
git push origin "$TAG"
log "已推送 $TAG 到 origin"
log "完成. 发布版本: $TAG"
