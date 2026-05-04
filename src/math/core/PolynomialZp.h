#pragma once

/*
* @file PolynomialZp.h
* @brief 有限域 F_p = Z/pZ 上的稠密多项式 (p 为奇素数或 2)
*
* 系数为 int64_t 并始终归约到 [0, p). p 应满足 2 <= p < 2^31, 内部乘法用 __int128 避免溢出.
* 提供基本运算、带余除法、欧几里得 gcd、模多项式快速幂、导数、平方自由分解、DDF、EDF (Cantor-Zassenhaus)、
* 完整不可约因式分解、不可约判定.
*
* @example
* PolynomialZp f({1, 0, 1}, 7);        // 1 + x^2 over F_7
* auto [q, r] = PolynomialZp::divmod(f, g);
* auto factors = PolynomialZp::factor(f);
*/

#include "core/Fraction.h"
#include "core/Polynomial.h"

#include <cstdint>
#include <iosfwd>
#include <vector>

namespace algemate::math {

class PolynomialZp;

class PolynomialZp {
public:
    using Z = int64_t;

    PolynomialZp();                                  // 零多项式, p 未设 (后续操作需显式设置)
    PolynomialZp(Z prime);                           // 零多项式 over F_p
    PolynomialZp(std::vector<Z> coeffsLowFirst, Z prime); // 系数低位优先, 自动归约

    static PolynomialZp fromPoly(const Polynomial<Fraction>& f, Z prime); // Q[x] → F_p[x] (分母须与 p 互素)
    Polynomial<Fraction> toPoly() const;                                  // 取代表元 ∈ [0, p) 转 Q[x]

    Z         prime()  const { return p_; }
    int       degree() const;                 // 零多项式返回 -1
    bool      isZero() const { return coeffs_.empty(); }
    bool      isOne()  const;
    Z         leading() const;                // 首项系数; 零多项式抛
    Z         at(std::size_t k) const;        // x^k 的系数; 越界返回 0
    const std::vector<Z>& coeffs() const { return coeffs_; }

    PolynomialZp makeMonic() const;           // 除以首项系数
    PolynomialZp derivative() const;          // 形式导数

    // 四则 (同 prime 前提)
    static PolynomialZp add(const PolynomialZp& a, const PolynomialZp& b);
    static PolynomialZp sub(const PolynomialZp& a, const PolynomialZp& b);
    static PolynomialZp mul(const PolynomialZp& a, const PolynomialZp& b);

    // 带余除法 a = q*b + r, deg r < deg b. b 不能为零.
    struct DivMod;
    static DivMod divmod(const PolynomialZp& a, const PolynomialZp& b);

    // gcd 返回 monic 结果
    static PolynomialZp gcd(PolynomialZp a, PolynomialZp b);

    // a^e mod m
    static PolynomialZp powMod(const PolynomialZp& a, Z e, const PolynomialZp& m);

    // 平方自由分解 (Yun + p-power 路径): 返回 {factor, mult}
    struct SqfFactor;
    static std::vector<SqfFactor> squarefreeFactorization(const PolynomialZp& f);

    // DDF: 输入 squarefree monic f, 返回 {g_d, d} 其中 g_d 是 f 中所有 deg-d 不可约因子的积
    struct DDFPart;
    static std::vector<DDFPart> distinctDegreeFactorization(const PolynomialZp& f);

    // EDF (Cantor-Zassenhaus): 输入 monic squarefree g, 它的所有不可约因子都同为 deg d, 返回 g 的所有 deg-d 不可约因子
    static std::vector<PolynomialZp> equalDegreeFactorization(const PolynomialZp& g, int d);

    // 完整因式分解: f = lc · ∏ f_i^{e_i}, 其中 f_i monic 不可约. 返回 {lc, [(f_i, e_i)]}
    struct Factorization;
    static Factorization factor(const PolynomialZp& f);

    // 不可约判定: deg f >= 1 时, f 不可约 ⇔ gcd(f, x^{p^i}-x) = 1 for 1 <= i <= deg f / 2 且 x^{p^deg} = x mod f
    static bool isIrreducible(const PolynomialZp& f);

    // 算数工具
    static Z   modp(Z a, Z p);                 // 归约到 [0, p)
    static Z   mulmod(Z a, Z b, Z p);          // (a*b) % p (使用 __int128)
    static Z   powmod(Z a, Z e, Z p);          // a^e mod p
    static Z   invmod(Z a, Z p);               // 费马小定理 (p 素数)

    // 比较
    bool operator==(const PolynomialZp& o) const;
    bool operator!=(const PolynomialZp& o) const { return !(*this == o); }

private:
    std::vector<Z> coeffs_;   // 低位优先, 末尾非零
    Z              p_ = 0;    // 素数; 零多项式且未设时为 0

    void normalize_();        // 去末尾零
    static void requireSamePrime_(const PolynomialZp& a, const PolynomialZp& b);
};

struct PolynomialZp::DivMod    { PolynomialZp q; PolynomialZp r; };
struct PolynomialZp::SqfFactor { PolynomialZp f; int e; };
struct PolynomialZp::DDFPart   { PolynomialZp g; int d; };
struct PolynomialZp::Factorization {
    PolynomialZp::Z                      lc;       // 首项系数 ∈ F_p
    std::vector<PolynomialZp::SqfFactor> factors;  // f_i monic 不可约
};

std::ostream& operator<<(std::ostream& os, const PolynomialZp& f);

} // namespace algemate::math
