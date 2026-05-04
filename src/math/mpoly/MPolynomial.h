// MPolynomial.h — 多元多项式 (字典序, Fraction 系数)
#pragma once

#include "Monomial.h"
#include "core/Fraction.h"

#include <map>
#include <string>
#include <vector>

namespace algemate::math::mpoly {

using Fraction = algemate::math::Fraction;

class MPolynomial {
public:
    MPolynomial() = default;
    explicit MPolynomial(const Fraction& c);  // 常数
    MPolynomial(const Fraction& c, Monomial m);  // 单项式

    bool isZero() const { return terms_.empty(); }
    std::string toString(const std::vector<std::string>& varNames) const;

    // 运算
    MPolynomial operator+(const MPolynomial& rhs) const;
    MPolynomial operator-(const MPolynomial& rhs) const;
    MPolynomial operator*(const MPolynomial& rhs) const;
    MPolynomial operator*(const Fraction& c) const;

    // 首项 (字典序最大)
    const Monomial& leadingMonomial() const { return terms_.begin()->first; }
    const Fraction& leadingCoefficient() const { return terms_.begin()->second; }

    const std::map<Monomial, Fraction, std::less<>>& terms() const { return terms_; }

private:
    // map 的 key 是 Monomial, 用 operator< (字典序) 排序
    // 首项在 begin(), 对应字典序最大
    std::map<Monomial, Fraction, std::less<>> terms_;
};

inline MPolynomial::MPolynomial(const Fraction& c) {
    if (!c.isZero()) terms_.emplace(Monomial(), c);
}

inline MPolynomial::MPolynomial(const Fraction& c, Monomial m) {
    if (!c.isZero() && !m.isOne()) terms_.emplace(std::move(m), c);
    else if (!c.isZero()) terms_.emplace(Monomial(), c);
}

inline MPolynomial MPolynomial::operator+(const MPolynomial& rhs) const {
    MPolynomial r;
    auto it = terms_.begin(), jt = rhs.terms_.begin();
    while (it != terms_.end() || jt != rhs.terms_.end()) {
        if (jt == rhs.terms_.end() || (it != terms_.end() && it->first < jt->first)) {
            r.terms_.emplace_hint(r.terms_.end(), it->first, it->second); ++it;
        } else if (it == terms_.end() || jt->first < it->first) {
            r.terms_.emplace_hint(r.terms_.end(), jt->first, jt->second); ++jt;
        } else {
            Fraction s = it->second + jt->second;
            if (!s.isZero()) r.terms_.emplace_hint(r.terms_.end(), it->first, s);
            ++it; ++jt;
        }
    }
    return r;
}

inline MPolynomial MPolynomial::operator-(const MPolynomial& rhs) const {
    return *this + (rhs * Fraction(-1));
}

inline MPolynomial MPolynomial::operator*(const Fraction& c) const {
    if (c.isZero()) return MPolynomial();
    MPolynomial r;
    for (const auto& [m, coef] : terms_)
        r.terms_.emplace_hint(r.terms_.end(), m, coef * c);
    return r;
}

inline MPolynomial MPolynomial::operator*(const MPolynomial& rhs) const {
    MPolynomial r;
    for (const auto& [m1, c1] : terms_)
        for (const auto& [m2, c2] : rhs.terms_) {
            Monomial m = m1 * m2;
            Fraction c = c1 * c2;
            auto it = r.terms_.find(m);
            if (it != r.terms_.end()) {
                Fraction s = it->second + c;
                if (s.isZero()) r.terms_.erase(it);
                else it->second = s;
            } else {
                r.terms_.emplace(std::move(m), c);
            }
        }
    return r;
}

inline std::string MPolynomial::toString(const std::vector<std::string>& varNames) const {
    if (terms_.empty()) return "0";
    std::string out;
    bool first = true;
    for (auto it = terms_.begin(); it != terms_.end(); ++it) {
        const auto& [m, c] = *it;
        bool neg = c.sign() < 0;
        Fraction ac = c.abs();
        if (first) { if (neg) out += "-"; }
        else { out += neg ? " - " : " + "; }
        bool isConst = true;
        for (std::size_t i = 0; i < m.vars(); ++i) {
            if (m.exps_[i] > 0) isConst = false;
        }
        if (isConst) { out += ac.toLatex(); first = false; continue; }
        if (!ac.isOne()) out += ac.toLatex();
        for (std::size_t i = 0; i < m.vars(); ++i) {
            if (m.exps_[i] > 0) {
                out += varNames[i];
                if (m.exps_[i] > 1) out += "^{" + std::to_string(m.exps_[i]) + "}";
            }
        }
        first = false;
    }
    return out;
}

} // namespace algemate::math::mpoly
