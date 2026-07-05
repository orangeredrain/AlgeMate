#pragma once

#include "core/Fraction.h"
#include "core/Matrix.h"
#include "core/Polynomial.h"
#include "trace/StepSequence.h"

#include <cstddef>
#include <vector>

namespace algemate::math {

struct SolveResult {
    bool             hasSolution = false; 
    Matrix<Fraction> particular;          
    Matrix<Fraction> nullspaceBasis;      
};

struct EigenPair {
    Fraction         value;            
    Matrix<Fraction> eigenspaceBasis;  
};

std::size_t      rref  (Matrix<Fraction>& M);
std::size_t      rref  (Matrix<Fraction>& M, StepSequence& trace);

Matrix<Fraction> rrefOf(const Matrix<Fraction>& M);
Matrix<Fraction> rrefOf(const Matrix<Fraction>& M, StepSequence& trace);

std::size_t      rank  (const Matrix<Fraction>& M);
std::size_t      rank  (const Matrix<Fraction>& M, StepSequence& trace);

Fraction         det   (const Matrix<Fraction>& M);
Fraction         det   (const Matrix<Fraction>& M, StepSequence& trace);

Matrix<Fraction> minorMatrix(const Matrix<Fraction>& M, std::size_t i, std::size_t j);

Fraction         cofactor(const Matrix<Fraction>& M, std::size_t i, std::size_t j);

Matrix<Fraction> adjugate(const Matrix<Fraction>& M);

Fraction         detByRowExpansion(const Matrix<Fraction>& M, std::size_t row);
Fraction         detByColExpansion(const Matrix<Fraction>& M, std::size_t col);

Matrix<Fraction> inverse(const Matrix<Fraction>& M);
Matrix<Fraction> inverse(const Matrix<Fraction>& M, StepSequence& trace);

SolveResult      solve  (const Matrix<Fraction>& A, const Matrix<Fraction>& b);
SolveResult      solve  (const Matrix<Fraction>& A, const Matrix<Fraction>& b, StepSequence& trace);

Matrix<Fraction> nullspace(const Matrix<Fraction>& M);
Matrix<Fraction> nullspace(const Matrix<Fraction>& M, StepSequence& trace);

struct EquivalenceResult {
    Matrix<Fraction> S;     
    Matrix<Fraction> P;     
    Matrix<Fraction> Q;     
    std::size_t      rank = 0;
};
EquivalenceResult equivalentNormalForm(const Matrix<Fraction>& A);

struct LUResult {
    Matrix<Fraction> L;   
    Matrix<Fraction> U;   
};
LUResult luDecompose(const Matrix<Fraction>& A);

Polynomial<Fraction>   charpoly(const Matrix<Fraction>& A);

std::vector<Fraction>  rationalEigenvalues(const Matrix<Fraction>& A);

std::vector<EigenPair> rationalEigenPairs(const Matrix<Fraction>& A);

}
