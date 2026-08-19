// SPDX-FileCopyrightText: 2026 testerxydw
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef UOSAI_H
#define UOSAI_H

#include <QString>

// UOS AI（com.deepin.copilot）联动封装。
// 设计原则：所有调用容错，UOS AI 不可用时静默失败，不影响转换器主功能。
namespace UosAi {

// 检测 UOS AI 是否可用（服务在线且已启用）
bool isAvailable();

// 将文本发送到 UOS AI 对话：
// 1) 注册本应用并登记命令提示（首调用时）
// 2) 拉起 AI 对话页（launchChatPage）
// 3) 将文本写入系统剪贴板，便于用户在对话中直接粘贴发送
// 返回是否成功发起（仅代表调用未被异常中断，不代表 AI 已收到）
bool sendToCopilot(const QString &text);

} // namespace UosAi

#endif // UOSAI_H
