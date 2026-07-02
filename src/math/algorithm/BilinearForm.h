#pragma once

#include "core/Fraction.h"
#include "core/Matrix.h"

#include <vector>

namespace algemate::math {

struct CongruenceResult {
    Matrix<Fraction> D;    
    Matrix<Fraction> P;    
};

struct QuadraticSignature {
    int positive = 0;  
    int negative = 0;  
    int zero     = 0;  
};

enum class DefiniteClass {
    PositiveDefinite,      
    PositiveSemidefinite,  
    NegativeDefinite,      
    NegativeSemidefinite,  
    Indefinite,            
    Zero                   
};

bool isSymmetric(const Matrix<Fraction>& A);

std::vector<Fraction> leadingPrincipalMinors(const Matrix<Fraction>& A);

CongruenceResult congruenceDiagonalize(const Matrix<Fraction>& A);

Matrix<Fraction> quadraticStandardForm(const Matrix<Fraction>& A);

QuadraticSignature quadraticSignature(const Matrix<Fraction>& A);

DefiniteClass classifyQuadraticForm(const Matrix<Fraction>& A);

bool isPositiveDefinite(const Matrix<Fraction>& A);

bool isPositiveSemidefinite(const Matrix<Fraction>& A);

bool isNegativeDefinite(const Matrix<Fraction>& A);

bool isNegativeSemidefinite(const Matrix<Fraction>& A);

}
