#include "JordanFormAlg.h"

#include "RealEigen.h"                 
#include "LinearAlgebra.h"             
#include "PolynomialAlg.h"             
#include "core/Polynomial.h"

#include <algorithm>
#include <cstddef>
#include <stdexcept>
#include <vector>

namespace algemate::math {

using PolyF = Polynomial<Fraction>;

namespace {

Matrix<Fraction> companionOf_(const PolyF& g) {
    const int dInt = g.degree();
    if (dInt <= 0) throw std::runtime_error("companionOf_: invalid minPoly");
    const std::size_t d = static_cast<std::size_t>(dInt);
    Matrix<Fraction> Cg(d, d);
    for (std::size_t i = 0; i + 1 < d; ++i) Cg(i + 1, i) = Fraction(1);
    Fraction lead = g.coeffs()[d];
    for (std::size_t i = 0; i < d; ++i) {
        Cg(i, d - 1) = Fraction(0) - g.coeffs()[i] / lead;
    }
    return Cg;
}

Matrix<Fraction> buildBlock_(const Matrix<Fraction>& A, const Matrix<Fraction>& Cg) {
    const std::size_t n = A.rows();
    const std::size_t d = Cg.rows();
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
    return B;
}

std::vector<PolyF> qVecToKVec_(const std::vector<Fraction>& w, std::size_t n, std::size_t d) {
    std::vector<PolyF> v(n);
    for (std::size_t i = 0; i < n; ++i) {
        PolyF f;
        for (std::size_t k = 0; k < d; ++k) {
            Fraction c = w[i * d + k];
            if (!c.isZero()) f = f + PolyF::monomial(k, c);
        }
        v[i] = f;
    }
    return v;
}

std::vector<Fraction> applyXTimes_(const std::vector<Fraction>& w,
                                   const Matrix<Fraction>& Cg,
                                   std::size_t n, std::size_t d) {
    const std::size_t nd = n * d;
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
}

Matrix<Fraction> appendCol_(const Matrix<Fraction>& B, const std::vector<Fraction>& v) {
    const std::size_t nd = v.size();
    Matrix<Fraction> out(nd, B.cols() + 1);
    for (std::size_t c = 0; c < B.cols(); ++c)
        for (std::size_t r = 0; r < nd; ++r) out(r, c) = B(r, c);
    for (std::size_t r = 0; r < nd; ++r) out(r, B.cols()) = v[r];
    return out;
}

std::vector<Fraction> extractCol_(const Matrix<Fraction>& M, std::size_t j) {
    std::vector<Fraction> v(M.rows());
    for (std::size_t r = 0; r < M.rows(); ++r) v[r] = M(r, j);
    return v;
}

std::vector<Fraction> applyB_(const Matrix<Fraction>& B, const std::vector<Fraction>& w) {
    const std::size_t nd = B.rows();
    std::vector<Fraction> out(nd, Fraction(0));
    for (std::size_t r = 0; r < nd; ++r) {
        Fraction s(0);
        for (std::size_t c = 0; c < nd; ++c) s = s + B(r, c) * w[c];
        out[r] = s;
    }
    return out;
}

bool isKIndependent_(const Matrix<Fraction>& spanned,
                     const std::vector<Fraction>& v,
                     const Matrix<Fraction>& Cg,
                     std::size_t n, std::size_t d) {
    Matrix<Fraction> cand = spanned;
    std::vector<Fraction> cur = v;
    for (std::size_t k = 0; k < d; ++k) {
        cand = appendCol_(cand, cur);
        if (k + 1 < d) cur = applyXTimes_(cur, Cg, n, d);
    }
    return rank(cand) == spanned.cols() + d;
}

void addKModule_(Matrix<Fraction>& spanned,
                 const std::vector<Fraction>& v,
                 const Matrix<Fraction>& Cg,
                 std::size_t n, std::size_t d) {
    std::vector<Fraction> cur = v;
    for (std::size_t k = 0; k < d; ++k) {
        spanned = appendCol_(spanned, cur);
        if (k + 1 < d) cur = applyXTimes_(cur, Cg, n, d);
    }
}

std::vector<std::vector<std::vector<PolyF>>> buildKChains_(
        const Matrix<Fraction>& A, const AlgReal& lam)
{
    const std::size_t n = A.rows();
    const PolyF& g = lam.minPoly();
    const int dInt = g.degree();
    if (dInt <= 0) throw std::runtime_error("buildKChains_: invalid minPoly");
    const std::size_t d = static_cast<std::size_t>(dInt);
    const std::size_t nd = n * d;

    Matrix<Fraction> Cg = companionOf_(g);
    Matrix<Fraction> B  = buildBlock_(A, Cg);

    std::vector<Matrix<Fraction>> nulls;
    nulls.push_back(Matrix<Fraction>(nd, 0));   
    Matrix<Fraction> curPow = B;
    nulls.push_back(nullspace(curPow));
    while (nulls.back().cols() > nulls[nulls.size() - 2].cols()) {
        if (nulls.size() > nd + 2) break;   
        curPow = curPow * B;
        nulls.push_back(nullspace(curPow));
    }
    int s = static_cast<int>(nulls.size()) - 1;  
    if (s < 1) return {};

    std::vector<int> dimsK(s + 1, 0);
    for (int k = 0; k <= s; ++k) dimsK[k] = static_cast<int>(nulls[k].cols()) / static_cast<int>(d);

    std::vector<int> rk(s + 2, 0);
    for (int k = 1; k <= s; ++k) rk[k] = dimsK[k] - dimsK[k - 1];

    Matrix<Fraction> spanned(nd, 0);

    std::vector<std::vector<std::vector<PolyF>>> chains;

    for (int k = s; k >= 1; --k) {
        int needed = rk[k] - rk[k + 1];
        if (needed <= 0) continue;

        Matrix<Fraction> pool = spanned;
        const Matrix<Fraction>& lower = nulls[k - 1];
        for (std::size_t j = 0; j < lower.cols(); ++j) {
            pool = appendCol_(pool, extractCol_(lower, j));
        }
        int found = 0;
        for (std::size_t j = 0; j < nulls[k].cols() && found < needed; ++j) {
            std::vector<Fraction> v = extractCol_(nulls[k], j);

            if (isKIndependent_(pool, v, Cg, n, d)) {

                std::vector<std::vector<PolyF>> chain;
                std::vector<Fraction> cur = v;
                for (int i = 0; i < k; ++i) {
                    chain.push_back(qVecToKVec_(cur, n, d));
                    if (i + 1 < k) cur = applyB_(B, cur);
                }

                std::vector<Fraction> cv = v;
                for (int i = 0; i < k; ++i) {
                    addKModule_(spanned, cv, Cg, n, d);
                    addKModule_(pool,    cv, Cg, n, d);
                    if (i + 1 < k) cv = applyB_(B, cv);
                }
                chains.push_back(std::move(chain));
                ++found;
            }
        }
        if (found < needed) {
            throw std::runtime_error("jordanFormReal: failed to find enough K-independent top vectors");
        }
    }
    return chains;
}

Matrix<AlgReal> buildJordan_(const std::vector<JordanRealBlock>& blocks) {
    std::size_t total = 0;
    for (const auto& b : blocks) total += static_cast<std::size_t>(b.size);
    Matrix<AlgReal> J(total, total);
    AlgReal one(Fraction(1));
    std::size_t off = 0;
    for (const auto& b : blocks) {
        for (int i = 0; i < b.size; ++i) {
            J(off + i, off + i) = b.eigenvalue;
            if (i + 1 < b.size) J(off + i, off + i + 1) = one;
        }
        off += static_cast<std::size_t>(b.size);
    }
    return J;
}

}  

JordanRealResult jordanFormReal(const Matrix<Fraction>& A) {
    if (!A.isSquare()) throw std::invalid_argument("jordanFormReal: matrix must be square");
    const std::size_t n = A.rows();

    PolyF cp = charpoly(A);
    std::vector<AlgReal> realEigs = AlgReal::realRootsOf(cp);

    {

    }

    std::vector<JordanRealBlock> blocks;
    std::vector<std::vector<std::vector<PolyF>>> allChainsK;   
    std::vector<AlgReal> chainLams;

    for (const AlgReal& lam : realEigs) {
        auto chains = buildKChains_(A, lam);

        std::sort(chains.begin(), chains.end(),
                  [](const std::vector<std::vector<PolyF>>& a,
                     const std::vector<std::vector<PolyF>>& b){
                      return a.size() > b.size();
                  });
        for (auto& chain : chains) {
            int k = static_cast<int>(chain.size());
            allChainsK.push_back(std::move(chain));
            chainLams.push_back(lam);
            blocks.push_back({ lam, k });
        }
    }

    std::size_t totalCols = 0;
    for (const auto& b : blocks) totalCols += static_cast<std::size_t>(b.size);
    if (totalCols != n) {
        throw std::domain_error("jordanFormReal: charpoly has non-real roots, use complex jordanForm");
    }

    Matrix<AlgReal> Q(n, n);
    std::size_t col = 0;
    for (std::size_t ci = 0; ci < allChainsK.size(); ++ci) {
        const auto& chain = allChainsK[ci];
        const AlgReal& lam = chainLams[ci];
        int k = static_cast<int>(chain.size());
        for (int i = k - 1; i >= 0; --i) {

            for (std::size_t r = 0; r < n; ++r) {
                Q(r, col) = AlgReal::evaluatePoly(chain[i][r], lam);
            }
            ++col;
        }
    }

    Matrix<AlgReal> J = buildJordan_(blocks);

    JordanRealResult out;
    out.J      = std::move(J);
    out.Q      = std::move(Q);
    out.blocks = std::move(blocks);
    return out;
}

}
