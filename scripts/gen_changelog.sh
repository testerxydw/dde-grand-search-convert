#!/usr/bin/env bash
# SPDX-FileCopyrightText: 2026 testerxydw
# SPDX-License-Identifier: GPL-3.0-or-later
#
# 生成 changelog：取上一个 tag 到当前 HEAD 的提交记录。
# 用法: ./scripts/gen_changelog.sh [tag_or_ref]
#   - 无参数: 取最近一个 tag..HEAD
#   - 有参数: 取该 ref..HEAD
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"

REF="${1:-}"
if [[ -z "$REF" ]]; then
    REF="$(git describe --tags --abbrev=0 2>/dev/null || echo '')"
fi

echo "# Changelog"
echo
if [[ -z "$REF" ]]; then
    echo "## 全部提交"
    git log --pretty=format:"- %s (%h)" 
else
    echo "## 自 $REF 起"
    git log "${REF}..HEAD" --pretty=format:"- %s (%h)"
fi
echo
