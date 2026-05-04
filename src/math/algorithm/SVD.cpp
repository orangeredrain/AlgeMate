#include "SVD.h"
#include "OrthogonalDiag.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <future>
#include <stdexcept>
#include <thread>
#include <utility>
#include <vector>

namespace algemate::math {

namespace {

// 计算 A^T A (Fraction 保持精确)
Matrix<Fraction> ataFraction_(const Matrix<Fraction>& A) {
    const std::size_t m = A.rows();
    const std::size_t n = A.cols();
    Matrix<Fraction> B(n, n);
    for (std::size_t i = 0; i < n; ++i) {
        for (std::size_t j = 0; j < n; ++j) {
            Fraction s(0);
            for (std::size_t k = 0; k < m; ++k) {
                s = s + A(k, i) * A(k, j);
            }
            B(i, j) = s;
        }
    }
    return B;
}

// 数值降级路径: 当奇异值 minPoly 次数 > 2 时使用
// one-sided Jacobi SVD (double), 稳定且不会触发高次 AlgReal 结式爆炸
SVDResult svdJacobiNumeric_(const Matrix<Fraction>& A) {
    const std::size_t m = A.rows();
    const std::size_t n = A.cols();

    // 将 A 转为 double 矩阵 Ad (在其上原地做列旋转)
    Matrix<double> Ad(m, n);
    for (std::size_t i = 0; i < m; ++i) {
        for (std::size_t j = 0; j < n; ++j) {
            Ad(i, j) = A(i, j).toDouble();
        }
    }
    Matrix<double> Vd(n, n);
    for (std::size_t i = 0; i < n; ++i) Vd(i, i) = 1.0;

    const double tol = 1e-14;
    const int maxSweeps = 80;
    for (int sweep = 0; sweep < maxSweeps; ++sweep) {
        double maxOff = 0.0;
        for (std::size_t p = 0; p + 1 < n; ++p) {
            for (std::size_t q = p + 1; q < n; ++q) {
                double alpha = 0.0, beta = 0.0, gamma = 0.0;
                for (std::size_t i = 0; i < m; ++i) {
                    alpha += Ad(i, p) * Ad(i, p);
                    beta  += Ad(i, q) * Ad(i, q);
                    gamma += Ad(i, p) * Ad(i, q);
                }
                double off = std::abs(gamma);
                if (off > maxOff) maxOff = off;
                double thresh = tol * std::sqrt(alpha * beta);
                if (off <= thresh) continue;
                double zeta = (beta - alpha) / (2.0 * gamma);
                double t;
                if (std::abs(zeta) > 1e150) {
                    t = 0.5 / zeta;
                } else {
                    double sgn = (zeta >= 0.0) ? 1.0 : -1.0;
                    t = sgn / (std::abs(zeta) + std::sqrt(1.0 + zeta * zeta));
                }
                double c = 1.0 / std::sqrt(1.0 + t * t);
                double s = c * t;
                for (std::size_t i = 0; i < m; ++i) {
                    double ap = Ad(i, p), aq = Ad(i, q);
                    Ad(i, p) = c * ap - s * aq;
                    Ad(i, q) = s * ap + c * aq;
                }
                for (std::size_t i = 0; i < n; ++i) {
                    double vp = Vd(i, p), vq = Vd(i, q);
                    Vd(i, p) = c * vp - s * vq;
                    Vd(i, q) = s * vp + c * vq;
                }
            }
        }
        if (maxOff < tol) break;
    }

    // σ_j = ||Ad 的第 j 列||, 按降序排列
    std::vector<double> sigma(n, 0.0);
    for (std::size_t j = 0; j < n; ++j) {
        double s2 = 0.0;
        for (std::size_t i = 0; i < m; ++i) s2 += Ad(i, j) * Ad(i, j);
        sigma[j] = std::sqrt(s2);
    }
    std::vector<std::size_t> order(n);
    for (std::size_t i = 0; i < n; ++i) order[i] = i;
    std::sort(order.begin(), order.end(),
              [&](std::size_t a, std::size_t b) { return sigma[a] > sigma[b]; });

    // V (n×n) 按 order 重排
    Matrix<AlgReal> Vout(n, n);
    for (std::size_t k = 0; k < n; ++k) {
        std::size_t src = order[k];
        for (std::size_t i = 0; i < n; ++i) {
            Vout(i, k) = AlgReal::fromDouble(Vd(i, src));
        }
    }

    // U (m×m): 前 r 列为 u_j = Ad_j / σ_j, 余下列以 GS 扩充标准基
    const double sigEps = 1e-10;
    std::size_t r = 0;
    while (r < n && sigma[order[r]] > sigEps) ++r;

    Matrix<double> Ud(m, m);
    for (std::size_t k = 0; k < r && k < m; ++k) {
        std::size_t src = order[k];
        double inv = 1.0 / sigma[src];
        for (std::size_t i = 0; i < m; ++i) Ud(i, k) = Ad(i, src) * inv;
    }
    std::size_t col = (r < m) ? r : m;
    for (std::size_t e = 0; e < m && col < m; ++e) {
        std::vector<double> v(m, 0.0);
        v[e] = 1.0;
        for (std::size_t k = 0; k < col; ++k) {
            double dot = 0.0;
            for (std::size_t i = 0; i < m; ++i) dot += Ud(i, k) * v[i];
            for (std::size_t i = 0; i < m; ++i) v[i] -= dot * Ud(i, k);
        }
        double norm2 = 0.0;
        for (std::size_t i = 0; i < m; ++i) norm2 += v[i] * v[i];
        double norm = std::sqrt(norm2);
        if (norm < 1e-10) continue;
        for (std::size_t i = 0; i < m; ++i) Ud(i, col) = v[i] / norm;
        ++col;
    }

    Matrix<AlgReal> Uout(m, m);
    for (std::size_t i = 0; i < m; ++i) {
        for (std::size_t j = 0; j < m; ++j) {
            Uout(i, j) = AlgReal::fromDouble(Ud(i, j));
        }
    }

    Matrix<AlgReal> Sigma(m, n);
    std::size_t minmn = (m < n) ? m : n;
    std::vector<AlgReal> sv;
    sv.reserve(n);
    for (std::size_t k = 0; k < n; ++k) {
        AlgReal sk = AlgReal::fromDouble(sigma[order[k]]);
        if (k < minmn) Sigma(k, k) = sk;
        sv.push_back(sk);
    }

    SVDResult res;
    res.U              = std::move(Uout);
    res.Sigma          = std::move(Sigma);
    res.V              = std::move(Vout);
    res.singularValues = std::move(sv);
    return res;
}

}  // anonymous namespace

SVDResult svdDecompose(const Matrix<Fraction>& A) {
    const std::size_t m = A.rows();
    const std::size_t n = A.cols();
    if (m == 0 || n == 0) {
        throw std::invalid_argument("svdDecompose: empty matrix");
    }

    // 精确路径在独立线程中执行, 限时 3 秒.
    // 超时 / 异常 / 卡死 均自动降级到 double Jacobi SVD.
    auto exactSvd = [](Matrix<Fraction> Ac) -> SVDResult {
        const std::size_t mm = Ac.rows();
        const std::size_t nn = Ac.cols();
        Matrix<Fraction> B = ataFraction_(Ac);
        OrthoDiagResult diag = orthogonalDiagonalize(B);

        // 特征值 / 特征向量 minPoly 次数 > 2 → 抛异常以触发降级
        for (std::size_t j = 0; j < nn; ++j) {
            if (diag.Lambda(j, j).minPoly().degree() > 2)
                throw std::runtime_error("high-degree");
        }
        for (std::size_t i = 0; i < diag.U.rows(); ++i)
            for (std::size_t j = 0; j < diag.U.cols(); ++j)
                if (diag.U(i, j).minPoly().degree() > 2)
                    throw std::runtime_error("high-degree");

        std::vector<std::pair<AlgReal, std::size_t>> eigs;
        eigs.reserve(nn);
        for (std::size_t j = 0; j < nn; ++j)
            eigs.emplace_back(diag.Lambda(j, j), j);
        std::sort(eigs.begin(), eigs.end(),
                  [](const std::pair<AlgReal, std::size_t>& a,
                     const std::pair<AlgReal, std::size_t>& b) {
                      return b.first < a.first;
                  });

        Matrix<AlgReal> V(nn, nn);
        for (std::size_t c = 0; c < nn; ++c) {
            std::size_t srcCol = eigs[c].second;
            for (std::size_t r = 0; r < nn; ++r) V(r, c) = diag.U(r, srcCol);
        }

        std::vector<AlgReal> sigma;
        sigma.reserve(nn);
        for (std::size_t i = 0; i < nn; ++i) {
            AlgReal lam = eigs[i].first;
            if (lam.sign() < 0) lam = AlgReal(Fraction(0));
            sigma.push_back(AlgReal::sqrt(lam));
        }

        Matrix<AlgReal> Aalg = toAlgReal(Ac);
        std::size_t r = 0;
        while (r < sigma.size() && !sigma[r].isZero()) ++r;

        Matrix<AlgReal> extended(mm, r + mm);
        for (std::size_t c = 0; c < r; ++c) {
            Matrix<AlgReal> vc(nn, 1);
            for (std::size_t i = 0; i < nn; ++i) vc(i, 0) = V(i, c);
            Matrix<AlgReal> u = Aalg * vc;
            AlgReal invSig = AlgReal(Fraction(1)) / sigma[c];
            for (std::size_t i = 0; i < mm; ++i) extended(i, c) = u(i, 0) * invSig;
        }
        for (std::size_t c = 0; c < mm; ++c)
            extended(c, r + c) = AlgReal(Fraction(1));
        Matrix<AlgReal> U = gramSchmidtOrthonormal(extended);
        if (U.cols() != mm) throw std::runtime_error("GS expansion failed");

        Matrix<AlgReal> Sigma(mm, nn);
        std::size_t minmn = (mm < nn) ? mm : nn;
        for (std::size_t i = 0; i < minmn; ++i) Sigma(i, i) = sigma[i];

        SVDResult res;
        res.U              = std::move(U);
        res.Sigma          = std::move(Sigma);
        res.V              = std::move(V);
        res.singularValues = std::move(sigma);
        return res;
    };

    // 在独立线程中执行精确路径, 通过 promise/future 传递结果
    std::promise<SVDResult> prom;
    auto fut = prom.get_future();
    std::thread worker([p = std::move(prom), Ac = A, exactSvd]() mutable {
        try {
            p.set_value(exactSvd(std::move(Ac)));
        } catch (...) {
            p.set_exception(std::current_exception());
        }
    });
    worker.detach();

    constexpr auto timeout = std::chrono::seconds(3);
    auto status = fut.wait_for(timeout);
    if (status == std::future_status::ready) {
        try {
            return fut.get();
        } catch (...) {
            return svdJacobiNumeric_(A);
        }
    }
    // 超时: 精确路径卡死, 直接返回数值结果
    return svdJacobiNumeric_(A);
}

}
