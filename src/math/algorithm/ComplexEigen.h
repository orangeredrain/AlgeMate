#pragma once

/*
* @file ComplexEigen.h
* @brief 复特征值/特征向量与 Matrix<Complex> 精确线性代数
*
* 对 Matrix<Fraction> A 返回全部复特征值 (按重数), 二次不可约因子用求根公式精确分裂,
* 三次及以上不可约因子保留在 unsolvedFactors 中不进行分裂.
* 复特征向量通过 Matrix<Complex> 的零空间计算, 元素精确 (AlgReal 实部虚部).
*
* @example
* Matrix<Fraction> A = { {0, -1}, {1, 0} };   // 旋转矩阵
* auto r = complexEigenvalues(A);
* // r.eigenvalues = { (i, 1), (-i, 1) }, r.unsolvedFactors = {}
* auto pairs = complexEigenPairs(A);
* // pairs[0].value = i, pairs[0].eigenspaceBasis = 1 列
*/

#include "core/Complex.h"
#include "core/Fraction.h"
#include "core/Matrix.h"
#include "core/Polynomial.h"

#include <cstddef>
#include <utility>
#include <vector>

namespace algemate::math {

// 把 Fraction 矩阵提升到 Complex 矩阵
Matrix<Complex> toComplex(const Matrix<Fraction>& A);

// Matrix<Complex> 的精确 RREF (就地), 返回秩
std::size_t     rrefComplex     (Matrix<Complex>& M);
Matrix<Complex> rrefComplexOf   (const Matrix<Complex>& M);

// Matrix<Complex> 的零空间基 (列向量), 返回 cols x k 矩阵
Matrix<Complex> nullspaceComplex(const Matrix<Complex>& M);

struct ComplexEigenvalue {
    Complex value;
    int     multiplicity;
};

struct ComplexEigenPair {
    Complex         value;
    int             multiplicity;
    Matrix<Complex> eigenspaceBasis;
};

struct ComplexEigenResult {
    std::vector<ComplexEigenvalue>                        eigenvalues;      // 按"剥离顺序", 已求出
    std::vector<std::pair<Polynomial<Fraction>, int>>     unsolvedFactors;  // (不可约因子, 重数)
};

// 仅返回复特征值
ComplexEigenResult            complexEigenvalues(const Matrix<Fraction>& A);

// 返回已求出特征值的复特征对
std::vector<ComplexEigenPair> complexEigenPairs (const Matrix<Fraction>& A);

}
