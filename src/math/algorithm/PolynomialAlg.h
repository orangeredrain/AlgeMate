#pragma once

/*
* @file PolynomialAlg.h
* @brief 高阶多项式算法: 无平方分解, 结式, 判别式, 有理根, Sturm 链, 代数数运算
*
* 元素类型固定为 Fraction, 保证精确运算
* 对应 Polynomial<Fraction> 的高阶算法层, 类似 LinearAlgebra 之于 Matrix<Fraction>
*
* @example
* Poly f = {Fraction(1), Fraction(-2), Fraction(1)};    // (x-1)^2
* Poly sp = squarefreePart(f);                          // x - 1
* Fraction disc = discriminant(f);                      // 0
* auto rs = rationalRoots(f);                           // {1}
* auto sturm = sturmSequence(squarefreePart(f));
* int cnt = countRealRootsInInterval(f, Fraction(-10), Fraction(10));
*/

#include "core/Fraction.h"
#include "core/Polynomial.h"

#include <cstddef>
#include <vector>

namespace algemate::math {

using Poly = Polynomial<Fraction>;

// 去除 f 的重根, 即每个重根只保留一个, 并且首一化: f / gcd(f, f')
Poly squarefreePart(const Poly& f);

// Yun 无平方分解: f = content * \prod{ factors[k]^(k+1) }, factors[k] 无平方且两两互素
struct SquarefreeFactorization {
    Fraction          content;  // 首项系数
    std::vector<Poly> factors;  // factors[k] 的重数 = k+1, monic
};
SquarefreeFactorization squarefreeFactorization(const Poly& f);

// 结式 Res(f, g): Euclidean PRS 方法
Fraction resultant(const Poly& f, const Poly& g);

// 判别式: Disc(f) = (-1)^(n(n-1)/2) / lc(f) * Res(f, f')
Fraction discriminant(const Poly& f);

// 有理根定理枚举,升序去重
std::vector<Fraction> rationalRoots(const Poly& f);

// Sturm 序列: S0 = f, S1 = f', S_{i+1} = -(S_{i-1} mod S_i)
std::vector<Poly> sturmSequence(const Poly& f);

// 指定点上的符号变化数
int sturmSignChanges(const std::vector<Poly>& seq, const Fraction& x);

// 开区间 (a, b] 内实根个数 (自动对 f 做 squarefree)
int countRealRootsInInterval(const Poly& f, const Fraction& a, const Fraction& b);

// Cauchy 根界: 所有复根 α 满足 |α| < 1 + max_{i < n}(|a_i| / |a_n|)
// 对常数多项式或零多项式返回 Fraction(1)
Fraction cauchyBound(const Poly& f);

// 无平方多项式 f 的全部实根隔离区间 [a_i, b_i] (内部恰一根, b_i - a_i <= tol)
// 若 f 含重根会先做 squarefree. 返回区间按下界升序
// tol 默认 1/1000
std::vector<std::pair<Fraction, Fraction>> isolateRealRoots(
    const Poly& f, const Fraction& tol = Fraction(1, 1000));

// α+β 的零化多项式: Res_y(f(y), g(x-y)), 经拉格朗日插值获得
// 可能不是最小多项式, 若需最小多项式, 需要自行 squarefree + 取含 α+β 的因子
Poly sumPoly(const Poly& fAlpha, const Poly& gBeta);

// α·β 的零化多项式: Res_y(y^n f(y), g(x/y) y^n), 同上可能非最小多项式
Poly productPoly(const Poly& fAlpha, const Poly& gBeta);

// f(α) 的零化多项式: Res_x(g(x), y - f(x))
// 给 α 满足 g(α)=0 (g degree >= 1), 返回 β=f(α) 在 y 中的多项式 (deg_y <= deg g)
// 结果可能非 squarefree / 非最小多项式, 但必有 β 为其根
Poly minPolyOfEval(const Poly& g, const Poly& f);

// Q[x] 不可约因式分解 (Hensel + Zassenhaus)
// 返回 {lc_rational, [(factor_i, e_i)]} 其中 factor_i ∈ Q[x] 首一不可约
// 注: 零多项式或常数多项式返回空 factors 列表
struct RationalFactorization {
    Fraction                                       leadingCoefficient; // f = lc · ∏ f_i^{e_i}
    std::vector<std::pair<Poly, int>>              factors;            // (factor_i, multiplicity)
};
RationalFactorization factorOverQ(const Poly& f);

}
