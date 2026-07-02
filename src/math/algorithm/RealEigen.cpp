#include "RealEigen.h"

#include "algorithm/LinearAlgebra.h"   

#include <stdexcept>
#include <utility>

namespace algemate::math {

using PolyF = Polynomial<Fraction>;

EigenspaceKBasis eigenspaceBasisK(const Matrix<Fraction>& A, const AlgReal& lam) {
    if (!A.isSquare()) throw std::invalid_argument("eigenspaceBasisK: matrix must be square");
    const std::size_t n = A.rows();
    const PolyF& g = lam.minPoly();
    const int dInt = g.degree();
    if (dInt <= 0) throw std::runtime_error("eigenspaceBasisK: invalid minPoly");
    const std::size_t d = static_cast<std::size_t>(dInt);

    Matrix<Fraction> Cg(d, d);
    for (std::size_t i = 0; i + 1 < d; ++i) Cg(i + 1, i) = Fraction(1);
    Fraction lead = g.coeffs()[d];
    for (std::size_t i = 0; i < d; ++i) {
        Cg(i, d - 1) = Fraction(0) - g.coeffs()[i] / lead;
    }

    const std::size_t nd = n * d;
    Matrix<Fraction> B(nd, nd);
    for (std::size_t i = 0; i < n; ++i) {
        for (std::size_t j = 0; j < n; ++j) {
            for (std::size_t k = 0; k < d; ++k) {
                B(i * d + k, j * d + k) = A(i, j);
            }
        }
    }
    for (std::size_t i = 0; i < n; ++i) {
        for (std::size_t r = 0; r < d; ++r) {
            for (std::size_t c = 0; c < d; ++c) {
                B(i * d + r, i * d + c) = B(i * d + r, i * d + c) - Cg(r, c);
            }
        }
    }

    Matrix<Fraction> null = nullspace(B);

    auto applyXTimes = [&](const std::vector<Fraction>& w) {
        std::vector<Fraction> out(nd, Fraction(0));
        for (std::size_t i = 0; i < n; ++i) {
            for (std::size_t r = 0; r < d; ++r) {
                Fraction s(0);
                for (std::size_t c = 0; c < d; ++c) {
                    s = s + Cg(r, c) * w[i * d + c];
                }
                out[i * d + r] = s;
            }
        }
        return out;
    };
    Matrix<Fraction> spanMat(nd, 0);
    std::vector<std::vector<Fraction>> repWs;
    for (std::size_t cidx = 0; cidx < null.cols(); ++cidx) {
        std::vector<Fraction> wc(nd);
        for (std::size_t r = 0; r < nd; ++r) wc[r] = null(r, cidx);
        std::size_t oldCols = spanMat.cols();
        Matrix<Fraction> cand(nd, oldCols + 1);
        for (std::size_t c = 0; c < oldCols; ++c)
            for (std::size_t r = 0; r < nd; ++r) cand(r, c) = spanMat(r, c);
        for (std::size_t r = 0; r < nd; ++r) cand(r, oldCols) = wc[r];
        std::size_t newRank = rank(cand);
        if (newRank > oldCols) {
            repWs.push_back(wc);
            std::vector<Fraction> cur = wc;
            for (std::size_t k = 0; k < d; ++k) {
                Matrix<Fraction> sm(nd, spanMat.cols() + 1);
                for (std::size_t cc = 0; cc < spanMat.cols(); ++cc)
                    for (std::size_t rr = 0; rr < nd; ++rr) sm(rr, cc) = spanMat(rr, cc);
                for (std::size_t rr = 0; rr < nd; ++rr) sm(rr, spanMat.cols()) = cur[rr];
                spanMat = sm;
                if (k + 1 < d) cur = applyXTimes(cur);
            }
        }
    }

    std::vector<std::vector<PolyF>> basisK;
    basisK.reserve(repWs.size());
    for (const auto& wc : repWs) {
        std::vector<PolyF> v(n);
        for (std::size_t i = 0; i < n; ++i) {
            PolyF f;
            for (std::size_t k = 0; k < d; ++k) {
                Fraction c = wc[i * d + k];
                if (!c.isZero()) f = f + PolyF::monomial(k, c);
            }
            v[i] = f;
        }
        basisK.push_back(std::move(v));
    }

    EigenspaceKBasis out;
    out.basisK = std::move(basisK);
    out.g      = g;
    return out;
}

Matrix<AlgReal> realEigenspaceBasis(const Matrix<Fraction>& A, const AlgReal& lam) {
    EigenspaceKBasis kb = eigenspaceBasisK(A, lam);
    const std::size_t n = A.rows();
    const std::size_t m = kb.basisK.size();
    Matrix<AlgReal> out(n, m);
    for (std::size_t c = 0; c < m; ++c) {
        for (std::size_t r = 0; r < n; ++r) {
            out(r, c) = AlgReal::evaluatePoly(kb.basisK[c][r], lam);
        }
    }
    return out;
}

std::vector<AlgReal> realEigenvalues(const Matrix<Fraction>& A) {
    if (!A.isSquare()) throw std::invalid_argument("realEigenvalues: matrix must be square");
    PolyF p = charpoly(A);
    return AlgReal::realRootsOf(p);
}

std::vector<RealEigenPair> realEigenPairs(const Matrix<Fraction>& A) {
    std::vector<AlgReal> eigs = realEigenvalues(A);
    std::vector<RealEigenPair> out;
    out.reserve(eigs.size());
    for (const AlgReal& lam : eigs) {
        RealEigenPair rp;
        rp.value = lam;
        rp.basis = realEigenspaceBasis(A, lam);
        out.push_back(std::move(rp));
    }
    return out;
}

}
