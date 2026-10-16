# 重新设计方案 · 贴合 dde-grand-search 官方仓库规范

> 依据官方仓库 `dde-grand-search` 的实际工程实践（构建/打包/CI/许可/国际化/示例插件），
> 对本插件（功能见 [FEATURES.md](FEATURES.md)）做整体重新设计。
> 功能设计（解析策略、交互、Roadmap）见 [DESIGN.md](DESIGN.md)，本文聚焦**工程规范对齐与落地路径**。

## 1. 目标与原则

**目标**：把 convert 插件从"能用的第三方插件"升级为"可贡献到官方 `dde-grand-search` 仓库
`examples/` 的一等示例"，即官方规范形态。

**原则**：
1. 功能行为不变（8 类转换 + help + 点击复制 + UOS AI），只做规范对齐与结构优化
2. 每个规范点都以官方仓库**实际存在的文件/写法**为依据，不臆造规范
3. 分阶段落地，每阶段可独立验证、可回滚

## 2. 官方规范依据（调研摘要）

| 维度 | 官方仓库实践 | 依据文件 |
|------|-------------|---------|
| 构建 | CMake ≥3.10、C++17、AUTOMOC；Qt6 优先回退 Qt5（Core/Gui/DBus）；`GNUInstallDirs` 安装到 `${CMAKE_INSTALL_FULL_LIBDIR}/dde-grand-search-daemon/plugins/searcher`；conf 由 `.in` 模板 `configure_file` 生成 | `examples/calculator-search-plugin/CMakeLists.txt` |
| 入口模式 | `QCoreApplication` + `SearchPluginAdaptor`（`QDBusAbstractAdaptor` 转发 search/stop/action）+ `registerService`/`registerObject` | `examples/.../main.cpp`、`searchpluginadaptor.cpp` |
| 源码头 | `// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.` + `// SPDX-License-Identifier: GPL-3.0-or-later` | 各 `.cpp/.h` 头部 |
| REUSE | `LICENSES/` 目录（GPL-3.0-or-later / CC-BY-4.0 / CC0-1.0 / MIT / LGPL-3.0-or-later）+ `.reuse/dep5`：源码 GPL-3.0、构建文件与 README CC-BY-4.0、debian 文件 CC0-1.0 | `.reuse/dep5` |
| Debian | `dh` 全量 + 限定并行度 + `override_dh_auto_configure` 传 CMake 参数（Release、前缀、版本号） | `debian/rules` L21-33 |
| CI | `cppcheck.yml`（linuxdeepin/action-cppcheck，PR target）、`call-clacheck.yml`（CLA）、`call-commitlint.yml`、`call-license-check.yml`、`call-update-changelog.yml` | `.github/workflows/` |
| 国际化 | `lupdate.sh` 生成 `.ts`、`.tx/`（transifex）托管、`translations/` 按组件目录存放 | `lupdate.sh`、`.tx/config` |
| 命名 | 内置搜索项 `com.deepin.dde-grand-search.*`（7 项，插件禁用冲突）；指南推荐插件 `Name` 用 `com.company.app.xxx` 反向域名 | `docs/plugin-development-guide.md` §内置搜索项 |

## 3. 差距分析

| # | 项 | 现状 | 官方规范 | 差距等级 |
|---|----|------|---------|---------|
| G1 | 插件命名空间 | `Name/DBusService = org.deepin.grandsearch.convert` | 第三方用自有反向域名（`com.company.app.xxx`）；`org.deepin.*` 属官方保留 | **高** |
| G2 | REUSE 体系 | 源码头有 SPDX，但无 `LICENSES/` 目录、无 `.reuse/dep5` | LICENSES + dep5 全覆盖 | **高**（license-check CI 直接相关） |
| G3 | 国际化机制 | `i18n.cpp` 硬编码中/英词表 | `.ts` + `lupdate.sh` + `.tx` 平台托管 | **高** |
| G4 | CI 规范 | 自建 `build.yml`（双架构）+ `release.yml` | cppcheck / clacheck / commitlint / license-check / update-changelog | **中** |
| G5 | 入口/Adaptor | 自写 `searchpluginadaptor`（接口名 `org.deepin.grandsearch.convert.SearchPlugin`） | 官方 XML `qdbusxml2cpp` 生成（已做），但接口名随 G1 需同步 | 低 |
| G6 | Debian 规范 | debhelper 11、自维护 changelog | 对齐官方 rules 传参模式；接 `call-update-changelog` | 中 |
| G7 | 测试 | 自建 run_tests.sh，8 类 provider 均有基础用例但缺边界（汇率降级/未识别反向/calc 深嵌套） | 官方无单测框架（自有实践可保留），需补边界 + dbus 验收矩阵 | 中 |
| G8 | 入口进程 | `QGuiApplication`（剪贴板需要） | 官方示例 `QCoreApplication` | 低（需说明差异合理性） |
| G9 | CMake 细节 | 声明了 `AUTORCC`（无资源，冗余）；额外依赖 `Network`（汇率需要） | 与官方 example 完全对齐 | 低 |

## 4. 重新设计总览

### 4.1 定位与两条路径

- **路径 A（本仓库，规范形态）**：以官方规范重构本仓库，作为可独立发布的第三方插件
- **路径 B（上游合入）**：以 `examples/convert-search-plugin` 形式提交 PR 到官方仓库
  （官方已有 `examples/calculator-search-plugin` 先例）。本文设计兼容两路径，A 是 B 的前置。

### 4.2 目标架构

```
用户输入(全局搜索栏)
      │  DBus (V1.0 JSON)
      ▼
dde-grand-search-daemon ──► convert-search-plugin (独立 DBus 服务)
                                │
                                ├─ QueryParser   类型判定 + 参数抽取（触发词最长优先）
                                ├─ Providers ×8  currency/unit/time/calc/program/datetime/color/energy
                                ├─ I18n          词表 + .ts 翻译（跟随系统语言）
                                ├─ Clipboard     点击复制（openitem Action）
                                ├─ UosAI         可选联动（容错，缺失零影响）
                                ├─ ResultBuilder 组装 V1.0 JSON
                                └─ SearchPluginAdaptor  官方 XML 生成
      │  DBus (V1.0 JSON)
      ▼
全局搜索结果界面（分组卡片）
```

### 4.3 目标目录树（对齐官方 example 布局）

```
convert-search-plugin/
├── LICENSES/                          # G2：REUSE 许可证文本
│   ├── GPL-3.0-or-later.txt
│   ├── CC-BY-4.0.txt
│   └── CC0-1.0.txt
├── .reuse/dep5                        # G2：版权/许可声明（对齐官方 dep5 分类）
├── .github/workflows/
│   ├── build.yml                      # 保留：amd64 原生 + arm64 QEMU
│   ├── release.yml                    # 保留：tag/关键词触发
│   └── cppcheck.yml                   # G4：新增，对齐官方静态检查
├── debian/                            # G6：对齐官方打包规范
├── docs/  (FEATURES.md / DESIGN.md / REDESIGN.md ...)
├── providers/                         # 8 个 provider（不变）
├── tests/                             # 单测 + dbus 验收脚本
├── CMakeLists.txt                     # G9：对齐官方写法
├── convert-search.conf.in             # G1：命名空间修正
├── main.cpp                           # 对齐官方入口模式
├── convertsearch.h/.cpp  queryparser.*  resultbuilder.*
├── i18n.h/.cpp  clipboard.*  uosai.*  searchpluginadaptor.*
└── README.md / README.zh_CN.md        # 双语 + 徽章（对齐官方 README 风格）
```

## 5. 详细设计

### 5.1 命名与 DBus 规范（G1，最高优先）

**问题**：`org.deepin.*` 命名空间按 D-Bus 惯例属 deepin 官方，第三方插件占用会与官方未来
服务冲突，也不符合官方指南"推荐 `com.company.app.xxx`"的要求。

**设计**（自有域名 + 官方示例风格）：

| 字段 | 现值 | 新值 |
|------|------|------|
| `Name` | `org.deepin.grandsearch.convert` | `io.github.testerxydw.convert` |
| `DBusService` | `org.deepin.grandsearch.convert` | `io.github.testerxydw.convert` |
| `DBusAddress` | `/org/deepin/grandsearch/convert` | `/io/github/testerxydw/convert` |
| `DBusInterface` | `org.deepin.grandsearch.convert.SearchPlugin` | `io.github.testerxydw.convert.SearchPlugin` |
| `Mode/Priority` | `Auto / 1` | 不变（Middle 守护最合适常驻转换服务） |
| `InterfaceVersion` | `1.0` | 不变 |

- 修改点集中在 3 处：`convert-search.conf.in`（Name/DBusService/DBusInterface）、
  `main.cpp`（`DBUS_SERVICE_NAME` 宏）、`searchpluginadaptor.h`（`Q_CLASSINFO("D-Bus Interface", …)`）；
  `README.md`/docs 同步更新引用；二进制名 `convert-search-plugin` 不变
- **路径 B**：若上游合入，Name 由官方定（预期 `com.deepin.dde-grand-search.convert` 或
  examples 的 `com.example.*` 风格），本文以 `io.github.*` 为路径 A 默认值
- **迁移**：conf 文件名 `convert-search.conf` 不变，覆盖安装即生效；`postinst` 中清理
  旧 DBus 名残留进程（`killall convert-search-plugin`）无需特殊处理，daemon 重启即重载

**校验**：不与内置 7 项（`com.deepin.dde-grand-search.file-deepin` 等）冲突 ✓

### 5.2 源码规范：SPDX + REUSE（G2）

1. **所有源文件**统一头部（对齐官方格式，署名替换为仓库维护者）：

```cpp
// SPDX-FileCopyrightText: 2026 testerxydw
//
// SPDX-License-Identifier: GPL-3.0-or-later
```

2. **`.reuse/dep5`**（分类对齐官方）：

```
Format: https://www.debian.org/doc/packaging-manuals/copyright-format/1.0/
Upstream-Name: convert-search-plugin
Source: https://github.com/testerxydw/dde-grand-search-convert

Files: *.cpp *.h providers/*.cpp providers/*.h tests/*.cpp
Copyright: 2026 testerxydw
License: GPL-3.0-or-later

Files: CMakeLists.txt *.in docs/*.md
Copyright: 2026 testerxydw
License: CC-BY-4.0

Files: debian/*
Copyright: 2026 testerxydw
License: CC0-1.0
```

3. **`LICENSES/`** 放入 GPL-3.0-or-later.txt、CC-BY-4.0.txt、CC0-1.0.txt 三份文本
   （与实际使用的许可一致，不引入未用的 MIT/LGPL）

### 5.3 解析引擎（QueryParser）——功能不变，规则固化

保持现有能力，设计上固化两点（代码已有行为，补文档与测试锚定）：

1. **优先级链**（判定顺序，先命中先得分，歧义由触发词兜底）：

```
显式触发词（最长前缀优先）
  → 货币符号/代码+数字（usd100 / 美元100 / $50）
  → 温度后缀 C/F
  → 颜色串（#hex / rgb() / hsl() / 色名）
  → 编码/哈希关键词（base64/url/md5/hash/ascii/时间戳）
  → 单位词（inch/kg/斤/…）
  → 城市/时区词（北京时间 / tokyo now）
  → 日期形态（YYYY-MM-DD / X月X日 / 今天±N天）
  → 计算器表达式（运算符/函数/进制）
  → 兜底：未识别提示卡（不静默）
```

2. **性能护栏**：全 provider 同步本地计算（<10ms），唯一网络路径为汇率（见 §5.6）；
   输入文本设长度上限（建议 256 字符），正则避免回溯爆炸

### 5.4 Provider 层——仅做对齐性微调

| 项 | 设计 |
|----|------|
| 接口 | 保持静态 `run(cont) → 结果对象` 的现有形态，不引入新抽象 |
| CalcProvider | 保留手写递归下降求值器（禁 eval），补齐输入长度/深度上限 |
| CurrencyProvider | 网络部分抽离到 §5.6 策略；其余不变 |
| 错误路径 | 三类错误统一：解析失败→help 提示卡；计算错误→友好错误卡；网络失败→静态降级说明 |
| 每组上限 | 结果组装时 `ResultBuilder` 截断到 100 项/组（协议硬限，主动截断优于被动忽略） |

### 5.5 国际化：对齐官方 `.ts` + `.tx` 流程（G3）

**现状问题**：`i18n.cpp` 硬编码中/英两列词表，第三方翻译者无法贡献，也不进 transifex。

**分两阶段迁移**：

- **P0（兼容态）**：保留 `i18n.cpp` 集中式词表（已有测试覆盖），不动
- **P1（对齐态）**：
  1. 词表键改为英文基准串，值改走 `QT_TR_NOOP` / `tr()`；运行时按系统语言自动取译文
  2. 新增 `translations/convert-search-plugin_zh_CN.ts`（`.ts` 格式，对齐官方
     `grand-search-shell-plugin/translations/` 的组织方式）
  3. 新增根级 `lupdate.sh`（对齐官方同名脚本：`lupdate src -ts translations/*.ts` + `lrelease`）
  4. `.ts` 资源经 `AUTORCC`（此时才有真实用途）打进二进制，`QTranslator::load(":/i18n/…")`
  5. 组名、币种名、单位名、标签、说明全部可翻译；`i18n.cpp` 退化为「tr() 门面」，调用方不变

### 5.6 网络（汇率）与隐私

- **API**：仅 `https://api.frankfurter.app/latest`（公开只读、无鉴权、无用户数据上传）
- **策略**：请求超时 ≤3s → TTL 缓存（内存 QHash + 过期时间）→ 失败降级内置静态表
- **Stop 语义**：`Stop(mID)` 时若该 mID 的汇率请求在途，通过请求取消标记使 `Search` 尽快返回
  （协议要求 Search 可中断；本地计算类天然满足）
- **隐私声明**：README 增加"除汇率外全部本地计算"的隐私说明（已有，保留）

### 5.7 剪贴板与 UOS AI

| 项 | 设计 |
|----|------|
| 剪贴板 | 保留 `QGuiApplication` + offscreen 回退（headless/CI 可启动）；文档说明与官方示例 `QCoreApplication` 的差异原因（Qt clipboard 需要平台插件），这是合理偏离 |
| UOS AI | 保持「全部容错、缺失零影响」；接口探测集中在 `uosai.cpp` 单文件，不外溢；`mID→cont` 缓存仅保留最近 N 条（建议 32），防长会话内存增长 |

### 5.8 CMake 构建（G9）

对齐官方 `examples/calculator-search-plugin/CMakeLists.txt` 写法，仅保留必要差异：

```cmake
cmake_minimum_required(VERSION 3.10)
project(convert-search-plugin)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_AUTOMOC ON)
# AUTORCC 仅在 P1 引入 .ts 资源后保留（G9：当前无资源，删除）
```

- Qt 探测块与官方逐字对齐；组件 `Core Gui DBus Network`（官方无 Network，此处为汇率必要依赖，注释说明）
- 安装路径：沿用 `GNUInstallDirs` + `CONVERT_PLUGIN_DIR` 覆盖变量 + `.in` 模板生成 conf
  （与官方 `CALCULATOR_PLUGIN_DIR` 模式一致，已达标）

### 5.9 Debian 打包（G6）

对齐官方 `debian/rules` 模式（`dh` + 限定并行度 + `override_dh_auto_configure` 传参）：

- `override_dh_auto_configure`：`-DCMAKE_BUILD_TYPE=Release`
  `-DCMAKE_INSTALL_PREFIX=/usr`，并行度按构建机 CPU 限制（防打包卡桌面）
- `control`：保留现有 `Depends: dde-grand-search`（强依赖合理，插件无 daemon 无意义）；
  `Standards-Version` 升至 4.5.1 不变；补 `Homepage` 字段（官方有）
- `copyright`：与 `.reuse/dep5` 内容一致，单一事实来源以 dep5 为准
- `changelog`：接官方 `call-update-changelog` 流程后自动维护（路径 B）；路径 A 继续手动

### 5.10 CI/CD（G4）

**路径 A（独立仓库）**：

| 工作流 | 设计 |
|--------|------|
| `build.yml` | 保留（amd64 原生 + arm64 QEMU，双 Qt 矩阵可后补），加单测步骤（先跑 `tests/run_tests.sh` 再打包） |
| `cppcheck.yml` | 新增，逐字对齐官方（`linuxdeepin/action-cppcheck`，PR target 触发） |
| `release.yml` | 保留，发布物加 SHA256SUMS（已有） |
| `call-clacheck/commitlint` | **不启用**：CLA 仅对 deepin 组织有意义，非组织仓库会卡审批；提交规范自守（conventional commits） |

**路径 B（上游 PR）**：删除自建 `release.yml`，启用官方全套
`cppcheck + call-clacheck + call-commitlint + call-license-check + call-update-changelog`。

### 5.11 测试与验收（G7）

1. **进程内单测**（`tests/run_tests.sh`，保留自建——官方无测试框架，属合理超出）：
   - 现有基础用例已覆盖 8 类 provider；补齐边界：currency 离线降级路径 / 未识别输入返回提示卡（反向验证）/
     calc 除零、√负数、超深嵌套 / time 未知城市
2. **DBus 验收矩阵**（`tests/dbus_acceptance.sh` 新增）：

```bash
# Search / Stop / Action 三接口 ×（汇率/单位/时间/计算/编码/日期/颜色/热量/help/未识别）
dbus-send --session --print-reply --dest=io.github.testerxydw.convert \
  /io/github/testerxydw/convert io.github.testerxydw.convert.SearchPlugin.Search \
  "string:{\"ver\":\"1.0\",\"mID\":\"t1\",\"cont\":\"100usd\"}"
```

3. **静态检查**：cppcheck 零新增告警
4. **手工验收**：daemon 重启 → 搜索界面 10 类输入逐条目视（README §六）

## 6. 上游贡献路径（路径 B 操作要点）

1. 前置：本文 §5 全部完成、CI 绿、`dconf`/许可检查通过
2. 目录迁移：整仓库内容 → 官方仓库 `examples/convert-search-plugin/`（保持相对结构）
3. README 改写为官方 examples 风格（对齐 `examples/calculator-search-plugin/README.md`）
4. 官方侧适配：`Name` 由官方定名；`.reuse/dep5` 并入官方 dep5；翻译并入官方 `.tx/config`
5. PR 引用 `docs/plugin-development-guide.md`，声明遵循 V1.0 协议与内置项命名检查

## 7. 分阶段落地计划

| 阶段 | 内容 | 涉及文件 | 验证 | 回滚 |
|------|------|---------|------|------|
| **P0 规范对齐**（不动功能） | ① G1 命名空间修正；② G2 LICENSES/ + dep5 + 头部统一；③ G4 cppcheck.yml；④ G9 CMake 清理（去 AUTORCC）；⑤ debian 补 Homepage | `conf.in`、`main.cpp`、`LICENSES/`、`.reuse/dep5`、`.github/workflows/`、`CMakeLists.txt`、`debian/control` | 构建 + `dbus-send` 新旧服务名验收 + cppcheck 绿 | 纯文件回退，`git revert` 单提交粒度 |
| **P1 工程重构**（功能不变） | ① i18n 迁移 tr()/.ts + lupdate.sh（§5.5）；② Stop 取消令牌；③ UOS AI 缓存上限；④ 单测补全 + dbus 验收脚本 | `i18n.*`、`convertsearch.cpp`、`uosai.cpp`、`translations/`、`tests/` | run_tests.sh 全绿 + dbus 矩阵 + 界面目视 10 类输入 | 每小点一个提交，逐点回退 |
| **P2 上游贡献** | §6 全流程 | 官方仓库 PR | 官方 CI 全套绿 | 路径 A 仓库继续独立维护，PR 不合并不影响 |

提交纪律：一个提交只解决一个规范点；P0 每项改动后必须可构建、可 `dpkg -i` 验证。

## 8. 风险与对策

| 风险 | 等级 | 对策 |
|------|------|------|
| DBus 名变更导致已装用户旧配置残留 | 低 | conf 同名覆盖即生效；发布说明注明需重启 daemon；路径 B 由官方定名 |
| Qt5 回退路径回归（i18n 迁移用了 Qt6 独有 API） | 中 | `.ts`/`tr()`/`QT_TR_NOOP` 均为 Qt5/6 通用 API；CI 补 Qt5 矩阵验证 |
| frankfurter.app 长期不可用 | 中 | TTL 缓存 + 静态表兜底（已有）；可再加第二 API 源轮询（P1+，可选） |
| CLA/commitlint 工作流在非 deepin 组织仓库不可用 | 低 | 路径 A 不启用（§5.10），路径 B 自动获得 |
| cppcheck 对现有代码报存量告警 | 低 | 先跑基线，仅阻断"新增"告警 |

## 9. 与既有文档的关系

- [FEATURES.md](FEATURES.md)：功能盘点（本文 §5.3/§5.4 的行为基准）
- [DESIGN.md](DESIGN.md)：产品功能设计与 Roadmap（本文不重复）
- [IMPLEMENTATION.md](IMPLEMENTATION.md)：上一轮实现记录（历史参考）
