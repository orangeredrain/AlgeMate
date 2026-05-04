#include "Orthogonalization.h"
#include "LinearAlgebra.h"

#include <stdexcept>
#include <vector>

namespace algemate::math {

// 辅助: 检查是否为列向量 (1 列) 且非空
static void ensureColumnVector_(const Matrix<Fraction>& v, const char* who) {
    if (v.cols() != 1 || v.rows() == 0) {
        throw std::invalid_argument(std::string(who) + ": expect non-empty column vector");
    }
}

Fraction dotProduct(const Matrix<Fraction>& u, const Matrix<Fraction>& v) {
    ensureColumnVector_(u, "dotProduct");
    ensureColumnVector_(v, "dotProduct");
    if (u.rows() != v.rows()) {
        throw std::invalid_argument("dotProduct: dimension mismatch");
    }
    Fraction s(0);
    for (std::size_t i = 0; i < u.rows(); ++i) {
        s = s + u(i, 0) * v(i, 0);
    }
    return s;
}

Fraction normSquared(const Matrix<Fraction>& v) {
    return dotProduct(v, v);
}

bool isOrthogonal(const Matrix<Fraction>& u, const Matrix<Fraction>& v) {
    return dotProduct(u, v) == Fraction(0);
}

bool isOrthogonalSet(const Matrix<Fraction>& V) {
    if (V.isEmpty()) return true;
    const std::size_t n = V.rows();
    const std::size_t k = V.cols();
    for (std::size_t i = 0; i < k; ++i) {
        for (std::size_t j = i + 1; j < k; ++j) {
            Fraction s(0);
            for (std::size_t r = 0; r < n; ++r) {
                s = s + V(r, i) * V(r, j);
            }
            if (!(s == Fraction(0))) return false;
        }
    }
    return true;
}

bool areLinearlyIndependent(const Matrix<Fraction>& V) {
    if (V.isEmpty()) return true;
    return rank(V) == V.cols();
}

Matrix<Fraction> gramSchmidt(const Matrix<Fraction>& V) {
    if (V.isEmpty()) return V;
    const std::size_t n = V.rows();
    const std::size_t k = V.cols();

    // 累积非零正交向量列表
    std::vector<Matrix<Fraction>> qs;
    std::vector<Fraction>         qNorm2;  // <q_i, q_i>
    qs.reserve(k);
    qNorm2.reserve(k);

    for (std::size_t i = 0; i < k; ++i) {
        // 取第 i 列
        Matrix<Fraction> v(n, 1);
        for (std::size_t r = 0; r < n; ++r) v(r, 0) = V(r, i);

        // q_i = v - sum_j (<v, q_j> / <q_j, q_j>) q_j
        Matrix<Fraction> q = v;
        for (std::size_t j = 0; j < qs.size(); ++j) {
            Fraction num(0);
            for (std::size_t r = 0; r < n; ++r) num = num + v(r, 0) * qs[j](r, 0);
            if (num == Fraction(0)) continue;
            Fraction coeff = num / qNorm2[j];
            for (std::size_t r = 0; r < n; ++r) {
                q(r, 0) = q(r, 0) - coeff * qs[j](r, 0);
            }
        }

        // 检查是否为零向量
        Fraction nn = normSquared(q);
        if (nn == Fraction(0)) continue;

        qs.push_back(q);
        qNorm2.push_back(nn);
    }

    if (qs.empty()) return Matrix<Fraction>(n, 0);
    Matrix<Fraction> Q(n, qs.size());
    for (std::size_t j = 0; j < qs.size(); ++j) {
        for (std::size_t r = 0; r < n; ++r) {
            Q(r, j) = qs[j](r, 0);
        }
    }
    return Q;
}

}
