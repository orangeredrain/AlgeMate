#include "OrthogonalDiag.h"
#include "BilinearForm.h"     // isSymmetric
#include "LinearAlgebra.h"    // charpoly, nullspace
#include "PolynomialAlg.h"    // rationalRoots, minPolyOfEval

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstdio>
#include <future>
#include <stdexcept>
#include <thread>
#include <vector>

#ifndef ALGEMATE_OD_DEBUG
#define ALGEMATE_OD_DEBUG 0
#endif

namespace algemate::math {

using PolyF = Polynomial<Fraction>;

// ---------- Matrix<AlgReal> 精确线代 ----------

Matrix<AlgReal> toAlgReal(const Matrix<Fraction>& A) {
    Matrix<AlgReal> R(A.rows(), A.cols());
    for (std::size_t i = 0; i < A.rows(); ++i)
        for (std::size_t j = 0; j < A.cols(); ++j)
            R(i, j) = AlgReal(A(i, j));
    return R;
}

std::size_t rrefAlg(Matrix<AlgReal>& M) {
    const std::size_t n = M.rows();
    const std::size_t m = M.cols();
    std::size_t pivotRow = 0;
    for (std::size_t c = 0; c < m && pivotRow < n; ++c) {
        // 寻找非零主元
        std::size_t pr = pivotRow;
        while (pr < n && M(pr, c).isZero()) ++pr;
        if (pr == n) continue;
        if (pr != pivotRow) M.swapRows(pivotRow, pr);
        AlgReal piv = M(pivotRow, c);
        for (std::size_t j = 0; j < m; ++j) {
            M(pivotRow, j) = M(pivotRow, j) / piv;
        }
        for (std::size_t r = 0; r < n; ++r) {
            if (r == pivotRow) continue;
            AlgReal factor = M(r, c);
            if (factor.isZero()) continue;
            for (std::size_t j = 0; j < m; ++j) {
                M(r, j) = M(r, j) - factor * M(pivotRow, j);
            }
        }
        ++pivotRow;
    }
    return pivotRow;
}

Matrix<AlgReal> nullspaceAlg(const Matrix<AlgReal>& Min) {
    const std::size_t n = Min.rows();
    const std::size_t m = Min.cols();
    if (m == 0) return Matrix<AlgReal>(n, 0);
    Matrix<AlgReal> R = Min;
    rrefAlg(R);
    // 主元列定位
    std::vector<int> pivotColOfRow;  // pivotColOfRow[i] = 行 i 的主元列 (或 -1)
    pivotColOfRow.reserve(R.rows());
    std::vector<bool> isPivot(m, false);
    for (std::size_t i = 0; i < R.rows(); ++i) {
        int pc = -1;
        for (std::size_t j = 0; j < m; ++j) {
            if (!R(i, j).isZero()) { pc = static_cast<int>(j); break; }
        }
        pivotColOfRow.push_back(pc);
        if (pc >= 0) isPivot[pc] = true;
    }
    // 自由列 => 基向量
    std::vector<std::size_t> freeCols;
    for (std::size_t j = 0; j < m; ++j) if (!isPivot[j]) freeCols.push_back(j);
    if (freeCols.empty()) return Matrix<AlgReal>(m, 0);

    Matrix<AlgReal> basis(m, freeCols.size());
    for (std::size_t k = 0; k < freeCols.size(); ++k) {
        std::size_t fc = freeCols[k];
        basis(fc, k) = AlgReal(Fraction(1));
        for (std::size_t i = 0; i < R.rows(); ++i) {
            int pc = pivotColOfRow[i];
            if (pc < 0) continue;
            basis(static_cast<std::size_t>(pc), k) =
                AlgReal() - R(i, fc);
        }
    }
    return basis;
}

AlgReal dotProductAlg(const Matrix<AlgReal>& u, const Matrix<AlgReal>& v) {
    if (u.cols() != 1 || v.cols() != 1 || u.rows() != v.rows()) {
        throw std::invalid_argument("dotProductAlg: expect same-length column vectors");
    }
    AlgReal s;
    for (std::size_t i = 0; i < u.rows(); ++i) s = s + u(i, 0) * v(i, 0);
    return s;
}

// ---------- 数值降级路径 + 精确路径内部实现 ----------
namespace {
constexpr auto kTimeout = std::chrono::seconds(3);

// 精确 Gram-Schmidt (AlgReal), 仅由线程超时包装器调用
Matrix<AlgReal> gsExactAlg_(const Matrix<AlgReal>& V) {
    if (V.isEmpty()) return V;
    const std::size_t n = V.rows();
    const std::size_t k = V.cols();
    std::vector<Matrix<AlgReal>> qs;
    qs.reserve(k);
    for (std::size_t i = 0; i < k; ++i) {
        Matrix<AlgReal> v(n, 1);
        for (std::size_t r = 0; r < n; ++r) v(r, 0) = V(r, i);
        for (const auto& q : qs) {
            AlgReal proj;
            for (std::size_t r = 0; r < n; ++r) proj = proj + v(r, 0) * q(r, 0);
            if (proj.isZero()) continue;
            for (std::size_t r = 0; r < n; ++r) v(r, 0) = v(r, 0) - proj * q(r, 0);
        }
        AlgReal nn;
        for (std::size_t r = 0; r < n; ++r) nn = nn + v(r, 0) * v(r, 0);
        if (nn.isZero()) continue;
        AlgReal norm = AlgReal::sqrt(nn);
        Matrix<AlgReal> q(n, 1);
        for (std::size_t r = 0; r < n; ++r) q(r, 0) = v(r, 0) / norm;
        qs.push_back(q);
    }
    if (qs.empty()) return Matrix<AlgReal>(n, 0);
    Matrix<AlgReal> Q(n, qs.size());
    for (std::size_t j = 0; j < qs.size(); ++j)
        for (std::size_t r = 0; r < n; ++r) Q(r, j) = qs[j](r, 0);
    return Q;
}

// double GS (从 AlgReal 矩阵读取 toDouble)
Matrix<AlgReal> gsNumericAlg_(const Matrix<AlgReal>& V) {
    const std::size_t m = V.rows();
    const std::size_t n = V.cols();
    const double eps = 1e-12;
    std::vector<std::vector<double>> qs;
    qs.reserve(n);
    for (std::size_t j = 0; j < n; ++j) {
        std::vector<double> v(m);
        for (std::size_t i = 0; i < m; ++i) v[i] = V(i, j).toDouble();
        for (const auto& q : qs) {
            double dot = 0.0;
            for (std::size_t i = 0; i < m; ++i) dot += v[i] * q[i];
            for (std::size_t i = 0; i < m; ++i) v[i] -= dot * q[i];
        }
        double norm2 = 0.0;
        for (std::size_t i = 0; i < m; ++i) norm2 += v[i] * v[i];
        double norm = std::sqrt(norm2);
        if (norm < eps) continue;
        for (std::size_t i = 0; i < m; ++i) v[i] /= norm;
        qs.push_back(std::move(v));
    }
    if (qs.empty()) return Matrix<AlgReal>(m, 0);
    Matrix<AlgReal> Q(m, qs.size());
    for (std::size_t j = 0; j < qs.size(); ++j)
        for (std::size_t i = 0; i < m; ++i)
            Q(i, j) = AlgReal::fromDouble(qs[j][i]);
    return Q;
}

// double GS (从 Fraction 矩阵读取 toDouble)
Matrix<AlgReal> gsNumeric_(const Matrix<Fraction>& V) {
    const std::size_t m = V.rows();
    const std::size_t n = V.cols();
    Matrix<double> Ad(m, n);
    for (std::size_t i = 0; i < m; ++i)
        for (std::size_t j = 0; j < n; ++j)
            Ad(i, j) = V(i, j).toDouble();
    const double eps = 1e-12;
    std::vector<std::vector<double>> qs;
    qs.reserve(n);
    for (std::size_t j = 0; j < n; ++j) {
        std::vector<double> v(m);
        for (std::size_t i = 0; i < m; ++i) v[i] = Ad(i, j);
        for (const auto& q : qs) {
            double dot = 0.0;
            for (std::size_t i = 0; i < m; ++i) dot += v[i] * q[i];
            for (std::size_t i = 0; i < m; ++i) v[i] -= dot * q[i];
        }
        double norm2 = 0.0;
        for (std::size_t i = 0; i < m; ++i) norm2 += v[i] * v[i];
        double norm = std::sqrt(norm2);
        if (norm < eps) continue;
        for (std::size_t i = 0; i < m; ++i) v[i] /= norm;
        qs.push_back(std::move(v));
    }
    if (qs.empty()) return Matrix<AlgReal>(m, 0);
    Matrix<AlgReal> Q(m, qs.size());
    for (std::size_t j = 0; j < qs.size(); ++j)
        for (std::size_t i = 0; i < m; ++i)
            Q(i, j) = AlgReal::fromDouble(qs[j][i]);
    return Q;
}

QRResult qrNumeric_(const Matrix<Fraction>& A) {
    const std::size_t m = A.rows();
    const std::size_t n = A.cols();
    Matrix<double> Ad(m, n);
    for (std::size_t i = 0; i < m; ++i)
        for (std::size_t j = 0; j < n; ++j)
            Ad(i, j) = A(i, j).toDouble();
    const double eps = 1e-12;
    std::vector<std::vector<double>> qs;
    qs.reserve(n);
    for (std::size_t j = 0; j < n; ++j) {
        std::vector<double> v(m);
        for (std::size_t i = 0; i < m; ++i) v[i] = Ad(i, j);
        for (const auto& q : qs) {
            double dot = 0.0;
            for (std::size_t i = 0; i < m; ++i) dot += v[i] * q[i];
            for (std::size_t i = 0; i < m; ++i) v[i] -= dot * q[i];
        }
        double norm2 = 0.0;
        for (std::size_t i = 0; i < m; ++i) norm2 += v[i] * v[i];
        double norm = std::sqrt(norm2);
        if (norm < eps) continue;
        for (std::size_t i = 0; i < m; ++i) v[i] /= norm;
        qs.push_back(std::move(v));
    }
    std::size_t k = qs.size();
    Matrix<AlgReal> Q(m, k);
    for (std::size_t j = 0; j < k; ++j)
        for (std::size_t i = 0; i < m; ++i)
            Q(i, j) = AlgReal::fromDouble(qs[j][i]);
    Matrix<AlgReal> R(k, n);
    for (std::size_t i = 0; i < k; ++i)
        for (std::size_t j = 0; j < n; ++j) {
            double s = 0.0;
            for (std::size_t l = 0; l < m; ++l) s += qs[i][l] * Ad(l, j);
            R(i, j) = AlgReal::fromDouble(s);
        }
    return {std::move(Q), std::move(R)};
}

// double QR (从 AlgReal 矩阵读取 toDouble)
QRResult qrNumericAlg_(const Matrix<AlgReal>& A) {
    const std::size_t m = A.rows();
    const std::size_t n = A.cols();
    Matrix<double> Ad(m, n);
    for (std::size_t i = 0; i < m; ++i)
        for (std::size_t j = 0; j < n; ++j)
            Ad(i, j) = A(i, j).toDouble();
    const double eps = 1e-12;
    std::vector<std::vector<double>> qs;
    qs.reserve(n);
    for (std::size_t j = 0; j < n; ++j) {
        std::vector<double> v(m);
        for (std::size_t i = 0; i < m; ++i) v[i] = Ad(i, j);
        for (const auto& q : qs) {
            double dot = 0.0;
            for (std::size_t i = 0; i < m; ++i) dot += v[i] * q[i];
            for (std::size_t i = 0; i < m; ++i) v[i] -= dot * q[i];
        }
        double norm2 = 0.0;
        for (std::size_t i = 0; i < m; ++i) norm2 += v[i] * v[i];
        double norm = std::sqrt(norm2);
        if (norm < eps) continue;
        for (std::size_t i = 0; i < m; ++i) v[i] /= norm;
        qs.push_back(std::move(v));
    }
    std::size_t k = qs.size();
    Matrix<AlgReal> Q(m, k);
    for (std::size_t j = 0; j < k; ++j)
        for (std::size_t i = 0; i < m; ++i)
            Q(i, j) = AlgReal::fromDouble(qs[j][i]);
    Matrix<AlgReal> R(k, n);
    for (std::size_t i = 0; i < k; ++i)
        for (std::size_t j = 0; j < n; ++j) {
            double s = 0.0;
            for (std::size_t l = 0; l < m; ++l) s += qs[i][l] * Ad(l, j);
            R(i, j) = AlgReal::fromDouble(s);
        }
    return {std::move(Q), std::move(R)};
}

}  // anonymous namespace

Matrix<AlgReal> gramSchmidtOrthonormal(const Matrix<AlgReal>& V) {
    if (V.isEmpty()) return V;
    std::promise<Matrix<AlgReal>> prom;
    auto fut = prom.get_future();
    std::thread worker([p = std::move(prom), Vc = V]() mutable {
        try {
            p.set_value(gsExactAlg_(Vc));
        } catch (...) {
            p.set_exception(std::current_exception());
        }
    });
    worker.detach();
    auto status = fut.wait_for(kTimeout);
    if (status == std::future_status::ready) {
        try { return fut.get(); } catch (...) { return gsNumericAlg_(V); }
    }
    return gsNumericAlg_(V);
}

Matrix<AlgReal> gramSchmidtOrthonormal(const Matrix<Fraction>& V) {
    std::promise<Matrix<AlgReal>> prom;
    auto fut = prom.get_future();
    std::thread worker([p = std::move(prom), Vc = V]() mutable {
        try {
            p.set_value(gsExactAlg_(toAlgReal(Vc)));
        } catch (...) {
            p.set_exception(std::current_exception());
        }
    });
    worker.detach();
    auto status = fut.wait_for(kTimeout);
    if (status == std::future_status::ready) {
        try { return fut.get(); } catch (...) { return gsNumeric_(V); }
    }
    return gsNumeric_(V);
}

// ---------- QR 分解 ----------

QRResult qrDecompose(const Matrix<AlgReal>& A) {
    std::promise<QRResult> prom;
    auto fut = prom.get_future();
    std::thread worker([p = std::move(prom), Ac = A]() mutable {
        try {
            Matrix<AlgReal> Q = gsExactAlg_(Ac);
            Matrix<AlgReal> R = Q.transpose() * Ac;
            p.set_value(QRResult{ std::move(Q), std::move(R) });
        } catch (...) {
            p.set_exception(std::current_exception());
        }
    });
    worker.detach();
    auto status = fut.wait_for(kTimeout);
    if (status == std::future_status::ready) {
        try { return fut.get(); } catch (...) { return qrNumericAlg_(A); }
    }
    return qrNumericAlg_(A);
}

QRResult qrDecompose(const Matrix<Fraction>& A) {
    std::promise<QRResult> prom;
    auto fut = prom.get_future();
    std::thread worker([p = std::move(prom), Ac = A]() mutable {
        try {
            auto Aalg = toAlgReal(Ac);
            Matrix<AlgReal> Q = gsExactAlg_(Aalg);
            Matrix<AlgReal> R = Q.transpose() * Aalg;
            p.set_value(QRResult{ std::move(Q), std::move(R) });
        } catch (...) {
            p.set_exception(std::current_exception());
        }
    });
    worker.detach();
    auto status = fut.wait_for(kTimeout);
    if (status == std::future_status::ready) {
        try { return fut.get(); } catch (...) { return qrNumeric_(A); }
    }
    return qrNumeric_(A);
}

// ---------- 特征值求解 (通过 AlgReal::realRootsOf 覆盖任意次数) ----------

std::vector<AlgReal> realSymmetricEigenvalues(const Matrix<Fraction>& A) {
    if (!isSymmetric(A)) {
        throw std::invalid_argument("realSymmetricEigenvalues: expect symmetric matrix");
    }
    PolyF p = charpoly(A);
    return AlgReal::realRootsOf(p);
}

// ---------- 正交对角化主函数 ----------

namespace {

// 扩展欧几里得 on Q[x]: 返回 u*a + v*b = gcd
void extGcdPoly_(const PolyF& a, const PolyF& b, PolyF& gOut, PolyF& uOut, PolyF& vOut) {
    if (b.isZero()) {
        gOut = a;
        uOut = PolyF(Fraction(1));
        vOut = PolyF();
        return;
    }
    PolyF g1, u1, v1;
    extGcdPoly_(b, a % b, g1, u1, v1);
    PolyF q = a / b;
    gOut = g1;
    uOut = v1;
    vOut = u1 - q * v1;
}

// a 在 Q[x]/g 中的逆 (gcd(a, g) 必须为常数)
PolyF invModPoly_(const PolyF& a, const PolyF& g) {
    PolyF a_r = a % g;
    if (a_r.isZero()) throw std::domain_error("invModPoly_: zero element");
    PolyF gcd_, u, v;
    extGcdPoly_(a_r, g, gcd_, u, v);
    if (gcd_.degree() != 0) throw std::domain_error("invModPoly_: non-invertible (g reducible and a in a proper ideal)");
    Fraction c = gcd_.coeffs()[0];
    PolyF inv = u * (Fraction(1) / c);
    return inv % g;
}

// 求 λ-特征子空间并在 K = Q[x]/(g) 上正交化, 然后 evaluate + AlgReal 归一
// 返回 Matrix<AlgReal> (n × k) 已正交归一
Matrix<AlgReal> eigenspaceBasisAlg_(const Matrix<Fraction>& A, const AlgReal& lam) {
    const std::size_t n = A.rows();
    const PolyF& g = lam.minPoly();
    const int dInt = g.degree();
    if (dInt <= 0) throw std::runtime_error("eigenspaceBasisAlg_: invalid minPoly");
    const std::size_t d = static_cast<std::size_t>(dInt);

    // Companion matrix C_g (d×d)
    Matrix<Fraction> Cg(d, d);
    for (std::size_t i = 0; i + 1 < d; ++i) Cg(i + 1, i) = Fraction(1);
    Fraction lead = g.coeffs()[d];
    for (std::size_t i = 0; i < d; ++i) {
        Cg(i, d - 1) = Fraction(0) - g.coeffs()[i] / lead;
    }

    // B = A ⊗ I_d - I_n ⊗ C_g
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

    // K-独立代表提取 (x · w 在 Q 上等价 Cg-block 作用)
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

    // K-表示基向量: basisK[i][j] ∈ K (PolyF, deg < d)
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

    // K-GS 正交化 (不归一): v_new = v - Σ (<v,q>_K / <q,q>_K) q
    std::vector<std::vector<PolyF>> qsK;
    for (auto& v : basisK) {
        for (const auto& q : qsK) {
            PolyF inner;
            for (std::size_t j = 0; j < n; ++j) inner = (inner + v[j] * q[j]) % g;
            PolyF den;
            for (std::size_t j = 0; j < n; ++j) den = (den + q[j] * q[j]) % g;
            if (den.isZero()) continue;  // K 可约时可能遇到, 跳过
            PolyF dInv = invModPoly_(den, g);
            PolyF factor = (inner * dInv) % g;
            for (std::size_t j = 0; j < n; ++j) v[j] = (v[j] - (factor * q[j]) % g) % g;
        }
        bool nonzero = false;
        for (const auto& f : v) if (!f.isZero()) { nonzero = true; break; }
        if (nonzero) qsK.push_back(std::move(v));
    }

    // evaluate + AlgReal 归一 (K 内预算 u_r = q[r]^2 * qqK^{-1} mod g,
    // 每分量只做一次 sqrt, 彻底绕过 AlgReal::operator/ 的 deg-9 squarefreePart 热点)
    Matrix<AlgReal> outM(n, qsK.size());
    for (std::size_t c = 0; c < qsK.size(); ++c) {
        const auto& q = qsK[c];
        // qqK = Σ q[j]^2 mod g  ∈ K
        PolyF qqK;
        for (std::size_t j = 0; j < n; ++j) qqK = (qqK + q[j] * q[j]) % g;
        if (qqK.isZero()) throw std::runtime_error("eigenspaceBasisAlg_: zero norm squared");
        // K 上的 1/qqK (一次扩展欧几里得, 纯 Q[x] 运算, 微秒级)
        PolyF qqK_inv = invModPoly_(qqK, g);

        for (std::size_t r = 0; r < n; ++r) {
            // K 内: u_r = q[r]^2 * qqK_inv mod g  (deg < d),
            //   u_r(lam) = q[r](lam)^2 / qqK(lam) = (q[r](lam) / norm)^2 ≥ 0
            PolyF qr2 = (q[r] * q[r]) % g;
            PolyF u_r = (qr2 * qqK_inv) % g;

            AlgReal mag = AlgReal::evaluatePoly(u_r, lam);   // deg ≤ d
            if (mag.isZero()) { outM(r, c) = AlgReal(Fraction(0)); continue; }
            AlgReal absVal = AlgReal::sqrt(mag);             // sqrt 输入 deg ≤ d, 快

            // 符号: q[r](lam) 决定 (qqA > 0)
            AlgReal vA = AlgReal::evaluatePoly(q[r], lam);
            int sgn = vA.sign();
            outM(r, c) = (sgn < 0) ? (AlgReal(Fraction(0)) - absVal) : absVal;
        }
    }
    return outM;
}

}

namespace {

// double Jacobi 对称矩阵特征分解
OrthoDiagResult orthoDiagNumeric_(const Matrix<Fraction>& A) {
    const std::size_t n = A.rows();
    Matrix<double> S(n, n);
    for (std::size_t i = 0; i < n; ++i)
        for (std::size_t j = 0; j < n; ++j)
            S(i, j) = A(i, j).toDouble();
    Matrix<double> V(n, n);
    for (std::size_t i = 0; i < n; ++i) V(i, i) = 1.0;
    const double tol = 1e-12;
    const int maxSweeps = 100;
    for (int sweep = 0; sweep < maxSweeps; ++sweep) {
        double maxOff = 0.0;
        for (std::size_t i = 0; i + 1 < n; ++i)
            for (std::size_t j = i + 1; j < n; ++j)
                if (std::abs(S(i, j)) > maxOff) maxOff = std::abs(S(i, j));
        if (maxOff < tol) break;
        for (std::size_t p = 0; p + 1 < n; ++p) {
            for (std::size_t q = p + 1; q < n; ++q) {
                if (std::abs(S(p, q)) < tol) continue;
                double tau = (S(q, q) - S(p, p)) / (2.0 * S(p, q));
                double t;
                if (std::abs(tau) > 1e150)
                    t = 0.5 / tau;
                else
                    t = ((tau >= 0.0) ? 1.0 : -1.0) / (std::abs(tau) + std::sqrt(1.0 + tau * tau));
                double c = 1.0 / std::sqrt(1.0 + t * t);
                double s = c * t;
                double Spp = S(p, p), Sqq = S(q, q), Spq = S(p, q);
                S(p, p) = Spp - t * Spq;
                S(q, q) = Sqq + t * Spq;
                S(p, q) = S(q, p) = 0.0;
                for (std::size_t k = 0; k < n; ++k) {
                    if (k == p || k == q) continue;
                    double skp = S(k, p), skq = S(k, q);
                    S(k, p) = S(p, k) = c * skp - s * skq;
                    S(k, q) = S(q, k) = s * skp + c * skq;
                }
                for (std::size_t k = 0; k < n; ++k) {
                    double vkp = V(k, p), vkq = V(k, q);
                    V(k, p) = c * vkp - s * vkq;
                    V(k, q) = s * vkp + c * vkq;
                }
            }
        }
    }
    // 特征值按升序排列 (与精确路径一致)
    std::vector<std::size_t> order(n);
    for (std::size_t i = 0; i < n; ++i) order[i] = i;
    std::sort(order.begin(), order.end(),
              [&](std::size_t a, std::size_t b) { return S(a, a) < S(b, b); });
    OrthoDiagResult res;
    Matrix<AlgReal> U(n, n);
    Matrix<AlgReal> Lambda(n, n);
    for (std::size_t k = 0; k < n; ++k) {
        std::size_t src = order[k];
        double lam = S(src, src);
        Lambda(k, k) = AlgReal::fromDouble(lam);
        for (std::size_t i = 0; i < n; ++i)
            U(i, k) = AlgReal::fromDouble(V(i, src));
        RealSymEigenPair pair{AlgReal::fromDouble(lam),
                              Matrix<AlgReal>(n, 1)};
        for (std::size_t i = 0; i < n; ++i)
            pair.basis(i, 0) = AlgReal::fromDouble(V(i, src));
        res.pairs.push_back(std::move(pair));
    }
    res.U = std::move(U);
    res.Lambda = std::move(Lambda);
    return res;
}

}  // anonymous namespace

OrthoDiagResult orthogonalDiagonalize(const Matrix<Fraction>& A) {
    if (!isSymmetric(A)) {
        throw std::invalid_argument("orthogonalDiagonalize: expect symmetric matrix");
    }

    auto exactOD = [](Matrix<Fraction> Ac) -> OrthoDiagResult {
        const std::size_t n = Ac.rows();
        std::vector<AlgReal> eigs = realSymmetricEigenvalues(Ac);

        OrthoDiagResult res;
        res.pairs.reserve(eigs.size());
        std::vector<Matrix<AlgReal>> columnBlocks;
        std::vector<AlgReal>         diagEntries;

        for (const AlgReal& lam : eigs) {
            if (ALGEMATE_OD_DEBUG) { std::fprintf(stderr, "[OD] lam=%g basis calc\n", lam.toDouble()); std::fflush(stderr); }
            Matrix<AlgReal> Q = eigenspaceBasisAlg_(Ac, lam);
            if (ALGEMATE_OD_DEBUG) { std::fprintf(stderr, "[OD] lam=%g basis cols=%zu\n", lam.toDouble(), Q.cols()); std::fflush(stderr); }
            if (Q.cols() == 0) {
                throw std::runtime_error("orthogonalDiagonalize: empty eigenspace");
            }
            RealSymEigenPair pair{lam, Q};
            res.pairs.push_back(pair);
            columnBlocks.push_back(Q);
            for (std::size_t c = 0; c < Q.cols(); ++c) diagEntries.push_back(lam);
        }

        std::size_t total = 0;
        for (const auto& Q : columnBlocks) total += Q.cols();
        Matrix<AlgReal> U(n, total);
        std::size_t offset = 0;
        for (const auto& Q : columnBlocks) {
            for (std::size_t c = 0; c < Q.cols(); ++c)
                for (std::size_t r = 0; r < n; ++r)
                    U(r, offset + c) = Q(r, c);
            offset += Q.cols();
        }
        Matrix<AlgReal> Lambda(n, n);
        for (std::size_t i = 0; i < n; ++i) Lambda(i, i) = diagEntries[i];

        res.U      = U;
        res.Lambda = Lambda;
        return res;
    };

    std::promise<OrthoDiagResult> prom;
    auto fut = prom.get_future();
    std::thread worker([p = std::move(prom), Ac = A, exactOD]() mutable {
        try {
            p.set_value(exactOD(std::move(Ac)));
        } catch (...) {
            p.set_exception(std::current_exception());
        }
    });
    worker.detach();
    auto status = fut.wait_for(kTimeout);
    if (status == std::future_status::ready) {
        try { return fut.get(); } catch (...) { return orthoDiagNumeric_(A); }
    }
    return orthoDiagNumeric_(A);
}

}
