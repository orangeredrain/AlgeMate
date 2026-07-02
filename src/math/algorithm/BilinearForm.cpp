#include "BilinearForm.h"
#include "LinearAlgebra.h"

#include <stdexcept>

namespace algemate::math {

bool isSymmetric(const Matrix<Fraction>& A) {
    if (!A.isSquare()) return false;
    const std::size_t n = A.rows();
    for (std::size_t i = 0; i < n; ++i) {
        for (std::size_t j = i + 1; j < n; ++j) {
            if (!(A(i, j) == A(j, i))) return false;
        }
    }
    return true;
}

std::vector<Fraction> leadingPrincipalMinors(const Matrix<Fraction>& A) {
    if (!A.isSquare()) {
        throw std::invalid_argument("leadingPrincipalMinors: expect square matrix");
    }
    const std::size_t n = A.rows();
    std::vector<Fraction> out;
    out.reserve(n);
    for (std::size_t k = 1; k <= n; ++k) {
        out.push_back(det(A.submatrix(0, 0, k, k)));
    }
    return out;
}

CongruenceResult congruenceDiagonalize(const Matrix<Fraction>& A) {
    if (!isSymmetric(A)) {
        throw std::invalid_argument("congruenceDiagonalize: expect symmetric matrix");
    }
    const std::size_t n = A.rows();
    Matrix<Fraction> D = A;
    Matrix<Fraction> P = Matrix<Fraction>::identity(n);

    for (std::size_t k = 0; k < n; ++k) {

        if (D(k, k) == Fraction(0)) {

            std::size_t swapIdx = n;
            for (std::size_t i = k + 1; i < n; ++i) {
                if (!(D(i, i) == Fraction(0))) {
                    swapIdx = i;
                    break;
                }
            }
            if (swapIdx < n) {
                D.swapRows(k, swapIdx);
                D.swapCols(k, swapIdx);
                P.swapCols(k, swapIdx);
            } else {

                std::size_t pi = n, pj = n;
                for (std::size_t i = k; i < n && pi == n; ++i) {
                    for (std::size_t j = k; j < n; ++j) {
                        if (i != j && !(D(i, j) == Fraction(0))) {
                            pi = i; pj = j;
                            break;
                        }
                    }
                }
                if (pi == n) {

                    continue;
                }

                D.addMulCol(pi, pj, Fraction(1));
                D.addMulRow(pi, pj, Fraction(1));
                P.addMulCol(pi, pj, Fraction(1));
                if (pi != k) {
                    D.swapRows(k, pi);
                    D.swapCols(k, pi);
                    P.swapCols(k, pi);
                }
            }
        }

        Fraction pivot = D(k, k);
        for (std::size_t i = k + 1; i < n; ++i) {
            if (D(i, k) == Fraction(0)) continue;
            Fraction c = Fraction(0) - D(i, k) / pivot;

            D.addMulRow(i, k, c);

            D.addMulCol(i, k, c);

            P.addMulCol(i, k, c);
        }
    }

    return {D, P};
}

Matrix<Fraction> quadraticStandardForm(const Matrix<Fraction>& A) {
    return congruenceDiagonalize(A).D;
}

QuadraticSignature quadraticSignature(const Matrix<Fraction>& A) {
    Matrix<Fraction> D = congruenceDiagonalize(A).D;
    QuadraticSignature sig;
    for (std::size_t i = 0; i < D.rows(); ++i) {
        const Fraction& d = D(i, i);
        if (d == Fraction(0))         ++sig.zero;
        else if (d > Fraction(0))     ++sig.positive;
        else                          ++sig.negative;
    }
    return sig;
}

DefiniteClass classifyQuadraticForm(const Matrix<Fraction>& A) {
    if (!isSymmetric(A)) {
        throw std::invalid_argument("classifyQuadraticForm: expect symmetric matrix");
    }
    QuadraticSignature sig = quadraticSignature(A);
    const int n = static_cast<int>(A.rows());
    if (sig.positive == 0 && sig.negative == 0) return DefiniteClass::Zero;
    if (sig.positive > 0 && sig.negative > 0)   return DefiniteClass::Indefinite;
    if (sig.negative == 0) {
        return sig.positive == n ? DefiniteClass::PositiveDefinite
                                  : DefiniteClass::PositiveSemidefinite;
    }

    return sig.negative == n ? DefiniteClass::NegativeDefinite
                              : DefiniteClass::NegativeSemidefinite;
}

bool isPositiveDefinite(const Matrix<Fraction>& A) {
    if (!isSymmetric(A)) return false;

    std::vector<Fraction> ms = leadingPrincipalMinors(A);
    for (const Fraction& m : ms) {
        if (!(m > Fraction(0))) return false;
    }
    return true;
}

bool isPositiveSemidefinite(const Matrix<Fraction>& A) {
    if (!isSymmetric(A)) return false;
    QuadraticSignature sig = quadraticSignature(A);
    return sig.negative == 0;
}

bool isNegativeDefinite(const Matrix<Fraction>& A) {
    if (!isSymmetric(A)) return false;

    Matrix<Fraction> neg = A * Fraction(-1);
    return isPositiveDefinite(neg);
}

bool isNegativeSemidefinite(const Matrix<Fraction>& A) {
    if (!isSymmetric(A)) return false;
    QuadraticSignature sig = quadraticSignature(A);
    return sig.positive == 0;
}

}
