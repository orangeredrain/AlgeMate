#pragma once

#include "core/Complex.h"
#include "core/Fraction.h"
#include "core/Matrix.h"

#include <cstddef>
#include <vector>

namespace algemate::math {

struct JordanBlock {
    Complex eigenvalue;
    int     size;
};

struct JordanResult {
    Matrix<Complex>          J;      
    Matrix<Complex>          Q;      
    std::vector<JordanBlock> blocks; 
};

JordanResult jordanForm(const Matrix<Fraction>& A);

}
