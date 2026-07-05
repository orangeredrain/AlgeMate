#pragma once

#include "core/AlgReal.h"
#include "core/Fraction.h"
#include "core/Matrix.h"

#include <cstddef>
#include <vector>

namespace algemate::math {

Matrix<AlgReal> toAlgReal(const Matrix<Fraction>& A);

std::size_t     rrefAlg(Matrix<AlgReal>& M);

Matrix<AlgReal> nullspaceAlg(const Matrix<AlgReal>& M);

AlgReal         dotProductAlg(const Matrix<AlgReal>& u, const Matrix<AlgReal>& v);

Matrix<AlgReal> gramSchmidtOrthonormal(const Matrix<AlgReal>& V);

Matrix<AlgReal> gramSchmidtOrthonormal(const Matrix<Fraction>& V);

struct QRResult {
    Matrix<AlgReal> Q;   
    Matrix<AlgReal> R;   
};

QRResult qrDecompose(const Matrix<Fraction>& A);
QRResult qrDecompose(const Matrix<AlgReal>& A);

struct RealSymEigenPair {
    AlgReal         value;  
    Matrix<AlgReal> basis;  
};

struct OrthoDiagResult {
    Matrix<AlgReal>               U;       
    Matrix<AlgReal>               Lambda;  
    std::vector<RealSymEigenPair> pairs;   
};

std::vector<AlgReal> realSymmetricEigenvalues(const Matrix<Fraction>& A);

OrthoDiagResult orthogonalDiagonalize(const Matrix<Fraction>& A);

}
