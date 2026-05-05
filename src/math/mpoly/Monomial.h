// Monomial.h — 多元单项式 (字典序)
#pragma once

#include <cstddef>
#include <initializer_list>
#include <string>
#include <vector>

namespace algemate::math::mpoly {

class Monomial {
public:
    Monomial() = default;
    explicit Monomial(std::size_t n) : exps_(n, 0) {}
    explicit Monomial(std::vector<int> exps) : exps_(std::move(exps)) {}
    Monomial(std::initializer_list<int> exps) : exps_(exps) {}

    std::size_t vars() const { return exps_.size(); }
    int operator[](std::size_t i) const { return i < exps_.size() ? exps_[i] : 0; }

    // 字典序
    bool operator<(const Monomial& rhs) const;
    // 乘法
    Monomial operator*(const Monomial& rhs) const;
    // 比较
    bool operator==(const Monomial& rhs) const;
    bool isOne() const;  // 全零指数
    int totalDegree() const;

    std::vector<int> exps_;
};

inline bool Monomial::operator<(const Monomial& rhs) const {
    std::size_t n = std::max(exps_.size(), rhs.exps_.size());
    for (std::size_t i = 0; i < n; ++i) {
        int a = (*this)[i], b = rhs[i];
        if (a != b) return a > b;  // 字典序
    }
    return false;
}

inline Monomial Monomial::operator*(const Monomial& rhs) const {
    std::size_t n = std::max(exps_.size(), rhs.exps_.size());
    std::vector<int> e(n);
    for (std::size_t i = 0; i < n; ++i)
        e[i] = (*this)[i] + rhs[i];
    return Monomial(std::move(e));
}

inline bool Monomial::operator==(const Monomial& rhs) const {
    std::size_t n = std::max(exps_.size(), rhs.exps_.size());
    for (std::size_t i = 0; i < n; ++i)
        if ((*this)[i] != rhs[i]) return false;
    return true;
}

inline bool Monomial::isOne() const {
    for (int e : exps_) if (e != 0) return false;
    return exps_.empty();
}

inline int Monomial::totalDegree() const {
    int d = 0;
    for (int e : exps_) d += e;
    return d;
}

} // namespace algemate::math::mpoly
