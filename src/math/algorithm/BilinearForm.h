#pragma once

/*
* @file BilinearForm.h
* @brief 二次型与合同标准形: 对称判定、顺序主子式、合同对角化、惯性指数、正定性分类
*
* 二次型 Q(x) = x^T A x 与实对称矩阵 A 一一对应 (x 为 n*1 列向量)
* 合同变换 x = P y 将 Q 化为 P^T A P = D, 其中 D 为对角阵
* 规范标准形: 正惯性指数 p 个 +1, 负惯性指数 q 个 -1, z = n - p - q 个 0 (Sylvester 惯性定理)
* 正定 <=> 所有顺序主子式均 > 0 (Sylvester 判据)
*
* 元素类型固定为 Fraction, 全程精确运算
*
* @example
* Matrix<Fraction> A = {{2, -1, 0},
*                       {-1, 2, -1},
*                       {0, -1, 2}};
* auto res = congruenceDiagonalize(A);             // P^T A P = D
* auto sig = quadraticSignature(A);                // (positive=3, negative=0, zero=0)
* bool pd  = isPositiveDefinite(A);                // true
*/

#include "core/Fraction.h"
#include "core/Matrix.h"

#include <vector>

namespace algemate::math {

// 合同对角化结果
struct CongruenceResult {
    Matrix<Fraction> D;    // 对角矩阵 (可能含 0, 表示退化)
    Matrix<Fraction> P;    // 可逆变换矩阵, 满足 P^T A P = D
};

// 二次型的惯性指数 (Sylvester 惯性定理)
struct QuadraticSignature {
    int positive = 0;  // 正特征值个数 (正惯性指数 p)
    int negative = 0;  // 负特征值个数 (负惯性指数 q)
    int zero     = 0;  // 零特征值个数 (退化维数)
};

// 二次型 / 对称矩阵的定性分类
enum class DefiniteClass {
    PositiveDefinite,      // 正定:  p = n,             Q(x) > 0 (x != 0)
    PositiveSemidefinite,  // 半正定: q = 0, z > 0,     Q(x) >= 0
    NegativeDefinite,      // 负定:  q = n,             Q(x) < 0 (x != 0)
    NegativeSemidefinite,  // 半负定: p = 0, z > 0,     Q(x) <= 0
    Indefinite,            // 不定:  p > 0 且 q > 0
    Zero                   // 零型:  A == 0
};

// 判断矩阵是否对称 A == A^T (非方阵返回 false)
bool isSymmetric(const Matrix<Fraction>& A);

// 顺序主子式序列 [det(A_1), det(A_2), ..., det(A_n)], 非方阵抛 std::invalid_argument
std::vector<Fraction> leadingPrincipalMinors(const Matrix<Fraction>& A);

// 合同对角化: 对称矩阵 A 返回 (D, P) 满足 P^T A P = D
// 要求 A 对称, 否则抛 std::invalid_argument
CongruenceResult congruenceDiagonalize(const Matrix<Fraction>& A);

// 二次型标准形: 返回对角的 D 本身, 可直接读出每项系数
// 等价于 congruenceDiagonalize(A).D
Matrix<Fraction> quadraticStandardForm(const Matrix<Fraction>& A);

// 惯性签名 (p, q, z), 要求 A 对称
QuadraticSignature quadraticSignature(const Matrix<Fraction>& A);

// 定性分类, 要求 A 对称
DefiniteClass classifyQuadraticForm(const Matrix<Fraction>& A);

// 正定判定 (要求对称) = Sylvester 判据: 所有顺序主子式 > 0
bool isPositiveDefinite(const Matrix<Fraction>& A);

// 半正定判定 (要求对称)
bool isPositiveSemidefinite(const Matrix<Fraction>& A);

// 负定判定 (要求对称)
bool isNegativeDefinite(const Matrix<Fraction>& A);

// 半负定判定 (要求对称)
bool isNegativeSemidefinite(const Matrix<Fraction>& A);

}
