#pragma once

/*
* @file OrthogonalDiag.h
* @brief 实对称矩阵的正交对角化 U^T A U = Λ (精确代数数 AlgReal)
*
* 对实对称矩阵 A, 找正交矩阵 U (U^T U = I) 使 U^T A U = Λ, Λ 对角, 对角元为实特征值
* 特征值用 AlgReal 精确表示 (任意次数代数数)
*
* 全部依靠 AlgReal::realRootsOf 的 Sturm 序列 + 区间隔离, 无次数上限
* 实对称矩阵特征多项式的根必为实数, 因此必会返回 n 个特征值 (计重数)
*
* AlgReal 精确线代工具链:
*   toAlgReal / rrefAlg / nullspaceAlg / gramSchmidtOrthonormal
* 亦可独立用于其他需要精确实数矩阵运算的场景
*
* @example
* Matrix<Fraction> A = {{2, 1}, {1, 2}};            // 特征值 1, 3
* auto res = orthogonalDiagonalize(A);
* // res.U^T * A * res.U == res.Lambda
*/

#include "core/AlgReal.h"
#include "core/Fraction.h"
#include "core/Matrix.h"

#include <cstddef>
#include <vector>

namespace algemate::math {

// ---------- Matrix<AlgReal> 精确线代工具 ----------

// 把 Matrix<Fraction> 元素逐个提升到 AlgReal
Matrix<AlgReal> toAlgReal(const Matrix<Fraction>& A);

// AlgReal 上的 RREF (就地), 返回秩
std::size_t     rrefAlg(Matrix<AlgReal>& M);

// AlgReal 上的零空间基 (列向量拼成矩阵), 无非平凡解时返回 n*0 空矩阵
Matrix<AlgReal> nullspaceAlg(const Matrix<AlgReal>& M);

// 列向量内积 <u, v>
AlgReal         dotProductAlg(const Matrix<AlgReal>& u, const Matrix<AlgReal>& v);

// Gram-Schmidt 正交单位化 (AlgReal 上)
// 输入 n*k 列向量组, 输出 n*k' 的正交单位矩阵 (k' = rank)
// 线性相关列自动丢弃
Matrix<AlgReal> gramSchmidtOrthonormal(const Matrix<AlgReal>& V);

// Gram-Schmidt 正交单位化 (Fraction 输入便捷重载): 内部自动提升到 AlgReal
Matrix<AlgReal> gramSchmidtOrthonormal(const Matrix<Fraction>& V);

// ---------- QR 分解 ----------

// QR 分解结果: A = Q R, Q 列正交单位 (n*k), R 上三角 (k*k), k = rank(A)
// 当 A 列满秩时 k = A.cols; 列不满秩时 Q 列数自动辟除.
struct QRResult {
    Matrix<AlgReal> Q;   // 列正交单位矩阵
    Matrix<AlgReal> R;   // 上三角 (k*A.cols), R = Q^T A
};

// A 输入默认为 Matrix<Fraction>, 内部提升到 AlgReal 后 Gram-Schmidt.
QRResult qrDecompose(const Matrix<Fraction>& A);
QRResult qrDecompose(const Matrix<AlgReal>& A);

// ---------- 实对称矩阵正交对角化 ----------

// 实对称矩阵的特征值-特征子空间对 (子空间已在 AlgReal 上正交单位化)
struct RealSymEigenPair {
    AlgReal         value;  // 特征值
    Matrix<AlgReal> basis;  // n*m 正交单位基, m = 该特征值的重数
};

// 正交对角化结果
struct OrthoDiagResult {
    Matrix<AlgReal>               U;       // 正交矩阵, U^T A U = Lambda
    Matrix<AlgReal>               Lambda;  // 对角阵, 对角元按 pairs 顺序排列
    std::vector<RealSymEigenPair> pairs;   // 按特征值升序排列, 每对含 basis
};

// 实对称矩阵的全部互异实特征值 (升序)
// A 必须对称, 否则抛 std::invalid_argument
std::vector<AlgReal> realSymmetricEigenvalues(const Matrix<Fraction>& A);

// 实对称矩阵的正交对角化, 要求 A 对称
// 支持任意次数代数特征值 (内部仍走 AlgReal 精确运算, 对 deg>=3 无理根规模较大矩阵可能较慢)
OrthoDiagResult orthogonalDiagonalize(const Matrix<Fraction>& A);

}
