#pragma once

/*
* @file NumberFormatter.h
* @brief 分数与小数的双向转换工具
*
* 支持 Fraction 到定长小数字符串, 可选四舍五入 / 截断 / 向上 / 向下取整
* 支持 Fraction 到循环小数字符串, 自动检测循环节, 如 0.(3)、0.1(6)
* 支持十进制 / 循环小数 / 科学计数字符串到 Fraction 精确解析
* 支持 double 到 Fraction 的最佳有理数逼近 (利用连分数算法)
*
* @example
* Fraction f(1, 8);
* std::string s = NumberFormatter::toDecimal(f, 6);                     // "0.125000"
* std::string r = NumberFormatter::toRepeatingDecimal(Fraction(1, 3));  // "0.(3)"
* Fraction a = NumberFormatter::fromDecimal("-3.14");                   // -157/50
* Fraction b = NumberFormatter::fromDecimal("0.1(6)");                  // 1/6
* Fraction c = NumberFormatter::fromDouble(3.14159265358979);
*/

#include "Fraction.h"

#include <string>

namespace algemate::math {

enum class RoundMode {
    Truncate, // 直接截断, 向零取整
    HalfUp,   // 四舍五入
    Floor,    // 向下取整
    Ceil      // 向上取整
};

class NumberFormatter {
public:
    // 将分数转换为定长小数字符串
    // f: 分数
    // precision: 小数位数, 默认 6
    // mode: 取整模式, 默认四舍五入
    static std::string toDecimal(const Fraction& f,
                                 int precision = 6,
                                 RoundMode mode = RoundMode::HalfUp);

    // 将分数转换为循环小数字符串
    // f: 分数
    // maxPeriod: 循环节最大长度, 默认 64
    static std::string toRepeatingDecimal(const Fraction& f,
                                          int maxPeriod = 64);

    // 将十进制 / 循环小数 / 科学计数法的字符串转换为分数
    // s: 字符串
    static Fraction fromDecimal(const std::string& s);

    // 将 double 转换为分数
    // v: double 值
    // maxDenominator: 分母最大值, 默认 1000000
    static Fraction fromDouble(double v,
                               long long maxDenominator = 1000000);
};

}
