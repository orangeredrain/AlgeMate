#pragma once

#include "Fraction.h"
#include "Polynomial.h"

#include <iosfwd>
#include <string>

namespace algemate::math {

class AlgReal {
public:
    using Poly = Polynomial<Fraction>;

    AlgReal();                                   
    AlgReal(const Fraction& q);                  
    AlgReal(long long q);                        

    static AlgReal fromRational(const Fraction& q);
    static AlgReal fromDouble  (double x, long maxDenom = 1000000);

    static AlgReal sqrt   (const Fraction& q);   
    static AlgReal sqrt   (const AlgReal& a);    
    static AlgReal cbrt   (const Fraction& q);   
    static AlgReal cbrt   (const AlgReal& a);
    static AlgReal nthRoot(const Fraction& q, int n); 
    static AlgReal nthRoot(const AlgReal& a, int n);

    static std::vector<AlgReal> realRootsOf(const Poly& p,
        const Fraction& tol = Fraction(1, 1000));

    static AlgReal evaluatePoly(const Poly& f, const AlgReal& alpha);

    bool     isZero()     const;
    bool     isRational() const;
    Fraction asRational() const;                 
    int      sign()       const;                 

    double      toDouble(double eps = 1e-15) const;
    std::string toString() const;                
    std::string toLatex () const;                

    AlgReal operator-() const;
    AlgReal operator+(const AlgReal& r) const;
    AlgReal operator-(const AlgReal& r) const;
    AlgReal operator*(const AlgReal& r) const;
    AlgReal operator/(const AlgReal& r) const;   

    AlgReal& operator+=(const AlgReal& r) { *this = *this + r; return *this; }
    AlgReal& operator-=(const AlgReal& r) { *this = *this - r; return *this; }
    AlgReal& operator*=(const AlgReal& r) { *this = *this * r; return *this; }
    AlgReal& operator/=(const AlgReal& r) { *this = *this / r; return *this; }

    bool operator==(const AlgReal& r) const;
    bool operator!=(const AlgReal& r) const { return !(*this == r); }
    bool operator< (const AlgReal& r) const;
    bool operator<=(const AlgReal& r) const { return !(r < *this); }
    bool operator> (const AlgReal& r) const { return r < *this; }
    bool operator>=(const AlgReal& r) const { return !(*this < r); }

    const Poly&     minPoly()         const { return p_; }
    const Fraction& intervalLower()   const { return a_; }
    const Fraction& intervalUpper()   const { return b_; }

private:
    Poly             p_;   
    mutable Fraction a_;   
    mutable Fraction b_;   

    AlgReal(Poly p, Fraction a, Fraction b);     
    void normalize_();                           
    int  signAt_(const Fraction& x) const;       
    void refineTo_(const Fraction& tol) const;   
};

std::ostream& operator<<(std::ostream& os, const AlgReal& a);

}
