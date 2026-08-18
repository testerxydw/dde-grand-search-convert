# deepin-skills 调用实证（归档）

> 本文件为「AI 调用 deepin Skills」的详细归档。**发帖所需的 skill 实证已整合进
> `docs/FORUM_POST.md` 的「三、deepin-skills 调用实证」章节**，发帖只需参考该文档。
> 本文件保留作内部归档与截图依据，生成时间：2026-08-18。

## 1. deepin-skills 安装事实（可复现）

```bash
$ ls ~/.codebuddy/skills/
dde-control-center-development/  dde-shell-development/
dde-tray-development/            dtk-development/
```

安装来源：GitHub `linuxdeepin/deepin-skills`（经 `ghfast.top` 镜像下载，
官方 `scripts/install.sh` + `SKILL_HOME=~/.codebuddy` 安装）。

## 2. AI 调用 deepin Skill 实证（use_skill）

本会话通过 `use_skill` 工具实际加载了 `dtk-development` skill，返回其完整
开发指南（DTK 架构、Qt5/Qt6 兼容、DBus/应用入口文档路由表等）。
本项目正是基于该 skill 的 **DBus / Qt 应用开发约束** 完成的 dde-grand-search 插件。

调用记录（节选）：

```
use_skill: dtk-development
→ commandMessage: "The dtk-development skill is loading"
→ Base directory: /home/dp25/.codebuddy/skills/dtk-development
→ 返回 DTK 开发指南正文（架构/CMake/DBus/主题等路由表）
```

说明：deepin-skills 官方仓库当前仅含 4 个 skill（control-center / shell /
tray / dtk），**暂未提供 dde-grand-search 专属 skill**。本插件遵循的是
dde-grand-search 官方 V1.0 插件协议（独立 DBus 服务进程 + Search/Stop/Action
三方法，参考官方 `calculator-search-plugin` 示例），并在开发过程中调用了
deepin-skills 的 `dtk-development` 框架 skill 获取 Qt/DBus 构建规范。

## 3. 工程与仓库实证

- 仓库已推送：`https://github.com/testerxydw/dde-grand-search-convert`
  （`git push -u origin main` 成功，SSH 认证通过）
- 双架构自动构建：`.github/workflows/build.yml`（amd64 原生 + arm64 QEMU 容器）
- 本地编译验证：amd64 编译通过，dbus-send 联调各功能返回正确结果卡片；
  并已打包为 `convert-search-plugin_1.0.0_amd64.deb` 安装 + daemon 端到端验证通过。

## 4. 功能验证摘要（本地 dbus-send 联调）

| 查询 | 结果 |
|------|------|
| `100usd` | 实时汇率换算（联网成功） |
| `1kg=?斤` | 2 斤 |
| `北京时间` | Asia/Shanghai 当前时间 + 时差 |
| `12*8+sqrt(16)` | 100 + HEX/BIN/OCT |
| `255 to hex` | 0xff |
| `base64 encode hello` | aGVsbG8= |
| `md5 hello` | 5d41402abc4b2a76b9719d911017c592 |
| `时间戳` | Unix 秒 / 毫秒 |
| `距 2027-01-01 还有几天` | 还有 136 天 |
| `#ff8800` | RGB rgb(255,136,0) / HSL hsl(32,100%,50%) |

（以上均通过 provider 静态方法直接验证，输出正确。）
