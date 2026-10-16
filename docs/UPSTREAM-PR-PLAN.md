# 上游 PR 实现计划 · 插件随包模式（方案 B，已实施）

> **当前状态（2026-10-16）**：用户选定插件随包模式（工作量最小、改动最少）。
> 内置搜索器方案（原方案 C）已实施提交 1 后按用户决策切换；骨架代码存档于
> **`archive/builtin-searcher`** 分支（含全部接口核实成果与 queryparser 修复，可随时恢复）。
> PR 分支 **`feat/convert-search-plugin`**（基于 master，2 提交，PR 直接可用）：
> - `ca43622` feat: add convert search plugin（32 文件，src/convert-search-plugin/，整树编译零警告，install 验证通过）
> - `14f5343` fix: parser edge cases and valid DBus names（DBus 实测发现的 3 个解析 bug + 命名非法修正）
>
> **整包验证与实测（2026-10-16 完成）**：
> - `dpkg-buildpackage` 整包构建成功，插件二进制 + conf 自动进包（`dpkg -c` 确认，debian/ 零改动）
> - 本机实测 18 场景全绿（dbus 直调插件 Search 接口 + 系统包验证）
> - 实测发现并修复 3 个存量 bug + 1 个命名问题：
>   1. Latin 触发词无词边界："base64 encode hello" 被 calc 触发词 "base" 吞掉按算术求值 → 加 Latin 整词判定（中文连写 "算12*8" 不受影响）
>   2. 触发词回丢动作词："hash hello" 命中 program 触发词后递归 parse 剩余内容失败，回退时只给 rest → provider 收到 "hello" 无法识别哈希动作返回空 → 回退改给完整原文
>   3. 色名表不一致：queryparser 识别单字色名（红/蓝）但 provider 色名表只有双字 → 空；补单字后暴露 contains("") 恒真 bug（全色变红）→ 空名跳过
>   4. DBus 对象路径不允许连字符：/org/deepin/dde-grand-search/convert 非法 → 官方 CamelCase 族 org.deepin.dde.GrandSearchConvert（对齐 GrandSearchDaemon 命名）
> - 修复已同步本仓库（queryparser/colorprovider），测试 64/64（含 6 个新回归断言）
> - 安装注意：本机若有独立包 convert-search-plugin 需先卸载（同路径文件冲突）
>
> 方案 B 实施要点：
> - 官方仓库改动仅 **2 处既有文件**：根 CMakeLists.txt（+1 行 add_subdirectory）、无其他
> - 插件源码 `src/convert-search-plugin/`：本仓库插件实现直迁 + queryparser 全角冒号 bug 修复
>   （`QLatin1Char('：')` → `QChar(0xFF1A)`，原插件版该分支从未生效）+ 死代码清理
> - **UOS AI 联动已剥离**（4 处调用删除，留独立仓库版本）
> - 命名升级：`org.deepin.dde-grand-search.convert`（DBus 服务/路径/接口/conf 统一）
> - 组 key 沿用显示名方案——**经核实与官方 calculator 插件先例一致**（calculatorsearch.cpp L149 组 key="计算器"），前端零改动
> - install 产物：二进制 + conf 进 `lib/.../dde-grand-search-daemon/plugins/searcher/`（daemon 运行时自动加载，postinst 自动重启）
>
> 以下为方案 C（内置搜索器）时期的历史分析，接口核实结论仍有效；提交序列与文件清单以本节为准。



> **【2026-10-16 路径决策更新——当前执行推荐：路径3（先插件随包，后内置化）】**
>
> 发现 `searchplugin/convertors/`（协议编解码层）+ `ExtendSearcher`（Searcher 子类，外部插件经
> PluginLoader 扫 conf → addExtendSearcher 后**与内置项走同一调度链路**）后重估：插件形态是
> 官方一等公民机制。按"改动小/工作量少/维护少"三指标，插件随包全部占优：
> - 改动：daemon 核心 0 处（仅构建加子目录 + 安装规则） vs 方案 C 改 6 处核心文件
> - 工作量：现有项目整体直收（已验证全绿） vs 约 2-3 倍（provider 迁移 + 前端适配 + 实测）
> - 维护：协议由 convertors 兼容层管理且同仓连带维护 vs 持平；插件另有进程崩溃隔离
> - 唯一代价：一次 DBus 往返（异步搜索下感知可忽略）
>
> **注意**：`searchplugin/convertors`（协议数据转换）与本功能"单位换算"纯属命名巧合、业务无关、
> 不可复用；PR 描述需说明，避免评审混淆。
>
> **方案 C 全文保留**（下方），作为路径3 第二步内置化的实施依据；骨架代码已存于官方仓库
> `feat/builtin-convert-searcher` 分支（convertsearcher/convertworker/queryparser/i18n/clipboard，
> 未提交），届时直接续用。

将转换功能**不作为插件**，以内置搜索器形态合并进官方 `dde-grand-search` 的 daemon，
与文件/应用/语义搜索同级，随整包 deb 分发。（原方案 C 前提，现为路径3 的第二步目标）
全部"官方做法"经源码核实，标注文件+行号；本文为实施主计划。

## 1. 形态结论与取舍

| 决策项 | 结论 | 依据 |
|--------|------|------|
| 形态 | daemon 进程内**内置搜索器** `ConvertSearcher`（Searcher + ProxyWorker 模型） | 对齐官方内置项实现（semantic/web 等均为该模型） |
| 代码位置 | `src/dde-grand-search-daemon/searcher/convert/` | 对齐 `searcher/semantic/`、`searcher/web/` 布局 |
| 随包分发 | ✓ daemon 链入主二进制，无独立文件 | 无需单独打包 |
| 内置项名 | `GRANDSEARCH_CLASS_CONVERT = "com.deepin.dde-grand-search.convert"`（新增宏） | 对齐 [builtinsearch.h](../../dde-grand-search/src/global/builtinsearch.h#L9-L15) 现有 7 项命名，不冲突 |
| 结果分组 | **单组** `GRANDSEARCH_GROUP_CONVERT = "com.deepin.dde-grand-search.group.convert"`，8 类细分走 item 的 `type` 字段 | 最小化前端改动（每多一组就多一组前端翻译词条） |
| Worker 模式 | **声明异步 worker**（对齐 semantic 的 isAsync 语义）：本地 7 类计算完成后即 `tryNotify()` 出结果；汇率若有解析命中，网络返回后**增量补充 `unearthed`** | TaskCommander 支持 worker 多次增量通知（semanticworker tryNotify 模式）；避免同步队列被汇率 3s 超时阻塞 |
| 图标 | 结果 item 配 8 类各一枚 16px 图标（优先复用官方 `grand-search/gui/exhibition/matchresult/builtin/icons` 现有资源或官方图标风格新绘 SVG） | 内置项展示有图标体系（semantic_16px.svg 先例）；纯文本结果在评审与 UI 一致性上均薄弱 |
| best-match | **不参与** `GRANDSEARCH_GROUP_BEST` | 转换结果是并列多组参考值，无单一"最佳"；best-match 权重方法（builtinsearch.h L42-45）为文件/应用/设置定制 |
| 点击复制 | 在 `ConvertSearcher::action()` 中直接写 QClipboard | action 链路：前端 → `OpenWithPlugin`（grandsearchinterface.cpp L207）→ `searcherAction`（maincontroller.cpp L313）→ `Searcher::action`；daemon 用 DApplication（QApplication 子类，daemon main.cpp L127），QClipboard 可用 |
| UOS AI 联动 | **剥离**，不进本 PR | 内置功能耦合外部 DBus 服务，产品争议大；留独立仓库版本 |
| 汇率网络 | daemon 补链 `Qt${QT_NS}::Network`（daemon 现未链接，semantic 的联网走 D-Bus 服务故无需） | daemon [CMakeLists.txt](../../dde-grand-search/src/dde-grand-search-daemon/CMakeLists.txt) L191-207 链接表核实；援引 semantic 已有联网搜索先例降低评审阻力 |
| 配置开关 | **零新增键**：maincontroller 已有通用开关机制——`ConfigerIns->group(GRANDSEARCH_PREF_SEARCHERENABLED)` 组下以 searcher name 为 key（`config->value(name, true)`，maincontroller.cpp L56/L80），convert 项自动获得开关；`isActive()` 只需返回 true | 已核实 maincontroller.cpp L56-80 配置读取链路 |
| **checkSearcher 适配（阻断性）** | **必须改**：普通关键字搜索时 checkSearcher（maincontroller.cpp L147-153）只返回 file/fulltext/app/ocr 四个固定 searcher，convert 默认被过滤不参与——需在其默认分支追加 `data.append(GRANDSEARCH_CLASS_CONVERT)` | 已核实 checkSearcher L125-158 过滤逻辑；searcherData 非空时按名单过滤（L81） |
| i18n | 组名经前端 `datadefine.h` `tr()` + `.ts` 词条；结果文本沿用词表式 `i18n.cpp`（daemon 侧本地化） | 前端组名机制核实：datadefine.h L12-38 GroupName_* 走 tr() |

**代码复用**：`queryparser` + 8 个 `providers` + `i18n`（业务核心 ~2600 行）直迁；
`resultbuilder`（V1.0 JSON 组装）**废弃**，替换为 worker 内 `MatchedItem` 适配；
`main.cpp`/`searchpluginadaptor`/`convertsearch`（DBus 层）**废弃**；`clipboard` 保留改造。

## 2. 官方接入机制（核实摘要）

```
搜索请求 → MainController(L75-96 遍历 searcher: isActive→activate→createWorker)
        → TaskCommander(L261-279: worker 入同步/异步队列, 连接 unearthed 信号)
        → ConvertWorker::working() 本地解析+计算
        → ProxyWorker::unearthed → onUnearthed(taskcommander.cpp L51) → worker->takeAll()
        → MatchedItemMap → matched 信号 → 前端展示（组名经 datadefine.h tr() 翻译）
点击结果 → OpenWithPlugin(grandsearchinterface.cpp L207) → searcherAction(maincontroller.cpp L313)
        → ConvertSearcher::action() → 写剪贴板
```

- `Searcher` 四虚方法：`name()/isActive()/createWorker()/action()`（searcher.h L13-23）
- `ProxyWorker` 关键接口（已核实 proxyworker.h L16-33）：`setContext(QString&)`（关键词为纯字符串，
  与插件版 cont 同构）、`working(void*)`、`terminate()`、`hasItem()`、`takeAll()→MatchedItemMap`
- **调度模型（已核实 taskcommander.cpp L155-197）**：`isAsync()` 决定 worker 入异步/同步队列；
  同步队列走 `QtConcurrent::map`，异步走 `asyncFinished`。convert worker 声明 async（见 §4.1）
- **结果汇聚（已核实 taskcommander.cpp L51-83/L248-279）**：`join()` 连接 `unearthed` → `onUnearthed()`
  → `worker->takeAll()` 合并至 `m_results/m_buffer` → `matched` 信号；worker 可多次 unearthed 增量通知
- **MatchedItem 构造先例（已核实 semanticworker.cpp L80-115）**：去重 → `FileSearchUtils::packItem(...)` →
  extra/weight 设置 → 按 `GRANDSEARCH_GROUP_FILE_SEMANTIC` 插入组。convert **不复用** packItem
  （它属 file 域 utils，跨业务复用违反单一职责），worker 内直接构造 MatchedItem（3-5 行）；
  convert 各 provider 单类型触发、结果天然互异，**无需去重**，仅保留 ≤100 条截断
- **点击入口（已核实 grandsearchinterface.cpp L207-212）**：`OpenWithPlugin(searcher, item)` →
  `searcherAction(..., PLUGIN_PROTOCOL_ACTION_OPEN, item)` → 对应 `Searcher::action()`——方法名带 Plugin
  但为统一点击通道（R2-③ 关闭，剩两点见 R2）
- **content 双态格式（已核实，需适配）**：无 `:` 输入（`100usd` 等）→ parseKeyword 返回 false
  （searchhelper.cpp L58），content 原样传递，全部 searcher 参与（searcherData 空 → L81 不过滤）✓；
  **含 `:` 输入**（`计算:12*8` 等）→ parseKeyword 解析 → buildKeywordInJson（maincontroller.cpp L161-184）
  把 content 改写为 `{"group":[],"suffix":[],"keyword":["计算","12*8"]}`——ConvertWorker 必须兼容：
  working() 开头尝试 JSON 解析，含 `keyword` 数组则 join(" ") 作转换文本（触发词分隔符本就支持空格，
  `计算 12*8` 可命中）；含 `group`/`suffix` 数组（用户显式指定文件类目）则**跳过转换**不出结果
- **semantic 实为同步 worker**（isAsync()=false，semanticworker.cpp L157-160）——convert 异步决策
  不变，但异步模板以 proxyworker.h 契约为准（`asyncFinished` 信号必发）；状态机对齐 semantic 写法
  （QAtomicInt testAndSetRelease Ready→Runing→Completed，terminate 时 storeRelease(Terminated)，
  working 内长操作检查状态，无需额外 m_cancelled 原子量）
- 注册点：`searchergroup.cpp` `initBuiltin()`（L23-51）实例化加入 `m_builtin`

## 3. 文件清单

### 3.1 新增（官方仓库 src/dde-grand-search-daemon/searcher/convert/）

| 文件 | 说明 |
|------|------|
| `convertsearcher.h/.cpp` | Searcher 子类：name() 返回内置项宏；isActive() 读配置开关；createWorker() 返回 ConvertWorker；action() 写剪贴板（openitem） |
| `convertworker.h/.cpp` | ProxyWorker 子类：setContext 取关键词 → QueryParser 判型 → Provider 计算 → 转 MatchedItemMap（单组，type 区分 8 类）→ unearthed/takeAll；terminate() 置取消标记（汇率请求中断） |
| `queryparser.h/.cpp` | 直迁（namespace GrandSearch，SPDX 头替换） |
| `providers/`（16 文件） | 直迁 8 类 provider |
| `i18n.h/.cpp` | 直迁（结果文本本地化词表） |
| `clipboard.h/.cpp` | 保留（QClipboard 封装，去 DBus 回退——daemon 有事件循环，不需要） |

### 3.2 修改（官方仓库既有文件，共 7 处）

| 文件 | 改动 |
|------|------|
| `src/dde-grand-search-daemon/CMakeLists.txt` | L68-114 SEARCHER 显式列表追加 convert/ 9 组源文件；find_package/target_link_libraries 补 `Qt${QT_NS}::Network` |
| `src/dde-grand-search-daemon/searchergroup.cpp` | initBuiltin() 实例化 ConvertSearcher |
| `src/global/builtinsearch.h` | 新增 `GRANDSEARCH_CLASS_CONVERT` + `GRANDSEARCH_GROUP_CONVERT` 宏 |
| `src/dde-grand-search-daemon/maincontroller/maincontroller.cpp` | checkSearcher() 默认分支（L147-153）追加 `data.append(GRANDSEARCH_CLASS_CONVERT)`——否则普通关键字搜索不触发转换（阻断性） |
| `src/global/searchconfigdefine.h` | **不改**（通用开关机制已覆盖，见 §1） |
| `src/grand-search/gui/datadefine.h` | 新增 `GroupObjName_Convert`/`GroupName_Convert`（tr() 基准英文，机制已核实 datadefine.h L13-36） |
| `src/grand-search` 翻译 `.ts` | 新增 "Convert" 词条（走官方 .tx 翻译流程；.ts 具体位置提交 6 时核实 lupdate 产物路径） |
| `.reuse/dep5` | convert/ 目录许可声明段 |

### 3.3 本仓库保留（不进 PR）

`.github/workflows`、`tests/`、`scripts/`、`docs/`、`debian/` + `package.sh`
（独立 deb 双渠道：PR 合入前/被拒时仍可分发插件版——内置化与插件版代码同源，provider 层零改动）

## 4. 关键设计

### 4.1 ConvertWorker 结果适配（替代原 ResultBuilder）

```
QueryParser.parse(cont)
  → Provider 结果（多条，每条含 name 文本/说明）
  → MatchedItem{ item: "convert-<序号>", name: 文本, icon: 8类图标, type: "convert/<类名>" }
  → 单组 key = GRANDSEARCH_GROUP_CONVERT，组内 ≤100 条（协议硬限同源）
  → tryNotify() 即刻出结果；汇率命中时网络返回后增量补充 unearthed
```

- **异步模型（调度已核实）**：worker 声明 async（`isAsync()` true）→ 入 TaskCommander 异步队列。
  working() 内先跑本地 7 类（<10ms）→ tryNotify；若解析命中汇率，在 worker 线程内用
  QNetworkAccessManager + 线程局部 QEventLoop 等待（超时 ≤3s），terminate() 置取消标记时立即退出
  事件循环——同步 get 不可中断，禁用。增量 unearthed 前检查取消标记，避免已终止 worker 仍发结果
- **item ID 全局唯一性（实施核实点）**：`convert-<序号>` 前缀冲突概率低，但需核实官方
  内置项 item ID 生成惯例（matcheditem.h 及各 searcher 实现），对齐后定稿
- **缓存生命周期**：`QHash<item, 剪贴板文本>` 为 worker 实例成员，随 worker 销毁释放；
  每次 setContext 时清空重建，杜绝跨搜索串结果（插件版 m_lastResults 同思路，无 mID 后靠生命周期隔离）
- **MatchedItem 必填字段（实施核实点）**：icon/type/name 的必填与取值规范在改造前
  对照 matcheditem.h 与 semanticworker 结果构造逐字段核实，本计划"icon 可空"结论待验证

### 4.2 解析与展示行为

- 解析优先级链、触发词最长优先、省略目标全列、未识别返回"输入 help"提示卡——**行为与插件版完全一致**（基准见 [FEATURES.md](FEATURES.md)）
- 组内展示顺序用 `MatchedItem.extra` 的 `itemLevel`（builtinsearch.h L36-39）标注首条为第一等级

### 4.3 骨架代码设计（签名级草案，对齐已核实接口）

```cpp
// convertsearcher.h —— 模板对齐 semanticsearcher.cpp L23-71
class ConvertSearcher final : public Searcher {
public:
    QString name() const override;          // 返回 GRANDSEARCH_CLASS_CONVERT 宏
    bool isActive() const override;         // 返回 true（开关由 maincontroller 通用 config 机制处理）
    bool activate() override;               // 轻量初始化（词表预加载），返回 true
    ProxyWorker *createWorker() override;   // return new ConvertWorker(name(), this)
    bool action(const QString &action,
                const QString &item) override; // ACTION_OPEN: 查缓存 → QClipboard 复制
};

// convertworker.h —— 接口对齐已核实的 proxyworker.h（全文）
class ConvertWorker final : public ProxyWorker {
    Q_OBJECT
public:
    explicit ConvertWorker(const QString &name, QObject *parent = nullptr)
        : ProxyWorker(name, parent) {}      // 基类构造必须传 searcher name
    void setContext(const QString &context) override; // 保存关键词；清空结果缓存
    bool isAsync() const override { return true; }    // 异步队列（§4.1）
    bool working(void *context) override;   // 主流程（见下），结束时 emit asyncFinished(this)
    void terminate() override;              // 置 m_cancelled（原子），汇率事件循环立即退出
    Status status() override;               // Ready/Runing/Completed/Terminated 状态机
    bool hasItem() const override;          // m_results 非空
    MatchedItemMap takeAll() override;      // 返回 {GRANDSEARCH_GROUP_CONVERT: m_results} 并清空（增量语义）
signals:
    // unearthed/asyncFinished 继承自基类
private:
    void parseAndCompute(const QString &text);  // QueryParser → Provider → 结果列表
    void tryNotify();                           // emit unearthed(this)（emit 前查 m_cancelled）
    QHash<QString, QString> m_copyMap;          // item ID → 剪贴板文本（action 用）
};
```

`working()` 主流程：状态机 Ready→Runing → **content 适配**（JSON 解析：含 keyword 数组则
join(" ") 为输入文本；含 group/suffix 数组则直接 Completed 返回）→ `QueryParser.parse(text)` →
未识别出 help 提示卡 → 本地 provider 逐条构造 `MatchedItem{item: "convert-N", name, icon,
type: "convert/<类>", searcher: GRANDSEARCH_CLASS_CONVERT}`（**searcher 字段必填**——action 链路
按 item.searcher 定位 searcher，matcheditem.h L18；N 单调递增，`m_copyMap[item]=复制文本`）→
截断 ≤100 → `tryNotify()`（emit 前查状态≠Terminated）→ 若命中汇率且未终止 → 异步网络（≤3s
事件循环，循环内查状态）→ 结果 append → `tryNotify()` 二次增量 → 状态 Runing→Completed →
`emit asyncFinished(this)` → 返回 true。

## 5. 提交序列（分支 `feat/builtin-convert-searcher`，conventional commits）

| # | 提交信息 | 内容 | 可构建 |
|---|---------|------|:---:|
| 1 | `feat(daemon): add convert searcher skeleton` | convert/ 骨架（searcher+worker+queryparser+i18n+clipboard）+ CMake 源列表 + Network 链接 + searchergroup/builtinsearch.h 注册 + **checkSearcher 默认分支加 convert（阻断性，maincontroller.cpp L150-152）**（空结果行为） | ✅ 已完成 `adba85a`（2026-10-16）：Qt6 daemon 整树编译通过；Network 链接顺延至提交 3（骨架无网络代码，避免无使用依赖）；顺手修直迁代码 2 个真问题——全角冒号 `QLatin1Char('：')` multichar bug（改 `QChar(0xFF1A)`，原插件版该分支从未生效）+ 删未用 `isTemperatureScale` |
| 2 | `feat(daemon): add local conversion providers` | providers/ 7 类本地转换 + worker 适配 MatchedItemMap | ✓ |
| 3 | `feat(daemon): add currency provider with offline fallback` | currency + 网络取消标记 | ✓ |
| 4 | `feat(daemon): add clipboard action for convert results` | action() 实现 | ✓ |
| 5 | `feat(daemon): integrate convert searcher into settings ui` | 确认设置界面 searcher 开关来源（GrandSearchSettingService 是否自动遍历 searcher，或固定列表加 convert 一项）；通用开关机制已覆盖后端（§1），此提交仅 UI 接入 | ✓ |
| 6 | `feat(gui): add group name for convert results` | 前端 datadefine.h + .ts 词条 | ✓ |
| 7 | `chore(reuse): declare licensing for convert searcher` | dep5 追加 + SPDX 头全量核对 | ✓ |

每提交后在官方源码树整树构建通过再继续。**分阶段验证**：提交 1-5 期间前端组名未映射，
结果可能不在界面显示（前端组白名单行为未核实）——此阶段用 dbus 侧/日志确认
`takeAll()` 返回含 `group.convert` 的 MatchedItemMap 即可；提交 6 完成后才做界面目视验证。

**双渠道维护决策**：本仓库为开发主仓（tests/ 在此持续运行）；上游合入后 provider 层改动
需同步两处——上游小改直接在本仓库镜像提交，大改先在本仓库验证再上游 PR；本仓库插件版
deb 保留发布（覆盖未升级官方包的旧系统用户）。

## 6. 验证清单

1. **直迁回归等价验证（关键）**：providers/queryparser/i18n 迁入 daemon 树后，把本仓库
   `tests/run_tests.sh` 的 include 路径指向新目录跑通全部断言（官方仓库不含测试框架，
   回归验证在工作区完成，PR 附断言结果）——这是 R4 崩溃风险的缓解闭环
2. 整树构建 Qt6 / Qt5 各一遍
3. `dpkg-buildpackage -us -uc -b` → `dpkg -c` 确认新 daemon 二进制进包（内置无独立文件，对比包内 daemon 变化）
4. 安装 → postinst 自动重启 daemon → 界面逐条验证 10 类输入
   （`100usd`/`1kg=?斤`/`北京时间`/`12*8+sqrt(16)`/`base64 encode hello`/`距 2027-01-01 还有几天`/`#ff8800`/`100 kcal`/`help`/乱输入须出提示卡）
5. 点击结果 → 剪贴板验证；设置项开关转换搜索后不参与搜索
6. **异步时序验证**：汇率输入下先出本地结果卡、网络返回后增量补卡；terminate 后无延迟返回
7. cppcheck 零新增告警；SPDX/dep5 全覆盖

## 7. PR 描述模板

```markdown
## Summary
Add a built-in convert searcher to dde-grand-search-daemon: an all-in-one
converter (currency / unit / timezone / calc / programmer tools / datetime /
color / energy + inline help) alongside file/app/semantic searchers,
following the existing Searcher + ProxyWorker model.

- Ships in the daemon binary; group shown via datadefine.h tr() entry
- Click-to-copy implemented in ConvertSearcher::action() (QClipboard)
- Only currency touches network (frankfurter.app, 3s timeout + TTL cache +
  static fallback), with cancel-aware terminate(); adds Qt::Network linkage
  (semantic searcher already performs online search)
- User switch convert.enabled follows the semantic.enabled precedent
- 7 of 8 converters are pure local computation, no privacy upload

## Test plan
- [ ] full-tree build Qt5 & Qt6
- [ ] 10-class input matrix via global search UI, incl. help & unknown-input hint
- [ ] click result → clipboard; settings switch disables searcher
- [ ] cppcheck clean
```

## 8. 风险与应对

| # | 风险 | 等级 | 应对 |
|---|------|------|------|
| R1 | 产品级合入被拒（改动 daemon 核心 + 前端，官方把 calculator 仅放 examples） | **高** | PR 强调对齐 Searcher+ProxyWorker 既有模型、单组最小前端改动、7/8 类零网络；被拒则退回方案 B（主包插件）/examples 双渠道，provider 层零改动可整体回迁 |
| R2 | 未知项终态：**0 阻断项**。仅剩 3 个提交内 10 分钟级实测动作：① exhibition 侧未知组展示行为（提交 6）；② 设置 UI 开关来源（提交 5，后端通用机制已确认）；③ .ts 文件位置（提交 6，跑一次 lupdate 即得）。已关闭的核实：activate/worker 每搜索新建（L75-96）、checkSearcher 过滤（阻断性适配已入提交 1）、MatchedItem POD 结构与 searcher 字段必填（matcheditem.h）、content 双态格式与 JSON 适配（searchhelper.cpp L56-76 + maincontroller.cpp L161-184）、semantic 实为同步 worker（semanticworker.cpp L157） | 低（降级） | 各自并入对应提交实施时完成 |
| R3 | daemon 引入 Qt Network 依赖被质疑 | 中 | 援引 semantic 在线搜索先例；汇率失败静默降级，无网络时行为与其他内置项一致 |
| R4 | 崩溃隔离丧失（转换 bug 拖垮全局搜索） | 中 | 求值器输入长度/嵌套深度上限；正则防回溯爆炸；直迁回归等价验证（§6.1）+ 边界单测（本仓库 tests/ 持续维护） |
| R9 | 点击复制与内置项"点击=打开"语义不一致，评审可能质疑交互 | 低 | 结果文案/`itemTailer` 标注"点击复制"；PR 描述说明设计动机（轻量参考值无打开目标） |
| R5 | 前端 .ts 词条流程（官方翻译平台） | 低 | PR 只加 tr() 基准英文 + zh_CN 词条，其余语言由 .tx 平台补齐 |
| R6 | CLA 未签 | 中 | 提 PR 前签 deepin CLA |
| R7 | cppcheck 存量告警 | 低 | 提交前对新增目录预跑清理 |
| R8 | commitlint 拒绝提交信息 | 低 | 按 §5 conventional commits |

## 9. 与既有文档的关系

- [REDESIGN.md](REDESIGN.md)：总体设计；本文为方案 C 落地细化，冲突以本文为准
- [FEATURES.md](FEATURES.md)：功能行为基准（内置化后行为完全一致）
- 本仓库 debian/package.sh：双渠道兜底（PR 被拒时发布独立插件版 deb）
