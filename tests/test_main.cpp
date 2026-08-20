// SPDX-FileCopyrightText: 2026 testerxydw
// SPDX-License-Identifier: GPL-3.0-or-later
//
// 转换器插件单元测试（轻量，链接编译产物目标文件运行，无需 DBus/显示）。

#include "queryparser.h"
#include "resultbuilder.h"
#include "providers/unitprovider.h"
#include "providers/currencyprovider.h"
#include "providers/calcprovider.h"
#include "providers/colorprovider.h"
#include "providers/datetimeprovider.h"
#include "providers/energyprovider.h"
#include "providers/programprovider.h"
#include "i18n.h"

#include <QCoreApplication>
#include <QList>
#include <QRegularExpression>
#include <cstdio>
#include <cstdlib>

static int g_fail = 0;
static int g_pass = 0;

#define CHECK(cond, msg) do { \
    if (cond) { ++g_pass; } \
    else { ++g_fail; printf("  [FAIL] %s (%s:%d)\n", msg, __FILE__, __LINE__); } \
} while (0)

static void test_parser()
{
    auto p = QueryParser::parse("help");
    CHECK(p.type == QueryParser::Type::Help, "help -> Help");

    p = QueryParser::parse("?");
    CHECK(p.type == QueryParser::Type::Help, "? -> Help");

    p = QueryParser::parse("1kg 斤");
    CHECK(p.type == QueryParser::Type::Unit, "1kg 斤 -> Unit");
    CHECK(p.from == "kg", "1kg 斤 from=kg");
    CHECK(p.to == "斤", "1kg 斤 to=斤");

    p = QueryParser::parse("1kg=?斤");
    CHECK(p.type == QueryParser::Type::Unit && p.to == "斤", "1kg=?斤 to=斤");

    p = QueryParser::parse("100 kcal");
    CHECK(p.type == QueryParser::Type::Energy, "100 kcal -> Energy");
    CHECK(p.from == "kcal", "100 kcal from=kcal");

    // 温度：100F -> Unit(temp)
    p = QueryParser::parse("100F");
    CHECK(p.type == QueryParser::Type::Unit, "100F -> Unit");
    CHECK(p.from == "f", "100F from=f");

    // 重量：100kg=?斤 -> Unit, to=斤
    p = QueryParser::parse("100kg=?斤");
    CHECK(p.type == QueryParser::Type::Unit, "100kg=?斤 -> Unit");
    CHECK(p.from == "kg", "100kg=?斤 from=kg");
    CHECK(p.to == "斤", "100kg=?斤 to=斤");

    // 数字时间戳：1690000000 -> Program(time)
    p = QueryParser::parse("1690000000");
    CHECK(p.type == QueryParser::Type::Program, "1690000000 -> Program");


    p = QueryParser::parse("12inch");
    CHECK(p.type == QueryParser::Type::Unit && p.to.isEmpty(), "12inch 无 to");

    p = QueryParser::parse("随便写点啥");
    CHECK(p.type == QueryParser::Type::None, "乱写 -> None");

    p = QueryParser::parse("255 to hex");
    CHECK(p.type == QueryParser::Type::Calc, "255 to hex -> Calc");

    // 触发词前缀（多语言）短路，剩余内容沿用特征匹配
    p = QueryParser::parse("汇率 100usd");
    CHECK(p.type == QueryParser::Type::Currency, "汇率 100usd -> Currency");

    p = QueryParser::parse("convert 1kg 斤");
    CHECK(p.type == QueryParser::Type::Unit && p.to == "斤", "convert 1kg 斤 -> Unit");

    p = QueryParser::parse("转时间戳 2026-08-20 15:03:28");
    CHECK(p.type == QueryParser::Type::DateTime, "转时间戳 ... -> DateTime");

    p = QueryParser::parse("时间戳 1690000000");
    CHECK(p.type == QueryParser::Type::Program, "时间戳 1690000000 -> Program");

    p = QueryParser::parse("date 2025-01-01 到 2025-12-31");
    CHECK(p.type == QueryParser::Type::DateTime, "date ... -> DateTime");

    // 触发词 + 冒号分隔
    p = QueryParser::parse("颜色:#ff8800");
    CHECK(p.type == QueryParser::Type::Color, "颜色:#ff8800 -> Color");
}

static void test_providers()
{
    // 货币：省略目标 → 全币种列（>=10 条，含 USD/JPY，自身 CNY 剔除）
    CurrencyProvider curProv;
    auto cur = curProv.convert(100, "CNY", "");
    CHECK(cur.size() >= 10, "100cny 省略目标 → 全币种列(>=10条)");
    bool hasUsd = false, hasJpy = false, hasSelfTarget = false;
    for (const auto &it : cur) {
        if (it.name.contains("USD") || it.name.contains("美元")) hasUsd = true;
        if (it.name.contains("JPY") || it.name.contains("日元")) hasJpy = true;
        // 来源已是 CNY；若某条"目标"也是 CNY（即 = X CNY(...) 形态）说明未剔除自身
        if (QRegularExpression("= [\\d,.]+ CNY").match(it.name).hasMatch()) hasSelfTarget = true;
    }
    CHECK(hasUsd && hasJpy, "100cny 全列含 USD/JPY");
    CHECK(!hasSelfTarget, "100cny 全列剔除自身 CNY(无 = X CNY 目标)");
    // 指定目标仍精确单条
    CHECK(curProv.convert(100, "USD", "CNY").size() == 1, "100usd=cny 精确单条");

    // 单位：1kg = 2 斤（近似）
    auto u = UnitProvider::convert(1, "kg", "斤");
    CHECK(!u.isEmpty(), "1kg=?斤 有结果");
    if (!u.isEmpty())
        CHECK(u.first().name.contains("2"), "1kg≈2斤");

    // 温度：100F -> 37.8°C（100°F = 37.78°C）
    auto tf = UnitProvider::convert(100, "f", "c");
    CHECK(!tf.isEmpty() && tf.first().name.contains("37.8"), "100F≈37.8°C");

    // 重量：100kg = 200 斤
    auto kg = UnitProvider::convert(100, "kg", "斤");
    CHECK(!kg.isEmpty() && kg.first().name.contains("200"), "100kg=200斤");

    // 省略目标单位 → 同类全列（重量类含 斤/两/g/lb 等，条数应 >= 5）
    auto kgAll = UnitProvider::convert(100, "kg", "");
    CHECK(kgAll.size() >= 5, "100kg 省略目标 → 同类全列(>=5条)");
    bool hasJin = false, hasLb = false;
    for (const auto &it : kgAll) {
        if (it.name.contains("斤")) hasJin = true;
        if (it.name.contains("lb") || it.name.contains("磅")) hasLb = true;
    }
    CHECK(hasJin && hasLb, "100kg 全列含 斤 与 磅");
    // 指定目标仍精确单条
    CHECK(UnitProvider::convert(100, "kg", "斤").size() == 1, "100kg=斤 精确单条");

    // 数字时间戳：1690000000 -> 2023-07-22
    auto ts = ProgramProvider::run("1690000000");
    CHECK(!ts.isEmpty() && ts.first().name.contains("2023-07-22"),
          "1690000000 -> 2023-07-22");


    // 计算器：100 整数应含 HEX
    auto c = CalcProvider::eval("100");
    CHECK(!c.isEmpty(), "100 有计算结果");

    // 颜色：#ff8800 -> rgb(255, 136, 0)
    auto col = ColorProvider::convert("#ff8800");
    bool has255 = false;
    for (const auto &it : col)
        if (it.name.contains("255")) { has255 = true; break; }
    CHECK(!col.isEmpty() && has255, "#ff8800 -> 255");

    // 程序：md5 hello
    auto pr = ProgramProvider::run("md5 hello");
    CHECK(!pr.isEmpty() && pr.first().name.contains("5d41402abc4b2a76b9719d911017c592"),
          "md5 hello 正确");

    // 日期：2025-01-01 到 2025-12-31
    auto d = DateTimeProvider::run("2025-01-01 到 2025-12-31");
    CHECK(!d.isEmpty() && d.first().name.contains("364"), "日期差 364 天");

    // 热量：100 kcal -> 418.40 kj
    auto e = EnergyProvider::convert(100, "kcal", "kj");
    CHECK(!e.isEmpty() && e.first().name.contains("418.4"), "100kcal=418.4kj");
}

static void test_i18n()
{
    // 单位名至少含原单位
    CHECK(I18n::unitName("km").contains("km"), "unitName 含 km");
    CHECK(I18n::currencyName("USD").contains("USD"), "currencyName 含 USD");
    CHECK(!I18n::groupName("currency").isEmpty(), "groupName 非空");
}

int main(int argc, char **argv)
{
    QCoreApplication a(argc, argv);
    printf("=== 转换器插件单元测试 ===\n");
    test_parser();
    test_providers();
    test_i18n();
    printf("\n通过 %d / 失败 %d\n", g_pass, g_fail);
    return g_fail == 0 ? 0 : 1;
}
