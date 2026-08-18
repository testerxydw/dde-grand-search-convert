// SPDX-FileCopyrightText: 2026 testerxydw
// SPDX-License-Identifier: GPL-3.0-or-later

#include "datetimeprovider.h"

#include <QRegularExpression>
#include <QDate>
#include <QLocale>

namespace {

// 提取文本中所有 YYYY-MM-DD / YYYY.M.D / YYYY/MM/DD 形式的日期
QList<QDate> extractDates(const QString &s)
{
    QList<QDate> out;
    QRegularExpression re(R"((\d{4})[-./](\d{1,2})[-./](\d{1,2}))");
    QRegularExpressionMatchIterator it = re.globalMatch(s);
    while (it.hasNext()) {
        QRegularExpressionMatch m = it.next();
        QDate d = QDate(m.captured(1).toInt(), m.captured(2).toInt(), m.captured(3).toInt());
        if (d.isValid())
            out.append(d);
    }
    // 中文：X月X日（默认今年）
    QRegularExpression cnRe(R"((\d{1,2})月(\d{1,2})[日号])");
    QRegularExpressionMatchIterator cnIt = cnRe.globalMatch(s);
    int year = QDate::currentDate().year();
    while (cnIt.hasNext()) {
        QRegularExpressionMatch m = cnIt.next();
        QDate d = QDate(year, m.captured(1).toInt(), m.captured(2).toInt());
        if (d.isValid())
            out.append(d);
    }
    return out;
}

QString weekdayText(const QDate &d)
{
    return QLocale(QLocale::Chinese).toString(d, "yyyy-MM-dd dddd");
}

} // namespace

QList<ResultBuilder::Item> DateTimeProvider::run(const QString &text)
{
    QList<ResultBuilder::Item> items;
    QList<QDate> dates = extractDates(text);
    QDate today = QDate::currentDate();
    QString lower = text.toLower();

    // 情况1：两个日期 → 日期差
    if (dates.size() >= 2) {
        qint64 days = qAbs(dates[0].daysTo(dates[1]));
        ResultBuilder::Item it;
        it.key = "date-diff";
        it.name = QString("%1 至 %2 相差 %3 天")
            .arg(weekdayText(dates[0])).arg(weekdayText(dates[1])).arg(days);
        it.icon = "office-calendar"; it.type = "convert/date-diff";
        items.append(it);
        return items;
    }

    // 情况2：含「倒数/倒计时/距/还有几天/距离」→ 距目标日倒数
    if (dates.size() == 1
        && (lower.contains("倒数") || lower.contains("倒计时") || lower.contains("距")
            || lower.contains("还有几天") || lower.contains("距离") || lower.contains("daysuntil")
            || lower.contains("countdown"))) {
        qint64 days = today.daysTo(dates[0]);
        QString rel = (days >= 0) ? QString("还有 %1 天").arg(days)
                                  : QString("已过去 %1 天").arg(-days);
        ResultBuilder::Item it;
        it.key = "date-countdown";
        it.name = QString("%1 · %2").arg(weekdayText(dates[0])).arg(rel);
        it.icon = "office-calendar"; it.type = "convert/date-countdown";
        items.append(it);
        return items;
    }

    // 情况3：单个日期 → 展示星期与距今天数
    if (dates.size() == 1) {
        qint64 days = today.daysTo(dates[0]);
        ResultBuilder::Item it;
        it.key = "date-info";
        it.name = QString("%1 · 距今天 %2 天")
            .arg(weekdayText(dates[0])).arg(days);
        it.icon = "office-calendar"; it.type = "convert/date-info";
        items.append(it);
        return items;
    }

    // 情况4：仅「今天/明天/昨天/today」→ 展示今天信息
    if (lower.contains("今天") || lower.contains("today")) {
        ResultBuilder::Item it;
        it.key = "date-today";
        it.name = weekdayText(today);
        it.icon = "office-calendar"; it.type = "convert/date-today";
        items.append(it);
        return items;
    }

    return items;
}
