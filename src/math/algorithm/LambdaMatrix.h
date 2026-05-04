#pragma once

/*
* @file LambdaMatrix.h
* @brief λ矩阵 (系数为 Polynomial<Fraction>) 的 Smith 标准形与因子体系
*
* 提供 Smith 标准形 (U*M*V=S, S 对角且前一整除后一)
* 在 Smith 基础上派生: 行列式因子 D_k(λ), 不变因子 d_i(λ), 初等因子 {(prime, power)}
* ℚ[x] 上的不可约分解受限: 支持 deg<=2 的精确不可约判定, 以及 deg>=3 无有理根时作为整块保留
*
* @example
* Matrix<Fraction> A = {...};
* auto elem = elementaryDivisorsOf(A);
* auto cM = lambdaMinus(A);
* auto smith = smithNormalForm(cM);
* // smith.invariantFactors 即 A 的不变因子 d_1 | d_2 | ...
*/

#include "core/Fraction.h"
#include "core/Matrix.h"
#include "core/Polynomial.h"

#include <cstddef>
#include <vector>

namespace algemate::math {

using LambdaMatrix = Matrix<Polynomial<Fraction>>;

// 由 Matrix<Fraction> A 构造特征 λ 矩阵 λI - A
LambdaMatrix lambdaMinus(const Matrix<Fraction>& A);

// Smith 标准形: S = U * M * V, S = diag(d1, d2, ..., dr, 0, ..., 0), d_i | d_{i+1}
struct SmithResult {
    LambdaMatrix                       S;                   // 对角 λ 矩阵
    LambdaMatrix                       U;                   // 左变换, 方阵 rows x rows
    LambdaMatrix                       V;                   // 右变换, 方阵 cols x cols
    std::vector<Polynomial<Fraction>>  invariantFactors;    // 过滤 1 后的 (首一) 不变因子列表
};
SmithResult smithNormalForm(const LambdaMatrix& M);

// 行列式因子 D_k(λ) = gcd(所有 k 阶子式), 首一, k = 1..min(rows,cols)
std::vector<Polynomial<Fraction>> determinantalDivisors(const LambdaMatrix& M);

// 不变因子 d_i(λ) = D_i / D_{i-1}, 过滤常数 1
std::vector<Polynomial<Fraction>> invariantFactors(const LambdaMatrix& M);

// 初等因子: 对每个不变因子做 Q[x] 不可约分解后的 (prime, power) 列表
struct ElementaryDivisor {
    Polynomial<Fraction> prime;   // 首一不可约
    int                  power;   // 幂次
};
std::vector<ElementaryDivisor> elementaryDivisors  (const LambdaMatrix& M);
std::vector<ElementaryDivisor> elementaryDivisorsOf(const Matrix<Fraction>& A);

}
