# 功能全景 · dde-grand-search-convert

> DDE 全局搜索「万能转换器」插件的功能盘点。基于 `1.0.0` 源码（约 3.3k 行）整理，
> 与 [README](../README.md) 互为补充：README 面向使用者，本文面向开发与评审。

## 1. 产品定位

在 DDE 全局搜索栏中直接完成 8 类高频轻量查询（换算/换汇/时间/计算/编码/日期/颜色/热量），
结果以分组卡片展示，**点击即复制**。面向办公、学生/科研、程序员、设计师、控卡人群。
插件以独立 DBus 服务接入 `dde-grand-search-daemon`，遵循官方 V1.0 插件协议。

## 2. 能力总览

| # | 功能 | 典型输入 | 数据来源 | 网络 | 实现（行数） |
|---|------|---------|---------|------|--------------|
| 1 | 汇率换算 | `100usd` / `100 USD to CNY` | frankfurter.app 公开 API + 静态表降级 + TTL 缓存 | 有 | `providers/currencyprovider.cpp` (130) |
| 2 | 单位换算 | `12inch` / `1kg=?斤` / `100F` | 内置 7 类单位表 | 无 | `providers/unitprovider.cpp` (210) |
| 3 | 时区时间 | `北京时间` / `tokyo now` / `时间` | 内置城市→IANA 映射 | 无 | `providers/timeprovider.cpp` (135) |
| 4 | 科学/程序员计算器 | `12*8+sqrt(16)` / `255 进制` | 安全递归下降求值器（禁 eval） | 无 | `providers/calcprovider.cpp` (350) |
| 5 | 程序员工具 | `base64 encode hello` / `md5 x` / `时间戳` | Qt 本地编解码/哈希 | 无 | `providers/programprovider.cpp` (271) |
| 6 | 日期/倒数日 | `距 2027-01-01 还有几天` / `今天加30天` | QDate 本地计算 | 无 | `providers/datetimeprovider.cpp` (200) |
| 7 | 颜色转换 | `#ff8800` / `红色` / `颜色 随机` | 本地换算 + 预设色板 | 无 | `providers/colorprovider.cpp` (205) |
| 8 | 热量转换 | `100 kcal` / `500 千焦` | 本地换算 + 食物份数表 | 无 | `providers/energyprovider.cpp` (110) |
| 9 | 内联帮助 | `help` / `?` / `使用说明` | 静态卡片 | 无 | `queryparser.cpp` |

## 3. 各功能规格

### 3.1 汇率换算（CurrencyProvider）
- **触发词**：`汇率` / `换汇` / `currency` / `exchange`
- **语法**：`100usd`、`$50`、`100美元`、`人民币100`；目标币种 `100 USD to CNY` / `100usd=?cny` / `100usd cny`
- **币种**：符号（`$ € ¥ £`）+ 15 代码（USD CNY EUR JPY GBP HKD KRW RUB AUD CAD SGD THB INR TWD MOP）
- **省略目标 = 全币种列出**（15 条，无上限截断）
- **网络策略**：`https://api.frankfurter.app/latest` 获取 → 本地缓存带 TTL → 失败降级内置静态表（覆盖全部 15 币种），保证离线可用
- **输出**：`100 USD（美元） = 728.50 CNY（人民币）`，数值去无意义尾零

### 3.2 单位换算（UnitProvider）
- **触发词**：`换算` / `单位` / `convert` / `unit`
- **7 大类**：长度、重量、温度（`100F`/`37C`/`32 F to C`）、面积、体积、速度、数据量
- **目标单位写法任选**：`1kg to 斤` / `1kg=?斤` / `1kg 斤`
- **省略目标 = 同类全列**：`100kg` → 斤/两/g/lb/oz/吨 全部可换算单位

### 3.3 时区时间（TimeProvider）
- **触发词**：`时间` / `时区` / `time` / `timezone` / `now` / `现在几点`
- **无城市词 = 世界时钟**：北京/东京/纽约/伦敦/巴黎/悉尼 并排本地时刻
- **指定城市**：单条 + 与本地时差；支持多城市 `东京 纽约 伦敦`

### 3.4 科学/程序员计算器（CalcProvider）
- **触发词**：`计算` / `算` / `calc` / `calculate` / `进制` / `base`
- **表达式**：`12*8+sqrt(16)`、`(2+3)^2`、`log(100)`、`sin(pi/2)`、位运算 `& | ~`、常量 `pi e`
- **函数**：`sqrt/sin/cos/tan/log/ln/abs/floor/ceil/round/exp`
- **进制**：`255 to hex` / `0xff+1`；`255 进制` → DEC/HEX/BIN/OCT 四条全给
- **结果增强**：整数附 HEX/BIN/OCT；小数附科学计数法 + 百分比
- **安全**：手写递归下降求值器，**禁用 eval**，杜绝表达式注入；除零/√负数/未知函数返回友好错误卡片

### 3.5 程序员工具（ProgramProvider）
- **触发词**：`编码` / `哈希` / `时间戳` / `encode` / `hash` / `timestamp`
- **能力**：
  - `base64 encode hello`（附解码验证）/ `base64 decode xxx`
  - `url encode a b` / `url decode xxx`
  - `hash hello` → MD5/SHA1/SHA256 三条全给；`md5 x` / `sha1 x` / `sha256 x` 单给
  - `ascii A` / `ascii 65`（字符 ↔ 码，大小写敏感）；`ascii hello` → 逐字符映射
  - `时间戳` / `timestamp` → 当前 Unix 秒/毫秒

### 3.6 日期/倒数日（DateTimeProvider）
- **触发词**：`日期` / `倒数` / `date` / `countdown` / `转时间戳` / `时间戳转`
- **能力**：倒数日（`距 2027-01-01 还有几天`）、日期差（`2025-01-01 到 2025-12-31`）、
  星期几（`2026-08-20 星期几`）、日期偏移（`今天加30天` / `昨天-3天`）、今日（`今天`）、
  日期转时间戳（`转时间戳 2026-08-20 15:03:28`）
- **形态**：`YYYY-MM-DD`、`X月X日`

### 3.7 颜色转换（ColorProvider）
- **触发词**：`颜色` / `color`
- **能力**：`#ff8800` / `#f80` / `rgb(255,136,0)` / `hsl(32,100%,50%)` → HEX+RGB+HSL 三形态全列；
  色名快捷（`红色` / `blue`）；随机色（`颜色 随机`）；任意结果附**互补色**参考

### 3.8 热量转换（EnergyProvider）
- **触发词**：`热量` / `卡路里` / `energy` / `calorie`
- **能力**：kcal/kJ/cal/J 互转 + 换算常见食物份数（`≈ 0.9 碗白米饭`、`1.9 个苹果`）

## 4. 公共交互机制

| 机制 | 说明 |
|------|------|
| **分组卡片** | 结果按类型分组成卡片，每组可含多条（V1.0 `cont[].group/items`） |
| **点击即复制** | 收到 `openitem` Action 时由插件直接写入系统剪贴板（`clipboard.cpp`）。插件是独立 DBus 服务，无法拦截前端 Ctrl+C，故统一「点击即复制」 |
| **触发词** | 任意查询前加触发词可强制走对应转换，避免歧义；分隔符支持 `:` / `：` / `?` / 空格。解析时收集所有前缀命中触发词，**最长者优先**（避免 `时间` 误吞 `时间戳`） |
| **省略目标全列** | 单位/货币/热量三类省略 `=` 后目标即列出同类全部可换算单位，不舍上限 |
| **国际化** | 全部结果说明经集中式 `i18n.cpp` 按 `QLocale` 系统语言输出（中文 `km（千米/公里）`，英文 `km (kilometer)`），数值去无意义尾零 |
| **内联帮助** | `help` / `?` / `？` / `使用说明` 等返回全部功能示例卡片 |
| **未识别提示** | 无法识别时返回「输入 help 查看全部功能与示例」提示卡，不静默无结果 |
| **UOS AI 联动** | 有实质结果且系统启用 UOS AI 时，追加「用 UOS AI 处理：<输入>」卡片；点击经 DBus 调起 `com.deepin.copilot` 对话页，自动探测发送接口（`SendMessage`/`SendText`/`PushQuestion`），并写入剪贴板兜底；首次调用 `registerApp`。全部容错，UOS AI 缺失不影响主功能（`uosai.cpp`） |

## 5. 协议实现（V1.0）

- **三接口**：`Search(json)→json`、`Stop(json)→bool`、`Action(json)→bool`，由官方 XML 经
  `qdbusxml2cpp` 生成 `searchpluginadaptor`，转发到 `ConvertSearch` 业务主体
- **配置**（`convert-search.conf.in` 生成）：

```ini
[Grand Search]
Name=org.deepin.grandsearch.convert
Mode=Auto          # daemon 管理，首次搜索时拉起
Priority=1         # Middle：用户搜索时启动并守护（最多重启 3 次）
InterfaceVersion=1.0
```

- **进程**：`QGuiApplication`（剪贴板需要）；无显示环境自动回退 `QT_QPA_PLATFORM=offscreen`
- **关键约束**：Search 可中断；单组 ≤100 项；`item/name/type` 必填非空；响应 <25s
  （仅汇率联网，设超时 + 缓存）；`mID` 任务号全程透传，`m_lastResults` 缓存供 Action 定位

## 6. 代码地图

| 文件 | 行数 | 职责 |
|------|------|------|
| `main.cpp` | 46 | 入口：QGuiApplication + DBus 注册（service/object） |
| `convertsearch.h/.cpp` | 40/267 | DBus 业务主体：parseInput → QueryParser → Provider → ResultBuilder；任务缓存与 Stop |
| `queryparser.h/.cpp` | 43/383 | 类型判定 + 参数抽取（触发词表、正则、优先级链） |
| `resultbuilder.h/.cpp` | 40/78 | 组装 V1.0 JSON 结果 |
| `i18n.h/.cpp` | 36/193 | 集中式国际化词表（组名/单位名/币种名/杂项） |
| `clipboard.h/.cpp` | 28/88 | 剪贴板写入（Qt clipboard，DBus 回退） |
| `uosai.h/.cpp` | 26/114 | UOS AI 联动（接口探测/注册/调起） |
| `searchpluginadaptor.h/.cpp` | 28/33 | 官方 XML 生成的 DBus Adaptor |
| `providers/*` | 8 组 | 见 §2 表 |
| `tests/test_main.cpp` + `run_tests.sh` | — | 单测：parser / providers（program/datetime 等）/ i18n |
| `scripts/package.sh` | — | 多架构 deb + 裸二进制 + SHA256SUMS 打包 |
| `.github/workflows/build.yml` / `release.yml` | — | amd64 原生 + arm64 QEMU 双架构；tag/关键词触发 Release |

## 7. 测试现状

- `./tests/run_tests.sh`：纯进程内自测（不依赖 DBus），覆盖 QueryParser（help/单位目标/热量解析）、
  各 Provider（program/datetime 等）与 i18n 输出
- 手工验收：`dbus-send` 直调 Search/Stop/Action（见 README §六）

## 8. 已知限制

1. **DBus 命名空间**：`org.deepin.*` 前缀按惯例属官方所有，第三方插件占用不合规范（见 REDESIGN §5.1）
2. **i18n 词表硬编码**：仅中/英两列，无法通过翻译平台贡献（见 REDESIGN §5.5）
3. **汇率单一 API**：frankfurter.app 不可用且缓存过期时，仅静态表兜底（币种少、无实时性）
4. **单测边界缺口**：8 类 provider 均有基础用例，但缺汇率联网/降级路径、未识别输入反向验证、calc 深嵌套等边界断言
5. **QGuiApplication 依赖**：链接 Qt Gui，headless 依赖 offscreen 平台插件
