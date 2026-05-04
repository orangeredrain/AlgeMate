// PowerSum.h — 幂和 s_k = Σ x_i^k 表示为初等对称多项式 σ_i 的多项式
#pragma once

#include "MPolynomial.h"
#include "core/Fraction.h"

#include <cstddef>
#include <string>
#include <vector>

namespace algemate::math::mpoly {

using Fraction = algemate::math::Fraction;

// s_k = Σ_{i=1}^n x_i^k 表示为 σ_1,...,σ_m (m = min(k,n)) 的多项式
// 用 Newton 恒等式迭代计算
inline MPolynomial powerSumToSym(int k, int n) {
    if (k < 1) return MPolynomial();
    if (n < 1) return MPolynomial();
    int m = std::min(k, n);  // 实际使用的 σ 个数

    // σ_i 用单项式表示: m 维向量, 第 (i-1) 位置为 1
    auto sigma = [m](int i) -> MPolynomial {
        std::vector<int> exps(m, 0);
        exps[i-1] = 1;
        return MPolynomial(Fraction(1), Monomial(std::move(exps)));
    };

    // s[0] = n (常数项: s_0 = x_1^0 + ... + x_n^0 = n)
    MPolynomial s0{Fraction(n)};

    // s[1..k] 逐步计算
    std::vector<MPolynomial> s(k + 1);
    s[0] = s0;

    for (int i = 1; i <= k; ++i) {
        MPolynomial si;
        // si = Σ_{j=1}^{i-1} (-1)^{j-1} σ_j · s_{i-j}  +  (-1)^{i-1} i · σ_i
        for (int j = 1; j <= i - 1; ++j) {
            if (j > m) break;  // σ_j = 0 for j > n
            MPolynomial term = sigma(j) * s[i - j];
            if (j % 2 == 0) term = term * Fraction(-1);  // (-1)^{j-1}: even j → negative
            si = si + term;
        }
        if (i <= m) {
            MPolynomial lastTerm = sigma(i) * Fraction(i);
            if (i % 2 == 0) lastTerm = lastTerm * Fraction(-1);  // (-1)^{i-1}: even i → negative
            si = si + lastTerm;
        }
        s[i] = si;
    }
    return s[k];
}

// 格式化输出: s_k = ...  用 σ_1, σ_2, ... 表示
inline std::string powerSumToSymString(int k, int n) {
    auto poly = powerSumToSym(k, n);
    int m = std::min(k, n);
    std::vector<std::string> names;
    for (int i = 1; i <= m; ++i)
        names.push_back("s_" + std::to_string(i));
    return poly.toString(names);
}

// σ_k = (1/k) Σ_{i=1}^{k} (-1)^{i-1} σ_{k-i} · s_i   (σ_0 = 1)
// 表示为 s_1,...,s_k 的多项式, k ≤ n
inline MPolynomial symToPowerSum(int k, int n) {
    if (k < 1) return MPolynomial();
    if (k > n) return MPolynomial();  // caller should check

    // s_i 用单项式表示: m 维向量, 第 (i-1) 位置为 1
    int m = k;  // 最多用到 s_k
    auto powerSum = [m](int i) -> MPolynomial {
        std::vector<int> exps(m, 0);
        exps[i-1] = 1;
        return MPolynomial(Fraction(1), Monomial(std::move(exps)));
    };

    // σ_0 = 1
    MPolynomial sigma0{Fraction(1)};
    std::vector<MPolynomial> sigma(k + 1);
    sigma[0] = sigma0;

    for (int i = 1; i <= k; ++i) {
        MPolynomial si;
        for (int j = 1; j <= i; ++j) {
            // (-1)^{j-1} · σ_{i-j} · s_j
            MPolynomial term = sigma[i - j] * powerSum(j);
            if (j % 2 == 0) term = term * Fraction(-1);  // (-1)^{j-1}: even j → negative
            si = si + term;
        }
        si = si * Fraction(1, i);  // multiply by 1/k
        sigma[i] = si;
    }
    return sigma[k];
}

// 格式化输出
inline std::string symToPowerSumString(int k, int n) {
    auto poly = symToPowerSum(k, n);
    std::vector<std::string> names;
    for (int i = 1; i <= k; ++i)
        names.push_back("s_{" + std::to_string(i) + "}");
    std::string result = "\\sigma_{" + std::to_string(k) + "} = " + poly.toString(names);
    return result;
}

} // namespace algemate::math::mpoly
