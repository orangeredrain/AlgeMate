#pragma once

#include <cstdint>
#include <iosfwd>
#include <string>
#include <vector>

/*
* Doxygen 规范如下
* @file: 说明当前文件的名称
* @brief: 简短描述
* @example: 使用实例
* @param: 描述函数的参数
* @throws: 描述可能抛出的异常
* @tparam: 描述模板参数
*/


/*
* @file BigInt.h
* @brief 任意精度的有符号大整数类
*
* 支持从 long long 或者十进制字符串构造
* 支持四则运算、比较、取模、幂、最大公因数、类型转换
* 内部使用 uint32_t 存储
*
* @example
* BigInt a("123");
* BigInt b = 42;
* BitInt c = a * b;
* std::cout << c << std::endl;
* BigInt g = BigInt::gcd(48, 18);
* BigInt p = BigInt::pow(3, 10);
*/

namespace algemate::math {

// BigInt 类
class BigInt {
public:
    BigInt();
    BigInt(long long v);                   // 从 long long 构造
    explicit BigInt(const std::string& s); // explicit 防止隐式转换

    static BigInt fromString(const std::string& s); // 从字符串构造

    // 四则运算和取模
    BigInt  operator-() const;
    BigInt  operator+(const BigInt& r) const;
    BigInt  operator-(const BigInt& r) const;
    BigInt  operator*(const BigInt& r) const;
    BigInt  operator/(const BigInt& r) const;
    BigInt  operator%(const BigInt& r) const;

    // 赋值
    BigInt& operator+=(const BigInt& r);
    BigInt& operator-=(const BigInt& r);
    BigInt& operator*=(const BigInt& r);
    BigInt& operator/=(const BigInt& r);
    BigInt& operator%=(const BigInt& r);

    // 前置/后置 自增/自减
    BigInt& operator++();
    BigInt  operator++(int);
    BigInt& operator--();
    BigInt  operator--(int);

    // 比较
    bool operator==(const BigInt& r) const;
    bool operator!=(const BigInt& r) const;
    bool operator<(const BigInt& r) const;
    bool operator<=(const BigInt& r) const;
    bool operator>(const BigInt& r) const;
    bool operator>=(const BigInt& r) const;

    bool isZero() const;     // 是否为零
    bool isOne() const;      // 是否为 1
    bool isNegative() const; // 是否为负数
    bool isPositive() const; // 是否为正数
    int  sign() const;       // 返回符号 -1, 0, 1

    BigInt abs() const;      // 返回绝对值
    BigInt negate() const;   // 返回相反数

    std::string toString() const;   // 转为十进制字符串
    long long   toLongLong() const; // 转为 long long (注意可能会截断)
    double      toDouble() const;   // 转换为 double (注意可能丢失精度)

    static BigInt gcd(BigInt a, BigInt b);                  // 最大公因数
    static BigInt pow(BigInt base, unsigned int exponent);  // 快速幂

private:
    std::vector<uint32_t> limbs_; // 每个 limb 是 32 位无符号整数
    bool negative_ = false;       // true表示负数, false表示正数

    void normalize_(); // 移除高位的零
    void setFromUnsigned_(unsigned long long v); // 从 unsigned long long 设置 limbs_

    static int    absCompare_(const BigInt& a, const BigInt& b); // -1表示|a|<|b|, 1表示|a|>|b|, 0相等
    static BigInt absAdd_(const BigInt& a, const BigInt& b); // 绝对值相加
    static BigInt absSub_(const BigInt& a, const BigInt& b); // 绝对值相减
    static BigInt absMul_(const BigInt& a, const BigInt& b); // 绝对值相乘
    static void   absDivMod_(const BigInt& a, const BigInt& b, BigInt& q, BigInt& r); // 绝对值带余除法

    bool        bitAt_(std::size_t pos) const;  // 获取第 pos 位
    void        setBit_(std::size_t pos);       // 将第 pos 位设置为 1
    std::size_t bitLength_() const;             // 返回大整数的二进制有效位数 即最高位的1的位置 0返回0 1返回1 2返回2
    void        shiftLeftOneBit_();             // 左移一位 <<
};

std::ostream& operator<<(std::ostream& os, const BigInt& v); // 输出流运算符重载


}
