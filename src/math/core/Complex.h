#pragma once

#include "AlgReal.h"
#include "Fraction.h"

#include <iosfwd>
#include <string>
#include <utility>

namespace algemate::math {

class Complex {
public:
    Complex();
    Complex(const AlgReal& re);
    Complex(const AlgReal& re, const AlgReal& im);
    Complex(const Fraction& re);
    Complex(long long re);

    static Complex i();                                        
    static Complex fromString(const std::string& s);
    static Complex fromDouble(double re, double im = 0.0);

    const AlgReal& real() const { return re_; }
    const AlgReal& imag() const { return im_; }
    bool           isReal() const { return im_.isZero(); }
    bool           isZero() const { return re_.isZero() && im_.isZero(); }

    Complex operator-() const;
    Complex operator+(const Complex& r) const;
    Complex operator-(const Complex& r) const;
    Complex operator*(const Complex& r) const;
    Complex operator/(const Complex& r) const;                 

    Complex& operator+=(const Complex& r) { *this = *this + r; return *this; }
    Complex& operator-=(const Complex& r) { *this = *this - r; return *this; }
    Complex& operator*=(const Complex& r) { *this = *this * r; return *this; }
    Complex& operator/=(const Complex& r) { *this = *this / r; return *this; }

    bool operator==(const Complex& r) const;
    bool operator!=(const Complex& r) const { return !(*this == r); }

    Complex conjugate()      const;
    AlgReal modulusSquared() const;
    AlgReal modulus()        const;

    static Complex sqrt   (const Complex& z);
    static Complex cbrt   (const Complex& z);
    static Complex nthRoot(const Complex& z, int n);

    std::string toString() const;
    std::string toLatex () const;
    std::pair<double, double> toDouble() const;

private:
    AlgReal re_, im_;
};

std::ostream& operator<<(std::ostream& os, const Complex& z);

}
