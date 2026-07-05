#include "algorithm/PolynomialAlg.h"

#include "core/BigInt.h"

#include <algorithm>
#include <cstdint>
#include <stdexcept>

namespace algemate::math {

namespace {

Fraction fracPow_(const Fraction& base, unsigned int n) {
    Fraction r(1);
    Fraction b = base;
    while (n) {
        if (n & 1u) r = r * b;
        n >>= 1;
        if (n) b = b * b;
    }
    return r;
}

Fraction resultantImpl_(Poly f, Poly g) {
    if (f.isZero() || g.isZero()) return Fraction(0);
    Fraction sign(1);
    Fraction factor(1);

    while (true) {
        int df = f.degree();
        int dg = g.degree();

        if (df < dg) {

            if (((df * dg) & 1) != 0) sign = -sign;
            std::swap(f, g);
            std::swap(df, dg);
        }

        if (dg == 0) {

            factor = factor * fracPow_(g.leading(), static_cast<unsigned int>(df));
            return sign * factor;
        }

        Poly r = f % g;
        if (r.isZero()) return Fraction(0);
        int dr = r.degree();

        if (((df * dg) & 1) != 0) sign = -sign;

        factor = factor * fracPow_(g.leading(), static_cast<unsigned int>(df - dr));

        f = g;
        g = r;
    }
}

std::vector<long long> divisors_(long long value) {
    std::vector<long long> result;
    if (value < 0) value = -value;
    if (value == 0) return result;
    for (long long d = 1; d * d <= value; ++d) {
        if (value % d == 0) {
            result.push_back(d);
            if (d != value / d) result.push_back(value / d);
        }
    }
    std::sort(result.begin(), result.end());
    return result;
}

Poly lagrangeInterp_(const std::vector<Fraction>& xs, const std::vector<Fraction>& ys) {
    const std::size_t n = xs.size();
    Poly result;  
    for (std::size_t i = 0; i < n; ++i) {
        Poly     Li(Fraction(1));
        Fraction denom(1);
        for (std::size_t j = 0; j < n; ++j) {
            if (j == i) continue;

            Li = Li * Poly({-xs[j], Fraction(1)});
            denom = denom * (xs[i] - xs[j]);
        }
        result = result + Li * (ys[i] / denom);
    }
    return result;
}

}  

Poly squarefreePart(const Poly& f) {
    if (f.isZero()) return f;
    if (f.degree() == 0) return Poly(Fraction(1));
    Poly d = f.derivative();
    Poly g = gcd(f, d);        
    Poly sp = f / g;
    if (!sp.isZero()) sp = sp.monic();
    return sp;
}

SquarefreeFactorization squarefreeFactorization(const Poly& f) {
    SquarefreeFactorization out;
    if (f.isZero()) {
        out.content = Fraction(0);
        return out;
    }
    out.content = f.leading();
    Poly fm = f.monic();
    if (fm.degree() == 0) return out;

    Poly fp = fm.derivative();
    Poly a0 = gcd(fm, fp);         
    Poly b  = fm / a0;             
    Poly c  = fp / a0;             
    Poly d  = c - b.derivative();  

    const int maxIter = fm.degree() + 1;
    for (int iter = 0; iter < maxIter && b.degree() > 0; ++iter) {
        Poly ai = gcd(b, d);                           
        out.factors.push_back(ai.isZero() ? Poly(Fraction(1)) : ai.monic());
        if (ai.degree() == 0) {
            d = d - b.derivative();
        } else {
            b = b / ai;
            if (b.degree() == 0 && b.isOne()) break;
            d = (d / ai) - b.derivative();
        }
    }
    return out;
}

Fraction resultant(const Poly& f, const Poly& g) {
    return resultantImpl_(f, g);
}

Fraction discriminant(const Poly& f) {
    if (f.degree() < 1) return Fraction(0);
    const int n = f.degree();
    Fraction res = resultantImpl_(f, f.derivative());

    if (((n * (n - 1) / 2) & 1) != 0) res = -res;
    res = res / f.leading();
    return res;
}

std::vector<Fraction> rationalRoots(const Poly& f) {
    std::vector<Fraction> out;
    if (f.isZero() || f.degree() == 0) return out;

    BigInt D(1);
    for (const Fraction& c : f.coeffs()) {
        BigInt den = c.denominator();
        BigInt g   = BigInt::gcd(D, den);
        D = (D / g) * den;
    }
    std::vector<BigInt> gInt;
    gInt.reserve(f.size());
    for (const Fraction& c : f.coeffs()) {
        BigInt num = c.numerator() * D;
        BigInt den = c.denominator();
        BigInt q   = num / den;
        gInt.push_back(q);
    }

    BigInt cont(0);
    for (const BigInt& v : gInt) cont = BigInt::gcd(cont, v.abs());
    if (cont.sign() == 0) return out;  
    if (cont > BigInt(1)) {
        for (BigInt& v : gInt) v = v / cont;
    }

    bool zeroIsRoot = false;
    std::size_t firstNonZero = 0;
    while (firstNonZero < gInt.size() && gInt[firstNonZero].sign() == 0) ++firstNonZero;
    if (firstNonZero > 0) {
        zeroIsRoot = true;
        gInt.erase(gInt.begin(), gInt.begin() + static_cast<std::ptrdiff_t>(firstNonZero));
    }
    if (gInt.size() < 2) {
        if (zeroIsRoot) out.push_back(Fraction(0));
        return out;
    }

    BigInt c0 = gInt.front().abs();
    BigInt cn = gInt.back().abs();

    BigInt maxLL(static_cast<long long>(INT64_MAX));
    if (c0 > maxLL || cn > maxLL) {
        if (zeroIsRoot) out.push_back(Fraction(0));
        return out;
    }
    long long c0ll = c0.toLongLong();
    long long cnll = cn.toLongLong();

    std::vector<long long> ps = divisors_(c0ll);
    std::vector<long long> qs = divisors_(cnll);

    std::vector<Fraction> candidates;
    for (long long p : ps) {
        for (long long q : qs) {

            long long a = p, b = q;
            while (b != 0) { long long t = a % b; a = b; b = t; }
            if (a != 1) continue;
            Fraction pos{BigInt(p), BigInt(q)};
            Fraction neg{BigInt(-p), BigInt(q)};
            candidates.push_back(pos);
            candidates.push_back(neg);
        }
    }

    std::vector<Fraction> roots;
    if (zeroIsRoot) roots.push_back(Fraction(0));
    for (const Fraction& r : candidates) {
        if (f(r).sign() == 0) roots.push_back(r);
    }

    std::sort(roots.begin(), roots.end());
    roots.erase(std::unique(roots.begin(), roots.end()), roots.end());
    return roots;
}

std::vector<Poly> sturmSequence(const Poly& f) {
    std::vector<Poly> seq;
    if (f.isZero()) return seq;
    seq.push_back(f);
    Poly d = f.derivative();
    if (d.isZero()) return seq;
    seq.push_back(d);
    while (true) {
        const Poly& a = seq[seq.size() - 2];
        const Poly& b = seq[seq.size() - 1];
        Poly r = a % b;
        if (r.isZero()) break;
        seq.push_back(-r);
    }
    return seq;
}

int sturmSignChanges(const std::vector<Poly>& seq, const Fraction& x) {
    int changes  = 0;
    int prevSign = 0;
    for (const Poly& p : seq) {
        Fraction v = p(x);
        int      s = v.sign();
        if (s == 0) continue;
        if (prevSign != 0 && prevSign != s) ++changes;
        prevSign = s;
    }
    return changes;
}

int countRealRootsInInterval(const Poly& f, const Fraction& a, const Fraction& b) {
    if (f.isZero()) throw std::domain_error("countRealRootsInInterval: zero polynomial");
    if (!(a < b)) return 0;
    Poly sp = squarefreePart(f);
    if (sp.degree() <= 0) return 0;
    std::vector<Poly> seq = sturmSequence(sp);
    int va = sturmSignChanges(seq, a);
    int vb = sturmSignChanges(seq, b);
    return va - vb;
}

Fraction cauchyBound(const Poly& f) {
    if (f.isZero() || f.degree() <= 0) return Fraction(1);
    const auto& c = f.coeffs();
    Fraction lead = c.back().abs();
    if (lead.isZero()) return Fraction(1);
    Fraction m(0);
    for (std::size_t i = 0; i + 1 < c.size(); ++i) {
        Fraction v = c[i].abs();
        if (v > m) m = v;
    }
    return Fraction(1) + m / lead;
}

namespace {

void isolateImpl_(const Poly& sp, const std::vector<Poly>& seq,
                  const Fraction& A, const Fraction& B,
                  const Fraction& tol,
                  std::vector<std::pair<Fraction, Fraction>>& out,
                  int depth) {
    if (depth > 80) return;
    if (!(A < B)) return;
    int vA = sturmSignChanges(seq, A);
    int vB = sturmSignChanges(seq, B);
    int cnt = vA - vB;    
    if (cnt == 0) return;
    int sa = sp(A).sign();
    int sb = sp(B).sign();
    if (cnt == 1 && sa != 0 && sb != 0 && sa != sb) {

        Fraction a = A, b = B;
        int lsa = sa;
        for (int it = 0; it < 400; ++it) {
            if ((b - a) < tol) break;
            Fraction mm = (a + b) / Fraction(2);
            int sm = sp(mm).sign();
            if (sm == 0) { a = mm; b = mm; break; }
            if (sm == lsa) a = mm; else b = mm;
        }
        out.emplace_back(a, b);
        return;
    }
    Fraction m = (A + B) / Fraction(2);
    if (sp(m).sign() == 0) {

        Fraction eps = (B - A) / Fraction(1024);
        if (eps.isZero()) { out.emplace_back(m, m); return; }
        out.emplace_back(m, m);
        isolateImpl_(sp, seq, A, m - eps, tol, out, depth + 1);
        isolateImpl_(sp, seq, m + eps, B, tol, out, depth + 1);
        return;
    }
    isolateImpl_(sp, seq, A, m, tol, out, depth + 1);
    isolateImpl_(sp, seq, m, B, tol, out, depth + 1);
}

}  

std::vector<std::pair<Fraction, Fraction>> isolateRealRoots(const Poly& f, const Fraction& tol) {
    std::vector<std::pair<Fraction, Fraction>> out;
    if (f.isZero() || f.degree() <= 0) return out;
    Poly sp = squarefreePart(f);
    if (sp.degree() <= 0) return out;

    Fraction R = cauchyBound(sp) + Fraction(1);
    Fraction A = Fraction(0) - R;
    Fraction B = R;
    std::vector<Poly> seq = sturmSequence(sp);

    isolateImpl_(sp, seq, A, B, tol, out, 0);
    std::sort(out.begin(), out.end(),
              [](const std::pair<Fraction, Fraction>& x,
                 const std::pair<Fraction, Fraction>& y) { return x.first < y.first; });
    return out;
}

Poly sumPoly(const Poly& fAlpha, const Poly& gBeta) {
    if (fAlpha.isZero() || gBeta.isZero()) throw std::domain_error("sumPoly: zero polynomial");
    const int m = fAlpha.degree();
    const int n = gBeta.degree();
    const int N = m * n;
    if (N <= 0) return Poly(Fraction(1));  

    std::vector<Fraction> xs;
    std::vector<Fraction> ys;
    xs.reserve(static_cast<std::size_t>(N + 1));
    ys.reserve(static_cast<std::size_t>(N + 1));

    for (int k = 0; k <= N; ++k) {
        Fraction xi(k);

        Poly gi = gBeta.shift(xi).scale(Fraction(-1));
        Fraction v = resultantImpl_(fAlpha, gi);
        xs.push_back(xi);
        ys.push_back(v);
    }
    return lagrangeInterp_(xs, ys);
}

static Poly gOfXiOverY_timesYn_(const Poly& g, const Fraction& xi) {

    const int n = g.degree();
    Poly out;
    for (int k = 0; k <= n; ++k) {
        const Fraction& ck = g[static_cast<std::size_t>(k)];
        if (ck.sign() == 0) continue;
        Fraction coef = ck * fracPow_(xi, static_cast<unsigned int>(k));
        out = out + Poly::monomial(static_cast<std::size_t>(n - k), coef);
    }
    return out;
}

Poly productPoly(const Poly& fAlpha, const Poly& gBeta) {
    if (fAlpha.isZero() || gBeta.isZero()) throw std::domain_error("productPoly: zero polynomial");
    const int m = fAlpha.degree();
    const int n = gBeta.degree();
    const int N = m * n;
    if (N <= 0) return Poly(Fraction(1));

    std::vector<Fraction> xs;
    std::vector<Fraction> ys;
    xs.reserve(static_cast<std::size_t>(N + 1));
    ys.reserve(static_cast<std::size_t>(N + 1));

    for (int k = 1; k <= N + 1; ++k) {
        Fraction xi(k);
        Poly     gi = gOfXiOverY_timesYn_(gBeta, xi);
        Fraction v  = resultantImpl_(fAlpha, gi);
        xs.push_back(xi);
        ys.push_back(v);
    }
    return lagrangeInterp_(xs, ys);
}

Poly minPolyOfEval(const Poly& g, const Poly& f) {
    if (g.isZero() || g.degree() == 0) {
        throw std::domain_error("minPolyOfEval: g must have degree >= 1");
    }
    const int d = g.degree();

    if (f.isZero()) return Poly::x();

    if (f.degree() == 0) {
        return Poly::x() - Poly(f[0]);
    }
    std::vector<Fraction> xs;
    std::vector<Fraction> ys;
    xs.reserve(static_cast<std::size_t>(d + 1));
    ys.reserve(static_cast<std::size_t>(d + 1));
    for (int k = 0; k <= d; ++k) {
        Fraction yi(k);

        Poly r = Poly(yi) - f;
        Fraction v = resultantImpl_(g, r);
        xs.push_back(yi);
        ys.push_back(v);
    }
    return lagrangeInterp_(xs, ys);
}

}
