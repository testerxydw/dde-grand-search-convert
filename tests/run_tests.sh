#!/usr/bin/env bash
# SPDX-FileCopyrightText: 2026 testerxydw
# SPDX-License-Identifier: GPL-3.0-or-later
#
# 运行单元测试：构建项目后将测试与编译产物链接执行。
# 用法: ./tests/run_tests.sh
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"

BUILD="$ROOT/build/test"
mkdir -p "$BUILD"

echo "[test] 配置并编译主项目..."
cmake -B "$BUILD" -DCMAKE_BUILD_TYPE=Debug >/dev/null 2>&1
cmake --build "$BUILD" -j"$(nproc)" >/dev/null 2>&1

echo "[test] 链接并编译测试..."
# 收集所有 .o 目标文件
OBJS=()
while IFS= read -r f; do OBJS+=("$f"); done < <(find "$BUILD/CMakeFiles/convert-search-plugin.dir" -name '*.o')

LIBS="$(pkg-config --libs Qt6Core Qt6Gui Qt6DBus Qt6Network)"
INCS="$(pkg-config --cflags Qt6Core Qt6Gui Qt6DBus Qt6Network)"

g++ -std=c++17 "$ROOT/tests/test_main.cpp" \
    -I"$ROOT" $INCS \
    "${OBJS[@]}" $LIBS \
    -o "$BUILD/test_main"

echo "[test] 运行..."
"$BUILD/test_main"
