#include "JordanForm.h"
#include "ComplexEigen.h"
#include "LinearAlgebra.h"

#include <algorithm>
#include <stdexcept>
#include <vector>

namespace algemate::math {

namespace {

using MatC = Matrix<Complex>;

std::size_t rankComplex(const MatC& M) {
    MatC copy = M;
    return rrefComplex(copy);
}

MatC appendColumn(const MatC& B, const MatC& v) {
    if (B.cols() == 0) return v;
    return B.augment(v);
}

MatC extractColumn(const MatC& M, std::size_t j) {
    MatC out(M.rows(), 1);
    for (std::size_t i = 0; i < M.rows(); ++i) out(i, 0) = M(i, j);
    return out;
}

std::vector<std::vector<MatC>> buildChainsForEigenvalue(
        const MatC& Ac, const Complex& lambda, int mult)
{
    const std::size_t n = Ac.rows();
    MatC I = MatC::identity(n);
    MatC M = Ac - I * lambda;

    std::vector<MatC> nulls;
    MatC empty(n, 0);
    nulls.push_back(empty);  
    MatC curPow = M;
    nulls.push_back(nullspaceComplex(curPow));
    while (static_cast<int>(nulls.back().cols()) < mult) {
        curPow = curPow * M;
        nulls.push_back(nullspaceComplex(curPow));
        if (nulls.size() > n + 3) break; 
    }
    int s = static_cast<int>(nulls.size()) - 1;

    std::vector<int> dims(s + 1);
    for (int k = 0; k <= s; ++k) dims[k] = static_cast<int>(nulls[k].cols());

    std::vector<int> r(s + 2, 0);
    for (int k = 1; k <= s; ++k) r[k] = dims[k] - dims[k - 1];

    std::vector<std::vector<MatC>> chains;

    MatC spanned(n, 0);

    for (int k = s; k >= 1; --k) {
        int needed = r[k] - r[k + 1];
        if (needed <= 0) continue;

        MatC pool = spanned;
        for (std::size_t j = 0; j < nulls[k - 1].cols(); ++j) {
            pool = appendColumn(pool, extractColumn(nulls[k - 1], j));
        }
        int found = 0;
        for (std::size_t j = 0; j < nulls[k].cols() && found < needed; ++j) {
            MatC v = extractColumn(nulls[k], j);
            MatC aug = appendColumn(pool, v);
            if (rankComplex(aug) > rankComplex(pool)) {

                std::vector<MatC> chain;
                MatC cur = v;
                for (int i = 0; i < k; ++i) {
                    chain.push_back(cur);
                    cur = M * cur;
                }

                for (auto& cv : chain) spanned = appendColumn(spanned, cv);
                pool = appendColumn(pool, v);
                chains.push_back(chain);
                ++found;
            }
        }
    }
    return chains;
}

MatC buildJordanMatrix(const std::vector<JordanBlock>& blocks) {
    std::size_t total = 0;
    for (const auto& b : blocks) total += static_cast<std::size_t>(b.size);
    MatC J(total, total);
    std::size_t off = 0;
    Complex one = Complex(Fraction(1));
    for (const auto& b : blocks) {
        for (int i = 0; i < b.size; ++i) {
            J(off + i, off + i) = b.eigenvalue;
            if (i + 1 < b.size) {
                J(off + i, off + i + 1) = one;
            }
        }
        off += static_cast<std::size_t>(b.size);
    }
    return J;
}

} 

JordanResult jordanForm(const Matrix<Fraction>& A) {
    if (!A.isSquare()) throw std::invalid_argument("jordanForm: A must be square");

    auto ce = complexEigenvalues(A);
    if (!ce.unsolvedFactors.empty()) {
        throw std::domain_error("jordanForm: irreducible factor of degree >= 3, cannot split");
    }

    std::vector<ComplexEigenvalue> merged;
    for (const auto& ev : ce.eigenvalues) {
        bool found = false;
        for (auto& m : merged) {
            if (m.value == ev.value) {
                m.multiplicity += ev.multiplicity;
                found = true;
                break;
            }
        }
        if (!found) merged.push_back(ev);
    }

    const std::size_t n = A.rows();
    Matrix<Complex> Ac = toComplex(A);

    std::vector<JordanBlock> blocks;
    std::vector<Matrix<Complex>> Qcols;

    for (const auto& ev : merged) {
        auto chains = buildChainsForEigenvalue(Ac, ev.value, ev.multiplicity);

        std::sort(chains.begin(), chains.end(),
                  [](const std::vector<Matrix<Complex>>& a, const std::vector<Matrix<Complex>>& b){
                      return a.size() > b.size();
                  });
        for (auto& chain : chains) {
            int k = static_cast<int>(chain.size());

            for (int i = k - 1; i >= 0; --i) {
                Qcols.push_back(chain[i]);
            }
            blocks.push_back({ ev.value, k });
        }
    }

    Matrix<Complex> Q(n, n);
    for (std::size_t c = 0; c < Qcols.size(); ++c) {
        for (std::size_t r = 0; r < n; ++r) Q(r, c) = Qcols[c](r, 0);
    }
    Matrix<Complex> J = buildJordanMatrix(blocks);

    JordanResult out;
    out.J = std::move(J);
    out.Q = std::move(Q);
    out.blocks = std::move(blocks);
    return out;
}

} 
