// SPDX-FileCopyrightText: 2026 testerxydw
// SPDX-License-Identifier: GPL-3.0-or-later

#include "convertsearch.h"
#include "queryparser.h"
#include "resultbuilder.h"
#include "providers/unitprovider.h"
#include "providers/timeprovider.h"
#include "providers/calcprovider.h"
#include "providers/programprovider.h"
#include "providers/datetimeprovider.h"
#include "providers/colorprovider.h"
#include "providers/energyprovider.h"
#include "i18n.h"

#include <QJsonDocument>
#include <QJsonParseError>
#include <QJsonArray>
#include <QGuiApplication>
#include <QClipboard>
#include <QLoggingCategory>

Q_LOGGING_CATEGORY(logConvert, "convert.search.plugin")

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
    default:
        break; // None：返回空，不影响其他搜索项
    }

    QJsonDocument doc(root);
    QString out = doc.toJson(QJsonDocument::Compact);

    {
        QMutexLocker locker(&m_mutex);
        m_lastResults.insert(mID, root);
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

    if (action != "openitem" || item.isEmpty())
        return false;

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

    // 仅在存在显示会话时操作剪贴板，避免无 GUI 环境（headless）下阻塞
    const bool hasDisplay = !qEnvironmentVariableIsEmpty("DISPLAY")
                            || !qEnvironmentVariableIsEmpty("WAYLAND_DISPLAY");
    if (!hasDisplay)
        return false;
    QGuiApplication::clipboard()->setText(text);
    qCInfo(logConvert) << "Copied to clipboard:" << text;
    return true;
}
