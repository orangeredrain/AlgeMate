#pragma once

/*
* @file Orthogonalization.h
* @brief 向量组正交化与线性无关性判定 (元素类型固定为 Fraction)
*
* 列向量统一用 n*k 矩阵表示, 每一列是一个向量
* 内积采用标准欧氏内积 <u, v> = sum u_i * v_i
* Gram-Schmidt 输出的正交向量组未归一 (归一需要 sqrt, 应在 AlgReal 层做)
*
* @example
* Matrix<Fraction> V = {{1, 1, 0},
*                       {1, 0, 1},
*                       {0, 1, 1}};                  // 3 列向量
* auto Q = gramSchmidt(V);                           // 列两两正交, 未归一
* bool ind = areLinearlyIndependent(V);              // true
*/

#include "core/Fraction.h"
#include "core/Matrix.h"

#include <cstddef>

namespace algemate::math {

// 列向量内积 <u, v>, u 与 v 必须是同长度的 n*1 列向量
Fraction dotProduct(const Matrix<Fraction>& u, const Matrix<Fraction>& v);

// 列向量欧氏范数平方 ||v||^2 = <v, v>, 避免 sqrt
Fraction normSquared(const Matrix<Fraction>& v);

// 判断两个列向量是否正交
bool isOrthogonal(const Matrix<Fraction>& u, const Matrix<Fraction>& v);

// 判断矩阵 V 的所有列向量两两正交 (允许含零列)
bool isOrthogonalSet(const Matrix<Fraction>& V);

// 判断矩阵 V 的列向量组是否线性无关
bool areLinearlyIndependent(const Matrix<Fraction>& V);

// Gram-Schmidt 正交化 (不归一)
// 输入: n*k 矩阵 V, 每列一个向量
// 输出: n*k' 矩阵 Q, 列两两正交, span(Q) = span(V), k' = rank(V)
// 线性相关的列被自动丢弃
Matrix<Fraction> gramSchmidt(const Matrix<Fraction>& V);

}
