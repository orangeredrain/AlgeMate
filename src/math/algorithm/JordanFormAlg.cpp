#include "JordanFormAlg.h"

#include "RealEigen.h"                 // realEigenvalues (AlgReal)
#include "LinearAlgebra.h"             // charpoly, nullspace, rank
#include "PolynomialAlg.h"             // squarefreePart (for multiplicity)
#include "core/Polynomial.h"

#include <algorithm>
#include <cstddef>
#include <stdexcept>
#include <vector>

namespace algemate::math {

using PolyF = Polynomial<Fraction>;

namespace {

// 构造 g 的伴随矩阵 C_g (d×d): x 作用于 K=Q[x]/(g) 的矩阵表示
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

// 构造 B = A ⊗ I_d - I_n ⊗ C_g, \in Q^{nd×nd}
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

// 把 nd 维 ℚ 向量 w 转为 K^n 向量 (每个分量 = PolyF deg < d)
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

// x · w 在 K 分块上: 每个 d-块以 C_g 左乘
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

// 列向量追加 (Matrix<Fraction>): B (nd × k) + v (nd × 1) -> nd × (k+1)
Matrix<Fraction> appendCol_(const Matrix<Fraction>& B, const std::vector<Fraction>& v) {
    const std::size_t nd = v.size();
    Matrix<Fraction> out(nd, B.cols() + 1);
    for (std::size_t c = 0; c < B.cols(); ++c)
        for (std::size_t r = 0; r < nd; ++r) out(r, c) = B(r, c);
    for (std::size_t r = 0; r < nd; ++r) out(r, B.cols()) = v[r];
    return out;
}

// 把 (M : nd × k) 中第 j 列提成 vector<Fraction>
std::vector<Fraction> extractCol_(const Matrix<Fraction>& M, std::size_t j) {
    std::vector<Fraction> v(M.rows());
    for (std::size_t r = 0; r < M.rows(); ++r) v[r] = M(r, j);
    return v;
}

// B·w (矩阵 × 向量)
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

// K-独立性检测: 给定已张成的 Q 子空间 spanned (nd × k), 检查把 (v, x·v, ..., x^{d-1} v)
// 全部加入是否严格增维 d 个. 若是, 表明 v 产生的 K-模在已有 K-模外, 可作为 top 向量.
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

// 把 K-模 v, x·v, ..., x^{d-1} v 全部加入 spanned
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

// 对 lam 构建全部 Jordan 链 (K-层面)
// 返回 std::vector<std::vector<std::vector<PolyF>>>:
//   chains[c] = 一条链, 长度 = Jordan 块大小
//   chains[c][i] = 第 i 个链元素 (i=0: top = 长度-1 次生成, 后续为 M 作用)
//   chains[c][i][r] = 该元素第 r 分量 ∈ K (PolyF, deg < d)
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

    // 迭代计算 ker(B^k) 直到饱和 (维度不再增长)
    std::vector<Matrix<Fraction>> nulls;
    nulls.push_back(Matrix<Fraction>(nd, 0));   // ker(B^0) = {0}
    Matrix<Fraction> curPow = B;
    nulls.push_back(nullspace(curPow));
    while (nulls.back().cols() > nulls[nulls.size() - 2].cols()) {
        if (nulls.size() > nd + 2) break;   // 安全保护
        curPow = curPow * B;
        nulls.push_back(nullspace(curPow));
    }
    int s = static_cast<int>(nulls.size()) - 1;  // 最大链长 candidate
    if (s < 1) return {};

    // K-维度序列 (每个 ℚ 维度 = d * K 维度)
    std::vector<int> dimsK(s + 1, 0);
    for (int k = 0; k <= s; ++k) dimsK[k] = static_cast<int>(nulls[k].cols()) / static_cast<int>(d);
    // r[k] = dimsK[k] - dimsK[k-1] = 链长 ≥ k 的 K-链数
    std::vector<int> rk(s + 2, 0);
    for (int k = 1; k <= s; ++k) rk[k] = dimsK[k] - dimsK[k - 1];

    // spanned: 已选链对应的 ℚ 向量张成 (含所有链元素 + x 作用下的完整 K-模)
    Matrix<Fraction> spanned(nd, 0);
    // pool_{k-1}: spanned + ker(B^{k-1}) 的 K-模子空间 (等价判据 via rank)
    // 这里按 Complex 版的朴素思路, 每次在 ker(B^k) 中挑 K-独立于 "spanned ∪ ker(B^{k-1})" 的向量

    std::vector<std::vector<std::vector<PolyF>>> chains;

    for (int k = s; k >= 1; --k) {
        int needed = rk[k] - rk[k + 1];
        if (needed <= 0) continue;
        // pool = spanned + ker(B^{k-1})
        Matrix<Fraction> pool = spanned;
        const Matrix<Fraction>& lower = nulls[k - 1];
        for (std::size_t j = 0; j < lower.cols(); ++j) {
            pool = appendCol_(pool, extractCol_(lower, j));
        }
        int found = 0;
        for (std::size_t j = 0; j < nulls[k].cols() && found < needed; ++j) {
            std::vector<Fraction> v = extractCol_(nulls[k], j);
            // K-独立性: v 加 (v, x v, ..., x^{d-1} v) 使 pool 增维 d
            if (isKIndependent_(pool, v, Cg, n, d)) {
                // 生成链: (v, Bv, B²v, ..., B^{k-1}v)  共 k 个 ℚ 向量, 每个可转 K^n
                std::vector<std::vector<PolyF>> chain;
                std::vector<Fraction> cur = v;
                for (int i = 0; i < k; ++i) {
                    chain.push_back(qVecToKVec_(cur, n, d));
                    if (i + 1 < k) cur = applyB_(B, cur);
                }
                // 把所有 k 个链元素对应的 K-模 (总 k*d 个 ℚ 向量) 加入 spanned 与 pool
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

// 组装 Jordan 矩阵 (block diag, 上三角 λ, 次对角 1)
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

}  // anonymous namespace

JordanRealResult jordanFormReal(const Matrix<Fraction>& A) {
    if (!A.isSquare()) throw std::invalid_argument("jordanFormReal: matrix must be square");
    const std::size_t n = A.rows();

    // 特征多项式 ℚ 层: 所有实根分裂 + 复根检测
    PolyF cp = charpoly(A);
    std::vector<AlgReal> realEigs = AlgReal::realRootsOf(cp);
    // 检查 charpoly 的总实重数 = n (否则存在复特征值)
    {
        // sum of multiplicities of distinct real roots:
        // 我们不直接知道每个 λ 的代数重数, 但可在下面 K-construction 里统计 nulls 饱和维度
        // 先判 simple case: 根数 = n 说明无重根且无复根
        // 后面组装时再校验总 Q 列数 == n
    }

    std::vector<JordanRealBlock> blocks;
    std::vector<std::vector<std::vector<PolyF>>> allChainsK;   // 顺序 flatten
    std::vector<AlgReal> chainLams;

    for (const AlgReal& lam : realEigs) {
        auto chains = buildKChains_(A, lam);
        // 按链长从大到小排序
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

    // 总列数校验: 若实部不足 n, 说明 charpoly 存在复根 → 拒绝
    std::size_t totalCols = 0;
    for (const auto& b : blocks) totalCols += static_cast<std::size_t>(b.size);
    if (totalCols != n) {
        throw std::domain_error("jordanFormReal: charpoly has non-real roots, use complex jordanForm");
    }

    // 组装 Q: 每条链 k 个链元素, Q 列顺序按 (B^{k-1}v, B^{k-2}v, ..., v)
    //   即 chain 存储顺序 [v, Bv, ..., B^{k-1}v], 倒序放入 Q
    Matrix<AlgReal> Q(n, n);
    std::size_t col = 0;
    for (std::size_t ci = 0; ci < allChainsK.size(); ++ci) {
        const auto& chain = allChainsK[ci];
        const AlgReal& lam = chainLams[ci];
        int k = static_cast<int>(chain.size());
        for (int i = k - 1; i >= 0; --i) {
            // chain[i] 是 n 维 K-向量, evaluatePoly 每分量
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
