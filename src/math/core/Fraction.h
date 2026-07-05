#pragma once

#include "BigInt.h"

#include <iosfwd>
#include <string>

namespace algemate::math {

class Fraction {
public:
    Fraction() = default;
    Fraction(const BigInt& num);
    Fraction(const long long& num);
    Fraction(const BigInt& num, const BigInt& den);
    explicit Fraction(const std::string& s);

    static Fraction fromString(const std::string& s);

    const BigInt& numerator() const { return num_; }
    const BigInt& denominator() const { return den_; }

    Fraction operator-() const;
    Fraction operator+(const Fraction& other) const;
    Fraction operator-(const Fraction& other) const;
    Fraction operator*(const Fraction& other) const;
    Fraction operator/(const Fraction& other) const;

    Fraction& operator+=(const Fraction& other);
    Fraction& operator-=(const Fraction& other);
    Fraction& operator*=(const Fraction& other);
    Fraction& operator/=(const Fraction& other);

    Fraction& operator++();
    Fraction  operator++(int);
    Fraction& operator--();
    Fraction  operator--(int);

    bool operator==(const Fraction& other) const;
    bool operator!=(const Fraction& other) const;
    bool operator<(const Fraction& other) const;
    bool operator<=(const Fraction& other) const;
    bool operator>(const Fraction& other) const;
    bool operator>=(const Fraction& other) const;

    bool isZero() const;
    bool isOne() const;
    bool isInteger() const;
    int  sign() const;

    double      toDouble() const;
    std::string toString() const;
    std::string toLatex() const;

    Fraction abs() const;

private:
    BigInt num_;
    BigInt den_ = BigInt(1);

    void reduce();
    void normalize();
};

std::ostream& operator<<(std::ostream& os, const Fraction& f);

}
