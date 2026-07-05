#pragma once

#include "core/Complex.h"
#include "core/Fraction.h"
#include "core/Matrix.h"
#include "core/Polynomial.h"

#include <cstddef>
#include <utility>
#include <vector>

namespace algemate::math {

using LambdaMatrix = Matrix<Polynomial<Fraction>>;

LambdaMatrix lambdaMinus(const Matrix<Fraction>& A);

struct SmithResult {
    LambdaMatrix                       S;                   
    LambdaMatrix                       U;                   
    LambdaMatrix                       V;                   
    std::vector<Polynomial<Fraction>>  invariantFactors;    
};
SmithResult smithNormalForm(const LambdaMatrix& M);

std::vector<Polynomial<Fraction>> determinantalDivisors(const LambdaMatrix& M);

std::vector<Polynomial<Fraction>> invariantFactors(const LambdaMatrix& M);

struct ElementaryDivisor {
    Polynomial<Fraction> prime;   
    int                  power;   
};
std::vector<ElementaryDivisor> elementaryDivisors  (const LambdaMatrix& M);
std::vector<ElementaryDivisor> elementaryDivisorsOf(const Matrix<Fraction>& A);

std::vector<ElementaryDivisor> elementaryDivisorsOfPoly(const Polynomial<Fraction>& monicPoly);

struct JordanBlockDesc {
    Complex eigenvalue;
    int     size;
};
struct JordanCanonicalResult {
    Matrix<Complex>               J;       
    std::vector<JordanBlockDesc>  blocks;  
};
JordanCanonicalResult jordanFromElementaryDivisors(const std::vector<ElementaryDivisor>& divisors);
JordanCanonicalResult jordanCanonicalFormOf(const Matrix<Fraction>& A);

JordanCanonicalResult jordanFromBlocks(const std::vector<std::pair<Complex, int>>& blocks);

}
