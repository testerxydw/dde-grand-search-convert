<!--
发帖说明（请勿在论坛正文里贴这部分）：
- 版块：AI 开发实验室
- 标题：【deepin插件开发活动】万能转换器插件（dde-grand-search-convert）
- 由 AI（CodeBuddy + deepin-skills）代发，作者/开发者：xiyidaiwa
- 发帖时请附：
  1) 本会话中 use_skill 加载 dtk-development 的截图（见 docs/SKILL_EVIDENCE.md §2）
  2) dbus-send 联调结果截图
  3) GitHub 仓库页截图（https://github.com/xiyidaiwa/dde-grand-search-convert）
  4) GitHub Actions 双架构构建运行截图
-->

# 【deepin插件开发活动】万能转换器插件（dde-grand-search-convert）

> 本文由 AI（CodeBuddy + deepin-skills）代为整理发布，开发者：xiyidaiwa。
> 项目仓库：https://github.com/xiyidaiwa/dde-grand-search-convert

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

## 二、技术实现

- 遵循 dde-grand-search 官方 **V1.0 插件协议**：独立 DBus 服务进程，实现
  `Search` / `Stop` / `Action` 三方法，JSON 报文（ver/mID/cont/action/item）。
- DBus 服务名 `org.deepin.grandsearch.convert`，配置文件安装到
  `/usr/lib/<triplet>/dde-grand-search-daemon/plugins/searcher/`。
- 架构：`QueryParser` 解析类型 → 多 `provider`（汇率/单位/时区/计算器/程序员工具/
  日期/颜色）→ `ResultBuilder` 组装 V1.0 JSON。
- 计算器采用**安全递归下降求值器**（禁用 eval），避免表达式注入风险。
- Qt5/Qt6 自动探测；汇率联网失败时自动降级到内置静态表，离线可用。
- 除汇率外，其余 6 类功能**全部本地计算，零网络依赖、零隐私上传**。

## 三、deepin-skills 调用实证（活动硬性要求）

本插件由 AI 借助已安装的 **deepin-skills** 完成开发：

1. 通过 `use_skill` 工具实际加载 `dtk-development` skill，获取 DTK/Qt/DBus
   构建规范（截图见附件①）。
2. 遵循 dde-grand-search 官方 V1.0 插件协议与 `calculator-search-plugin` 示例
   实现 `SearchPlugin` 接口。
3. 完整开发过程均基于 deepin-skills 框架约束（CMake 兼容、Qt 版本探测、DBus 通信）。

> 说明：deepin-skills 官方仓库当前含 4 个 skill（control-center / shell / tray /
> dtk），暂未提供 dde-grand-search 专属 skill；本插件调用了其中的
> `dtk-development` 框架 skill 作为开发依据，并严格遵循 dde-grand-search 官方协议。

## 四、构建与验证

- 本地 amd64 编译通过，`dbus-send` 联调各功能返回正确结果卡片（部分见下表）。
- 已配置 **GitHub Actions 双架构自动构建**：`.github/workflows/build.yml`
  同时构建 amd64（原生）与 arm64（QEMU 容器），作为活动加分项验证（截图见附件④）。

| 查询 | 结果 |
|------|------|
| `1kg=?斤` | 2 斤 |
| `12*8+sqrt(16)` | 100 + HEX 0x64 / BIN 0b1100100 / OCT 0o144 |
| `base64 encode hello` | aGVsbG8= |
| `距 2027-01-01 还有几天` | 还有 136 天 |
| `#ff8800` | RGB rgb(255,136,0) / HSL hsl(32,100%,50%) |

## 五、许可证与参与

- 许可证：**GPL-3.0-or-later**（与 dde-grand-search 示例一致，满足活动开源要求）。
- 仓库：https://github.com/xiyidaiwa/dde-grand-search-convert
- 欢迎提 Issue / PR，扩充更多币种、城市、单位与功能。

---
*代发声明：本帖内容由 AI（CodeBuddy 调用 deepin-skills）辅助生成与整理，
开发者 xiyidaiwa 对所有技术实现与发布内容负责。*
