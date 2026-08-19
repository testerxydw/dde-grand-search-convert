// SPDX-FileCopyrightText: 2026 testerxydw
// SPDX-License-Identifier: GPL-3.0-or-later

#include "uosai.h"
#include "clipboard.h"

#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QDBusInterface>
#include <QDBusReply>
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

// 服务是否在线（仅看 bus 上是否已注册，不强制要求 isCopilotEnabled 返回 true，
// 避免不同 UOS AI 版本接口差异导致整个联动静默失效）。
bool isServiceOnline()
{
    QDBusConnectionInterface *bus = QDBusConnection::sessionBus().interface();
    if (!bus)
        return false;
    return bus->isServiceRegistered(QString::fromLatin1(kService));
}

// 探测并调用「把文本送入 AI 输入框」的方法。各 UOS AI 版本接口名不同，
// 依次尝试候选方法，任一成功即返回 true。签名不确定，调用容错、失败忽略。
bool trySendText(QDBusInterface &iface, const QString &text)
{
    // 候选方法（常见命名），参数统一按 (string text) 试
    static const char *kSendMethods[] = {
        "SendMessage", "sendMessage", "SendText", "sendText",
        "PushQuestion", "pushQuestion", "ProcessText", "processText",
        nullptr
    };
    for (int i = 0; kSendMethods[i]; ++i) {
        QString method = QString::fromLatin1(kSendMethods[i]);
        // QDBusInterface 不缓存远程 introspection，直接尝试调用，失败则忽略
        QDBusMessage msg = iface.call(method, text);
        if (msg.type() != QDBusMessage::ErrorMessage) {
            qCInfo(logUosAi) << "UOS AI text sent via method:" << method;
            return true;
        }
    }
    return false;
}

} // namespace

// 探测可用性：服务在线即视为可用（不强制 isCopilotEnabled，见 isServiceOnline）。
bool isAvailable()
{
    return isServiceOnline();
}

bool sendToCopilot(const QString &text)
{
    if (text.isEmpty())
        return false;
    if (!isServiceOnline()) {
        qCWarning(logUosAi) << "UOS AI service not online:" << kService;
        return false;
    }

    QDBusInterface iface = makeInterface();
    if (!iface.isValid()) {
        qCWarning(logUosAi) << "UOS AI interface invalid";
        return false;
    }

    // 首次调用：注册本应用并登记命令提示，让 UOS AI 对话中可出现「万能转换器」入口
    if (!g_registered) {
        QDBusReply<QString> appId = iface.call("registerApp");
        if (appId.isValid())
            qCInfo(logUosAi) << "Registered to UOS AI, appId:" << appId.value();
        QVariantMap meta;
        meta.insert("name", "万能转换器");
        meta.insert("description", "在全局搜索中完成单位/汇率/时间/计算/编码/颜色/热量转换");
        iface.call("registerAppCmdPrompts", QVariant::fromValue(meta),
                   QStringList{"万能转换器", "convert"});
        g_registered = true;
    }

    // 优先：把文本直接送入 AI 输入框（各版本接口名不同，容错探测）
    bool sent = trySendText(iface, text);

    // 无论如何都拉起 AI 对话页，保证用户能看到窗口
    iface.call("launchChatPage");
    qCInfo(logUosAi) << "launchChatPage called, sent=" << sent;

    // 兜底：写入系统剪贴板，若自动输入未生效，用户可手动粘贴
    const bool copied = Clipboard::setText(text);
    qCInfo(logUosAi) << "UOS AI fallback clipboard set:" << copied;

    // 只要窗口已拉起即视为已发起（sent/copied 任一成功代表内容已传递）
    return true;
}

} // namespace UosAi
