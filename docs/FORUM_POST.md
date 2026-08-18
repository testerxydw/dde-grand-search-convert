<!--
发帖说明（请勿在论坛正文里贴这部分）：
- 版块：AI 开发实验室
- 标题：【deepin插件开发活动】万能转换器插件（dde-grand-search-convert）
- 由 AI（CodeBuddy + deepin-skills）代发，作者/开发者：testerxydw
- 发帖时请附：
  1) 本会话中 use_skill 加载 dtk-development 的截图（见本文「三、deepin-skills 调用实证」）
  2) dbus-send 联调结果截图（见本文「四、功能验证」）
  3) GitHub 仓库页截图（https://github.com/testerxydw/dde-grand-search-convert）
  4) GitHub Actions 双架构构建运行截图
-->

# 【deepin插件开发活动】万能转换器插件（dde-grand-search-convert）

> 本文由 AI（CodeBuddy + deepin-skills）代为整理发布，开发者：testerxydw。
> 项目仓库：https://github.com/testerxydw/dde-grand-search-convert

## 零、开发用时概览

### 选题来源（AI 读取活动贴后自动整理选定）

本插件的**选题方向也由 AI 自动完成**：AI 先读取 deepin 论坛活动贴
（[post/300665](https://bbs.deepin.org/post/300665)、
[post/300912](https://bbs.deepin.org/post/300912)），梳理出大赛新增的多个开发方向，
结合"蓝海程度、实用价值、创意/技术潜力"给出若干候选方向供参考；开发者从中选定
**方向 2：全局搜索「单位换算 / 汇率 / 时间」插件（dde-grand-search）**——当时 0 人提交、
竞争最小且实用价值高，随后进入设计与开发。

本项目在 **2026-08-18 当日**由 AI 借助 deepin-skills 集中完成，从需求到可发布
全流程约 **1 个工作日**（含设计、编码、丰富化扩展、打包验证、发帖物料准备）。
各阶段概览：

| 阶段 | 内容 | 产出 |
|------|------|------|
| 1. 环境准备 | 安装 deepin-skills 到 CodeBuddy | 4 个 skill 可用（dtk/shell/tray/control-center） |
| 2. 选题与调研 | **AI 读取活动贴自动整理方向并选定**；参考 macOS/Ubuntu 实用插件、出设计文档 | 候选方向列表 + `docs/DESIGN.md` 草图 + 解析策略 |
| 3. 核心开发 | 实现 V1.0 插件协议（Search/Stop/Action）、汇率/单位/时区 | M1–M6 里程碑完成，amd64 联调通过 |
| 4. 丰富化扩展 | 新增计算器/程序员工具/日期倒数/颜色 4 类能力 | 4 个 provider，覆盖办公/学生/程序员/设计师 |
| 5. 打包与集成 | deb 打包、安装、daemon 端到端联调、修复解析 bug | `convert-search-plugin_1.0.0_amd64.deb` 安装验证通过 |
| 6. 提交与发帖 | GitHub 仓库 + 双架构 CI + 发帖文档 | 仓库已推送、CI 双架构构建、本文档 |

> 说明：以上为单日集中开发的阶段划分，体现 AI 辅助下从选题到可发布的高效迭代。

## 一、这是什么

一个面向 DDE 全局搜索（dde-grand-search）的扩展插件，让用户在全局搜索栏直接完成
**7 类高频轻量查询**，覆盖办公党、学生/科研、程序员、设计师各类人群：

| 能力 | 适用人群 | 示例 |
|------|---------|------|
| 汇率换算 | 所有人 | `100usd`、`100 USD to CNY`（联网+缓存+静态降级） |
| 单位换算 | 学生/科研 | `12inch`、`1kg=?斤`、`100F` |
| 跨时区时间 | 出差/远程 | `北京时间`、`tokyo now` |
| 科学/程序员计算器 | 学生/程序员 | `12*8+sqrt(16)`、`255 to hex`（整数附 HEX/BIN/OCT） |
| 程序员工具 | 程序员 | `base64 encode hello`、`md5 hello`、`时间戳`、`ascii A` |
| 日期/倒数日 | 办公党 | `距 2027-01-01 还有几天`、`2025-01-01 到 2025-12-31` |
| 颜色转换 | 设计师 | `#ff8800` ↔ `rgb(255,136,0)` ↔ `hsl(32,100%,50%)` |

灵感来自 macOS 聚焦搜索的单位/汇率/计算能力、Ubuntu Calculator 的进制与表达式，
聚合为 deepin 生态当前缺失的一站式转换器。

## 二、使用说明

### 安装

```bash
# 方式一：deb 包安装（推荐）
sudo dpkg -i convert-search-plugin_1.0.0_amd64.deb

# 方式二：从源码构建
cmake -B build -DCMAKE_INSTALL_PREFIX=/usr
cmake --build build -j$(nproc)
sudo cmake --build build --target install
```

### 启用

```bash
# 重启 dde-grand-search-daemon 以加载插件（Mode=Auto，首次搜索自动拉起）
killall dde-grand-search-daemon && dde-grand-search-daemon &
```

### 使用

直接在 **DDE 全局搜索栏** 输入下面的查询即可，结果以卡片展示，**点击即复制到剪贴板**：

```
100usd                          汇率：100 USD = 约 725 CNY
1kg=?斤                         单位：1 kg = 2 斤
北京时间                        时区：北京当前时间 + 本地时差
12*8+sqrt(16)                   计算器：100（附 HEX/BIN/OCT）
255 to hex                     进制：0xff
base64 encode hello            程序员工具：aGVsbG8=
md5 hello                       程序员工具：5d41402abc4b2a76b9719d911017c592
时间戳                         程序员工具：当前 Unix 秒 / 毫秒
ascii A                        程序员工具：'A' = ASCII 65
距 2027-01-01 还有几天          日期：还有 136 天
#ff8800                        颜色：RGB rgb(255,136,0) / HSL hsl(32,100%,50%)
```

> 提示：汇率功能联网获取实时汇率（失败自动降级到内置静态表，离线也可用）；
> 其余 6 类功能全部本地计算，零网络依赖。

## 三、deepin-skills 调用实证（活动硬性要求）

本插件由 AI 借助已安装的 **deepin-skills** 完成开发。

### 3.1 deepin-skills 安装事实（可复现）

```bash
$ ls ~/.codebuddy/skills/
dde-control-center-development/  dde-shell-development/
dde-tray-development/            dtk-development/
```

安装来源：GitHub `linuxdeepin/deepin-skills`（经镜像下载，官方 `scripts/install.sh`
+ `SKILL_HOME=~/.codebuddy` 安装）。

### 3.2 AI 调用 deepin Skill 实证（use_skill）

本会话通过 `use_skill` 工具**实际加载**了 `dtk-development` skill，返回其完整
开发指南（DTK 架构、Qt5/Qt6 兼容、DBus/应用入口文档路由表等）。本项目正是基于该
skill 的 **DBus / Qt 应用开发约束** 完成的 dde-grand-search 插件。

调用记录（节选）：

```
use_skill: dtk-development
→ commandMessage: "The dtk-development skill is loading"
→ Base directory: /home/dp25/.codebuddy/skills/dtk-development
→ 返回 DTK 开发指南正文（架构/CMake/DBus/主题等路由表）
```

### 3.3 开发过程如何依托 skill

1. 通过 `use_skill` 加载 `dtk-development` skill，获取 DTK/Qt/DBus 构建规范。
2. 遵循 dde-grand-search 官方 V1.0 插件协议与 `calculator-search-plugin` 示例
   实现 `SearchPlugin` 接口。
3. 完整开发过程均基于 deepin-skills 框架约束（CMake 兼容、Qt 版本探测、DBus 通信）。

> 说明：deepin-skills 官方仓库当前含 4 个 skill（control-center / shell / tray /
> dtk），**暂未提供 dde-grand-search 专属 skill**；本插件调用了其中的
> `dtk-development` 框架 skill 作为开发依据，并严格遵循 dde-grand-search 官方协议。

## 四、技术实现

- 遵循 dde-grand-search 官方 **V1.0 插件协议**：独立 DBus 服务进程，实现
  `Search` / `Stop` / `Action` 三方法，JSON 报文（ver/mID/cont/action/item）。
- DBus 服务名 `org.deepin.grandsearch.convert`，配置文件安装到
  `/usr/lib/<triplet>/dde-grand-search-daemon/plugins/searcher/`。
- 架构：`QueryParser` 解析类型 → 多 `provider`（汇率/单位/时区/计算器/程序员工具/
  日期/颜色）→ `ResultBuilder` 组装 V1.0 JSON。
- 计算器采用**安全递归下降求值器**（禁用 eval），避免表达式注入风险。
- Qt5/Qt6 自动探测；汇率联网失败时自动降级到内置静态表，离线可用。
- 除汇率外，其余 6 类功能**全部本地计算，零网络依赖、零隐私上传**。

## 五、构建与验证

- 本地 amd64 编译通过，并打包为 `convert-search-plugin_1.0.0_amd64.deb`。
- **端到端集成验证**：安装 deb → 重启 daemon → 触发搜索 → daemon 自动拉起插件
  → `org.deepin.grandsearch.convert` 服务注册 → 查询返回正确结果卡片。
- 已配置 **GitHub Actions 双架构自动构建**：`.github/workflows/build.yml`
  同时构建 amd64（原生）与 arm64（QEMU 容器），作为活动加分项验证（截图见附件④）。

| 查询 | 结果 |
|------|------|
| `100usd` | 实时汇率换算（联网成功） |
| `1kg=?斤` | 2 斤 |
| `北京时间` | Asia/Shanghai 当前时间 + 时差 |
| `12*8+sqrt(16)` | 100 + HEX 0x64 / BIN 0b1100100 / OCT 0o144 |
| `255 to hex` | 0xff |
| `base64 encode hello` | aGVsbG8= |
| `md5 hello` | 5d41402abc4b2a76b9719d911017c592 |
| `时间戳` | Unix 秒 / 毫秒 |
| `距 2027-01-01 还有几天` | 还有 136 天 |
| `#ff8800` | RGB rgb(255,136,0) / HSL hsl(32,100%,50%) |

## 六、许可证与参与

- 许可证：**GPL-3.0-or-later**（与 dde-grand-search 示例一致，满足活动开源要求）。
- 仓库：https://github.com/testerxydw/dde-grand-search-convert
- 欢迎提 Issue / PR，扩充更多币种、城市、单位与功能。

---
*代发声明：本帖内容由 AI（CodeBuddy 调用 deepin-skills）辅助生成与整理，
开发者 testerxydw 对所有技术实现与发布内容负责。*
