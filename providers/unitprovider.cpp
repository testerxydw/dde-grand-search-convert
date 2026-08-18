// SPDX-FileCopyrightText: 2026 testerxydw
// SPDX-License-Identifier: GPL-3.0-or-later

#include "unitprovider.h"

#include <QRegularExpression>
#include <QLocale>

namespace {
// 规范化单位键（去空格、转小写，中文保留）
QString norm(const QString &u)
{
    QString s = u.toLower();
    s.remove(' ');
    s.remove('.');
    return s;
}

// 单位 → 中文说明（按系统语言返回，当前优先中文）
static const QHash<QString, QString> kUnitDisplay = {
    // 长度
    {"m", "米"}, {"km", "千米/公里"}, {"cm", "厘米"}, {"mm", "毫米"},
    {"inch", "英寸"}, {"in", "英寸"}, {"feet", "英尺"}, {"ft", "英尺"},
    {"yard", "码"}, {"yd", "码"}, {"mile", "英里"},
    // 重量
    {"kg", "千克/公斤"}, {"g", "克"}, {"mg", "毫克"},
    {"lb", "磅"}, {"lbs", "磅"}, {"oz", "盎司"},
    {"斤", "市斤"}, {"两", "市两"}, {"t", "吨"}, {"吨", "吨"},
    // 面积
    {"m2", "平方米"}, {"km2", "平方千米"}, {"cm2", "平方厘米"},
    {"公顷", "公顷"}, {"ha", "公顷"}, {"亩", "亩"}, {"acre", "英亩"},
    // 体积
    {"l", "升"}, {"liter", "升"}, {"litre", "升"}, {"ml", "毫升"},
    {"gal", "加仑"}, {"m3", "立方米"},
    // 速度
    {"kmh", "千米每小时"}, {"kph", "千米每小时"}, {"km/h", "千米每小时"},
    {"mph", "英里每小时"}, {"mps", "米每秒"}, {"米每秒", "米每秒"},
    {"knot", "节"}, {"节", "节"},
    // 数据
    {"b", "字节"}, {"kb", "千字节"}, {"mb", "兆字节"}, {"gb", "吉字节"}, {"tb", "太字节"},
};

static bool isChineseLocale()
{
    return QLocale().language() == QLocale::Chinese;
}

// 输出格式：原单位（中文名），便于用户理解缩写含义
static QString unitDisplay(const QString &u)
{
    QString n = norm(u);
    QString cn = kUnitDisplay.value(n);
    if (cn.isEmpty())
        return u; // 未知单位保持原样
    return isChineseLocale() ? QString("%1（%2）").arg(u).arg(cn) : u;
}
} // namespace

double UnitProvider::lengthFactor(const QString &u, bool &ok)
{
    ok = true;
    QString s = norm(u);
    if (s == "m" || s == "米") return 1.0;
    if (s == "km" || s == "千米" || s == "公里") return 1000.0;
    if (s == "cm" || s == "厘米") return 0.01;
    if (s == "mm" || s == "毫米") return 0.001;
    if (s == "inch" || s == "in" || s == "英寸") return 0.0254;
    if (s == "feet" || s == "ft" || s == "英尺") return 0.3048;
    if (s == "yard" || s == "yd" || s == "码") return 0.9144;
    if (s == "mile" || s == "英里") return 1609.344;
    ok = false; return 0.0;
}

double UnitProvider::massFactor(const QString &u, bool &ok)
{
    ok = true;
    QString s = norm(u);
    if (s == "kg" || s == "千克" || s == "公斤") return 1.0;
    if (s == "g" || s == "克") return 0.001;
    if (s == "mg" || s == "毫克") return 0.000001;
    if (s == "lb" || s == "lbs" || s == "磅") return 0.45359237;
    if (s == "oz" || s == "盎司") return 0.0283495231;
    if (s == "斤") return 0.5;          // 市斤 = 0.5 kg
    if (s == "两") return 0.05;         // 市两 = 50 g
    if (s == "吨" || s == "t") return 1000.0;
    ok = false; return 0.0;
}

double UnitProvider::areaFactor(const QString &u, bool &ok)
{
    ok = true;
    QString s = norm(u);
    if (s == "m2" || s == "平方米") return 1.0;
    if (s == "km2" || s == "平方千米") return 1e6;
    if (s == "cm2" || s == "平方厘米") return 0.0001;
    if (s == "公顷" || s == "ha") return 10000.0;
    if (s == "亩") return 666.6666667;
    if (s == "acre" || s == "英亩") return 4046.8564224;
    ok = false; return 0.0;
}

double UnitProvider::volumeFactor(const QString &u, bool &ok)
{
    ok = true;
    QString s = norm(u);
    if (s == "l" || s == "升") return 1.0;
    if (s == "ml" || s == "毫升") return 0.001;
    if (s == "gal" || s == "加仑") return 3.785411784;     // 美制加仑
    if (s == "m3" || s == "立方米") return 1000.0;
    ok = false; return 0.0;
}

double UnitProvider::speedFactor(const QString &u, bool &ok)
{
    ok = true;
    QString s = norm(u);
    if (s == "kmh" || s == "kph" || s == "km/h" || s == "千米每小时") return 1.0;
    if (s == "mph" || s == "英里每小时") return 1.609344;
    if (s == "mps" || s == "米每秒") return 3.6;
    if (s == "节" || s == "knot") return 1.852;
    ok = false; return 0.0;
}

double UnitProvider::dataFactor(const QString &u, bool &ok)
{
    ok = true;
    QString s = norm(u);
    if (s == "b" || s == "字节") return 1.0;
    if (s == "kb" || s == "千字节") return 1024.0;
    if (s == "mb" || s == "兆字节") return 1024.0 * 1024.0;
    if (s == "gb" || s == "吉字节") return 1024.0 * 1024.0 * 1024.0;
    if (s == "tb" || s == "太字节") return 1024.0 * 1024.0 * 1024.0 * 1024.0;
    ok = false; return 0.0;
}

bool UnitProvider::tempConvert(double value, const QString &fromScale,
                               const QString &toScale, double &out)
{
    QString f = norm(fromScale);
    QString t = norm(toScale);
    auto toK = [](double v, const QString &sc) -> double {
        if (sc == "c") return v + 273.15;
        if (sc == "f") return (v - 32.0) * 5.0 / 9.0 + 273.15;
        return v; // k
    };
    auto fromK = [](double k, const QString &sc) -> double {
        if (sc == "c") return k - 273.15;
        if (sc == "f") return (k - 273.15) * 9.0 / 5.0 + 32.0;
        return k;
    };
    double k = toK(value, f);
    out = fromK(k, t);
    return true;
}

static QString tempLabel(const QString &scale)
{
    QString s = norm(scale);
    if (s == "c") return isChineseLocale() ? "°C（摄氏度）" : "°C";
    if (s == "f") return isChineseLocale() ? "°F（华氏度）" : "°F";
    if (s == "k") return isChineseLocale() ? "K（开尔文）" : "K";
    return scale;
}

QList<ResultBuilder::Item> UnitProvider::convert(double value, const QString &from,
                                                 const QString &to)
{
    QList<ResultBuilder::Item> items;
    QString f = norm(from);

    // 温度分支
    if (f == "c" || f == "f" || f == "k") {
        QStringList targets;
        if (!to.isEmpty() && (norm(to) == "c" || norm(to) == "f" || norm(to) == "k"))
            targets << norm(to);
        else
            targets << "c" << "f" << "k";
        for (const QString &ts : targets) {
            if (ts == f) continue;
            double out = 0.0;
            if (!tempConvert(value, f, ts, out)) continue;
            ResultBuilder::Item it;
            it.key = QString("unit-temp-%1-%2").arg(value).arg(ts);
            it.name = QString("%1%2 = %3%4")
                          .arg(QLocale().toString(value, 'f', 1))
                          .arg(tempLabel(f))
                          .arg(QLocale().toString(out, 'f', 1))
                          .arg(tempLabel(ts));
            it.icon = "preferences-system";
            it.type = "convert/unit-temp";
            items.append(it);
        }
        return items;
    }

    // 线性单位：确定类别并求系数
    auto tryCategory = [&](const QString &cat,
                           std::function<double(const QString &, bool &)> factorFn,
                           const QString &type) -> bool {
        bool okF = false;
        double ff = factorFn(f, okF);
        if (!okF) return false;
        QStringList targets;
        if (!to.isEmpty()) targets << to;
        else targets << "m" << "cm" << "km" << "inch" << "feet" << "mile"; // 默认展示常见单位
        for (const QString &tt : targets) {
            bool okT = false;
            double tf = factorFn(tt, okT);
            if (!okT) continue;
            if (norm(tt) == f) continue;
            double out = value * ff / tf;
            ResultBuilder::Item it;
            it.key = QString("unit-%1-%2-%3").arg(cat).arg(value).arg(norm(tt));
            it.name = QString("%1 %2 = %3 %4")
                          .arg(QLocale().toString(value, 'f', 2)).arg(unitDisplay(from))
                          .arg(QLocale().toString(out, 'f', 4)).arg(unitDisplay(tt));
            it.icon = "preferences-system";
            it.type = type;
            items.append(it);
        }
        return true;
    };

    if (tryCategory("len", lengthFactor, "convert/unit-length")) return items;
    if (tryCategory("mass", massFactor, "convert/unit-mass")) return items;
    if (tryCategory("area", areaFactor, "convert/unit-area")) return items;
    if (tryCategory("vol", volumeFactor, "convert/unit-volume")) return items;
    if (tryCategory("speed", speedFactor, "convert/unit-speed")) return items;
    if (tryCategory("data", dataFactor, "convert/unit-data")) return items;

    return items;
}
