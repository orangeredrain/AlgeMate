#pragma once

/*
* @file JordanForm.h
* @brief Jordan 标准形 (复数域) 与相似变换矩阵
*
* 给定 Matrix<Fraction> A, 若特征多项式在 ℚ 上的不可约因子均为一次或二次,
* 则返回 J = Q^{-1} A Q, J 为 Jordan 块分块对角, Q 的列为广义特征向量链.
* 若存在 3 次及以上的不可约因子, 抛 std::domain_error (无法精确分裂).
*
* 对每个特征值 λ (代数重数 m):
*   1. 构造 M = A - λI (Complex)
*   2. 计算 dim ker(M^k) 序列, 得到各链长度分布
*   3. 递归挑选 "top" 广义特征向量, 生成链 (v, Mv, M^2 v, ...)
*   4. 所有链拼接 -> Q 的列; 每条链长度对应一个 J_k(λ) 块
*
* @example
* Matrix<Fraction> A = { {5, 1}, {0, 5} };
* auto r = jordanForm(A);
* // r.J = [[5,1],[0,5]], r.blocks = {(5, 2)}, r.Q * r.J = A * r.Q
*/

#include "core/Complex.h"
#include "core/Fraction.h"
#include "core/Matrix.h"

#include <cstddef>
#include <vector>

namespace algemate::math {

struct JordanBlock {
    Complex eigenvalue;
    int     size;
};

struct JordanResult {
    Matrix<Complex>          J;      // Jordan 矩阵 (上三角, 对角为特征值, 次对角 1)
    Matrix<Complex>          Q;      // 变换矩阵, Q^{-1} A Q = J
    std::vector<JordanBlock> blocks; // 分块列表 (按排列顺序)
};

JordanResult jordanForm(const Matrix<Fraction>& A);

}
