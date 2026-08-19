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
// 1) 注册本应用并登记命令提示（首调用时，主接口 /com/deepin/copilot）
// 2) 调用 org.deepin.copilot.chat.inputPrompt(text, {}) 把文本送入对话输入框
//    （接口路径 /org/deepin/copilot/chat，签名为 s a{ss}，已本机验证）
// 3) inputPrompt 失败时回退：launchChatPage + 写剪贴板让用户手动粘贴
// 返回是否成功发起（true 代表 inputPrompt 已发送或剪贴板已写入）
bool sendToCopilot(const QString &text);

} // namespace UosAi

#endif // UOSAI_H
