# DDE 全局搜索 · 万能转换器 插件

> DDE Grand Search · All-in-One Converter (currency / unit / time / calc / dev / date / color / energy)

[![Build amd64+arm64](https://github.com/testerxydw/dde-grand-search-convert/actions/workflows/build.yml/badge.svg)](https://github.com/testerxydw/dde-grand-search-convert/actions/workflows/build.yml)
[![License: GPL-3.0](https://img.shields.io/badge/license-GPL--3.0-blue.svg)](LICENSE)

- 仓库 / Repository: https://github.com/testerxydw/dde-grand-search-convert
- 基于 deepin-skills 的 dde-grand-search 插件开发指南 V1.0 完成。

一个为 DDE 全局搜索（dde-grand-search）开发的扩展搜索插件。面向**各类人群**：办公党、
科研/学生、程序员、设计师、减肥人群都能用。在全局搜索栏输入自然语言式查询，插件实时
解析并返回结果卡片。参考 macOS Spotlight、Ubuntu Calculator 等系统的实用搜索能力，集成
了 8 类高频轻量查询。

> **一句话上手**：在搜索栏输入 `help`（或 `?` / `使用说明`）即可查看全部功能与示例；
> 任意结果**点击即复制到剪贴板**。

---

## 一、功能速览 / Features

| 功能 | 适用人群 | 典型输入 | 说明 |
|------|---------|---------|------|
| 汇率换算 | 所有人 | `100usd`、`100 USD to CNY`、`100美元` | 实时汇率+缓存+静态降级 |
| 单位换算 | 学生/科研 | `12inch`、`1kg=?斤`、`3km`、`100F` | 长度/重量/温度/面积/体积/速度/数据 |
| 时区时间 | 出差/远程 | `北京时间`、`tokyo now`、`纽约时间` | 城市当前时间+与本地时差 |
| 科学/程序员计算器 | 学生/程序员 | `12*8+sqrt(16)`、`255 to hex` | 表达式+进制+位运算，整数附 HEX/BIN/OCT |
| 程序员工具 | 程序员 | `base64 encode hello`、`md5 hello`、`时间戳`、`ascii A` | 编解码/哈希/ASCII/时间戳 |
| 日期/倒数日 | 办公党 | `距 2027-01-01 还有几天`、`2025-01-01 到 2025-12-31`、`今天` | 日期差/倒数/星期 |
| 颜色转换 | 设计师 | `#ff8800`、`rgb(255,136,0)`、`hsl(32,100%,50%)` | HEX/RGB/HSL 三向互转 |
| 热量转换 | 减肥/控卡 | `100 kcal`、`500 千焦` | 千卡/千焦/卡互转 + 食物份数参考 |

### 通用交互 / Common
- **分组卡片**：结果按类型分组成卡片，每组可含多条。
- **点击复制**：点击任意结果即复制到系统剪贴板（复用官方 Action 模式）。
  复制由插件在收到前端 `openitem` Action 时直接写入系统剪贴板完成，效果等同于 Ctrl+C；
  插件运行在独立 DBus 服务中，无法拦截搜索框的 Ctrl+C 键盘事件，故统一以"点击即复制"实现。
- **触发词（显式指令）**：任意查询前可加**触发词**强制走对应转换，避免与其他规则歧义；
  触发词支持中/英文，后接 `:` / `：` / `?` / 空格 任一分隔符。无触发词时仍按特征自动识别。
  例如 `汇率 100usd`、`convert 1kg 斤`、`转时间戳 2026-08-20 15:03:28`、`date 2025-01-01 到 2025-12-31`。
  触发词表见下文各功能语法开头。
- **省略目标 = 同类全列**：单位/货币/热量三类转换中，**省略 `=` 后的目标单位即列出同类全部可换算单位**（不舍上限，可展开）。
  例如 `100kg` → 斤/两/g/lb/oz/吨 全列；`100usd` → 全部 15 币种；`100kcal` → 千焦/卡/焦耳 + 食物份数。
  指定目标仍精确单条：`100kg=斤`、`100 USD to CNY`。
- **国际化说明**：所有结果跟随**系统语言**附加说明，例如
  - 中文：`3 km（千米/公里） = 3000 m（米）`、`100 USD（美元） = 725 CNY（人民币）`
  - 英文：`3 km (kilometer) = 3000 m (meter)`、`100 USD (US Dollar) = 725 CNY (Chinese Yuan)`
  （数值自动去除无意义尾零，如 `100kg=斤` 显示 `200` 而非 `200.0000`）
- **零网络依赖**：除汇率外，其余 7 类功能全部本地计算，不涉及隐私上传。

---

## 二、在搜索框里查看使用说明 / Inline Help

无需记忆语法。直接在 DDE 全局搜索栏输入以下任一内容，即可看到全部功能与示例卡片：

```
help       ?        ？        使用说明      帮助      用法      怎么用      功能
```

返回示例（中文环境）：

```
万能转换使用说明
├─ 汇率：100usd / 100 USD to CNY（省略目标=全币种）
├─ 单位：12inch / 3km / 100kg=?斤（省略目标=同类全列）
├─ 时区：北京时间 / tokyo now / 时间（无城市=世界时钟）
├─ 计算器：12*8+sqrt(16) / 255 to hex / 255 进制（全进制）
├─ 程序员工具：base64 encode hello / hash hello（MD5/SHA1/SHA256）/ 时间戳 / ascii A
├─ 日期：2025-01-01 到 2025-12-31 / 今天加30天 / 2026-08-20 星期几
├─ 颜色：#ff8800 / 红色 / 颜色 随机 / rgb(255,136,0)
├─ 热量：100 kcal（→ 千焦/卡 + 食物份数）
└─ 提示：结果点击即复制到剪贴板；省略目标单位即列出同类全部可换算单位
```

> 若输入无法识别的内容，插件会提示「未识别此查询，输入 help 查看全部功能与示例」，
> 而不是静默无结果。

---

## 三、各功能语法详解 / Syntax

### 1. 汇率换算（Currency）
- **触发词**：`汇率` / `换汇` / `currency` / `exchange`
- `100usd` / `$50` / `100美元` / `人民币100`
- 目标币种：`100 USD to CNY`、`100usd=?cny`、`100usd cny`
- **省略目标 = 全币种**：`100usd` / `100 USD` → 全部 15 个支持币种（不舍上限，可展开）。
- 支持符号 `$ € ¥ £` 与代码 `USD CNY EUR JPY GBP HKD KRW RUB AUD CAD SGD THB INR TWD MOP`
- 联网获取（公开 API），失败回退内置静态汇率表（覆盖全部 15 币种），本地缓存带 TTL。

### 2. 单位换算（Unit）
- **触发词**：`换算` / `单位` / `convert` / `unit`
- 长度：`12inch`、`3km`、`1 mile`、`5 cm`
- 重量：`1kg=?斤`、`2lb`、`500g`
- 温度：`100F`、`37C`、`32 F to C`
- 面积/体积/速度/数据：`1 acre`、`2 gal`、`100 kmh`
- **省略目标 = 同类全列**：`100kg` / `3km` → 该类别（重量/长度…）全部可换算单位（不舍上限，可展开）。
- **目标单位写法（任选）**：`1kg to 斤` / `1kg=?斤` / `1kg 斤`
- 纯本地、零网络依赖。

### 3. 时区时间（Time）
- **触发词**：`时间` / `时区` / `time` / `timezone` / `now` / `现在几点`
- `北京时间`、`tokyo now`、`纽约时间`、`what time in london`
- **无城市词 → 世界时钟全列**：`时间` / `now` / `时区` → 北京/东京/纽约/伦敦/巴黎/悉尼当前时间并排本地时刻。
- 指定城市 → 单条 + 并排"本地此刻"；支持多城市：`东京 纽约 伦敦`。
- 内置城市→IANA 时区映射。

### 4. 科学 / 程序员计算器（Calc）
- **触发词**：`计算` / `算` / `calc` / `calculate` / `进制` / `base`
- 表达式：`12*8+sqrt(16)`、`(2+3)^2`、`log(100)`、`sin(pi/2)`
- 进制转换：`255 to hex`、`0xff+1`、`100 to bin`
- **进制一次全给**：`255 进制` / `255 base` → DEC/HEX/BIN/OCT 四条。
- 整数结果附 HEX / BIN / OCT；小数结果附科学计数法 + 百分比。
- 失败（除零/√负数/未知函数）→ 友好错误提示卡，而非静默。
- 支持位运算 `& | ~`、常量 `pi e`、函数 `sqrt/sin/cos/tan/log/ln/abs/floor/ceil/round/exp`。
- 采用安全递归下降求值器（禁用 eval），避免表达式注入。

### 5. 程序员工具（Programmer）
- **触发词**：`编码` / `哈希` / `时间戳` / `encode` / `hash` / `timestamp`
  （注：`时间戳` 单独使用=输出当前时间戳；`转时间戳`/`时间戳转`=把给定时间转时间戳，见 §6）
- `base64 encode hello`（附解码验证）/ `base64 decode xxx`
- `url encode a b` / `url decode xxx`
- `hash hello` → MD5/SHA1/SHA256 **全给三条**；指定算法 `md5 hello` / `sha1 xxx` / `sha256 xxx` 仍单给。
- `ascii A` / `ascii 65`（字符 ↔ ASCII 码，大小写敏感）；`ascii hello` → 逐字符映射。
- `时间戳` / `timestamp`（当前 Unix 秒 / 毫秒）

### 6. 日期 / 倒数日（DateTime）
- **触发词**：`日期` / `倒数` / `date` / `countdown` / `转时间戳` / `时间戳转` / `to-timestamp` / `timestamp-of`
  （`转时间戳 2026-08-20 15:03:28` → 输出秒级/毫秒级时间戳；普通日期仍走日期差/倒数）
- 倒数日：`距 2027-01-01 还有几天`、`倒计时 生日`
- 日期差：`2025-01-01 到 2025-12-31`
- **星期几**：`2026-08-20 星期几`、`星期几`（今天）
- **日期偏移**：`今天加30天`、`2026-08-20 +100天`、`昨天-3天` → 目标日期 + 星期
- 今日：`今天` / `today`
- 支持 `YYYY-MM-DD`、`X月X日` 形态。

### 7. 颜色转换（Color）
- **触发词**：`颜色` / `color`
- `#ff8800` / `#f80` → HEX + RGB + HSL 三形态全列
- `rgb(255,136,0)` → HEX + RGB + HSL
- `hsl(32,100%,50%)` → HEX + RGB + HSL
- **色名快捷**：`红色` / `blue` / `灰色` → 预设色板
- **随机色**：`颜色 随机` / `color random`
- 任意输入附**互补色**参考（辅助配色）

### 8. 热量转换（Energy）— 减肥/控卡人群
- **触发词**：`热量` / `卡路里` / `energy` / `calorie`
- `100 kcal` / `500 千焦` / `大卡`
- 输出千卡/千焦/卡/焦耳互转，并换算成常见食物份数（如 `≈ 0.9 碗白米饭`、
  `1.9 个苹果`），直观感知摄入，帮助戒嘴瘾。

---

## 四、效率提升场景 / Why it saves time

- **少切换 App**：汇率、单位、时区、计算、哈希、日期、颜色、热量，
  全部在一个搜索框完成，无需打开计算器/浏览器/单位换算网站。
- **即时复制**：结果点击即复制，粘贴到聊天/文档零摩擦。
- **零学习成本**：自然语言式输入（`100美元`、`距元旦还有几天`），不懂语法也能用；
  忘记语法输入 `help` 即时获得示例。
- **离线可用**：除汇率外全部本地计算，高铁/飞机上也能用。
- **国际化**：结果说明跟随系统语言，中文用户看懂缩写、海外用户看到英文全称。

---

## 五、架构设计 / Architecture

插件以 **独立 DBus 服务进程** 形式接入 dde-grand-search，遵循官方 V1.0 插件协议
（Search / Stop / Action 三个 DBus 方法，JSON 报文）。

```
用户输入(全局搜索栏)
      │  DBus (V1.0 JSON)
      ▼
dde-grand-search-daemon ──► ConvertSearchPlugin (本进程)
                                │
                                ├─ QueryParser   解析 cont，判定类型(含 Help) + 抽取参数
                                ├─ providers/
                                │    ├─ CurrencyProvider  汇率（联网+缓存+静态降级）
                                │    ├─ UnitProvider      单位换算（本地表）
                                │    ├─ TimeProvider      时区时间（本地表）
                                │    ├─ CalcProvider      科学/程序员计算器
                                │    ├─ ProgramProvider   程序员工具
                                │    ├─ DateTimeProvider  日期/倒数日
                                │    ├─ ColorProvider     颜色转换
                                │    └─ EnergyProvider    热量转换
                                ├─ i18n                  集中式国际化（跟随系统语言）
                                └─ ResultBuilder  组装 V1.0 JSON 结果
      │  DBus (V1.0 JSON)
      ▼
全局搜索结果界面（分组卡片）
```

### 进程与接入 / Process & Integration
- 模式：`Auto`，`Priority=1`（首次搜索由 daemon 拉起，常驻后台）。
- DBus 服务名：`org.deepin.grandsearch.convert`（唯一，不与内置项冲突）。
- 配置文件安装到
  `/usr/lib/<triplet>/dde-grand-search-daemon/plugins/searcher/convert-search.conf`。

### 数据模型 / Data Model
输入 JSON（daemon → 插件）：
```json
{ "ver": "1.0", "mID": "task-001", "cont": "100usd" }
```
输出 JSON（插件 → daemon）：
```json
{
  "ver": "1.0", "mID": "task-001",
  "cont": [
    { "group": "汇率", "items": [
      { "item": "cur-xxx", "name": "100 USD = 728.50 CNY", "icon": "..." , "type": "convert/currency" }
    ]}
  ]
}
```

---

## 六、构建与安装 / Build & Install

```bash
# 从源码构建
cmake -B build -DCMAKE_INSTALL_PREFIX=/usr
cmake --build build -j$(nproc)
sudo cmake --build build --target install

# 重启 daemon 以加载插件
killall dde-grand-search-daemon && dde-grand-search-daemon &

# 或：deb 包安装（推荐）
sudo dpkg -i convert-search-plugin_1.0.0_amd64.deb
```

联调（本机已装 dde-grand-search-daemon）：
```bash
dbus-send --session --print-reply --dest=org.deepin.grandsearch.convert \
  /org/deepin/grandsearch/convert \
  org.deepin.grandsearch.convert.SearchPlugin.Search \
  "string:{\"ver\":\"1.0\",\"mID\":\"t1\",\"cont\":\"help\"}"
```

---

## 七、常见问题 / FAQ

- **Q：为什么有时没结果？** 输入无法识别时会提示「输入 help 查看全部功能」。
  常见原因：单位词不在内置表、币种不支持、语法过于复杂。
- **Q：汇率不准/不更新？** 汇率联网获取，失败会降级到内置静态表；可检查网络或稍后重试。
- **Q：结果看不懂缩写？** 结果已按系统语言附加中文/英文说明；如需其他语言可在
  `i18n.cpp` 的映射表扩展。
- **Q：支持自定义单位/城市吗？** 目前为内置表，欢迎提 Issue/PR 扩充。
- **Q：UOS AI 联动不显示？** 需系统已安装并启用 UOS AI（`com.deepin.copilot`）。
  未启用时插件照常工作，仅不显示「用 UOS AI 处理」入口。

---

## 八、UOS AI 联动 / UOS AI Integration

插件可把搜索内容一键发送到 **UOS AI**（`com.deepin.copilot`）对话，实现「输入框即入口、
AI 即处理」的无缝联动。

- 当系统已启用 UOS AI，且有实质查询结果时，结果区会多出一条
  **「用 UOS AI 处理：<你的输入>」** 卡片（即「检测到输入后可调用 UOS AI 自动处理」的体现）。
- 点击该卡片，插件通过 DBus 调起 UOS AI 对话页（`launchChatPage`）；若系统 UOS AI 提供
  「直接送入输入框」的接口（`SendMessage`/`SendText`/`PushQuestion` 等，按版本自动探测），
  则文本会**自动填入** AI 输入框；并始终把原始查询写入系统剪贴板作为兜底，便于你直接粘贴发起对话。
  首次调用会向 UOS AI 注册本应用与命令提示（`registerApp` / `registerAppCmdPrompts`）。
- 实现位于 `uosai.cpp`，所有调用**容错**：UOS AI 不可用时不显示入口、不影响主功能。
  若自动填入未生效，请在终端执行 `qdbus com.deepin.copilot /com/deepin/copilot` 确认接口名，
  并把方法列表反馈给开发者以补全对应发送方法。
- 说明：全局搜索**输入框下方的提示条**属 dde-grand-search 前端 QML（不在本插件仓库范围）；
  本插件提供后端联动能力（结果卡片 + DBus 调起），前端提示条可由 dde-grand-search 基于
  本插件的 `convert/uosai` 结果项实现。

---

## 九、一键打包 / Package Script

提供 `scripts/package.sh`，自动完成**依赖检查 → 编译 → 打包 → 生成发布产物**，支持多架构。

```bash
./scripts/package.sh              # 本机架构 (amd64)
./scripts/package.sh amd64 arm64  # 指定架构（arm64 需 docker + QEMU）
```

产物目录 `build/dist/<arch>/`：

```
convert-search-plugin_<ver>_<arch>.deb    # deb 安装包
convert-search-plugin_<arch>              # 裸二进制
SHA256SUMS_<arch>.txt                     # 单架构校验和
SHA256SUMS.txt                             # 汇总校验和
```

---

## 十、测试制度 / Testing

每次修改后运行单元测试，快速验证解析与各 provider 行为：

```bash
./tests/run_tests.sh
```

测试源码 `tests/test_main.cpp` 覆盖：`help`/单位目标单位/热量解析、单位/计算/颜色/程序/
日期/热量各 provider 结果、i18n 输出。全部通过显示 `通过 N / 失败 0`。

---

## 十一、自动构建与发布 / CI & Release

- **构建**：`.github/workflows/build.yml` 在 push/PR 时对 **amd64（原生）+ arm64（QEMU
  容器）** 双架构编译，仅依赖 Qt6（含 GUI 开发包），失败时打印完整日志。
- **发布**：`.github/workflows/release.yml` 在以下情况自动触发：
  1. 推送 **tag**（`v*`）；
  2. 提交信息含 **`release` / `publish`** 关键词。
  触发后自动构建双架构 **deb + 裸二进制 + SHA256 校验和**，生成 CHANGELOG（基于 git 提交），
  并创建 GitHub Release 上传全部 Assets（主流发包做法：多架构二进制包 + 校验文件）。
- 发布前建议执行 `./tests/run_tests.sh` 确保测试通过。

---

## 十二、后续规划 / Roadmap

见 [docs/DESIGN.md](docs/DESIGN.md) §7，方向包括：农历节气、全球时区穿透、健康运动
（BMI/步数）、购物比价、限行提醒、密码生成、编码扩展、单位增强（油耗/尺码/纸张）、
快递识别、财务短语等。

---

## 十三、许可证 / License

GPL-3.0-or-later（与 dde-grand-search 示例一致，满足活动开源要求）。
