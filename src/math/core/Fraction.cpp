#include "Fraction.h"

#include <ostream>
#include <stdexcept>

namespace algemate::math {

Fraction::Fraction(const BigInt& num)
    : num_(num), den_(1) {
}

Fraction::Fraction(const long long& num)
    : Fraction(BigInt(num)) {
}

Fraction::Fraction(const BigInt& num, const BigInt& den)
    : num_(num), den_(den) {
    if (den_.isZero())
        throw std::domain_error("Fraction: denominator cannot be zero");
    normalize();
}

Fraction::Fraction(const std::string& s) {
    *this = fromString(s);
}

Fraction Fraction::fromString(const std::string& s) {
    auto start = s.find_first_not_of(" \t\n\r");
    auto end = s.find_last_not_of(" \t\n\r");
    if (start == std::string::npos || end == std::string::npos)
        throw std::invalid_argument("Fraction: empty string");
    std::string trimmed = s.substr(start, end - start + 1);

    size_t slash = trimmed.find('/');
    if (slash == std::string::npos) {
        return Fraction(BigInt::fromString(trimmed));
    }
    std::string num_str = trimmed.substr(0, slash);
    std::string den_str = trimmed.substr(slash + 1);
    BigInt n = BigInt::fromString(num_str);
    BigInt d = BigInt::fromString(den_str);
    return Fraction(n, d);
}

void Fraction::reduce() {
    if (num_.isZero()) {
        den_ = BigInt(1);
        return;
    }
    BigInt g = BigInt::gcd(num_.abs(), den_);
    if (g.isZero()) return;
    num_ = num_ / g;
    den_ = den_ / g;
}

void Fraction::normalize() {
    if (den_.isNegative()) {
        num_ = -num_;
        den_ = -den_;
    }
    reduce();
}

Fraction Fraction::operator-() const {
    Fraction res = *this;
    res.num_ = -res.num_;
    return res;
}

Fraction Fraction::operator+(const Fraction& other) const {
    BigInt new_num = num_ * other.den_ + other.num_ * den_;
    BigInt new_den = den_ * other.den_;
    return Fraction(new_num, new_den);
}

Fraction Fraction::operator-(const Fraction& other) const {
    return *this + (-other);
}

Fraction Fraction::operator*(const Fraction& other) const {
    BigInt new_num = num_ * other.num_;
    BigInt new_den = den_ * other.den_;
    return Fraction(new_num, new_den);
}

Fraction Fraction::operator/(const Fraction& other) const {
    if (other.isZero())
        throw std::domain_error("Fraction: division by zero");
    BigInt new_num = num_ * other.den_;
    BigInt new_den = den_ * other.num_;
    return Fraction(new_num, new_den);
}

Fraction& Fraction::operator+=(const Fraction& other) { *this = *this + other; return *this; }
Fraction& Fraction::operator-=(const Fraction& other) { *this = *this - other; return *this; }
Fraction& Fraction::operator*=(const Fraction& other) { *this = *this * other; return *this; }
Fraction& Fraction::operator/=(const Fraction& other) { *this = *this / other; return *this; }

Fraction& Fraction::operator++() { *this = *this + Fraction(BigInt(1)); return *this; }
Fraction  Fraction::operator++(int) { Fraction old = *this; ++*this; return old; }
Fraction& Fraction::operator--() { *this = *this - Fraction(BigInt(1)); return *this; }
Fraction  Fraction::operator--(int) { Fraction old = *this; --*this; return old; }

bool Fraction::operator==(const Fraction& other) const { return num_ == other.num_ && den_ == other.den_; }
bool Fraction::operator!=(const Fraction& other) const { return !(*this == other); }
bool Fraction::operator<(const Fraction& other) const { return (num_ * other.den_) < (other.num_ * den_); }
bool Fraction::operator<=(const Fraction& other) const { return !(other < *this); }
bool Fraction::operator>(const Fraction& other) const { return other < *this; }
bool Fraction::operator>=(const Fraction& other) const { return !(*this < other); }

bool Fraction::isZero() const { return num_.isZero(); }
bool Fraction::isOne() const { return num_ == den_ && !num_.isZero(); }
bool Fraction::isInteger() const { return den_.isOne(); }
int  Fraction::sign() const { return num_.sign(); }

double Fraction::toDouble() const {
    return num_.toDouble() / den_.toDouble();
}

std::string Fraction::toString() const {
    if (den_.isOne()) return num_.toString();
    return num_.toString() + "/" + den_.toString();
}

std::string Fraction::toLatex() const {
    if (den_.isOne()) return num_.toString();
    return "\\frac{" + num_.toString() + "}{" + den_.toString() + "}";
}

Fraction Fraction::abs() const {
    Fraction res = *this;
    if (res.num_.isNegative()) res.num_ = -res.num_;
    return res;
}

std::ostream& operator<<(std::ostream& os, const Fraction& f) {
    os << f.toString();
    return os;
}

}
