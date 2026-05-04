#pragma once

/*
* @file LinearAlgebra.h
* @brief 线性代数算法集: RREF, 秩, 行列式, 求逆, 解方程组, 零空间, 特征值
*
* 每个算法提供两组重载:
*   纯计算版 用于科学计算
*   带追踪版 用于教学/演示
* 两组重载共用同一份内部实现
*
* 元素类型固定为 Fraction, 保证精确运算
*
* @example
* Matrix<Fraction> A = {...};
* auto R = rrefOf(A);                       // 纯计算
* StepSequence trace;
* auto R2 = rrefOf(A, trace);               // 带过程记录
* Fraction d = det(A);
* auto Ainv = inverse(A);
* auto res  = solve(A, b);
* auto N    = nullspace(A);
* auto p    = charpoly(A);
* auto eps  = rationalEigenPairs(A);
*/

#include "core/Fraction.h"
#include "core/Matrix.h"
#include "core/Polynomial.h"
#include "trace/StepSequence.h"

#include <cstddef>
#include <vector>

namespace algemate::math {

// 解方程组 Ax = b 的结果
struct SolveResult {
    bool             hasSolution = false; // 方程组是否相容
    Matrix<Fraction> particular;          // 特解 (n x 1), 无解时为空
    Matrix<Fraction> nullspaceBasis;      // 零空间基 (n x k), 列为基向量, 唯一解时为空
};

// 特征值与特征向量组
struct EigenPair {
    Fraction         value;            // 特征值
    Matrix<Fraction> eigenspaceBasis;  // 特征子空间基 (列为基向量, n x k)
};

// RREF: 就地化简, 返回秩
std::size_t      rref  (Matrix<Fraction>& M);
std::size_t      rref  (Matrix<Fraction>& M, StepSequence& trace);

// rrefOf: 返回化简后的副本, 不修改原矩阵
Matrix<Fraction> rrefOf(const Matrix<Fraction>& M);
Matrix<Fraction> rrefOf(const Matrix<Fraction>& M, StepSequence& trace);

// rank: 矩阵的秩
std::size_t      rank  (const Matrix<Fraction>& M);
std::size_t      rank  (const Matrix<Fraction>& M, StepSequence& trace);

// det: 方阵行列式, 非方阵抛 std::invalid_argument
Fraction         det   (const Matrix<Fraction>& M);
Fraction         det   (const Matrix<Fraction>& M, StepSequence& trace);

// 余子式矩阵 minor(i,j): 删除第 i 行、第 j 列后的 (rows-1)×(cols-1) 矩阵
Matrix<Fraction> minorMatrix(const Matrix<Fraction>& M, std::size_t i, std::size_t j);

// 代数余子式 A_{i,j} = (-1)^{i+j} * det(minor(i,j)), i,j 从 0 起
Fraction         cofactor(const Matrix<Fraction>& M, std::size_t i, std::size_t j);

// 伴随矩阵 adj(A) = [A_{j,i}] (代数余子式矩阵的转置), 满足 A * adj(A) = det(A) * I
Matrix<Fraction> adjugate(const Matrix<Fraction>& M);

// Laplace 按第 row 行展开计算 det（教学版, 递归到 2×2 基础绨，O(n!) 仅用于展示/小矩阵）
Fraction         detByRowExpansion(const Matrix<Fraction>& M, std::size_t row);
Fraction         detByColExpansion(const Matrix<Fraction>& M, std::size_t col);

// inverse: 方阵求逆, 非方阵抛 std::invalid_argument, 奇异矩阵抛 std::domain_error
Matrix<Fraction> inverse(const Matrix<Fraction>& M);
Matrix<Fraction> inverse(const Matrix<Fraction>& M, StepSequence& trace);

// solve: 解 Ax = b, 自动判断唯一解/无穷多解/无解
SolveResult      solve  (const Matrix<Fraction>& A, const Matrix<Fraction>& b);
SolveResult      solve  (const Matrix<Fraction>& A, const Matrix<Fraction>& b, StepSequence& trace);

// nullspace: 零空间基 (列向量拼成矩阵), 无非平凡解时返回 n x 0 空矩阵
Matrix<Fraction> nullspace(const Matrix<Fraction>& M);
Matrix<Fraction> nullspace(const Matrix<Fraction>& M, StepSequence& trace);

// 相抵标准形: 对任意 Q 上矩阵 A ∈ Q^{m×n}, 存在可逆 P(m×m), Q(n×n) 使 P·A·Q = S,
// 其中 S = diag(1,...,1,0,...,0), 1 的个数 = rank(A) = r
struct EquivalenceResult {
    Matrix<Fraction> S;     // [I_r 0; 0 0]
    Matrix<Fraction> P;     // m×m 可逆, 左变换
    Matrix<Fraction> Q;     // n×n 可逆, 右变换
    std::size_t      rank = 0;
};
EquivalenceResult equivalentNormalForm(const Matrix<Fraction>& A);

// LU 分解 (Doolittle): A = L U, L 为单位下三角, U 为上三角
// 要求 A 为方阵且不需要行交换主元 (即所有顺序主子式非零).
// 若消元过程出现 0 主元, 抛 std::domain_error (本实现不做 PA=LU).
struct LUResult {
    Matrix<Fraction> L;   // 单位下三角, 对角元为 1
    Matrix<Fraction> U;   // 上三角
};
LUResult luDecompose(const Matrix<Fraction>& A);

// charpoly: 特征多项式 p(x) = det(xI - A), 首一, 非方阵抛 std::invalid_argument
// 采用 Faddeev-LeVerrier 递推算法
Polynomial<Fraction>   charpoly(const Matrix<Fraction>& A);

// rationalEigenvalues: 有理特征值 (升序去重)
std::vector<Fraction>  rationalEigenvalues(const Matrix<Fraction>& A);

// rationalEigenPairs: 有理特征值 + 对应特征子空间基, 按特征值升序
std::vector<EigenPair> rationalEigenPairs(const Matrix<Fraction>& A);

}
