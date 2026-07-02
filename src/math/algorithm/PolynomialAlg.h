#pragma once

#include "core/Fraction.h"
#include "core/Polynomial.h"

#include <cstddef>
#include <vector>

namespace algemate::math {

using Poly = Polynomial<Fraction>;

Poly squarefreePart(const Poly& f);

struct SquarefreeFactorization {
    Fraction          content;  
    std::vector<Poly> factors;  
};
SquarefreeFactorization squarefreeFactorization(const Poly& f);

Fraction resultant(const Poly& f, const Poly& g);

Fraction discriminant(const Poly& f);

std::vector<Fraction> rationalRoots(const Poly& f);

std::vector<Poly> sturmSequence(const Poly& f);

int sturmSignChanges(const std::vector<Poly>& seq, const Fraction& x);

int countRealRootsInInterval(const Poly& f, const Fraction& a, const Fraction& b);

Fraction cauchyBound(const Poly& f);

std::vector<std::pair<Fraction, Fraction>> isolateRealRoots(
    const Poly& f, const Fraction& tol = Fraction(1, 1000));

Poly sumPoly(const Poly& fAlpha, const Poly& gBeta);

Poly productPoly(const Poly& fAlpha, const Poly& gBeta);

Poly minPolyOfEval(const Poly& g, const Poly& f);

struct RationalFactorization {
    Fraction                                       leadingCoefficient; 
    std::vector<std::pair<Poly, int>>              factors;            
};
RationalFactorization factorOverQ(const Poly& f);

}
