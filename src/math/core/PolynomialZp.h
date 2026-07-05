#pragma once

#include "core/Fraction.h"
#include "core/Polynomial.h"

#include <cstdint>
#include <iosfwd>
#include <vector>

namespace algemate::math {

class PolynomialZp;

class PolynomialZp {
public:
    using Z = int64_t;

    PolynomialZp();                                  
    PolynomialZp(Z prime);                           
    PolynomialZp(std::vector<Z> coeffsLowFirst, Z prime); 

    static PolynomialZp fromPoly(const Polynomial<Fraction>& f, Z prime); 
    Polynomial<Fraction> toPoly() const;                                  

    Z         prime()  const { return p_; }
    int       degree() const;                 
    bool      isZero() const { return coeffs_.empty(); }
    bool      isOne()  const;
    Z         leading() const;                
    Z         at(std::size_t k) const;        
    const std::vector<Z>& coeffs() const { return coeffs_; }

    PolynomialZp makeMonic() const;           
    PolynomialZp derivative() const;          

    static PolynomialZp add(const PolynomialZp& a, const PolynomialZp& b);
    static PolynomialZp sub(const PolynomialZp& a, const PolynomialZp& b);
    static PolynomialZp mul(const PolynomialZp& a, const PolynomialZp& b);

    struct DivMod;
    static DivMod divmod(const PolynomialZp& a, const PolynomialZp& b);

    static PolynomialZp gcd(PolynomialZp a, PolynomialZp b);

    static PolynomialZp powMod(const PolynomialZp& a, Z e, const PolynomialZp& m);

    struct SqfFactor;
    static std::vector<SqfFactor> squarefreeFactorization(const PolynomialZp& f);

    struct DDFPart;
    static std::vector<DDFPart> distinctDegreeFactorization(const PolynomialZp& f);

    static std::vector<PolynomialZp> equalDegreeFactorization(const PolynomialZp& g, int d);

    struct Factorization;
    static Factorization factor(const PolynomialZp& f);

    static bool isIrreducible(const PolynomialZp& f);

    static Z   modp(Z a, Z p);                 
    static Z   mulmod(Z a, Z b, Z p);          
    static Z   powmod(Z a, Z e, Z p);          
    static Z   invmod(Z a, Z p);               

    bool operator==(const PolynomialZp& o) const;
    bool operator!=(const PolynomialZp& o) const { return !(*this == o); }

private:
    std::vector<Z> coeffs_;   
    Z              p_ = 0;    

    void normalize_();        
    static void requireSamePrime_(const PolynomialZp& a, const PolynomialZp& b);
};

struct PolynomialZp::DivMod    { PolynomialZp q; PolynomialZp r; };
struct PolynomialZp::SqfFactor { PolynomialZp f; int e; };
struct PolynomialZp::DDFPart   { PolynomialZp g; int d; };
struct PolynomialZp::Factorization {
    PolynomialZp::Z                      lc;       
    std::vector<PolynomialZp::SqfFactor> factors;  
};

std::ostream& operator<<(std::ostream& os, const PolynomialZp& f);

} 
