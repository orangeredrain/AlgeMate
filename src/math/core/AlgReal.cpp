#include "AlgReal.h"

#include "algorithm/PolynomialAlg.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <ostream>
#include <sstream>
#include <stdexcept>

namespace algemate::math {

namespace {

using Poly = Polynomial<Fraction>;

Fraction abs_(const Fraction& x) { return x.abs(); }

Fraction half_(const Fraction& a, const Fraction& b) {
    return (a + b) / Fraction(2);
}

Fraction doubleToFraction_(double x, long maxDenom) {
    if (!std::isfinite(x)) throw std::domain_error("fromDouble: x is not finite");
    if (maxDenom <= 0) maxDenom = 1;
    bool neg = x < 0;
    if (neg) x = -x;

    long long h1 = 1, h0 = 0;
    long long k1 = 0, k0 = 1;
    double y = x;
    const int maxIter = 64;
    long long lastH = 0, lastK = 1;
    for (int i = 0; i < maxIter; ++i) {
        long long a = static_cast<long long>(std::floor(y));
        long long nh = a * h1 + h0;
        long long nk = a * k1 + k0;
        if (nk > maxDenom || nk <= 0) break;
        lastH = nh; lastK = nk;
        h0 = h1; h1 = nh;
        k0 = k1; k1 = nk;
        double frac = y - static_cast<double>(a);
        if (frac < 1e-18) break;
        y = 1.0 / frac;
    }
    if (lastK == 0) { lastH = static_cast<long long>(std::llround(x)); lastK = 1; }
    if (neg) lastH = -lastH;
    return Fraction(BigInt(lastH), BigInt(lastK));
}

Poly xnMinusQ_(int n, const Fraction& q) {
    Poly p = Poly::monomial(static_cast<std::size_t>(n), Fraction(1));
    return p - Poly(q);
}

Poly composeNegX_(const Poly& p) {
    Poly out;
    const auto& c = p.coeffs();
    Fraction sign(1);
    for (std::size_t i = 0; i < c.size(); ++i) {
        if (!c[i].isZero()) out = out + Poly::monomial(i, sign * c[i]);
        sign = sign * Fraction(-1);
    }
    return out;
}

Poly composeXpowN_(const Poly& p, int n) {
    if (n <= 0) throw std::invalid_argument("composeXpowN_: n must be positive");
    Poly out;
    const auto& c = p.coeffs();
    for (std::size_t i = 0; i < c.size(); ++i) {
        if (!c[i].isZero()) out = out + Poly::monomial(i * static_cast<std::size_t>(n), c[i]);
    }
    return out;
}

Poly reciprocalPoly_(const Poly& p) {
    return p.reverse();
}

int polySignAt_(const Poly& p, const Fraction& x) {
    return p(x).sign();
}

int countRootsInInterval_(const Poly& p, const Fraction& a, const Fraction& b) {
    return countRealRootsInInterval(p, a, b);
}

void refineInterval_(const Poly& p, Fraction& a, Fraction& b, const Fraction& tol) {
    int sa = polySignAt_(p, a);
    int sb = polySignAt_(p, b);
    if (sa == 0) { b = a; return; }
    if (sb == 0) { a = b; return; }
    if (sa == sb) throw std::runtime_error("refineInterval_: endpoint signs equal");

    const int maxIter = 400;
    for (int it = 0; it < maxIter; ++it) {
        if ((b - a) < tol) return;
        Fraction m = half_(a, b);
        int sm = polySignAt_(p, m);
        if (sm == 0) { a = m; b = m; return; }
        if (sm == sa) a = m; else b = m;
    }
}

bool perfectNthRoot_(const Fraction& q, int n, Fraction& out) {
    if (q.isZero()) { out = Fraction(0); return true; }
    int sgn = q.sign();
    if (sgn < 0 && n % 2 == 0) return false;
    Fraction aq = q.abs();
    const BigInt& num = aq.numerator();
    const BigInt& den = aq.denominator();

    auto perfectPow = [&](const BigInt& v, BigInt& root) -> bool {
        double d = v.toDouble();
        if (!std::isfinite(d) || d < 0) return false;
        double r = std::pow(d, 1.0 / n);
        long long k = static_cast<long long>(std::llround(r));
        for (int delta = -2; delta <= 2; ++delta) {
            long long kk = k + delta;
            if (kk < 0) continue;
            BigInt b(kk);
            BigInt pw(1);
            for (int i = 0; i < n; ++i) pw = pw * b;
            if (pw == v) { root = b; return true; }
        }
        return false;
    };
    BigInt rn, rd;
    if (!perfectPow(num, rn)) return false;
    if (!perfectPow(den, rd)) return false;
    Fraction f(rn, rd);
    out = (sgn < 0) ? -f : f;
    return true;
}

void isolateRoots_(const Poly& p, const Fraction& A, const Fraction& B,
                   const Fraction& tol, std::vector<std::pair<Fraction, Fraction>>& out,
                   int depth = 0) {
    if (depth > 60) return;
    int cnt = countRootsInInterval_(p, A, B);
    if (cnt == 0) return;
    int sa = polySignAt_(p, A);
    int sb = polySignAt_(p, B);
    if (cnt == 1 && sa != sb) {
        Fraction a = A, b = B;
        if (sa == 0) a = A;
        refineInterval_(p, a, b, tol);
        out.emplace_back(a, b);
        return;
    }
    Fraction m = half_(A, B);
    isolateRoots_(p, A, m, tol, out, depth + 1);
    isolateRoots_(p, m, B, tol, out, depth + 1);
}

bool pickIntervalContaining_(const Poly& p, const Fraction& A, const Fraction& B,
                             double approx, Fraction& outA, Fraction& outB) {

    std::vector<Poly> seq = sturmSequence(p);
    if (!seq.empty()) {
        auto countIn = [&](const Fraction& a, const Fraction& b) -> int {
            return sturmSignChanges(seq, a) - sturmSignChanges(seq, b);
        };

        for (long long denom = 1; denom <= 1000000000LL; denom *= 4) {
            double scaled = approx * static_cast<double>(denom);
            if (!std::isfinite(scaled)) break;
            long long numLow  = static_cast<long long>(std::floor(scaled)) - 1;
            long long numHigh = static_cast<long long>(std::ceil(scaled))  + 1;
            Fraction lo{BigInt(numLow), BigInt(denom)};
            Fraction hi{BigInt(numHigh), BigInt(denom)};
            if (lo < A) lo = A;
            if (hi > B) hi = B;
            if (!(lo < hi)) continue;
            int cnt = countIn(lo, hi);
            if (cnt == 1) {
                int sLo = polySignAt_(p, lo);
                int sHi = polySignAt_(p, hi);
                if (sLo == 0) { outA = lo; outB = lo; return true; }
                if (sHi == 0) { outA = hi; outB = hi; return true; }
                if (sLo != sHi) {
                    refineInterval_(p, lo, hi, Fraction(BigInt(1), BigInt(10000)));
                    outA = lo; outB = hi;
                    return true;
                }
            }
        }
    }

    Fraction tol(BigInt(1), BigInt(1000));
    for (int round = 0; round < 20; ++round) {
        std::vector<std::pair<Fraction, Fraction>> iv;
        isolateRoots_(p, A, B, tol, iv);
        int hits = 0;
        for (const auto& [la, lb] : iv) {
            double da = la.toDouble();
            double db = lb.toDouble();
            if (approx >= da - 1e-9 && approx <= db + 1e-9) {
                outA = la; outB = lb; ++hits;
            }
        }
        if (hits == 1) return true;
        tol = tol / Fraction(10);
    }
    return false;
}

} 

AlgReal::AlgReal()
    : p_(Poly::x()), a_(Fraction(0)), b_(Fraction(0)) {
}

AlgReal::AlgReal(const Fraction& q)
    : p_(Poly::x() - Poly(q)), a_(q), b_(q) {
}

AlgReal::AlgReal(long long q)
    : AlgReal(Fraction(q)) {
}

AlgReal::AlgReal(Poly p, Fraction a, Fraction b)
    : p_(std::move(p)), a_(std::move(a)), b_(std::move(b)) {
    normalize_();
}

AlgReal AlgReal::fromRational(const Fraction& q) {
    return AlgReal(q);
}

AlgReal AlgReal::fromDouble(double x, long maxDenom) {
    return AlgReal(doubleToFraction_(x, maxDenom));
}

void AlgReal::normalize_() {
    if (p_.isZero()) throw std::domain_error("AlgReal: zero polynomial");
    p_ = squarefreePart(p_).monic();
    if (p_.degree() <= 0) throw std::domain_error("AlgReal: degenerate polynomial");

    if (p_.degree() == 1) {
        Fraction q = -p_[0] / p_[1];
        a_ = q; b_ = q;
        return;
    }

    if (a_ > b_) std::swap(a_, b_);

    int sa = signAt_(a_);
    int sb = signAt_(b_);
    if (sa == 0) {

        p_ = Poly::x() - Poly(a_);
        b_ = a_;
        return;
    }
    if (sb == 0) {
        p_ = Poly::x() - Poly(b_);
        a_ = b_;
        return;
    }
    if (sa == sb) {
        throw std::runtime_error("AlgReal::normalize_: endpoint signs agree (non-isolating interval)");
    }
    int cnt = countRootsInInterval_(p_, a_, b_);
    if (cnt != 1) {
        throw std::runtime_error("AlgReal::normalize_: interval does not isolate exactly one root");
    }

    auto rats = rationalRoots(p_);
    for (const Fraction& q : rats) {
        if (!(q < a_) && !(b_ < q)) {
            p_ = Poly::x() - Poly(q);
            a_ = q; b_ = q;
            return;
        }
    }
}

int AlgReal::signAt_(const Fraction& x) const {
    return polySignAt_(p_, x);
}

void AlgReal::refineTo_(const Fraction& tol) const {
    if (p_.degree() == 1) return;
    refineInterval_(p_, a_, b_, tol);
}

bool AlgReal::isRational() const {
    return p_.degree() == 1;
}

Fraction AlgReal::asRational() const {
    if (!isRational()) throw std::domain_error("AlgReal::asRational: not rational");
    return -p_[0] / p_[1];
}

bool AlgReal::isZero() const {
    if (isRational()) return asRational().isZero();
    if (a_.sign() > 0 || b_.sign() < 0) return false;
    if (p_.constant().isZero() && a_.sign() <= 0 && b_.sign() >= 0) return true;
    return false;
}

int AlgReal::sign() const {
    if (isRational()) return asRational().sign();
    if (a_.sign() >= 0) return +1;
    if (b_.sign() <= 0) return -1;
    if (p_.constant().isZero()) return 0;
    Fraction tol(BigInt(1), BigInt(1000000));
    const int maxRounds = 200;
    for (int r = 0; r < maxRounds; ++r) {
        refineTo_(tol);
        if (a_.sign() >= 0) return +1;
        if (b_.sign() <= 0) return -1;
        tol = tol / Fraction(8);
    }
    throw std::runtime_error("AlgReal::sign: failed to refine");
}

double AlgReal::toDouble(double eps) const {
    if (isRational()) return asRational().toDouble();
    Fraction tol(BigInt(1), BigInt(1000000));
    const int maxRounds = 100;
    for (int r = 0; r < maxRounds; ++r) {
        refineTo_(tol);
        double da = a_.toDouble();
        double db = b_.toDouble();
        if (std::abs(db - da) < eps) return 0.5 * (da + db);
        tol = tol / Fraction(16);
    }
    return 0.5 * (a_.toDouble() + b_.toDouble());
}

AlgReal AlgReal::operator-() const {
    if (isRational()) return AlgReal(-asRational());
    Poly np = composeNegX_(p_);
    return AlgReal(std::move(np), -b_, -a_);
}

AlgReal AlgReal::operator+(const AlgReal& r) const {
    if (isRational() && r.isRational()) return AlgReal(asRational() + r.asRational());

    if (isRational()) {
        Fraction q = asRational();
        Poly np = r.p_.shift(-q);
        return AlgReal(np, r.a_ + q, r.b_ + q);
    }
    if (r.isRational()) return r + *this;

    Poly h = squarefreePart(sumPoly(p_, r.p_));
    double approx = toDouble() + r.toDouble();

    Fraction tol(BigInt(1), BigInt(1000));
    refineTo_(tol);
    r.refineTo_(tol);
    Fraction A = a_ + r.a_;
    Fraction B = b_ + r.b_;

    Fraction outA, outB;
    for (int round = 0; round < 40; ++round) {
        if (pickIntervalContaining_(h, A, B, approx, outA, outB)) {
            return AlgReal(h, outA, outB);
        }
        tol = tol / Fraction(4);
        refineTo_(tol);
        r.refineTo_(tol);
        A = a_ + r.a_;
        B = b_ + r.b_;
    }
    throw std::runtime_error("AlgReal::operator+: failed to isolate sum root");
}

AlgReal AlgReal::operator-(const AlgReal& r) const {
    return *this + (-r);
}

AlgReal AlgReal::operator*(const AlgReal& r) const {
    if (isRational() && r.isRational()) return AlgReal(asRational() * r.asRational());
    if (isZero() || r.isZero()) return AlgReal(Fraction(0));

    if (isRational()) {
        Fraction q = asRational();
        Poly np = r.p_.scale(Fraction(1) / q);
        Fraction A = (q.sign() > 0) ? r.a_ * q : r.b_ * q;
        Fraction B = (q.sign() > 0) ? r.b_ * q : r.a_ * q;
        return AlgReal(np, A, B);
    }
    if (r.isRational()) return r * *this;

    Poly h = squarefreePart(productPoly(p_, r.p_));
    double approx = toDouble() * r.toDouble();

    Fraction tol(BigInt(1), BigInt(1000));
    auto ivProd = [&](Fraction& A, Fraction& B) {
        Fraction c1 = a_ * r.a_, c2 = a_ * r.b_, c3 = b_ * r.a_, c4 = b_ * r.b_;
        A = std::min({c1, c2, c3, c4});
        B = std::max({c1, c2, c3, c4});
    };
    refineTo_(tol);
    r.refineTo_(tol);
    Fraction A, B;
    ivProd(A, B);

    Fraction outA, outB;
    for (int round = 0; round < 40; ++round) {
        if (pickIntervalContaining_(h, A, B, approx, outA, outB)) {
            return AlgReal(h, outA, outB);
        }
        tol = tol / Fraction(4);
        refineTo_(tol);
        r.refineTo_(tol);
        ivProd(A, B);
    }
    throw std::runtime_error("AlgReal::operator*: failed to isolate product root");
}

AlgReal AlgReal::operator/(const AlgReal& r) const {
    if (r.isZero()) throw std::domain_error("AlgReal: division by zero");
    if (r.isRational()) return *this * AlgReal(Fraction(1) / r.asRational());

    (void)r.sign();   
    Poly np = r.p_.reverse();
    Fraction A = Fraction(1) / r.b_;
    Fraction B = Fraction(1) / r.a_;
    if (A > B) std::swap(A, B);
    AlgReal inv(np, A, B);
    return (*this) * inv;
}

bool AlgReal::operator==(const AlgReal& r) const {
    if (isRational() && r.isRational()) return asRational() == r.asRational();
    AlgReal d = *this - r;
    return d.isZero();
}

bool AlgReal::operator<(const AlgReal& r) const {
    if (isRational() && r.isRational()) return asRational() < r.asRational();
    AlgReal d = *this - r;
    return d.sign() < 0;
}

AlgReal AlgReal::nthRoot(const Fraction& q, int n) {
    if (n <= 0) throw std::invalid_argument("nthRoot: n must be positive");
    if (n % 2 == 0 && q.sign() < 0)
        throw std::domain_error("nthRoot: even root of negative");
    if (q.isZero()) return AlgReal(Fraction(0));
    if (n == 1) return AlgReal(q);

    Fraction exact;
    if (perfectNthRoot_(q, n, exact)) return AlgReal(exact);

    Poly p = xnMinusQ_(n, q);
    int  signQ = q.sign();
    double absQ = std::abs(q.toDouble());
    double approxVal = std::pow(absQ, 1.0 / n);
    if (signQ < 0) approxVal = -approxVal;

    long long lo, hi;
    if (approxVal >= 0) {
        lo = 0;
        hi = static_cast<long long>(std::ceil(approxVal)) + 1;
    } else {
        lo = static_cast<long long>(std::floor(approxVal)) - 1;
        hi = 0;
    }
    Fraction A(lo), B(hi);

    Fraction outA, outB;
    for (int round = 0; round < 8; ++round) {
        if (pickIntervalContaining_(p, A, B, approxVal, outA, outB)) {
            return AlgReal(p, outA, outB);
        }
        A = A - Fraction(10);
        B = B + Fraction(10);
    }
    throw std::runtime_error("nthRoot(Fraction, n): failed to isolate");
}

AlgReal AlgReal::nthRoot(const AlgReal& a, int n) {
    if (n <= 0) throw std::invalid_argument("nthRoot: n must be positive");
    if (a.isZero()) return AlgReal(Fraction(0));
    if (n == 1) return a;
    if (a.isRational()) return nthRoot(a.asRational(), n);
    if (n % 2 == 0 && a.sign() < 0)
        throw std::domain_error("nthRoot: even root of negative");

    Poly p = composeXpowN_(a.p_, n);
    double da = a.toDouble();
    int  signA = (da >= 0) ? +1 : -1;
    double approxVal = signA * std::pow(std::abs(da), 1.0 / n);

    double absUp = std::pow(std::max(std::abs(a.a_.toDouble()),
                                     std::abs(a.b_.toDouble())), 1.0 / n) + 1.0;
    Fraction A, B;
    if (approxVal >= 0) {
        A = Fraction(0);
        B = Fraction(static_cast<long long>(std::ceil(absUp)));
    } else {
        A = Fraction(-static_cast<long long>(std::ceil(absUp)));
        B = Fraction(0);
    }

    Fraction outA, outB;
    for (int round = 0; round < 8; ++round) {
        if (pickIntervalContaining_(p, A, B, approxVal, outA, outB)) {
            return AlgReal(p, outA, outB);
        }
        A = A - Fraction(10);
        B = B + Fraction(10);
    }
    throw std::runtime_error("nthRoot(AlgReal, n): failed to isolate");
}

AlgReal AlgReal::sqrt(const Fraction& q)  { return nthRoot(q, 2); }
AlgReal AlgReal::sqrt(const AlgReal& a)   { return nthRoot(a, 2); }
AlgReal AlgReal::cbrt(const Fraction& q)  { return nthRoot(q, 3); }
AlgReal AlgReal::cbrt(const AlgReal& a)   { return nthRoot(a, 3); }

std::vector<AlgReal> AlgReal::realRootsOf(const Poly& p, const Fraction& tol) {
    std::vector<AlgReal> out;
    if (p.isZero() || p.degree() <= 0) return out;
    Poly sp = squarefreePart(p).monic();
    if (sp.degree() <= 0) return out;

    auto rats = rationalRoots(sp);
    std::sort(rats.begin(), rats.end());
    rats.erase(std::unique(rats.begin(), rats.end()), rats.end());
    for (const Fraction& q : rats) {
        Poly lin({Fraction(0) - q, Fraction(1)});
        if ((sp % lin).isZero()) {
            sp = sp / lin;
            out.push_back(AlgReal(q));
        }
    }
    if (sp.degree() > 0) {

        std::vector<std::pair<Fraction, Fraction>> ivs = isolateRealRoots(sp, tol);
        for (const auto& iv : ivs) {
            out.push_back(AlgReal(sp, iv.first, iv.second));
        }
    }
    std::sort(out.begin(), out.end(),
              [](const AlgReal& a, const AlgReal& b) {
                  return a.toDouble() < b.toDouble();
              });
    return out;
}

AlgReal AlgReal::evaluatePoly(const Poly& f, const AlgReal& alpha) {
    if (alpha.isRational()) {
        return AlgReal(f(alpha.asRational()));
    }
    if (f.isZero()) return AlgReal();
    if (f.degree() == 0) return AlgReal(f[0]);
    Poly q = minPolyOfEval(alpha.p_, f);
    if (q.isZero() || q.degree() <= 0) {
        return AlgReal();
    }
    Poly sp = squarefreePart(q).monic();
    if (sp.degree() == 0) return AlgReal();
    if (sp.degree() == 1) {
        Fraction qroot = -sp[0] / sp[1];
        return AlgReal(qroot);
    }

    auto rats = rationalRoots(sp);

    double alphaD = alpha.toDouble(1e-18);
    double betaD = 0.0;
    const auto& cs = f.coeffs();
    for (int i = static_cast<int>(cs.size()) - 1; i >= 0; --i) {
        betaD = betaD * alphaD + cs[static_cast<std::size_t>(i)].toDouble();
    }

    if (!rats.empty()) {
        double bestDiff = 1e300;
        Fraction bestR(0);
        for (const Fraction& r : rats) {
            double d = std::abs(r.toDouble() - betaD);
            if (d < bestDiff) { bestDiff = d; bestR = r; }
        }

        if (bestDiff < 1e-6) {
            Poly lin({Fraction(0) - bestR, Fraction(1)});
            if ((sp % lin).isZero()) return AlgReal(bestR);
        }
    }

    Fraction R = cauchyBound(sp) + Fraction(1);
    Fraction BNeg = Fraction(0) - R;
    Fraction BPos = R;
    Fraction outA, outB;
    if (pickIntervalContaining_(sp, BNeg, BPos, betaD, outA, outB)) {
        return AlgReal(sp, outA, outB);
    }

    std::vector<AlgReal> roots = AlgReal::realRootsOf(sp);
    if (roots.empty()) {
        throw std::runtime_error("evaluatePoly: result poly has no real roots");
    }
    int bestIdx = 0;
    double bestDiff = std::abs(roots[0].toDouble() - betaD);
    for (std::size_t i = 1; i < roots.size(); ++i) {
        double d = std::abs(roots[i].toDouble() - betaD);
        if (d < bestDiff) { bestDiff = d; bestIdx = static_cast<int>(i); }
    }
    return roots[static_cast<std::size_t>(bestIdx)];
}

namespace {

std::pair<int, Fraction> matchXnMinusQ_(const Poly& p) {
    if (p.degree() < 2) return {-1, Fraction(0)};
    const auto& c = p.coeffs();
    if (!(c.back() == Fraction(1))) return {-1, Fraction(0)};
    for (std::size_t i = 1; i + 1 < c.size(); ++i) {
        if (!c[i].isZero()) return {-1, Fraction(0)};
    }
    return {static_cast<int>(c.size()) - 1, -c[0]};
}
} 

std::string AlgReal::toString() const {
    if (isRational()) return asRational().toString();
    auto mp = matchXnMinusQ_(p_);
    int n = mp.first;
    Fraction q = mp.second;
    if (n >= 2) {
        std::ostringstream oss;
        bool neg = false;
        if (n % 2 == 0) {
            double mid = 0.5 * (a_.toDouble() + b_.toDouble());
            if (mid < 0) neg = true;
        }
        if (neg) oss << "-";
        if (n == 2)      oss << "sqrt(" << q.toString() << ")";
        else if (n == 3) oss << "cbrt(" << q.toString() << ")";
        else             oss << "root(" << q.toString() << ", " << n << ")";
        return oss.str();
    }
    std::ostringstream oss;
    oss << "alg(" << p_.toString() << ", [" << a_.toString() << ", " << b_.toString() << "])";
    return oss.str();
}

std::string AlgReal::toLatex() const {
    if (isRational()) return asRational().toLatex();
    auto mp = matchXnMinusQ_(p_);
    int n = mp.first;
    Fraction q = mp.second;
    if (n >= 2) {
        std::ostringstream oss;
        bool neg = false;
        if (n % 2 == 0) {
            double mid = 0.5 * (a_.toDouble() + b_.toDouble());
            if (mid < 0) neg = true;
        }
        if (neg) oss << "-";
        if (n == 2) oss << "\\sqrt{" << q.toLatex() << "}";
        else        oss << "\\sqrt[" << n << "]{" << q.toLatex() << "}";
        return oss.str();
    }
    std::ostringstream oss;
    oss << "\\mathrm{alg}\\!\\left(" << p_.toString()
        << ",\\;[" << a_.toLatex() << ",\\;" << b_.toLatex() << "]\\right)";
    return oss.str();
}

std::ostream& operator<<(std::ostream& os, const AlgReal& a) {
    return os << a.toString();
}

}
