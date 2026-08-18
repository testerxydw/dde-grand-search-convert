// SPDX-FileCopyrightText: 2026 testerxydw
// SPDX-License-Identifier: GPL-3.0-or-later

#include "currencyprovider.h"
#include "i18n.h"

#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QUrl>
#include <QUrlQuery>

const QStringList CurrencyProvider::kDefaultTargets = {
    "CNY", "USD", "EUR", "JPY", "GBP", "HKD",
};

static QString currencyDisplay(const QString &code)
{
    return I18n::currencyName(code);
}

// 内置静态汇率（相对 CNY），作为离线降级。数值为示意，联网后会被覆盖。
const QHash<QString, double> kStaticRates = {
    {"CNY", 1.0}, {"USD", 7.25}, {"EUR", 7.85}, {"JPY", 0.048},
    {"GBP", 9.20}, {"HKD", 0.93}, {"KRW", 0.0053},
};

CurrencyProvider::CurrencyProvider(QObject *parent)
    : QObject(parent)
{
    loadStaticTable();
    fetchRates(); // 启动即尝试刷新缓存
}

void CurrencyProvider::loadStaticTable()
{
    for (auto it = kStaticRates.begin(); it != kStaticRates.end(); ++it)
        m_rates[it.key()] = it.value();
    m_cacheTime = QDateTime::currentDateTime();
}

bool CurrencyProvider::rate(const QString &code, double &out) const
{
    if (!m_rates.contains(code))
        return false;
    out = m_rates.value(code);
    return true;
}

QList<ResultBuilder::Item> CurrencyProvider::convert(double value, const QString &from,
                                                     const QString &to)
{
    QList<ResultBuilder::Item> items;
    double fromRate = 0.0;
    if (!rate(from.toUpper(), fromRate) || fromRate <= 0.0)
        return items;

    double cny = value * fromRate; // 先折算到 CNY

    QStringList targets;
    if (!to.isEmpty())
        targets << to.toUpper();
    else
        targets = kDefaultTargets;

    for (const QString &tc : targets) {
        if (tc == from.toUpper())
            continue;
        double toRate = 0.0;
        if (!rate(tc, toRate) || toRate <= 0.0)
            continue;
        double out = cny / toRate;
        ResultBuilder::Item it;
        it.key = QString("cur-%1-%2-%3").arg(from.toUpper()).arg(value).arg(tc);
        it.name = QString("%1 %2 = %3 %4")
            .arg(QLocale().toString(value, 'f', 2)).arg(currencyDisplay(from.toUpper()))
            .arg(QLocale().toString(out, 'f', 2)).arg(currencyDisplay(tc));
        it.icon = "preferences-system"; // 货币图标主题名，依环境
        it.type = "convert/currency";
        items.append(it);
    }
    return items;
}

void CurrencyProvider::fetchRates()
{
    if (m_fetching)
        return;
    m_fetching = true;

    // 使用只读公开 API（frankfurter.app，免 key，EUR 基准，转换为相对 CNY）
    // 失败时保持静态表，不影响功能。
    static QNetworkAccessManager *nam = nullptr;
    if (!nam)
        nam = new QNetworkAccessManager(this);

    QUrl url("https://api.frankfurter.app/latest?from=CNY&to=USD,EUR,JPY,GBP,HKD,KRW");
    QNetworkReply *reply = nam->get(QNetworkRequest(url));
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        m_fetching = false;
        if (reply->error() == QNetworkReply::NoError) {
            QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
            if (doc.isObject()) {
                QJsonObject root = doc.object();
                QJsonObject rates = root.value("rates").toObject();
                // frankfurter 返回「1 CNY = X 目标币」，需取倒数得到「目标币 → CNY」
                for (auto it = rates.begin(); it != rates.end(); ++it) {
                    double v = it.value().toDouble();
                    if (v > 0.0)
                        m_rates[it.key()] = 1.0 / v;
                }
                m_rates["CNY"] = 1.0;
                m_cacheTime = QDateTime::currentDateTime();
            }
        }
        reply->deleteLater();
    });
}
