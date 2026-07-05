#include "LambdaMatrix.h"
#include "PolynomialAlg.h"

#include <algorithm>
#include <stdexcept>

namespace algemate::math {

namespace {

using Poly = Polynomial<Fraction>;

Poly constPoly(const Fraction& c) { return Poly(c); }

Poly polyLambda() { return Poly::x(); }

Poly polyQuot(const Poly& a, const Poly& b) {
    return a.divmod(b).quotient;
}

bool polyDivides(const Poly& a, const Poly& b) {
    if (a.isZero()) return true;
    return (a % b).isZero();
}

void makeRowMonic_(LambdaMatrix& S, LambdaMatrix& U, std::size_t k) {
    const Poly& pivot = S(k, k);
    if (pivot.isZero()) return;
    const Fraction& lc = pivot.leading();
    if (lc == Fraction(1)) return;
    Poly inv = constPoly(Fraction(1) / lc);
    S.scaleRow(k, inv);
    U.scaleRow(k, inv);
}

bool findPivot_(const LambdaMatrix& S, std::size_t k, std::size_t& pi, std::size_t& pj) {
    const std::size_t m = S.rows(), n = S.cols();
    bool found = false;
    int bestDeg = 0;
    for (std::size_t i = k; i < m; ++i) {
        for (std::size_t j = k; j < n; ++j) {
            const Poly& p = S(i, j);
            if (p.isZero()) continue;
            int d = p.degree();
            if (!found || d < bestDeg) {
                found = true;
                bestDeg = d;
                pi = i;
                pj = j;
                if (d == 0) return true; 
            }
        }
    }
    return found;
}

std::vector<Poly> decomposeSquarefree_(Poly f) {
    std::vector<Poly> primes;
    if (f.degree() <= 0) return primes;

    auto roots = rationalRoots(f);
    for (const auto& r : roots) {
        Poly lin = {-r, Fraction(1)}; 
        while (!f.isZero() && (f % lin).isZero() && f.degree() >= 1) {
            f = f / lin;
            primes.push_back(lin);

            break;
        }
    }
    if (f.degree() >= 1) {
        primes.push_back(f.monic());
    }
    return primes;
}

std::vector<ElementaryDivisor> factorMonic_(const Poly& d) {
    std::vector<ElementaryDivisor> out;
    if (d.degree() <= 0) return out;

    auto sqf = squarefreeFactorization(d);

    for (std::size_t k = 0; k < sqf.factors.size(); ++k) {
        const Poly& sf = sqf.factors[k];
        if (sf.degree() <= 0) continue;
        int power = static_cast<int>(k + 1);
        auto primes = decomposeSquarefree_(sf);
        for (auto& p : primes) {
            out.push_back({p.monic(), power});
        }
    }
    return out;
}

} 

LambdaMatrix lambdaMinus(const Matrix<Fraction>& A) {
    if (!A.isSquare()) {
        throw std::invalid_argument("lambdaMinus: A must be square");
    }
    const std::size_t n = A.rows();
    LambdaMatrix out(n, n);
    Poly L = polyLambda();
    for (std::size_t i = 0; i < n; ++i) {
        for (std::size_t j = 0; j < n; ++j) {
            if (i == j) {

                out(i, j) = L - constPoly(A(i, j));
            } else {
                out(i, j) = -constPoly(A(i, j));
            }
        }
    }
    return out;
}

SmithResult smithNormalForm(const LambdaMatrix& Min) {
    SmithResult result;
    const std::size_t m = Min.rows();
    const std::size_t n = Min.cols();
    LambdaMatrix S = Min;
    LambdaMatrix U = LambdaMatrix::identity(m);
    LambdaMatrix V = LambdaMatrix::identity(n);

    const std::size_t limit = std::min(m, n);
    std::size_t k = 0;
    while (k < limit) {

        std::size_t pi = k, pj = k;
        if (!findPivot_(S, k, pi, pj)) break; 

        if (pi != k) { S.swapRows(k, pi); U.swapRows(k, pi); }
        if (pj != k) { S.swapCols(k, pj); V.swapCols(k, pj); }

        bool rowChanged = true, colChanged = true;
        while (rowChanged || colChanged) {
            rowChanged = false;
            colChanged = false;

            for (std::size_t j = k + 1; j < n; ++j) {
                const Poly& e = S(k, j);
                if (e.isZero()) continue;
                Poly q = polyQuot(e, S(k, k));
                if (!q.isZero()) {

                    S.addMulCol(j, k, -q);
                    V.addMulCol(j, k, -q);
                }
                if (!S(k, j).isZero()) {

                    S.swapCols(k, j);
                    V.swapCols(k, j);
                    colChanged = true;
                    j = k; 
                }
            }

            for (std::size_t i = k + 1; i < m; ++i) {
                const Poly& e = S(i, k);
                if (e.isZero()) continue;
                Poly q = polyQuot(e, S(k, k));
                if (!q.isZero()) {

                    S.addMulRow(i, k, -q);
                    U.addMulRow(i, k, -q);
                }
                if (!S(i, k).isZero()) {
                    S.swapRows(k, i);
                    U.swapRows(k, i);
                    rowChanged = true;
                    i = k;
                }
            }
        }

        bool divisibilityOk = true;
        for (std::size_t i = k + 1; i < m && divisibilityOk; ++i) {
            for (std::size_t j = k + 1; j < n; ++j) {
                if (!S(i, j).isZero() && !polyDivides(S(i, j), S(k, k))) {

                    Poly one = constPoly(Fraction(1));
                    S.addMulRow(k, i, one);
                    U.addMulRow(k, i, one);
                    divisibilityOk = false;
                    break;
                }
            }
        }
        if (!divisibilityOk) {

            continue;
        }

        makeRowMonic_(S, U, k);
        ++k;
    }

    std::vector<Poly> invs;
    for (std::size_t i = 0; i < limit; ++i) {
        const Poly& d = S(i, i);
        if (d.isZero()) break;
        if (d.degree() == 0 && d.constant() == Fraction(1)) continue;
        invs.push_back(d);
    }

    result.S = std::move(S);
    result.U = std::move(U);
    result.V = std::move(V);
    result.invariantFactors = std::move(invs);
    return result;
}

std::vector<Polynomial<Fraction>> determinantalDivisors(const LambdaMatrix& M) {
    SmithResult sm = smithNormalForm(M);
    std::vector<Poly> D;
    const std::size_t limit = std::min(sm.S.rows(), sm.S.cols());
    Poly prod = constPoly(Fraction(1));
    for (std::size_t i = 0; i < limit; ++i) {
        const Poly& d = sm.S(i, i);
        if (d.isZero()) break;
        prod = prod * d;
        D.push_back(prod);
    }
    return D;
}

std::vector<Polynomial<Fraction>> invariantFactors(const LambdaMatrix& M) {
    return smithNormalForm(M).invariantFactors;
}

std::vector<ElementaryDivisor> elementaryDivisors(const LambdaMatrix& M) {
    auto facs = invariantFactors(M);
    std::vector<ElementaryDivisor> out;
    for (const auto& d : facs) {
        auto pieces = factorMonic_(d);
        for (auto& p : pieces) out.push_back(std::move(p));
    }
    return out;
}

std::vector<ElementaryDivisor> elementaryDivisorsOf(const Matrix<Fraction>& A) {
    return elementaryDivisors(lambdaMinus(A));
}

std::vector<ElementaryDivisor> elementaryDivisorsOfPoly(const Polynomial<Fraction>& monicPoly) {
    return factorMonic_(monicPoly);
}

JordanCanonicalResult jordanFromElementaryDivisors(const std::vector<ElementaryDivisor>& divisors) {
    std::vector<JordanBlockDesc> blocks;

    for (const auto& ed : divisors) {
        const Poly& prime = ed.prime;
        int power = ed.power;
        int deg = prime.degree();

        if (deg == 1) {

            Fraction a = Fraction(0) - prime.coeffs()[0];
            blocks.push_back({Complex(a), power});
        } else if (deg == 2) {

            Fraction p = prime.coeffs()[1];
            Fraction q = prime.coeffs()[0];
            Fraction disc = p * p - Fraction(4) * q;

            AlgReal negPOver2 = AlgReal(Fraction(0) - p) / AlgReal(Fraction(2));
            AlgReal sqrtAbs = AlgReal::sqrt(disc >= Fraction(0) ? disc : Fraction(0) - disc);
            AlgReal halfSqrt = sqrtAbs / AlgReal(Fraction(2));

            if (disc >= Fraction(0)) {

                Complex root1(negPOver2 + halfSqrt, AlgReal(Fraction(0)));
                Complex root2(negPOver2 - halfSqrt, AlgReal(Fraction(0)));
                blocks.push_back({root1, power});
                blocks.push_back({root2, power});
            } else {

                Complex root1(negPOver2, halfSqrt);
                Complex root2(negPOver2, AlgReal(Fraction(0)) - halfSqrt);
                blocks.push_back({root1, power});
                blocks.push_back({root2, power});
            }
        }

    }

    std::size_t total = 0;
    for (const auto& b : blocks) total += static_cast<std::size_t>(b.size);

    Matrix<Complex> J(total, total);
    std::size_t off = 0;
    for (const auto& b : blocks) {
        for (int i = 0; i < b.size; ++i) {
            J(off + i, off + i) = b.eigenvalue;
            if (i + 1 < b.size) {
                J(off + i, off + i + 1) = Complex(Fraction(1));
            }
        }
        off += static_cast<std::size_t>(b.size);
    }

    JordanCanonicalResult result;
    result.J = std::move(J);
    result.blocks = std::move(blocks);
    return result;
}

JordanCanonicalResult jordanCanonicalFormOf(const Matrix<Fraction>& A) {
    return jordanFromElementaryDivisors(elementaryDivisorsOf(A));
}

JordanCanonicalResult jordanFromBlocks(const std::vector<std::pair<Complex, int>>& blocksIn) {
    std::vector<JordanBlockDesc> blocks;
    blocks.reserve(blocksIn.size());
    std::size_t total = 0;
    for (const auto& [ev, sz] : blocksIn) {
        blocks.push_back({ev, sz});
        total += static_cast<std::size_t>(sz);
    }

    Matrix<Complex> J(total, total);
    std::size_t off = 0;
    for (const auto& b : blocks) {
        for (int i = 0; i < b.size; ++i) {
            J(off + i, off + i) = b.eigenvalue;
            if (i + 1 < b.size) {
                J(off + i, off + i + 1) = Complex(Fraction(1));
            }
        }
        off += static_cast<std::size_t>(b.size);
    }

    JordanCanonicalResult result;
    result.J = std::move(J);
    result.blocks = std::move(blocks);
    return result;
}

} 
