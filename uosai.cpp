// SPDX-FileCopyrightText: 2026 testerxydw
// SPDX-License-Identifier: GPL-3.0-or-later

#include "uosai.h"

#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QDBusInterface>
#include <QDBusReply>
#include <QGuiApplication>
#include <QClipboard>
#include <QLoggingCategory>

Q_LOGGING_CATEGORY(logUosAi, "convert.search.uosai")

namespace UosAi {
namespace {

const char kService[] = "com.deepin.copilot";
const char kPath[] = "/com/deepin/copilot";
const char kIface[] = "com.deepin.copilot";

// 进程内缓存是否已向 copilot 注册过本应用
bool g_registered = false;

QDBusInterface makeInterface()
{
    return QDBusInterface(kService, kPath, kIface, QDBusConnection::sessionBus());
}

} // namespace

bool isAvailable()
{
    if (!QDBusConnection::sessionBus().interface()->isServiceRegistered(kService))
        return false;
    QDBusInterface iface = makeInterface();
    if (!iface.isValid())
        return false;
    QDBusReply<bool> reply = iface.call("isCopilotEnabled");
    if (!reply.isValid())
        return false;
    return reply.value();
}

bool sendToCopilot(const QString &text)
{
    if (text.isEmpty())
        return false;
    if (!isAvailable()) {
        qCWarning(logUosAi) << "UOS AI not available, skip";
        return false;
    }

    QDBusInterface iface = makeInterface();
    if (!iface.isValid())
        return false;

    // 首次调用：注册本应用并登记命令提示，让 UOS AI 对话中可出现「万能转换器」入口
    if (!g_registered) {
        QDBusReply<QString> appId = iface.call("registerApp");
        if (appId.isValid())
            qCInfo(logUosAi) << "Registered to UOS AI, appId:" << appId.value();
        // 登记命令提示（a{sv} 参数 + 命令列表），失败不影响主流程
        QVariantMap meta;
        meta.insert("name", "万能转换器");
        meta.insert("description", "在全局搜索中完成单位/汇率/时间/计算/编码/颜色/热量转换");
        iface.call("registerAppCmdPrompts", QVariant::fromValue(meta),
                   QStringList{"万能转换器", "convert"});
        g_registered = true;
    }

    // 拉起 AI 对话页
    iface.call("launchChatPage");

    // 将文本写入剪贴板，用户可在对话中直接粘贴发送（最稳的跨应用传递方式）
    const bool hasDisplay = !qEnvironmentVariableIsEmpty("DISPLAY")
                            || !qEnvironmentVariableIsEmpty("WAYLAND_DISPLAY");
    if (hasDisplay && qApp) {
        qApp->clipboard()->setText(text);
        qCInfo(logUosAi) << "Sent to UOS AI (clipboard set):" << text;
    }
    return true;
}

} // namespace UosAi
