// SPDX-FileCopyrightText: 2026 testerxydw
// SPDX-License-Identifier: GPL-3.0-or-later

#include "uosai.h"
#include "clipboard.h"

#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QDBusInterface>
#include <QDBusMessage>
#include <QDBusReply>
#include <QLoggingCategory>
#include <QVariantMap>

Q_LOGGING_CATEGORY(logUosAi, "convert.search.uosai")

namespace UosAi {
namespace {

const char kService[] = "com.deepin.copilot";
// 主接口（注册应用、拉起对话页等）
const char kMainPath[] = "/com/deepin/copilot";
const char kMainIface[] = "com.deepin.copilot";
// 聊天子接口（真正把文本送入对话输入框）
const char kChatPath[] = "/org/deepin/copilot/chat";
const char kChatIface[] = "org.deepin.copilot.chat";

// 进程内缓存是否已向 copilot 注册过本应用
bool g_registered = false;

bool isServiceOnline()
{
    QDBusConnectionInterface *bus = QDBusConnection::sessionBus().interface();
    if (!bus)
        return false;
    return bus->isServiceRegistered(QString::fromLatin1(kService));
}

// 调用 org.deepin.copilot.chat.inputPrompt(text, {}) 把文本送入 UOS AI 对话输入框。
// 真实接口（本机 introspect + python-dbus 验证通过）：
//   /org/deepin/copilot/chat  org.deepin.copilot.chat.inputPrompt(s a{ss})
// 第二个参数为空字典即可。
bool sendTextViaInputPrompt(const QString &text)
{
    QDBusInterface iface(kService, kChatPath, kChatIface,
                         QDBusConnection::sessionBus());
    if (!iface.isValid()) {
        qCWarning(logUosAi) << "UOS AI chat interface invalid";
        return false;
    }
    QDBusMessage msg = iface.call("inputPrompt", text, QVariantMap());
    if (msg.type() == QDBusMessage::ErrorMessage) {
        qCWarning(logUosAi) << "UOS AI inputPrompt failed:" << msg.errorMessage();
        return false;
    }
    qCInfo(logUosAi) << "UOS AI text sent via inputPrompt()";
    return true;
}

} // namespace

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

    // 主接口：首次调用时注册本应用，便于 UOS AI 对话中识别「万能转换器」入口
    if (!g_registered) {
        QDBusInterface mainIface(kService, kMainPath, kMainIface,
                                 QDBusConnection::sessionBus());
        if (mainIface.isValid()) {
            QDBusReply<QString> appId = mainIface.call("registerApp");
            if (appId.isValid())
                qCInfo(logUosAi) << "Registered to UOS AI, appId:" << appId.value();
            QVariantMap meta;
            meta.insert("name", "万能转换器");
            meta.insert("description", "在全局搜索中完成单位/汇率/时间/计算/编码/颜色/热量转换");
            mainIface.call("registerAppCmdPrompts", QVariant::fromValue(meta),
                           QStringList{"万能转换器", "convert"});
        }
        g_registered = true;
    }

    // 优先：通过 chat.inputPrompt 把文本直接送入对话输入框
    bool sent = sendTextViaInputPrompt(text);

    if (!sent) {
        // 兜底：拉起对话页 + 写剪贴板让用户手动粘贴
        QDBusInterface mainIface(kService, kMainPath, kMainIface,
                                 QDBusConnection::sessionBus());
        mainIface.call("launchChatPage");
        qCInfo(logUosAi) << "inputPrompt failed, fallback launchChatPage called";
        const bool copied = Clipboard::setText(text);
        qCInfo(logUosAi) << "UOS AI fallback clipboard set:" << copied;
        return copied;
    }

    qCInfo(logUosAi) << "UOS AI text sent successfully";
    return true;
}

} // namespace UosAi
