#include "RationalCanonical.h"
#include "LambdaMatrix.h"

#include <stdexcept>

namespace algemate::math {

using Poly = Polynomial<Fraction>;

namespace {

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

} 

RationalFormResult rationalCanonicalForm(const Matrix<Fraction>& A) {
    if (!A.isSquare()) {
        throw std::invalid_argument("rationalCanonicalForm: A must be square");
    }
    RationalFormResult out;
    out.invariantFactors = invariantFactors(lambdaMinus(A));

    std::vector<Matrix<Fraction>> blocks;
    for (const auto& d : out.invariantFactors) {

        if (d.degree() >= 1) {
            blocks.push_back(Matrix<Fraction>::companion(d));
        }
    }

    if (blocks.empty()) {

        out.F = Matrix<Fraction>(A.rows(), A.cols());
    } else {
        out.F = blockDiagVec(blocks);
    }

    if (!out.invariantFactors.empty()) {
        out.minimalPolynomial = out.invariantFactors.back();
    } else {

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

} 
