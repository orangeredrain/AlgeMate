#pragma once

#include "core/Fraction.h"
#include "core/Matrix.h"
#include "core/Polynomial.h"

#include <cstddef>
#include <vector>

namespace algemate::math {

struct RationalFormResult {
    Matrix<Fraction>                   F;                  
    std::vector<Polynomial<Fraction>>  invariantFactors;   
    Polynomial<Fraction>               minimalPolynomial;  
};

RationalFormResult rationalCanonicalForm(const Matrix<Fraction>& A);

Matrix<Fraction>     frobeniusForm     (const Matrix<Fraction>& A);

Polynomial<Fraction> minimalPolynomial (const Matrix<Fraction>& A);

}
