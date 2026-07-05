#pragma once

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
    Matrix<AlgReal>              J;        
    Matrix<AlgReal>              Q;        
    std::vector<JordanRealBlock> blocks;   
};

JordanRealResult jordanFormReal(const Matrix<Fraction>& A);

}
