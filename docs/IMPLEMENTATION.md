# 实现计划 · 里程碑

> 基于 deepin-skills 规范与官方计算器示例。环境：Qt5/Qt6 dev 已装，dde-grand-search-daemon 已装（可本机联调）。

## 里程碑

### M1 · 工程骨架（骨架可编译）
- [ ] CMakeLists.txt（Qt 自动探测、安装路径、conf 模板生成，参考 calculator 示例）
- [ ] main.cpp（DBus 服务注册）
- [ ] searchpluginadaptor（由官方 XML 经 qdbusxml2cpp 生成）
- [ ] convert-search.conf.in 配置模板
- [ ] 最小 stub：search 返回固定卡片，验证 daemon 加载

### M2 · 单位换算（纯本地，最先打通）
- [ ] queryparser：单位正则与类型判定
- [ ] unitprovider：长度/重量/温度/面积/体积/速度/数据 换算表
- [ ] resultbuilder：组装分组 JSON
- [ ] 联调：`12inch`、`1kg=?斤`、`100F`

### M3 · 时区时间（纯本地）
- [ ] timeprovider：城市→IANA 时区映射（覆盖主要城市）
- [ ] 当前时间 + 与本地时差计算
- [ ] 联调：`北京时间`、`tokyo now`

### M4 · 汇率换算（联网+缓存+降级）
- [ ] currencyprovider：符号/代码/中文币种解析
- [ ] 联网获取（QNetworkAccessManager + 超时），QSettings 本地缓存（TTL）
- [ ] 静态汇率降级表（离线可用）
- [ ] 联调：`100usd`、`100 usd to cny`

### M5 · Action 复制 & 国际化 & 打磨
- [ ] action：点击结果复制到剪贴板（m_lastResults 缓存）
- [ ] 分组名国际化（zh_CN / en）
- [ ] 图标、type 字段规范

### M6 · 打包与提交
- [ ] debian/（control/changelog/rules/copyright），依赖 dde-grand-search
- [ ] README（中/英）、设计文档、AI 对话截图归档（活动要求）
- [ ] GitHub 仓库 + 论坛发帖【deepin插件开发活动】+ 作品名
- [ ] amd64 构建验证；arm64 加分尝试

## 取舍说明（遵循最小实现原则）
- 不引入 exprtk 等 3rdparty 依赖：单位换算用内置表，比表达式引擎更可控、零依赖。
- 汇率优先用只读公开 API，失败降级静态表；不做复杂缓存失效策略，仅 TTL。
- 不抽象“通用 provider 基类”：三类 provider 业务概念不同（联网/本地表/时区），各自独立实现，避免过度抽象。
