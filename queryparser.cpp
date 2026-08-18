// SPDX-FileCopyrightText: 2026 Deepin Plugin Contest
// SPDX-License-Identifier: GPL-3.0-or-later

#include "queryparser.h"

#include <QRegularExpression>
#include <QLocale>

namespace QueryParser {

// 货币符号/代码/中文名 → ISO 代码
static const QHash<QString, QString> kCurrencyAliases = {
    {"$", "USD"}, {"us$", "USD"}, {"美元", "USD"}, {"美金", "USD"},
    {"€", "EUR"}, {"eur", "EUR"}, {"欧元", "EUR"},
    {"¥", "CNY"}, {"￥", "CNY"}, {"cny", "CNY"}, {"rmb", "CNY"}, {"人民币", "CNY"}, {"元", "CNY"},
    {"£", "GBP"}, {"gbp", "GBP"}, {"英镑", "GBP"},
    {"jpy", "JPY"}, {"日元", "JPY"}, {"円", "JPY"},
    {"hkd", "HKD"}, {"港币", "HKD"}, {"港币", "HKD"},
    {"krw", "KRW"}, {"韩元", "KRW"},
    {"usd", "USD"}, {"欧元", "EUR"},
};

// 单位键 → 是否温度
static bool isTemperatureScale(const QString &s)
{
    return s == "c" || s == "f" || s == "k" || s == "摄氏度" || s == "华氏度";
}

// 常见单位词（含中文），用于单位识别
static const QStringList kUnitWords = {
    // 长度
    "inch", "in", "英寸", "cm", "厘米", "mm", "毫米", "km", "千米", "公里",
    "m", "米", "mile", "英里", "feet", "ft", "英尺", "yard", "yd", "码",
    // 重量
    "kg", "千克", "公斤", "g", "克", "lb", "lbs", "磅", "oz", "盎司", "斤", "两",
    // 面积
    "m2", "平方米", "km2", "平方千米", "acre", "英亩", "公顷", "亩",
    // 体积
    "l", "升", "ml", "毫升", "gal", "加仑",
    // 速度
    "kmh", "kph", "km/h", "mph", "英里每小时",
    // 数据
    "kb", "mb", "gb", "tb", "千字节", "兆字节", "吉字节",
};

static bool containsUnitWord(const QString &s)
{
    QString lower = s.toLower();
    for (const QString &u : kUnitWords) {
        if (lower.contains(u))
            return true;
    }
    return false;
}

// 城市/时区关键词（中文名 + 英文名）
static const QStringList kTimeKeywords = {
    "北京", "上海", "东京", "纽约", "伦敦", "巴黎", "洛杉矶", "旧金山", "悉尼",
    "莫斯科", "首尔", "新加坡", "香港", "台北", "柏林", "罗马", "多伦多", "芝加哥",
    "beijing", "shanghai", "tokyo", "newyork", "nyc", "london", "paris", "losangeles",
    "sanfrancisco", "sydney", "moscow", "seoul", "singapore", "hongkong", "taipei",
    "berlin", "rome", "toronto", "chicago", "time",
};

static bool containsTimeKeyword(const QString &s)
{
    QString lower = s.toLower();
    lower.remove(' ');
    for (const QString &t : kTimeKeywords) {
        if (lower.contains(t))
            return true;
    }
    return false;
}

// 程序员工具关键词（中文名 + 英文名）
static const QStringList kProgramKeywords = {
    "base64", "ascii", "url", "hash", "md5", "sha1", "sha256", "sha",
    "hex", "编码", "哈希", "时间戳", "当前时间戳", "毫秒时间戳",
};

static bool containsProgramKeyword(const QString &s)
{
    QString lower = s.toLower();
    lower.remove(' ');
    for (const QString &t : kProgramKeywords) {
        if (lower.contains(t))
            return true;
    }
    return false;
}

// 日期/倒数日关键词
static const QStringList kDateKeywords = {
    "倒数日", "倒计时", "距", "还有几天", "生日", "日期差", "相隔", "相差", "daysuntil",
    "countdown", "days", "距离", "几天后", "date", "today", "今天", "明天", "昨天",
};

static bool containsDateKeyword(const QString &s)
{
    QString lower = s.toLower();
    lower.remove(' ');
    for (const QString &t : kDateKeywords) {
        if (lower.contains(t))
            return true;
    }
    return false;
}

// 匹配颜色串：#rgb / #rrggbb / rgb(r,g,b) / hsl(h,s%,l%)
static bool matchColor(const QString &s, QString &out)
{
    QString lower = s.toLower();
    lower.remove(' ');
    QRegularExpression colorRe(R"(^(#([0-9a-f]{3}|[0-9a-f]{6})|rgb\(\d{1,3},\d{1,3},\d{1,3}\)|hsl\(\d{1,3},\d{1,3}%,\d{1,3}%\))$)");
    QRegularExpressionMatch m = colorRe.match(lower);
    if (m.hasMatch()) {
        out = m.captured(0);
        return true;
    }
    return false;
}

// 科学/程序员计算器：算术表达式（含进制前缀 0x/0b/0o）或进制转换请求
static bool matchCalc(const QString &s, QString &expr)
{
    QString lower = s.toLower();
    // 进制转换：数值 + to + hex/bin/oct/dec
    QRegularExpression baseRe(R"(([0-9a-fxbo.]+)\s*(to|->|转为|转)\s*(hex|bin|oct|dec|二进制|八进制|十进制|十六进制))");
    QRegularExpressionMatch bm = baseRe.match(lower);
    if (bm.hasMatch()) {
        expr = lower;
        return true;
    }
    // 算术表达式：含运算符且整体为合法字符（含函数名/常量字母）
    QRegularExpression arithRe(R"(^[\s0-9a-zxbo.+\-*/%^()]+$)");
    if (lower.contains('+') || lower.contains('-') || lower.contains('*')
        || lower.contains('/') || lower.contains('%') || lower.contains('^')
        || lower.startsWith("0x") || lower.startsWith("0b") || lower.startsWith("0o")) {
        // 排除纯单位/货币（已被前面分支拦截），且至少含数字
        if (lower.contains(QRegularExpression(R"([0-9])")) && arithRe.match(lower).hasMatch()) {
            expr = lower;
            return true;
        }
    }
    return false;
}

Parsed parse(const QString &input)
{
    Parsed p;
    p.raw = input.trimmed();
    if (p.raw.isEmpty())
        return p;

    QString s = p.raw.toLower();

    // 1) 货币：符号或代码出现在数字附近
    // 形如 100usd / $50 / 100 usd to cny / 100美元 / 100usd=?cny
    QRegularExpression curRe(R"(([0-9]+(?:\.[0-9]+)?)\s*(usd|eur|cny|gbp|jpy|hkd|krw|美元|美金|欧元|人民币|元|英镑|日元|港币|韩元|\$|€|¥|￥|£))");
    QRegularExpressionMatch curM = curRe.match(s);
    if (curM.hasMatch()) {
        p.type = Type::Currency;
        p.value = curM.captured(1).toDouble();
        QString sym = curM.captured(2);
        p.from = kCurrencyAliases.value(sym, sym.toUpper());

        // 目标币种：to / =? / ? 之后
        QRegularExpression toRe(R"((?:to|=|\?)\s*(usd|eur|cny|gbp|jpy|hkd|krw|美元|美金|欧元|人民币|元|英镑|日元|港币|韩元))");
        QRegularExpressionMatch toM = toRe.match(s);
        if (toM.hasMatch())
            p.to = kCurrencyAliases.value(toM.captured(1), toM.captured(1).toUpper());
        return p;
    }
    // 仅货币符号/代码 + 数字（顺序相反：usd100 / 美元100）
    QRegularExpression curRe2(R"((usd|eur|cny|gbp|jpy|hkd|krw|美元|美金|欧元|人民币|元|英镑|日元|港币|韩元|\$|€|¥|￥|£)\s*([0-9]+(?:\.[0-9]+)?))");
    QRegularExpressionMatch curM2 = curRe2.match(s);
    if (curM2.hasMatch()) {
        p.type = Type::Currency;
        p.value = curM2.captured(2).toDouble();
        p.from = kCurrencyAliases.value(curM2.captured(1), curM2.captured(1).toUpper());
        return p;
    }

    // 2) 温度：数字 + C/F/K（k 需排除 kg/km/kb 等单位词，避免误判）
    QRegularExpression tempRe(R"(([0-9]+(?:\.[0-9]+)?)\s*(c|f|k|摄氏度|华氏度))");
    QRegularExpressionMatch tempM = tempRe.match(s);
    if (tempM.hasMatch() && !containsUnitWord(s)) {
        p.type = Type::Unit;
        p.value = tempM.captured(1).toDouble();
        p.from = tempM.captured(2) == "f" ? "f" : (tempM.captured(2) == "k" ? "k" : "c");
        // 目标温度标度：to / =? 之后
        QRegularExpression tToRe(R"((?:to|=|\?)\s*(c|f|k))");
        QRegularExpressionMatch tToM = tToRe.match(s);
        if (tToM.hasMatch())
            p.to = tToM.captured(1);
        return p;
    }

    // 3) 单位换算：数字 + 单位词
    QRegularExpression numUnitRe(R"(([0-9]+(?:\.[0-9]+)?)\s*([a-z\x{4e00}-\x{9fff}]+))");
    QRegularExpressionMatch nuM = numUnitRe.match(s);
    if (nuM.hasMatch() && containsUnitWord(nuM.captured(2))) {
        p.type = Type::Unit;
        p.value = nuM.captured(1).toDouble();
        p.from = nuM.captured(2);
        // 目标单位：to / =? / ? 之后
        QRegularExpression uToRe(R"((?:to|=|\?)\s*([a-z\x{4e00}-\x{9fff}]+))");
        QRegularExpressionMatch uToM = uToRe.match(s);
        if (uToM.hasMatch())
            p.to = uToM.captured(1);
        return p;
    }

    // 4) 时间：含城市/时区关键词
    if (containsTimeKeyword(s)) {
        p.type = Type::Time;
        p.city = s;
        return p;
    }

    // 5) 颜色转换：#rgb / #rrggbb / rgb() / hsl()
    {
        QString color;
        if (matchColor(s, color)) {
            p.type = Type::Color;
            p.text = color;
            return p;
        }
    }

    // 6) 程序员工具：含 base64/ascii/url/hash/时间戳 等关键词
    if (containsProgramKeyword(s) && !containsTimeKeyword(s)) {
        p.type = Type::Program;
        p.text = s;
        return p;
    }

    // 7) 日期/倒数日：提取到日期即命中（无需关键词，避免与计算器/单位冲突）
    {
        // 复用 DateTimeProvider 的日期抽取逻辑过于重，这里做轻量判定：
        // 含 YYYY-MM-DD 等日期形态，或含倒数日/距/date 等关键词
        QRegularExpression dateRe(R"(\d{4}[-./]\d{1,2}[-./]\d{1,2})");
        if (dateRe.match(s).hasMatch() || containsDateKeyword(s)) {
            p.type = Type::DateTime;
            p.text = s;
            return p;
        }
    }

    // 8) 科学/程序员计算器：算术表达式或进制转换
    {
        QString expr;
        if (matchCalc(s, expr)) {
            p.type = Type::Calc;
            p.text = expr;
            return p;
        }
    }

    p.type = Type::None;
    return p;
}

} // namespace QueryParser
