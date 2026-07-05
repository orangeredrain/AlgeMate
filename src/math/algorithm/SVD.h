#pragma once

#include "core/AlgReal.h"
#include "core/Fraction.h"
#include "core/Matrix.h"

#include <vector>

namespace algemate::math {

struct SVDResult {
    Matrix<AlgReal>      U;               
    Matrix<AlgReal>      Sigma;           
    Matrix<AlgReal>      V;               
    std::vector<AlgReal> singularValues;  
};

SVDResult svdDecompose(const Matrix<Fraction>& A);

}
