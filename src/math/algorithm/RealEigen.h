#pragma once

#include "core/AlgReal.h"
#include "core/Fraction.h"
#include "core/Matrix.h"
#include "core/Polynomial.h"

#include <cstddef>
#include <vector>

namespace algemate::math {

struct EigenspaceKBasis {
    std::vector<std::vector<Polynomial<Fraction>>> basisK;
    Polynomial<Fraction>                            g;
};

EigenspaceKBasis eigenspaceBasisK(const Matrix<Fraction>& A, const AlgReal& lam);

Matrix<AlgReal> realEigenspaceBasis(const Matrix<Fraction>& A, const AlgReal& lam);

std::vector<AlgReal> realEigenvalues(const Matrix<Fraction>& A);

struct RealEigenPair {
    AlgReal         value;
    Matrix<AlgReal> basis;   
};

std::vector<RealEigenPair> realEigenPairs(const Matrix<Fraction>& A);

}
