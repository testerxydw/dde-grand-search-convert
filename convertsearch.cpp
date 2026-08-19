// SPDX-FileCopyrightText: 2026 testerxydw
// SPDX-License-Identifier: GPL-3.0-or-later

#include "convertsearch.h"
#include "queryparser.h"
#include "resultbuilder.h"
#include "clipboard.h"
#include "providers/unitprovider.h"
#include "providers/timeprovider.h"
#include "providers/calcprovider.h"
#include "providers/programprovider.h"
#include "providers/datetimeprovider.h"
#include "providers/colorprovider.h"
#include "providers/energyprovider.h"
#include "uosai.h"
#include "i18n.h"

#include <QJsonDocument>
#include <QJsonParseError>
#include <QJsonArray>
#include <QGuiApplication>
#include <QLoggingCategory>

Q_LOGGING_CATEGORY(logConvert, "convert.search.plugin")

// 使用说明卡片：列出全部功能与示例，跟随系统语言展示。
static QList<ResultBuilder::Item> buildHelp()
{
    const bool zh = I18n::isChinese();
    auto mk = [](const QString &key, const QString &name) {
        ResultBuilder::Item it;
        it.key = key;
        it.name = name;
        it.icon = "dialog-information";
        it.type = "convert/help-item";
        return it;
    };
    QList<ResultBuilder::Item> items;
    if (zh) {
        items << mk("help-currency", "汇率：100usd / 100 USD to CNY / 100美元");
        items << mk("help-unit",     "单位：12inch / 1kg=?斤 / 3km / 100F");
        items << mk("help-time",     "时区：北京时间 / tokyo now / 纽约时间");
        items << mk("help-calc",     "计算器：12*8+sqrt(16) / 255 to hex（整数附 HEX/BIN/OCT）");
        items << mk("help-prog",     "程序员工具：base64 encode hello / md5 hello / 时间戳 / ascii A");
        items << mk("help-date",     "日期：距 2027-01-01 还有几天 / 2025-01-01 到 2025-12-31 / 今天");
        items << mk("help-color",    "颜色：#ff8800 / rgb(255,136,0) / hsl(32,100%,50%)");
        items << mk("help-energy",   "热量：100 kcal（→ 千焦/卡 + 食物份数，控卡必备）");
        items << mk("help-tip",      "提示：结果点击即复制到剪贴板；输入 help 随时查看本说明");
    } else {
        items << mk("help-currency", "Currency: 100usd / 100 USD to CNY / 100美元");
        items << mk("help-unit",     "Unit: 12inch / 1kg=?斤 / 3km / 100F");
        items << mk("help-time",     "Timezone: 北京时间 / tokyo now / 纽约时间");
        items << mk("help-calc",     "Calculator: 12*8+sqrt(16) / 255 to hex (HEX/BIN/OCT)");
        items << mk("help-prog",     "Developer: base64 encode hello / md5 hello / 时间戳 / ascii A");
        items << mk("help-date",     "Date: 距 2027-01-01 还有几天 / 2025-01-01 到 2025-12-31 / 今天");
        items << mk("help-color",    "Color: #ff8800 / rgb(255,136,0) / hsl(32,100%,50%)");
        items << mk("help-energy",   "Energy: 100 kcal (→ kJ/cal + food portions)");
        items << mk("help-tip",      "Tip: click a result to copy; type 'help' anytime");
    }
    return items;
}

ConvertSearch::ConvertSearch(QObject *parent)
    : QObject(parent), m_currency(new CurrencyProvider(this))
{
    qCInfo(logConvert) << "Convert search plugin initialized";
}

ConvertSearch::~ConvertSearch() { }

bool ConvertSearch::parseInput(const QString &json, QString &mID, QString &cont)
{
    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(json.toUtf8(), &err);
    if (err.error != QJsonParseError::NoError)
        return false;
    QJsonObject root = doc.object();
    mID = root.value("mID").toString();
    cont = root.value("cont").toString().trimmed();
    return !mID.isEmpty();
}

QString ConvertSearch::search(const QString &json)
{
    QString mID, cont;
    if (!parseInput(json, mID, cont)) {
        qCWarning(logConvert) << "Invalid search input";
        return ResultBuilder::buildEmpty(mID);
    }
    if (cont.isEmpty())
        return ResultBuilder::buildEmpty(mID);

    qCDebug(logConvert) << "Search mID:" << mID << "cont:" << cont;

    QueryParser::Parsed p = QueryParser::parse(cont);
    QJsonObject root;
    root["ver"] = "1.0";
    root["mID"] = mID;

    // 组名按系统语言返回（统一走 I18n，便于扩展更多语言）
    const QString grpCur = I18n::groupName("currency");
    const QString grpUnit = I18n::groupName("unit");
    const QString grpTime = I18n::groupName("time");
    const QString grpCalc = I18n::groupName("calculator");
    const QString grpProg = I18n::groupName("developer");
    const QString grpDate = I18n::groupName("date");
    const QString grpColor = I18n::groupName("color");
    const QString grpEnergy = I18n::groupName("energy");
    const QString grpHelp = I18n::isChinese() ? "使用说明" : "Help";

    switch (p.type) {
    case QueryParser::Type::Currency: {
        auto items = m_currency->convert(p.value, p.from, p.to);
        ResultBuilder::addGroup(root, grpCur, items);
        break;
    }
    case QueryParser::Type::Unit: {
        auto items = UnitProvider::convert(p.value, p.from, p.to);
        ResultBuilder::addGroup(root, grpUnit, items);
        break;
    }
    case QueryParser::Type::Time: {
        auto items = TimeProvider::query(p.city);
        ResultBuilder::addGroup(root, grpTime, items);
        break;
    }
    case QueryParser::Type::Calc: {
        auto items = CalcProvider::eval(p.text);
        ResultBuilder::addGroup(root, grpCalc, items);
        break;
    }
    case QueryParser::Type::Program: {
        auto items = ProgramProvider::run(p.text);
        ResultBuilder::addGroup(root, grpProg, items);
        break;
    }
    case QueryParser::Type::DateTime: {
        auto items = DateTimeProvider::run(p.text);
        ResultBuilder::addGroup(root, grpDate, items);
        break;
    }
    case QueryParser::Type::Color: {
        auto items = ColorProvider::convert(p.text);
        ResultBuilder::addGroup(root, grpColor, items);
        break;
    }
    case QueryParser::Type::Energy: {
        auto items = EnergyProvider::convert(p.value, p.from, p.to);
        ResultBuilder::addGroup(root, grpEnergy, items);
        break;
    }
    case QueryParser::Type::Help: {
        ResultBuilder::addGroup(root, grpHelp, buildHelp());
        break;
    }
    default: {
        // None：未识别时给出友好提示（而非静默空结果），引导用户查看说明
        ResultBuilder::Item it;
        it.key = "hint-unknown";
        it.name = I18n::isChinese()
            ? "未识别此查询，输入 help 查看全部功能与示例"
            : "Unrecognized query. Type 'help' to see all features & examples";
        it.icon = "dialog-information";
        it.type = "convert/help-hint";
        ResultBuilder::addGroup(root, grpHelp, {it});
        break;
    }
    }

    // UOS AI 联动：对有实质结果（非 help/未识别）的查询，附加「用 UOS AI 处理」卡片，
    // 点击后将原始查询发送给 UOS AI 对话（见 action 中 convert/uosai 处理）。
    if (p.type != QueryParser::Type::Help && p.type != QueryParser::Type::None
        && UosAi::isAvailable()) {
        ResultBuilder::Item ai;
        ai.key = "uosai-send";
        ai.name = I18n::isChinese()
            ? QString("用 UOS AI 处理：%1").arg(cont)
            : QString("Process with UOS AI: %1").arg(cont);
        ai.icon = "deepin-ai-assistant";
        ai.type = "convert/uosai";
        // 把原始查询存入 item 字段（复用 key 之外，name 已含文本；action 时直接用 cont）
        ResultBuilder::addGroup(root, grpHelp, {ai});
    }

    QJsonDocument doc(root);
    QString out = doc.toJson(QJsonDocument::Compact);

    {
        QMutexLocker locker(&m_mutex);
        m_lastResults.insert(mID, root);
        m_lastCont.insert(mID, cont);
    }
    return out;
}

bool ConvertSearch::stop(const QString &json)
{
    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(json.toUtf8(), &err);
    if (err.error != QJsonParseError::NoError)
        return false;
    QString mID = doc.object().value("mID").toString();
    QMutexLocker locker(&m_mutex);
    m_lastResults.remove(mID);
    m_lastCont.remove(mID);
    qCDebug(logConvert) << "Stop task:" << mID;
    return true;
}

bool ConvertSearch::action(const QString &json)
{
    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(json.toUtf8(), &err);
    if (err.error != QJsonParseError::NoError)
        return false;
    QJsonObject root = doc.object();
    QString action = root.value("action").toString();
    QString item = root.value("item").toString();
    QString mID = root.value("mID").toString();

    if (action != "openitem" || item.isEmpty())
        return false;

    // UOS AI 联动项：把原始查询发送给 UOS AI 对话，而非复制到剪贴板
    if (item == "uosai-send") {
        QMutexLocker locker(&m_mutex);
        QString cont = m_lastCont.value(mID);
        if (cont.isEmpty())
            return false;
        locker.unlock();
        bool ok = UosAi::sendToCopilot(cont);
        qCInfo(logConvert) << "UOS AI send:" << cont << "ok=" << ok;
        return ok;
    }

    // 从缓存结果中找到对应 item 的 name（展示文本），复制到剪贴板
    QString text;
    {
        QMutexLocker locker(&m_mutex);
        for (auto it = m_lastResults.begin(); it != m_lastResults.end(); ++it) {
            QJsonArray groups = it.value().value("cont").toArray();
            for (const QJsonValue &g : groups) {
                QJsonArray items = g.toObject().value("items").toArray();
                for (const QJsonValue &v : items) {
                    QJsonObject obj = v.toObject();
                    if (obj.value("item").toString() == item) {
                        text = obj.value("name").toString();
                        break;
                    }
                }
                if (!text.isEmpty())
                    break;
            }
            if (!text.isEmpty())
                break;
        }
    }
    if (text.isEmpty())
        return false;

    // 写入系统剪贴板（Qt clipboard 优先，失败回退 deepin 剪贴板 DBus 服务）。
    // 由 Clipboard::setText 统一处理无头环境，不依赖本进程 DISPLAY 环境变量。
    const bool ok = Clipboard::setText(text);
    if (ok)
        qCInfo(logConvert) << "Copied to clipboard:" << text;
    return ok;
}
