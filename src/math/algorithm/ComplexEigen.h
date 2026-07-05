#pragma once

#include "core/Complex.h"
#include "core/Fraction.h"
#include "core/Matrix.h"
#include "core/Polynomial.h"

#include <cstddef>
#include <utility>
#include <vector>

namespace algemate::math {

Matrix<Complex> toComplex(const Matrix<Fraction>& A);

std::size_t     rrefComplex     (Matrix<Complex>& M);
Matrix<Complex> rrefComplexOf   (const Matrix<Complex>& M);

Matrix<Complex> nullspaceComplex(const Matrix<Complex>& M);

struct ComplexEigenvalue {
    Complex value;
    int     multiplicity;
};

struct ComplexEigenPair {
    Complex         value;
    int             multiplicity;
    Matrix<Complex> eigenspaceBasis;
};

struct ComplexEigenResult {
    std::vector<ComplexEigenvalue>                        eigenvalues;      
    std::vector<std::pair<Polynomial<Fraction>, int>>     unsolvedFactors;  
};

ComplexEigenResult            complexEigenvalues(const Matrix<Fraction>& A);

std::vector<ComplexEigenPair> complexEigenPairs (const Matrix<Fraction>& A);

}
