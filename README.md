# DDE 全局搜索 · 万能转换器 插件

> DDE Grand Search · All-in-One Converter (currency / unit / time / calc / dev / date / color)

[![Build amd64+arm64](https://github.com/testerxydw/dde-grand-search-convert/actions/workflows/build.yml/badge.svg)](https://github.com/testerxydw/dde-grand-search-convert/actions/workflows/build.yml)
[![License: GPL-3.0](https://img.shields.io/badge/license-GPL--3.0-blue.svg)](LICENSE)

- 仓库 / Repository: https://github.com/testerxydw/dde-grand-search-convert
- 基于 deepin-skills 的 dde-grand-search 插件开发指南 V1.0 完成。

一个为 DDE 全局搜索（dde-grand-search）开发的扩展搜索插件。面向**各类人群**：办公党、
科研/学生、程序员、设计师都能用。在全局搜索栏输入自然语言式查询，插件实时解析并返回
结果卡片。参考 macOS Spotlight、Ubuntu Calculator 等系统的实用搜索能力，集成了：

- 汇率换算（联网 + 本地缓存 + 静态降级）
- 单位换算（长度/重量/温度/面积/体积/速度/数据）
- 跨时区时间查询
- 科学 / 程序员计算器（表达式、进制转换、位运算）
- 程序员工具（Base64 / URL 编解码、ASCII、哈希、时间戳）
- 日期 / 倒数日 / 日期差（办公友好）
- 颜色转换（HEX / RGB / HSL 互转，设计友好）

A search plugin for DDE Grand Search, inspired by macOS Spotlight and Ubuntu Calculator.
It bundles currency, unit, time, calculator, developer tools, date/countdown and color
conversion into one plugin, covering office, student, programmer and designer use cases.

---

## 一、功能设计 / Features

1. **汇率换算（Currency）**
   - 输入 `100usd`、`$50`、`100 USD to CNY`、`100美元` → 返回对应币种金额（按实时/缓存汇率）。
   - 支持常见币种符号 `$ € ¥ £` 与代码 `USD CNY EUR JPY GBP ...`。
   - 联网获取汇率（公开 API），失败回退到内置静态汇率表；本地缓存（带 TTL）。

2. **单位换算（Unit）**
   - 长度：`12inch`、`3km`、`1 mile`、`5 cm`。
   - 重量：`1kg=?斤`、`2lb`、`500g`。
   - 温度：`100F`、`37C`、`32 F to C`。
   - 面积/体积/速度/数据存储等可扩展。
   - 内置换算表，纯本地、零网络依赖。

3. **时区时间（Time）**
   - 输入 `北京时间`、`tokyo now`、`纽约时间`、`what time in london` → 返回目标城市当前时间 + 与本地时差。
   - 内置城市→时区映射（IANA 时区名）。

4. **科学 / 程序员计算器（Calc）**
   - 表达式：`12*8+sqrt(16)`、`(2+3)^2`、`log(100)`、`sin(pi/2)`。
   - 进制转换：`255 to hex`、`0xff+1`、`100 to bin`。
   - 整数结果同时给出 HEX / BIN / OCT（程序员友好）。
   - 位运算 `& | ~`、常量 `pi e`、函数 `sqrt/sin/cos/tan/log/ln/abs/floor/ceil/round/exp`。

5. **程序员工具（Programmer）**
   - `base64 encode hello` / `base64 decode xxx` → Base64 编解码。
   - `url encode a b` / `url decode xxx` → URL 百分号编解码。
   - `md5 hello` / `sha1 xxx` / `sha256 xxx` → 哈希值。
   - `ascii A` / `ascii 65` → 字符 ↔ ASCII 码。
   - `时间戳` / `timestamp` → 当前 Unix 秒 / 毫秒时间戳。

6. **日期 / 倒数日（DateTime）** — 办公党友好
   - `距 2027-01-01 还有几天` → 倒数日 + 星期。
   - `2025-01-01 到 2025-12-31` → 日期相差天数。
   - `今天` / `today` → 今日日期与星期。
   - 支持 `YYYY-MM-DD`、`X月X日` 等形态。

7. **颜色转换（Color）** — 设计 / 办公友好
   - `#ff8800` / `#f80` → RGB + HSL。
   - `rgb(255,136,0)` → HEX + HSL。
   - `hsl(32,100%,50%)` → HEX + RGB。
   - HEX / RGB / HSL 三向互转。

8. **通用交互**
   - 多结果分组展示（按类型分组：汇率 / 单位 / 时间 / 计算器 / 程序员工具 / 日期 / 颜色）。
   - 点击结果 → 复制到系统剪贴板（复用 calculator 示例的 Action 模式）。
   - 全部查询在本地完成解析；汇率仅在必要时联网，且失败可降级。

---

## 二、架构设计 / Architecture

插件以 **独立 DBus 服务进程** 形式接入 dde-grand-search，遵循官方 V1.0 插件协议
（Search / Stop / Action 三个 DBus 方法，JSON 报文）。

```
用户输入(全局搜索栏)
      │  DBus (V1.0 JSON)
      ▼
dde-grand-search-daemon ──► ConvertSearchPlugin (本进程)
                                │
                                ├─ QueryParser   解析 cont，判定类型 + 抽取参数
                                ├─ providers/
                                │    ├─ CurrencyProvider  汇率（联网+缓存+静态降级）
                                │    ├─ UnitProvider      单位换算（本地表）
                                │    ├─ TimeProvider      时区时间（本地表）
                                │    ├─ CalcProvider      科学/程序员计算器
                                │    ├─ ProgramProvider   程序员工具
                                │    ├─ DateTimeProvider  日期/倒数日
                                │    └─ ColorProvider     颜色转换
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

## 三、实现计划 / Plan

见 [docs/IMPLEMENTATION.md](docs/IMPLEMENTATION.md)。

---

## 四、构建与安装 / Build & Install

```bash
cmake -B build -DCMAKE_INSTALL_PREFIX=/usr
cmake --build build -j$(nproc)
sudo cmake --build build --target install
# 重启 daemon 以加载插件
killall dde-grand-search-daemon && dde-grand-search-daemon &
```

联调（本机已装 dde-grand-search-daemon）：
```bash
dbus-send --session --print-reply --dest=org.deepin.grandsearch.convert \
  /org/deepin/grandsearch/convert \
  org.deepin.grandsearch.convert.SearchPlugin.Search \
  "string:{\"ver\":\"1.0\",\"mID\":\"t1\",\"cont\":\"100usd\"}"
```

---

## 五、许可证 / License

GPL-3.0-or-later（与 dde-grand-search 示例一致，满足活动开源要求）。
