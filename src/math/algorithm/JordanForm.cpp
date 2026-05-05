#include "JordanForm.h"
#include "ComplexEigen.h"
#include "LinearAlgebra.h"

#include <algorithm>
#include <stdexcept>
#include <vector>

namespace algemate::math {

namespace {

using MatC = Matrix<Complex>;

// Complex 矩阵的秩 (基于就地 rrefComplex)
std::size_t rankComplex(const MatC& M) {
    MatC copy = M;
    return rrefComplex(copy);
}

// 把 n x 1 单列向量 v 追加为 B 新的一列; B 尺寸 n x k -> n x (k+1)
MatC appendColumn(const MatC& B, const MatC& v) {
    if (B.cols() == 0) return v;
    return B.augment(v);
}

// 从列矩阵 M 中提取第 j 列为 n x 1 矩阵
MatC extractColumn(const MatC& M, std::size_t j) {
    MatC out(M.rows(), 1);
    for (std::size_t i = 0; i < M.rows(); ++i) out(i, 0) = M(i, j);
    return out;
}

// 基于给定特征值 lambda 和代数重数 mult, 为 Ac (复矩阵) 构造广义特征向量链
// 返回的每条链按顺序: (top_v, M v, M^2 v, ..., M^{k-1} v) 即从 top 开始
std::vector<std::vector<MatC>> buildChainsForEigenvalue(
        const MatC& Ac, const Complex& lambda, int mult)
{
    const std::size_t n = Ac.rows();
    MatC I = MatC::identity(n);
    MatC M = Ac - I * lambda;

    // 计算 ker(M^k) 的基, 直到维数达到 mult
    std::vector<MatC> nulls;
    MatC empty(n, 0);
    nulls.push_back(empty);  // ker(M^0) = {0}
    MatC curPow = M;
    nulls.push_back(nullspaceComplex(curPow));
    while (static_cast<int>(nulls.back().cols()) < mult) {
        curPow = curPow * M;
        nulls.push_back(nullspaceComplex(curPow));
        if (nulls.size() > n + 3) break; // 安全保护
    }
    int s = static_cast<int>(nulls.size()) - 1;

    // dims[k] = dim(ker(M^k))
    std::vector<int> dims(s + 1);
    for (int k = 0; k <= s; ++k) dims[k] = static_cast<int>(nulls[k].cols());
    // r[k] = dims[k] - dims[k-1], 
    std::vector<int> r(s + 2, 0);
    for (int k = 1; k <= s; ++k) r[k] = dims[k] - dims[k - 1];

    std::vector<std::vector<MatC>> chains;
    // spanned: 已被选入某条链的所有向量生成的子空间基 (n x *)
    MatC spanned(n, 0);

    for (int k = s; k >= 1; --k) {
        int needed = r[k] - r[k + 1];
        if (needed <= 0) continue;
        // pool = spanned + ker(M^{k-1}); 新 top 向量需与 pool 线性独立
        MatC pool = spanned;
        for (std::size_t j = 0; j < nulls[k - 1].cols(); ++j) {
            pool = appendColumn(pool, extractColumn(nulls[k - 1], j));
        }
        int found = 0;
        for (std::size_t j = 0; j < nulls[k].cols() && found < needed; ++j) {
            MatC v = extractColumn(nulls[k], j);
            MatC aug = appendColumn(pool, v);
            if (rankComplex(aug) > rankComplex(pool)) {
                // 生成链 (v, M v, M^2 v, ..., M^{k-1} v)
                std::vector<MatC> chain;
                MatC cur = v;
                for (int i = 0; i < k; ++i) {
                    chain.push_back(cur);
                    cur = M * cur;
                }
                // 所有链向量加入 spanned
                for (auto& cv : chain) spanned = appendColumn(spanned, cv);
                pool = appendColumn(pool, v);
                chains.push_back(chain);
                ++found;
            }
        }
    }
    return chains;
}

// 从 JordanBlock 列表组装 Jordan 矩阵 (上三角形式: 对角 lambda, 次对角 1)
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

} // anonymous namespace

JordanResult jordanForm(const Matrix<Fraction>& A) {
    if (!A.isSquare()) throw std::invalid_argument("jordanForm: A must be square");

    auto ce = complexEigenvalues(A);
    if (!ce.unsolvedFactors.empty()) {
        throw std::domain_error("jordanForm: irreducible factor of degree >= 3, cannot split");
    }

    // 合并相同特征值 (累加重数)
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

    // 按特征值遍历, 收集所有 Jordan 块 + Q 的列
    std::vector<JordanBlock> blocks;
    std::vector<Matrix<Complex>> Qcols;

    for (const auto& ev : merged) {
        auto chains = buildChainsForEigenvalue(Ac, ev.value, ev.multiplicity);
        // 按链长从大到小排序
        std::sort(chains.begin(), chains.end(),
                  [](const std::vector<Matrix<Complex>>& a, const std::vector<Matrix<Complex>>& b){
                      return a.size() > b.size();
                  });
        for (auto& chain : chains) {
            int k = static_cast<int>(chain.size());
            // Q 列顺序: v_1 = M^{k-1} v, v_2 = M^{k-2} v, ..., v_k = v
            // 即按 chain 的倒序放
            for (int i = k - 1; i >= 0; --i) {
                Qcols.push_back(chain[i]);
            }
            blocks.push_back({ ev.value, k });
        }
    }

    // 组装 Q
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

} // namespace algemate::math
