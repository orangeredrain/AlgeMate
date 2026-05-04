#pragma once

/*
* @file JordanFormAlg.h
* @brief 精确 Jordan 标准形 (over AlgReal), 支持 charpoly 任意次数不可约实因子
*
* 与 JordanForm.h 的 Complex 版本对比:
*   - JordanForm: 使用 Complex, 仅支持 charpoly 不可约因子次数 ≤ 2
*   - JordanFormAlg: 使用 AlgReal, 支持任意次数实根分裂
*
* 若 charpoly 有非实根 (即存在严格复特征值), 抛 std::domain_error
* 做法: K = Q[x]/(g) 上做线性代数, 最后 evaluatePoly 到 AlgReal, 避开 AlgReal 矩阵乘法
*
* @example
* Matrix<Fraction> A = {...};                        // 含 deg-3 不可约特征因子
* auto r = jordanFormReal(A);
* // r.Q^{-1} * toAlgReal(A) * r.Q == r.J (Jordan 块对角形式)
*/

#include "core/AlgReal.h"
#include "core/Fraction.h"
#include "core/Matrix.h"

#include <cstddef>
#include <vector>

namespace algemate::math {

struct JordanRealBlock {
    AlgReal eigenvalue;
    int     size;
};

struct JordanRealResult {
    Matrix<AlgReal>              J;        // Jordan 矩阵 (上三角: 对角 λ, 次对角 1)
    Matrix<AlgReal>              Q;        // 相似变换, Q^{-1} A Q = J
    std::vector<JordanRealBlock> blocks;   // 分块列表 (按排列顺序)
};

// 实 Jordan 标准形 (要求所有特征值为实代数数)
JordanRealResult jordanFormReal(const Matrix<Fraction>& A);

}
