# 合规审查记录 · Compliance Review

> 对照官方插件开发文档
> https://github.com/linuxdeepin/dde-grand-search/blob/master/docs/plugin-development-guide.md
> 与官方示例 examples/calculator-search-plugin，对 `dde-grand-search-convert` 做多维度合规审查。
>
> 结论：**整体严格符合**官方 V1.0 插件协议，与 calculator 示例实现同级。

---

## 一、conf 配置对照

| 字段 | 官方示例 | 本项目 | 结论 |
|------|---------|--------|------|
| `Name` | `com.example.CalculatorSearch` | `org.deepin.grandsearch.convert` | ✅ 唯一、不与内置项冲突 |
| `Mode` | `Auto` | `Auto` | ✅ |
| `Priority` | `1` | `1` | ✅ |
| `DBusService` | `com.example.CalculatorSearch` | `org.deepin.grandsearch.convert` | ✅ 与 `main.cpp` 注册一致 |
| `DBusAddress` | `/com/example/CalculatorSearch` | `/org/deepin/grandsearch/convert` | ✅ 与 `main.cpp` 注册一致 |
| `DBusInterface` | `com.example.CalculatorSearch.SearchPlugin` | `org.deepin.grandsearch.convert.SearchPlugin` | ✅ 与 adaptor 一致 |
| `InterfaceVersion` | `1.0` | `1.0` | ✅ |
| `Exec` | `@CALCULATOR_PLUGIN_EXEC@` | `@CONVERT_PLUGIN_EXEC@` | ✅ 构建期替换 |

---

## 二、DBus 接口协议（V1.0）

- 三个方法 `Search(QString)→String` / `Stop(QString)→Bool` / `Action(QString)→Bool`
  与 adaptor（`searchpluginadaptor.h`）、官方签名逐项一致 ✅
- `Search` 返回 JSON 结构 `ver/mID/cont[]/group/items[]{item,name,icon,type}`
  与协议文档、官方示例 JSON 完全一致 ✅
- `item`/`name`/`type` 必填校验在 `resultbuilder.cpp` 已做空值跳过 ✅
- `Stop` 解析 `ver+mID` 返回 bool，与文档一致 ✅
- `Action` 仅处理 `openitem`，与文档规范一致 ✅

---

## 三、安装路径与打包

- 安装到 `${LIBDIR}/dde-grand-search-daemon/plugins/searcher`，运行时解析为
  `/usr/lib/x86_64-linux-gnu/...`，与文档指定路径完全一致 ✅
- `debian/*.install` 同时打包二进制 + conf ✅
- `postinst` 安装后通过 dbus `Restart`/`Reload` 或向 daemon 发 `HUP` 重载，符合"放入后重启后端生效" ✅

---

## 四、结果数据规范

- `group` / `name` 均走 `I18n` 跟随系统语言，符合文档"group/name 需国际化" ✅
- `icon` 可选，仅在非空时输出 ✅
- 每组上限 100 项：各 provider 单组条目远小于 100，无超限风险 ✅

---

## 五、搜索可中断性

文档要求 `Search` 须可中断。`Stop` 仅清理缓存（`m_lastResults.remove(mID)`）。
本项目 `search()` 为同步执行，与官方示例同范式；各 provider 计算均为内存/轻量运算，
且 `CurrencyProvider::convert` 只读缓存（静态降级表兜底），不阻塞 daemon 调用线程 ✅

---

## 六、UOS AI 扩展

- 文档规定 `Action` 仅 `openitem`。本项目对 `item=="uosai-send"` 走自定义逻辑，
  仍挂在 `openitem` action 下，未引入新 action 名 ✅
- `convert/uosai` 为自定义 `type`，daemon 透传，前端按「用 UOS AI 处理」卡片展示（已实测可显示）✅

---

## 七、结果展示顺序 / "最佳匹配"置顶 可行性分析 ⚠️

**结论：第三方插件无法控制结果排序，不能通过 conf 或 JSON 实现"最佳匹配"置顶。**

### 依据
1. **V1.0 协议未开放排序/权重字段**
   官方文档明确：*"扩展属性（如权重、拖尾信息等）由 daemon 端通过 `extra` 处理，
   第三方插件经 V1.0 传输时自动补全"*。插件只能返回
   `ver/mID/cont/group/items`，**无法主动设置权重或排序**。
2. **`Priority=1` 不是结果排序**
   conf 中的 `Priority` 是 Auto 模式下 daemon 对**插件进程**的
   启动/守护优先级（0=High / 1=Middle / 2=Low），与搜索结果在界面上的先后顺序无关。
3. **"最佳匹配"是内置搜索项能力**
   文件、应用等内置搜索项的最佳匹配/置顶逻辑属于 `dde-grand-search` 前端与框架内部实现，
   非第三方插件可配置项。

### 已做 / 可做的
- ✅ 已配置 `Mode=Auto` + `Priority=1`，保证插件进程常驻、尽量在线。
- ❌ 插件层（conf / JSON）无法实现结果置顶。
- 若确需让本插件结果优先展示，只能：
  - 修改 `dde-grand-search` 前端/框架源码，使收到本插件结果时优先展示；或
  - 等/升级到支持插件自定义 `weight` / `extra` 的新协议版本。

---

## 八、审查中发现并修复的问题

| 项 | 问题 | 修复 |
|----|------|------|
| UOS AI 调用签名 | `inputPrompt(s a{ss})` 第二参数误用 `QVariantMap`（会编成 `a{sv}`） | 改为 `QVariant::fromValue(QMap<QString,QString>())` |
| 汇率网络超时 | `fetchRates` 后台请求无超时，弱网时 reply 挂起、`m_fetching` 不再重试 | `QNetworkRequest::setTransferTimeout(8000)` |
| 数值尾零过多 | 固定 `'f',n` 小数位，整数结果也带尾零（如 `200.0000`） | 新增 `ResultBuilder::formatNumber` 去尾零，最多保留指定位 |
| 孤立小数点 | `formatNumber` 去尾零后残留 `.`（如 `100.`） | 调整去掉尾零后再清孤立小数点 |

---

*最近更新：2026-08-20*
