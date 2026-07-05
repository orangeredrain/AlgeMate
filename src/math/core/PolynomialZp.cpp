#include "core/PolynomialZp.h"
#include "core/BigInt.h"
#include "core/Fraction.h"

#include <algorithm>
#include <functional>
#include <ostream>
#include <random>
#include <stdexcept>

namespace algemate::math {

using Z = PolynomialZp::Z;

Z PolynomialZp::modp(Z a, Z p) {
    Z r = a % p;
    if (r < 0) r += p;
    return r;
}

Z PolynomialZp::mulmod(Z a, Z b, Z p) {

    __int128 r = (__int128)a * (__int128)b;
    return (Z)(r % p);
}

Z PolynomialZp::powmod(Z a, Z e, Z p) {
    Z r = 1 % p;
    a = modp(a, p);
    while (e > 0) {
        if (e & 1) r = mulmod(r, a, p);
        a = mulmod(a, a, p);
        e >>= 1;
    }
    return r;
}

Z PolynomialZp::invmod(Z a, Z p) {
    a = modp(a, p);
    if (a == 0) throw std::domain_error("PolynomialZp::invmod: zero not invertible");
    return powmod(a, p - 2, p);
}

PolynomialZp::PolynomialZp() : coeffs_(), p_(0) {}

PolynomialZp::PolynomialZp(Z prime) : coeffs_(), p_(prime) {
    if (prime < 2) throw std::invalid_argument("PolynomialZp: prime must be >= 2");
}

PolynomialZp::PolynomialZp(std::vector<Z> coeffsLowFirst, Z prime)
    : coeffs_(std::move(coeffsLowFirst)), p_(prime) {
    if (prime < 2) throw std::invalid_argument("PolynomialZp: prime must be >= 2");
    for (auto& c : coeffs_) c = modp(c, p_);
    normalize_();
}

void PolynomialZp::normalize_() {
    while (!coeffs_.empty() && coeffs_.back() == 0) coeffs_.pop_back();
}

void PolynomialZp::requireSamePrime_(const PolynomialZp& a, const PolynomialZp& b) {
    if (a.p_ != b.p_) {

        if (!(a.isZero() && a.p_ == 0) && !(b.isZero() && b.p_ == 0))
            throw std::invalid_argument("PolynomialZp: prime mismatch");
    }
}

int PolynomialZp::degree() const {
    if (coeffs_.empty()) return -1;
    return (int)coeffs_.size() - 1;
}

bool PolynomialZp::isOne() const {
    return coeffs_.size() == 1 && coeffs_[0] == 1;
}

Z PolynomialZp::leading() const {
    if (coeffs_.empty()) throw std::domain_error("PolynomialZp::leading on zero");
    return coeffs_.back();
}

Z PolynomialZp::at(std::size_t k) const {
    if (k >= coeffs_.size()) return 0;
    return coeffs_[k];
}

bool PolynomialZp::operator==(const PolynomialZp& o) const {
    if (p_ != o.p_ && !(isZero() && o.isZero())) return false;
    return coeffs_ == o.coeffs_;
}

PolynomialZp PolynomialZp::makeMonic() const {
    if (isZero()) return *this;
    Z lc = leading();
    if (lc == 1) return *this;
    Z inv = invmod(lc, p_);
    std::vector<Z> c = coeffs_;
    for (auto& v : c) v = mulmod(v, inv, p_);
    return PolynomialZp(std::move(c), p_);
}

PolynomialZp PolynomialZp::derivative() const {
    if (coeffs_.size() <= 1) return PolynomialZp(p_);
    std::vector<Z> r(coeffs_.size() - 1);
    for (std::size_t i = 1; i < coeffs_.size(); ++i) {
        r[i - 1] = mulmod(coeffs_[i], (Z)(i % p_), p_);
    }
    return PolynomialZp(std::move(r), p_);
}

PolynomialZp PolynomialZp::add(const PolynomialZp& a, const PolynomialZp& b) {
    requireSamePrime_(a, b);
    Z p = a.p_ ? a.p_ : b.p_;
    std::size_t n = std::max(a.coeffs_.size(), b.coeffs_.size());
    std::vector<Z> r(n);
    for (std::size_t i = 0; i < n; ++i) {
        Z x = i < a.coeffs_.size() ? a.coeffs_[i] : 0;
        Z y = i < b.coeffs_.size() ? b.coeffs_[i] : 0;
        Z s = x + y; if (s >= p) s -= p;
        r[i] = s;
    }
    return PolynomialZp(std::move(r), p);
}

PolynomialZp PolynomialZp::sub(const PolynomialZp& a, const PolynomialZp& b) {
    requireSamePrime_(a, b);
    Z p = a.p_ ? a.p_ : b.p_;
    std::size_t n = std::max(a.coeffs_.size(), b.coeffs_.size());
    std::vector<Z> r(n);
    for (std::size_t i = 0; i < n; ++i) {
        Z x = i < a.coeffs_.size() ? a.coeffs_[i] : 0;
        Z y = i < b.coeffs_.size() ? b.coeffs_[i] : 0;
        Z s = x - y; if (s < 0) s += p;
        r[i] = s;
    }
    return PolynomialZp(std::move(r), p);
}

PolynomialZp PolynomialZp::mul(const PolynomialZp& a, const PolynomialZp& b) {
    requireSamePrime_(a, b);
    Z p = a.p_ ? a.p_ : b.p_;
    if (a.isZero() || b.isZero()) return PolynomialZp(p);
    std::vector<Z> r(a.coeffs_.size() + b.coeffs_.size() - 1, 0);
    for (std::size_t i = 0; i < a.coeffs_.size(); ++i) {
        if (a.coeffs_[i] == 0) continue;
        for (std::size_t j = 0; j < b.coeffs_.size(); ++j) {
            Z prod = mulmod(a.coeffs_[i], b.coeffs_[j], p);
            Z s = r[i + j] + prod; if (s >= p) s -= p;
            r[i + j] = s;
        }
    }
    return PolynomialZp(std::move(r), p);
}

PolynomialZp::DivMod PolynomialZp::divmod(const PolynomialZp& a, const PolynomialZp& b) {
    requireSamePrime_(a, b);
    if (b.isZero()) throw std::domain_error("PolynomialZp::divmod by zero");
    Z p = b.p_;
    if (a.degree() < b.degree()) return {PolynomialZp(p), a};
    std::vector<Z> r = a.coeffs_;
    int m = b.degree();
    Z invLc = invmod(b.leading(), p);
    std::vector<Z> q((int)r.size() - m, 0);
    for (int k = (int)r.size() - 1; k >= m; --k) {
        Z c = mulmod(r[k], invLc, p);
        q[k - m] = c;
        if (c == 0) continue;
        for (int j = 0; j <= m; ++j) {
            Z t = mulmod(c, b.coeffs_[j], p);
            Z s = r[k - m + j] - t; if (s < 0) s += p;
            r[k - m + j] = s;
        }
    }
    r.resize(m);
    return {PolynomialZp(std::move(q), p), PolynomialZp(std::move(r), p)};
}

PolynomialZp PolynomialZp::gcd(PolynomialZp a, PolynomialZp b) {
    if (a.isZero() && b.isZero()) return a;
    while (!b.isZero()) {
        auto dm = divmod(a, b);
        a = b;
        b = dm.r;
    }
    return a.makeMonic();
}

PolynomialZp PolynomialZp::powMod(const PolynomialZp& base, Z e, const PolynomialZp& m) {
    if (m.isZero()) throw std::domain_error("PolynomialZp::powMod modulus zero");
    Z p = m.p_;
    PolynomialZp r({1}, p);
    PolynomialZp a = divmod(base, m).r;
    while (e > 0) {
        if (e & 1) r = divmod(mul(r, a), m).r;
        a = divmod(mul(a, a), m).r;
        e >>= 1;
    }
    return r;
}

PolynomialZp PolynomialZp::fromPoly(const Polynomial<Fraction>& f, Z prime) {
    if (f.isZero()) return PolynomialZp(prime);
    std::vector<Z> c(f.size());
    for (std::size_t i = 0; i < f.size(); ++i) {
        const Fraction& q = f[i];

        BigInt num = q.numerator();
        BigInt den = q.denominator();

        BigInt P = BigInt((long long)prime);
        BigInt nm = num % P;
        BigInt dm = den % P;
        long long nn = nm.toLongLong();
        long long dd = dm.toLongLong();
        Z nMod = modp((Z)nn, prime);
        Z dMod = modp((Z)dd, prime);
        if (dMod == 0) throw std::domain_error("PolynomialZp::fromPoly: denominator divisible by prime");
        Z invD = invmod(dMod, prime);
        c[i] = mulmod(nMod, invD, prime);
    }
    return PolynomialZp(std::move(c), prime);
}

Polynomial<Fraction> PolynomialZp::toPoly() const {
    if (isZero()) return Polynomial<Fraction>();
    std::vector<Fraction> c(coeffs_.size());
    for (std::size_t i = 0; i < coeffs_.size(); ++i) {
        c[i] = Fraction(BigInt((long long)coeffs_[i]));
    }

    Polynomial<Fraction> r;
    for (int i = (int)c.size() - 1; i >= 0; --i) {
        r = r * Polynomial<Fraction>::x() + Polynomial<Fraction>(c[i]);
    }
    return r;
}

static PolynomialZp pthRoot_(const PolynomialZp& f) {
    Z p = f.prime();
    std::vector<Z> c((f.degree() / p) + 1, 0);
    for (std::size_t i = 0; i < f.coeffs().size(); ++i) {
        if (f.coeffs()[i] == 0) continue;
        if ((Z)i % p != 0) throw std::logic_error("pthRoot_: exponent not multiple of p");

        c[i / p] = f.coeffs()[i];
    }
    return PolynomialZp(std::move(c), p);
}

std::vector<PolynomialZp::SqfFactor>
PolynomialZp::squarefreeFactorization(const PolynomialZp& fIn) {
    if (fIn.isZero() || fIn.degree() == 0) return {};
    PolynomialZp f = fIn.makeMonic();
    Z p = f.prime();
    std::vector<SqfFactor> result;

    std::function<void(PolynomialZp, int)> sqf = [&](PolynomialZp h, int mul0) {
        if (h.degree() <= 0) return;
        PolynomialZp fp = h.derivative();
        if (fp.isZero()) {

            PolynomialZp g = pthRoot_(h);
            sqf(g, mul0 * (int)p);
            return;
        }
        PolynomialZp c = gcd(h, fp);        
        PolynomialZp w = divmod(h, c).q;    
        int i = 1;
        while (!w.isOne()) {
            PolynomialZp y = gcd(w, c);
            PolynomialZp z = divmod(w, y).q; 
            if (!z.isOne()) {
                result.push_back({z, i * mul0});
            }
            w = y;
            c = divmod(c, y).q;
            ++i;
        }

        if (!c.isOne()) {
            PolynomialZp g = pthRoot_(c);
            sqf(g, mul0 * (int)p);
        }
    };
    sqf(f, 1);
    return result;
}

std::vector<PolynomialZp::DDFPart>
PolynomialZp::distinctDegreeFactorization(const PolynomialZp& fIn) {
    Z p = fIn.prime();
    PolynomialZp f = fIn.makeMonic();
    std::vector<DDFPart> result;
    PolynomialZp h({0, 1}, p); 
    int d = 1;
    while (f.degree() >= 2 * d) {

        h = powMod(h, p, f);
        PolynomialZp g = gcd(f, sub(h, PolynomialZp({0, 1}, p)));
        if (!g.isOne()) {
            result.push_back({g.makeMonic(), d});
            f = divmod(f, g).q;

            h = divmod(h, f).r;
        }
        ++d;
    }
    if (f.degree() > 0) {
        result.push_back({f, f.degree()});
    }
    return result;
}

static std::mt19937_64& czRng_() {
    static std::mt19937_64 g(0x12345678);
    return g;
}

static PolynomialZp czSplitOdd_(const PolynomialZp& g, int d) {

    Z p = g.prime();
    int dg = g.degree();
    auto& rng = czRng_();
    std::uniform_int_distribution<Z> dist(0, p - 1);
    while (true) {
        std::vector<Z> rc(dg);
        for (auto& v : rc) v = dist(rng);
        rc.push_back(1); 

        PolynomialZp r(std::move(rc), p);
        r = PolynomialZp::divmod(r, g).r;
        if (r.degree() <= 0) continue;
        __int128 pd = 1;
        for (int i = 0; i < d; ++i) pd *= (__int128)p;
        __int128 e = (pd - 1) / 2;

        PolynomialZp acc({1}, p);
        PolynomialZp base = r;
        while (e > 0) {
            if (e & 1) acc = PolynomialZp::divmod(PolynomialZp::mul(acc, base), g).r;
            base = PolynomialZp::divmod(PolynomialZp::mul(base, base), g).r;
            e >>= 1;
        }
        PolynomialZp a = PolynomialZp::sub(acc, PolynomialZp({1}, p));
        PolynomialZp h = PolynomialZp::gcd(g, a);
        if (!h.isOne() && h.degree() < g.degree()) return h;
    }
}

static PolynomialZp czSplitTwo_(const PolynomialZp& g, int d) {

    Z p = 2;
    int dg = g.degree();
    auto& rng = czRng_();
    std::uniform_int_distribution<Z> dist(0, 1);
    while (true) {
        std::vector<Z> rc(dg);
        for (auto& v : rc) v = dist(rng);
        rc.push_back(1);
        PolynomialZp r(std::move(rc), p);
        r = PolynomialZp::divmod(r, g).r;
        if (r.degree() <= 0) continue;
        PolynomialZp t = r;
        PolynomialZp acc = r;
        for (int i = 1; i < d; ++i) {
            acc = PolynomialZp::divmod(PolynomialZp::mul(acc, acc), g).r;
            t = PolynomialZp::add(t, acc);
        }
        PolynomialZp h = PolynomialZp::gcd(g, t);
        if (!h.isOne() && h.degree() < g.degree()) return h;
    }
}

std::vector<PolynomialZp>
PolynomialZp::equalDegreeFactorization(const PolynomialZp& gIn, int d) {
    PolynomialZp g = gIn.makeMonic();
    if (g.degree() == d) return {g};
    std::vector<PolynomialZp> out;
    std::function<void(PolynomialZp)> rec = [&](PolynomialZp h) {
        if (h.degree() == d) { out.push_back(h); return; }
        PolynomialZp s = (h.prime() == 2) ? czSplitTwo_(h, d) : czSplitOdd_(h, d);
        rec(s);
        rec(divmod(h, s).q.makeMonic());
    };
    rec(g);
    return out;
}

PolynomialZp::Factorization PolynomialZp::factor(const PolynomialZp& fIn) {
    Factorization out;
    if (fIn.isZero()) throw std::domain_error("PolynomialZp::factor: zero polynomial");
    out.lc = fIn.leading();
    if (fIn.degree() == 0) return out;
    PolynomialZp monic = fIn.makeMonic();
    auto sqfs = squarefreeFactorization(monic);
    for (const auto& sf : sqfs) {
        auto ddfs = distinctDegreeFactorization(sf.f);
        for (const auto& dp : ddfs) {
            auto edfs = equalDegreeFactorization(dp.g, dp.d);
            for (const auto& fi : edfs) {
                out.factors.push_back({fi, sf.e});
            }
        }
    }
    return out;
}

bool PolynomialZp::isIrreducible(const PolynomialZp& fIn) {
    if (fIn.degree() < 1) return false;
    Z p = fIn.prime();
    PolynomialZp f = fIn.makeMonic();
    int n = f.degree();

    PolynomialZp x({0, 1}, p);
    PolynomialZp h = x;
    for (int i = 0; i < n; ++i) {
        h = powMod(h, p, f);
    }
    if (!sub(h, x).isZero()) return false;

    std::vector<int> primes;
    int nn = n;
    for (int d = 2; (long long)d * d <= nn; ++d) {
        if (nn % d == 0) {
            primes.push_back(d);
            while (nn % d == 0) nn /= d;
        }
    }
    if (nn > 1) primes.push_back(nn);
    for (int q : primes) {
        int e = n / q;
        PolynomialZp hh = x;
        for (int i = 0; i < e; ++i) hh = powMod(hh, p, f);
        PolynomialZp g = gcd(f, sub(hh, x));
        if (!g.isOne()) return false;
    }
    return true;
}

std::ostream& operator<<(std::ostream& os, const PolynomialZp& f) {
    if (f.isZero()) { os << "0"; return os; }
    bool first = true;
    for (int i = f.degree(); i >= 0; --i) {
        Z c = f.at((std::size_t)i);
        if (c == 0) continue;
        if (!first) os << " + ";
        first = false;
        if (i == 0) os << c;
        else if (i == 1) {
            if (c == 1) os << "x";
            else os << c << "*x";
        } else {
            if (c == 1) os << "x^" << i;
            else os << c << "*x^" << i;
        }
    }
    os << " (mod " << f.prime() << ")";
    return os;
}

} 
