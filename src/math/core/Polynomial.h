#pragma once

#include "Fraction.h"

#include <algorithm>
#include <cstddef>
#include <initializer_list>
#include <ostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace algemate::math {

template<typename T = Fraction>
class Polynomial {
public:
    using value_type = T;
    using size_type  = std::size_t;

    struct DivMod {
        Polynomial quotient;
        Polynomial remainder;
    };

    Polynomial() = default;                              
    Polynomial(const T& constant);                       
    Polynomial(std::initializer_list<T> coeffsLowFirst); 

    static Polynomial fromCoeffsHighFirst(std::initializer_list<T> hi); 
    static Polynomial monomial(size_type degree, const T& coef = T(1)); 
    static Polynomial x();                                              

    bool      isZero() const { return coeffs_.empty(); } 
    bool      isOne()  const { return coeffs_.size() == 1 && coeffs_[0] == T(1); } 
    int       degree() const;                           
    size_type size()   const { return coeffs_.size(); } 

    const T&  leading()  const;                 
    const T&  constant() const;                 
    const T&  operator[](size_type i) const;    

    const std::vector<T>& coeffs() const { return coeffs_; } 

    T         operator()(const T& x) const;        
    Polynomial derivative() const;                 
    Polynomial integrate(const T& C = T(0)) const; 

    Polynomial operator+() const { return *this; }
    Polynomial operator-() const;
    Polynomial operator+(const Polynomial& r) const;
    Polynomial operator-(const Polynomial& r) const;
    Polynomial operator*(const Polynomial& r) const;
    Polynomial operator*(const T& k) const;

    Polynomial& operator+=(const Polynomial& r) { *this = *this + r; return *this; }
    Polynomial& operator-=(const Polynomial& r) { *this = *this - r; return *this; }
    Polynomial& operator*=(const Polynomial& r) { *this = *this * r; return *this; }
    Polynomial& operator*=(const T& k)          { *this = *this * k; return *this; }

    DivMod     divmod(const Polynomial& divisor) const;  
    Polynomial operator/(const Polynomial& r) const { return divmod(r).quotient; }
    Polynomial operator%(const Polynomial& r) const { return divmod(r).remainder; }

    Polynomial pow(unsigned int n) const;       

    Polynomial reverse() const;                 
    Polynomial shift(const T& a) const;         
    Polynomial scale(const T& k) const;         
    Polynomial monic()  const;                  
    Polynomial negate() const { return -(*this); } 

    bool operator==(const Polynomial& r) const { return coeffs_ == r.coeffs_; }
    bool operator!=(const Polynomial& r) const { return !(*this == r); }

    std::string toString(const std::string& var = "x") const;

private:
    std::vector<T> coeffs_; 

    void trim_();           
    static const T& zero_();
};

template<typename T>
const T& Polynomial<T>::zero_() {
    static const T z = T(0);
    return z;
}

template<typename T>
void Polynomial<T>::trim_() {
    while (!coeffs_.empty() && coeffs_.back() == T(0)) coeffs_.pop_back();
}

template<typename T>
Polynomial<T>::Polynomial(const T& constant) {
    if (!(constant == T(0))) coeffs_.push_back(constant);
}

template<typename T>
Polynomial<T>::Polynomial(std::initializer_list<T> coeffsLowFirst)
    : coeffs_(coeffsLowFirst) {
    trim_();
}

template<typename T>
Polynomial<T> Polynomial<T>::fromCoeffsHighFirst(std::initializer_list<T> hi) {
    Polynomial p;
    p.coeffs_.assign(hi.begin(), hi.end());
    std::reverse(p.coeffs_.begin(), p.coeffs_.end());
    p.trim_();
    return p;
}

template<typename T>
Polynomial<T> Polynomial<T>::monomial(size_type degree, const T& coef) {
    Polynomial p;
    if (coef == T(0)) return p;
    p.coeffs_.assign(degree + 1, T(0));
    p.coeffs_[degree] = coef;
    return p;
}

template<typename T>
Polynomial<T> Polynomial<T>::x() { return monomial(1, T(1)); }

template<typename T>
int Polynomial<T>::degree() const {
    if (coeffs_.empty()) return -1;
    return static_cast<int>(coeffs_.size()) - 1;
}

template<typename T>
const T& Polynomial<T>::leading() const {
    if (coeffs_.empty()) throw std::domain_error("Polynomial::leading: zero polynomial");
    return coeffs_.back();
}

template<typename T>
const T& Polynomial<T>::constant() const {
    if (coeffs_.empty()) return zero_();
    return coeffs_[0];
}

template<typename T>
const T& Polynomial<T>::operator[](size_type i) const {
    if (i >= coeffs_.size()) return zero_();
    return coeffs_[i];
}

template<typename T>
T Polynomial<T>::operator()(const T& x) const {
    if (coeffs_.empty()) return T(0);
    T result = coeffs_.back();
    for (size_type i = coeffs_.size() - 1; i > 0; --i) {
        result = result * x + coeffs_[i - 1];
    }
    return result;
}

template<typename T>
Polynomial<T> Polynomial<T>::derivative() const {
    Polynomial out;
    if (coeffs_.size() <= 1) return out;
    out.coeffs_.resize(coeffs_.size() - 1);
    for (size_type i = 1; i < coeffs_.size(); ++i) {
        out.coeffs_[i - 1] = coeffs_[i] * T(static_cast<long long>(i));
    }
    out.trim_();
    return out;
}

template<typename T>
Polynomial<T> Polynomial<T>::integrate(const T& C) const {
    Polynomial out;
    out.coeffs_.push_back(C);
    for (size_type i = 0; i < coeffs_.size(); ++i) {
        out.coeffs_.push_back(coeffs_[i] / T(static_cast<long long>(i + 1)));
    }
    out.trim_();
    return out;
}

template<typename T>
Polynomial<T> Polynomial<T>::operator-() const {
    Polynomial out;
    out.coeffs_.resize(coeffs_.size());
    for (size_type i = 0; i < coeffs_.size(); ++i) out.coeffs_[i] = -coeffs_[i];
    return out;
}

template<typename T>
Polynomial<T> Polynomial<T>::operator+(const Polynomial& r) const {
    Polynomial out;
    out.coeffs_.resize(std::max(coeffs_.size(), r.coeffs_.size()));
    for (size_type i = 0; i < out.coeffs_.size(); ++i) {
        const T& a = i < coeffs_.size()   ? coeffs_[i]   : zero_();
        const T& b = i < r.coeffs_.size() ? r.coeffs_[i] : zero_();
        out.coeffs_[i] = a + b;
    }
    out.trim_();
    return out;
}

template<typename T>
Polynomial<T> Polynomial<T>::operator-(const Polynomial& r) const {
    Polynomial out;
    out.coeffs_.resize(std::max(coeffs_.size(), r.coeffs_.size()));
    for (size_type i = 0; i < out.coeffs_.size(); ++i) {
        const T& a = i < coeffs_.size()   ? coeffs_[i]   : zero_();
        const T& b = i < r.coeffs_.size() ? r.coeffs_[i] : zero_();
        out.coeffs_[i] = a - b;
    }
    out.trim_();
    return out;
}

template<typename T>
Polynomial<T> Polynomial<T>::operator*(const Polynomial& r) const {
    if (coeffs_.empty() || r.coeffs_.empty()) return Polynomial();
    Polynomial out;
    out.coeffs_.assign(coeffs_.size() + r.coeffs_.size() - 1, T(0));
    for (size_type i = 0; i < coeffs_.size(); ++i) {
        if (coeffs_[i] == T(0)) continue;
        for (size_type j = 0; j < r.coeffs_.size(); ++j) {
            out.coeffs_[i + j] = out.coeffs_[i + j] + coeffs_[i] * r.coeffs_[j];
        }
    }
    out.trim_();
    return out;
}

template<typename T>
Polynomial<T> Polynomial<T>::operator*(const T& k) const {
    if (k == T(0)) return Polynomial();
    Polynomial out;
    out.coeffs_.resize(coeffs_.size());
    for (size_type i = 0; i < coeffs_.size(); ++i) out.coeffs_[i] = coeffs_[i] * k;
    out.trim_();
    return out;
}

template<typename T>
typename Polynomial<T>::DivMod Polynomial<T>::divmod(const Polynomial& divisor) const {
    if (divisor.isZero()) throw std::domain_error("Polynomial::divmod: division by zero");
    DivMod result;
    result.remainder = *this;
    const int dDeg = divisor.degree();
    const T& dLead = divisor.leading();
    while (result.remainder.degree() >= dDeg) {
        const int shift = result.remainder.degree() - dDeg;
        const T coef = result.remainder.leading() / dLead;
        Polynomial term = monomial(static_cast<size_type>(shift), coef);
        result.quotient = result.quotient + term;
        result.remainder = result.remainder - term * divisor;
    }
    return result;
}

template<typename T>
Polynomial<T> Polynomial<T>::pow(unsigned int n) const {
    Polynomial result(T(1));
    Polynomial base = *this;
    while (n != 0) {
        if (n & 1u) result = result * base;
        n >>= 1u;
        if (n != 0) base = base * base;
    }
    return result;
}

template<typename T>
Polynomial<T> Polynomial<T>::reverse() const {
    Polynomial out;
    out.coeffs_.assign(coeffs_.rbegin(), coeffs_.rend());
    out.trim_();
    return out;
}

template<typename T>
Polynomial<T> Polynomial<T>::shift(const T& a) const {
    Polynomial out;
    for (size_type i = coeffs_.size(); i > 0; --i) {
        out = out * Polynomial({a, T(1)}) + Polynomial(coeffs_[i - 1]);
    }
    return out;
}

template<typename T>
Polynomial<T> Polynomial<T>::scale(const T& k) const {
    Polynomial out = *this;
    T pow = T(1);
    for (size_type i = 0; i < out.coeffs_.size(); ++i) {
        out.coeffs_[i] = out.coeffs_[i] * pow;
        pow = pow * k;
    }
    out.trim_();
    return out;
}

template<typename T>
Polynomial<T> Polynomial<T>::monic() const {
    if (isZero()) return *this;
    const T lc = leading();
    if (lc == T(1)) return *this;
    Polynomial out;
    out.coeffs_.resize(coeffs_.size());
    for (size_type i = 0; i < coeffs_.size(); ++i) out.coeffs_[i] = coeffs_[i] / lc;
    return out;
}

template<typename T>
std::string Polynomial<T>::toString(const std::string& var) const {
    if (isZero()) return "0";
    std::ostringstream oss;
    bool first = true;
    for (size_type i = coeffs_.size(); i > 0; --i) {
        const T& c = coeffs_[i - 1];
        if (c == T(0)) continue;
        const size_type exp = i - 1;

        std::ostringstream cs;
        cs << c;
        std::string cstr = cs.str();

        bool negative = !cstr.empty() && cstr[0] == '-';
        if (negative) cstr = cstr.substr(1);

        if (first) {
            oss << (negative ? "-" : "");
        } else {
            oss << (negative ? " - " : " + ");
        }
        first = false;

        const bool cIsOne = (c == T(1) || (negative && cstr == "1"));
        if (!(cIsOne && exp > 0)) oss << cstr;
        if (exp == 1) oss << var;
        else if (exp > 1) oss << var << "^" << exp;
    }
    return oss.str();
}

template<typename T>
Polynomial<T> operator*(const T& k, const Polynomial<T>& p) { return p * k; }

template<typename T>
std::ostream& operator<<(std::ostream& os, const Polynomial<T>& p) { return os << p.toString(); }

template<typename T>
Polynomial<T> gcd(Polynomial<T> a, Polynomial<T> b) {
    while (!b.isZero()) {
        Polynomial<T> r = a % b;
        a = b;
        b = r;
    }
    return a.isZero() ? a : a.monic();
}

}
