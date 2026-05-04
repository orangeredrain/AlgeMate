#pragma once

/*
* @file SVD.h
* @brief 实矩阵的奇异值分解 A = U Σ V^T (精确代数数 AlgReal)
*
* 对任意实矩阵 A (m×n):
*   A = U Σ V^T
*   U 为 m×m 正交矩阵 (U^T U = I_m)
*   V 为 n×n 正交矩阵 (V^T V = I_n)
*   Σ 为 m×n 对角阵, 对角元为奇异值 σ_1 ≥ σ_2 ≥ ... ≥ 0
*
* 算法 (精确代数数路径):
*   1. B = A^T A (n×n 对称半正定)
*   2. orthogonalDiagonalize(B) 得到正交基 + 非负特征值 (升序)
*   3. 按特征值降序重排 → V, σ_i = sqrt(λ_i)
*   4. 对 σ_i > 0 的列: u_i = A v_i / σ_i
*   5. Gram-Schmidt 把 {u_1, ..., u_r, e_1, ..., e_m} 扩充到 m 维正交基
*   6. Σ 为 m×n 对角阵, 对角元为降序奇异值
*
* @example
*   Matrix<Fraction> A = {{1, 1}, {1, 1}};
*   auto res = svdDecompose(A);
*   // res.U Σ V^T == A
*/

#include "core/AlgReal.h"
#include "core/Fraction.h"
#include "core/Matrix.h"

#include <vector>

namespace algemate::math {

struct SVDResult {
    Matrix<AlgReal>      U;               // m×m 正交矩阵
    Matrix<AlgReal>      Sigma;           // m×n 对角阵 (奇异值降序排列)
    Matrix<AlgReal>      V;               // n×n 正交矩阵 (列为右奇异向量, 按奇异值降序排列)
    std::vector<AlgReal> singularValues;  // 奇异值降序排列, 长度为 min(m, n) 的有效值 (完整长度为 n)
};

// 奇异值分解 A = U Σ V^T
// 输入任意实矩阵 (Matrix<Fraction>), 输出精确代数数矩阵 (Matrix<AlgReal>)
SVDResult svdDecompose(const Matrix<Fraction>& A);

}
