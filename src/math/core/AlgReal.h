#pragma once

/*
* @file AlgReal.h
* @brief 代数实数: 最小多项式 + 隔离区间
*
* 表示任意满足整系数多项式方程的实数, 支持 Fraction, 嵌套 k 次根, 四则运算,
* 比较, 与 double / Fraction 的双向转换. 与 std::sqrt 等浮点函数的关系:
*   精确分支: AlgReal::sqrt(Fraction) 返回精确值, 可无损参与后续运算
*   近似分支: AlgReal::fromDouble(double) 先用连分数把 double 逼近成 Fraction,
*             再进入精确体系; 若只需浮点结果, 外层直接用 std::sqrt 即可
*
* 内部表示: (p, [a, b])
*   p 为 squarefree 多项式, p(α) = 0
*   [a, b] 为隔离区间, p(a) * p(b) <= 0, p 在 [a, b] 恰一实根 α
*   若 p 为一次式 x - q, 则 α = q 为有理数 (快速路径)
*
* @example
* AlgReal s2 = AlgReal::sqrt(Fraction(2));       // √2
* AlgReal s3 = AlgReal::sqrt(Fraction(3));       // √3
* AlgReal x  = s2 + s3;                          // √2 + √3
* AlgReal y  = x * (s2 - s3);                    // = -1
* bool ok    = y == AlgReal(Fraction(-1));       // true
*/

#include "Fraction.h"
#include "Polynomial.h"

#include <iosfwd>
#include <string>

namespace algemate::math {

class AlgReal {
public:
    using Poly = Polynomial<Fraction>;

    AlgReal();                                   // 0
    AlgReal(const Fraction& q);                  // 有理数
    AlgReal(long long q);                        // 整数

    static AlgReal fromRational(const Fraction& q);
    static AlgReal fromDouble  (double x, long maxDenom = 1000000);

    static AlgReal sqrt   (const Fraction& q);   // q >= 0
    static AlgReal sqrt   (const AlgReal& a);    // a >= 0
    static AlgReal cbrt   (const Fraction& q);   // cube root
    static AlgReal cbrt   (const AlgReal& a);
    static AlgReal nthRoot(const Fraction& q, int n); // n 偶数时 q >= 0
    static AlgReal nthRoot(const AlgReal& a, int n);

    // 求多项式 p (任意, 系数 ∈ Q) 的全部互异实根, 按数值升序
    // p 自动 squarefree化. 返回的 AlgReal 已隔离到 tol
    static std::vector<AlgReal> realRootsOf(const Poly& p,
        const Fraction& tol = Fraction(1, 1000));

    // 精确求值 β = f(α), 度数不膨胀 (最终 minPoly 度数 <= deg(α.minPoly))
    // 内部通过 Res_x(α.minPoly, y - f) 获得 β 的零化多项式, 然后数值定位
    static AlgReal evaluatePoly(const Poly& f, const AlgReal& alpha);

    bool     isZero()     const;
    bool     isRational() const;
    Fraction asRational() const;                 // 仅 isRational() 时合法
    int      sign()       const;                 // -1 / 0 / +1

    double      toDouble(double eps = 1e-15) const;
    std::string toString() const;                // CAS 风格: sqrt(2), root(5, 4) (root(x, n)), alg(...)
    std::string toLatex () const;                // \sqrt{2}, \sqrt[4]{5}, ...

    AlgReal operator-() const;
    AlgReal operator+(const AlgReal& r) const;
    AlgReal operator-(const AlgReal& r) const;
    AlgReal operator*(const AlgReal& r) const;
    AlgReal operator/(const AlgReal& r) const;   // r != 0, 否则 std::domain_error

    AlgReal& operator+=(const AlgReal& r) { *this = *this + r; return *this; }
    AlgReal& operator-=(const AlgReal& r) { *this = *this - r; return *this; }
    AlgReal& operator*=(const AlgReal& r) { *this = *this * r; return *this; }
    AlgReal& operator/=(const AlgReal& r) { *this = *this / r; return *this; }

    bool operator==(const AlgReal& r) const;
    bool operator!=(const AlgReal& r) const { return !(*this == r); }
    bool operator< (const AlgReal& r) const;
    bool operator<=(const AlgReal& r) const { return !(r < *this); }
    bool operator> (const AlgReal& r) const { return r < *this; }
    bool operator>=(const AlgReal& r) const { return !(*this < r); }

    const Poly&     minPoly()         const { return p_; }
    const Fraction& intervalLower()   const { return a_; }
    const Fraction& intervalUpper()   const { return b_; }

private:
    Poly             p_;   // squarefree, p(α) = 0
    mutable Fraction a_;   // 隔离区间下界
    mutable Fraction b_;   // 隔离区间上界

    AlgReal(Poly p, Fraction a, Fraction b);     // 原料构造, 假定不变量已满足
    void normalize_();                           // squarefree + 收缩到唯一根
    int  signAt_(const Fraction& x) const;       // sign(p_(x)) p_为多项式
    void refineTo_(const Fraction& tol) const;   // 精化区间宽度 < tol
};

std::ostream& operator<<(std::ostream& os, const AlgReal& a);

}
