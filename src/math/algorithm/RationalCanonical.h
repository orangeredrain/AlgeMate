#pragma once

/*
* @file RationalCanonical.h
* @brief 有理标准形 (Frobenius 标准形) 与最小多项式
*
* 给定 A \in Matrix<Fraction>^{n x n}:
*   1. 计算不变因子 d_1 | d_2 | ... | d_r (过滤常数 1 后)
*   2. 有理标准形 F = blockDiag(companion(d_1), ..., companion(d_r))
*   3. A 与 F 相似 (P^{-1} A P = F), F 是相似类唯一代表元 (在 Q 上)
*   4. 最小多项式 = d_r (最后一个不变因子)
*
* 本模块仅返回 F 与因子列表, 变换矩阵 P 的构造留作后续扩展
*
* @example
* Matrix<Fraction> A = {...};
* auto F = frobeniusForm(A);
* auto m = minimalPolynomial(A);
* // m(A) == 0 (Cayley-Hamilton 的精炼版)
*/

#include "core/Fraction.h"
#include "core/Matrix.h"
#include "core/Polynomial.h"

#include <cstddef>
#include <vector>

namespace algemate::math {

struct RationalFormResult {
    Matrix<Fraction>                   F;                  // 有理标准形矩阵
    std::vector<Polynomial<Fraction>>  invariantFactors;   // d_1 | d_2 | ... | d_r (首一, 过滤 1)
    Polynomial<Fraction>               minimalPolynomial;  // = d_r, A 为 0 时返回 1
};

// 完整有理标准形计算
RationalFormResult rationalCanonicalForm(const Matrix<Fraction>& A);

// 仅返回有理标准形 F
Matrix<Fraction>     frobeniusForm     (const Matrix<Fraction>& A);

// 最小多项式 (= 最大不变因子)
Polynomial<Fraction> minimalPolynomial (const Matrix<Fraction>& A);

}
