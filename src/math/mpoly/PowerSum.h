
#pragma once

#include "MPolynomial.h"
#include "core/Fraction.h"

#include <cstddef>
#include <string>
#include <vector>

namespace algemate::math::mpoly {

using Fraction = algemate::math::Fraction;

inline MPolynomial powerSumToSym(int k, int n) {
    if (k < 1) return MPolynomial();
    if (n < 1) return MPolynomial();
    int m = std::min(k, n);  

    auto sigma = [m](int i) -> MPolynomial {
        std::vector<int> exps(m, 0);
        exps[i-1] = 1;
        return MPolynomial(Fraction(1), Monomial(std::move(exps)));
    };

    MPolynomial s0{Fraction(n)};

    std::vector<MPolynomial> s(k + 1);
    s[0] = s0;

    for (int i = 1; i <= k; ++i) {
        MPolynomial si;

        for (int j = 1; j <= i - 1; ++j) {
            if (j > m) break;  
            MPolynomial term = sigma(j) * s[i - j];
            if (j % 2 == 0) term = term * Fraction(-1);  
            si = si + term;
        }
        if (i <= m) {
            MPolynomial lastTerm = sigma(i) * Fraction(i);
            if (i % 2 == 0) lastTerm = lastTerm * Fraction(-1);  
            si = si + lastTerm;
        }
        s[i] = si;
    }
    return s[k];
}

inline std::string powerSumToSymString(int k, int n) {
    auto poly = powerSumToSym(k, n);
    int m = std::min(k, n);
    std::vector<std::string> names;
    for (int i = 1; i <= m; ++i)
        names.push_back("s_" + std::to_string(i));
    return poly.toString(names);
}

inline MPolynomial symToPowerSum(int k, int n) {
    if (k < 1) return MPolynomial();
    if (k > n) return MPolynomial();  

    int m = k;  
    auto powerSum = [m](int i) -> MPolynomial {
        std::vector<int> exps(m, 0);
        exps[i-1] = 1;
        return MPolynomial(Fraction(1), Monomial(std::move(exps)));
    };

    MPolynomial sigma0{Fraction(1)};
    std::vector<MPolynomial> sigma(k + 1);
    sigma[0] = sigma0;

    for (int i = 1; i <= k; ++i) {
        MPolynomial si;
        for (int j = 1; j <= i; ++j) {

            MPolynomial term = sigma[i - j] * powerSum(j);
            if (j % 2 == 0) term = term * Fraction(-1);  
            si = si + term;
        }
        si = si * Fraction(1, i);  
        sigma[i] = si;
    }
    return sigma[k];
}

inline std::string symToPowerSumString(int k, int n) {
    auto poly = symToPowerSum(k, n);
    std::vector<std::string> names;
    for (int i = 1; i <= k; ++i)
        names.push_back("s_{" + std::to_string(i) + "}");
    std::string result = "\\sigma_{" + std::to_string(k) + "} = " + poly.toString(names);
    return result;
}

} 
