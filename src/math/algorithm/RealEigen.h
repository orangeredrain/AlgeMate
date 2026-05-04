#pragma once

/*
* @file RealEigen.h
* @brief 一般 Matrix<Fraction> 方阵的实特征值与实特征子空间基 (AlgReal 精确表示)
*
* 与 LinearAlgebra 中的 rationalEigenPairs 对比:
*   - rationalEigenPairs: 仅返回 Q 上特征值 (有理根)
*   - realEigenvalues / realEigenPairs: 返回 AlgReal 上所有实特征值 (任意次数代数数)
*
* 核心技巧: 设 λ 的最小多项式 g(x) ∈ Q[x], deg g = d, 则 K = Q[x]/(g) 是 d 维扩域.
* 计算 ker(A ⊗ I_d - I_n ⊗ C_g) ⊂ Q^{nd} (C_g 是 g 的伴随矩阵), 恰好是 A 的 λ 特征子空间
* 在 K 基下的 Q 表示. 通过 K-独立性提取代表向量, 最后 evaluatePoly 到 AlgReal.
*
* @example
* Matrix<Fraction> A = {{0, 1}, {1, 1}};                  // Fibonacci 矩阵
* auto eigs = realEigenvalues(A);                         // [(1-√5)/2, (1+√5)/2]
* auto pairs = realEigenPairs(A);
* // pairs[0].value ≈ -0.618, pairs[0].basis 是 2×1 AlgReal 列向量
*/

#include "core/AlgReal.h"
#include "core/Fraction.h"
#include "core/Matrix.h"
#include "core/Polynomial.h"

#include <cstddef>
#include <vector>

namespace algemate::math {

// λ 的特征子空间在 K = ℚ[x]/(g) 下的 K-表示基
// basisK[c] 是一个长度为 n 的向量, 每分量 ∈ K (PolyF deg < d)
// g 即 lam.minPoly()
struct EigenspaceKBasis {
    std::vector<std::vector<Polynomial<Fraction>>> basisK;
    Polynomial<Fraction>                            g;
};

// 计算 A 在特征值 lam 处的 K-表示基 (K = ℚ[x]/minPoly(lam))
// 用于 Jordan/正交对角化/实特征子空间共享基础
EigenspaceKBasis eigenspaceBasisK(const Matrix<Fraction>& A, const AlgReal& lam);

// A 在 lam 处的实特征子空间基, n × m 矩阵 (m = 几何重数)
// 未正交/未归一, 对应 ker(A - λI) 的一组基
Matrix<AlgReal> realEigenspaceBasis(const Matrix<Fraction>& A, const AlgReal& lam);

// A 的所有实特征值 (AlgReal 表示, 升序去重, 不计重数)
std::vector<AlgReal> realEigenvalues(const Matrix<Fraction>& A);

// 实特征值 + 对应特征子空间基
struct RealEigenPair {
    AlgReal         value;
    Matrix<AlgReal> basis;   // n × m, m = 几何重数
};

std::vector<RealEigenPair> realEigenPairs(const Matrix<Fraction>& A);

}
