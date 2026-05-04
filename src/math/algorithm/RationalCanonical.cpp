#include "RationalCanonical.h"
#include "LambdaMatrix.h"

#include <stdexcept>

namespace algemate::math {

using Poly = Polynomial<Fraction>;

namespace {

// 用 vector<Matrix> 构建分块对角矩阵 (Matrix::blockDiag 只接受 initializer_list, 此处提供通用版)
Matrix<Fraction> blockDiagVec(const std::vector<Matrix<Fraction>>& blocks) {
    std::size_t total = 0;
    for (const auto& b : blocks) {
        if (!b.isSquare() && !b.isEmpty())
            throw std::invalid_argument("blockDiagVec: block must be square");
        total += b.rows();
    }
    Matrix<Fraction> F(total, total);
    std::size_t offset = 0;
    for (const auto& b : blocks) {
        std::size_t sz = b.rows();
        for (std::size_t i = 0; i < sz; ++i) {
            for (std::size_t j = 0; j < sz; ++j) {
                F(offset + i, offset + j) = b(i, j);
            }
        }
        offset += sz;
    }
    return F;
}

} // anonymous namespace

RationalFormResult rationalCanonicalForm(const Matrix<Fraction>& A) {
    if (!A.isSquare()) {
        throw std::invalid_argument("rationalCanonicalForm: A must be square");
    }
    RationalFormResult out;
    out.invariantFactors = invariantFactors(lambdaMinus(A));

    std::vector<Matrix<Fraction>> blocks;
    for (const auto& d : out.invariantFactors) {
        // companion 接受首一且 degree >= 1
        if (d.degree() >= 1) {
            blocks.push_back(Matrix<Fraction>::companion(d));
        }
    }

    if (blocks.empty()) {
        // A 相似于 0 或空矩阵: 保留原尺寸的零矩阵
        out.F = Matrix<Fraction>(A.rows(), A.cols());
    } else {
        out.F = blockDiagVec(blocks);
    }

    // 最小多项式 = 最大不变因子 (链的最后一个)
    if (!out.invariantFactors.empty()) {
        out.minimalPolynomial = out.invariantFactors.back();
    } else {
        // 所有不变因子均为 1: 只在 A 为空时发生, 返回常数 1
        out.minimalPolynomial = Poly(Fraction(1));
    }
    return out;
}

Matrix<Fraction> frobeniusForm(const Matrix<Fraction>& A) {
    return rationalCanonicalForm(A).F;
}

Poly minimalPolynomial(const Matrix<Fraction>& A) {
    return rationalCanonicalForm(A).minimalPolynomial;
}

} // namespace algemate::math
