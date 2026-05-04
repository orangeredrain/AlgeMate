#pragma once

/*
* @file Complex.h
* @brief 代数复数: 实部/虚部均为 AlgReal
*
* 基于 AlgReal 以保证精确性, 支持整数/分数/嵌套根式作为实部或虚部.
* 提供小型表达式解析器 fromString, 接受如下写法:
* "3+4*i"  "3-4*i"  "i"  "-i"  "sqrt(-1)"  "sqrt(-4)"
* "sqrt(2)+sqrt(3)*i"  "1/2-2*sqrt(-1)"  "(1+i)*(1-i)"
* "cbrt(-8)"  "root(2, 4)"
* 不支持隐式乘法, "3i" 需写成 "3*i".
*
* @example
*   Complex z1 = Complex::fromString("3+4*i");
*   Complex z2 = Complex::i();
*   Complex z3 = z1 * z2.conjugate();
*   AlgReal m  = z1.modulus();           // = 5 (精确有理数)
*/

#include "AlgReal.h"
#include "Fraction.h"

#include <iosfwd>
#include <string>
#include <utility>

namespace algemate::math {

class Complex {
public:
    Complex();
    Complex(const AlgReal& re);
    Complex(const AlgReal& re, const AlgReal& im);
    Complex(const Fraction& re);
    Complex(long long re);

    static Complex i();                                        // 虚数单位
    static Complex fromString(const std::string& s);
    static Complex fromDouble(double re, double im = 0.0);

    const AlgReal& real() const { return re_; }
    const AlgReal& imag() const { return im_; }
    bool           isReal() const { return im_.isZero(); }
    bool           isZero() const { return re_.isZero() && im_.isZero(); }

    Complex operator-() const;
    Complex operator+(const Complex& r) const;
    Complex operator-(const Complex& r) const;
    Complex operator*(const Complex& r) const;
    Complex operator/(const Complex& r) const;                 // r != 0

    Complex& operator+=(const Complex& r) { *this = *this + r; return *this; }
    Complex& operator-=(const Complex& r) { *this = *this - r; return *this; }
    Complex& operator*=(const Complex& r) { *this = *this * r; return *this; }
    Complex& operator/=(const Complex& r) { *this = *this / r; return *this; }

    bool operator==(const Complex& r) const;
    bool operator!=(const Complex& r) const { return !(*this == r); }

    Complex conjugate()      const;
    AlgReal modulusSquared() const;
    AlgReal modulus()        const;

    static Complex sqrt   (const Complex& z);
    static Complex cbrt   (const Complex& z);
    static Complex nthRoot(const Complex& z, int n);

    std::string toString() const;
    std::string toLatex () const;
    std::pair<double, double> toDouble() const;

private:
    AlgReal re_, im_;
};

std::ostream& operator<<(std::ostream& os, const Complex& z);

}
