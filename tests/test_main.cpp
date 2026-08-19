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

    p = QueryParser::parse("12inch");
    CHECK(p.type == QueryParser::Type::Unit && p.to.isEmpty(), "12inch 无 to");

    p = QueryParser::parse("随便写点啥");
    CHECK(p.type == QueryParser::Type::None, "乱写 -> None");

    p = QueryParser::parse("255 to hex");
    CHECK(p.type == QueryParser::Type::Calc, "255 to hex -> Calc");
}

static void test_providers()
{
    // 单位：1kg = 2 斤（近似）
    auto u = UnitProvider::convert(1, "kg", "斤");
    CHECK(!u.isEmpty(), "1kg=?斤 有结果");
    if (!u.isEmpty())
        CHECK(u.first().name.contains("2."), "1kg≈2斤");

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
    CHECK(!e.isEmpty() && e.first().name.contains("418.40"), "100kcal=418.40kj");
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
