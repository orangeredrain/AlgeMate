#include "algorithm/PolynomialAlg.h"
#include "core/BigInt.h"
#include "core/Fraction.h"
#include "core/PolynomialZp.h"

#include <algorithm>
#include <cstdint>
#include <functional>
#include <stdexcept>
#include <vector>

namespace algemate::math {

using ZPoly = std::vector<BigInt>; 
using PZ    = PolynomialZp;

namespace {

void zpTrim_(ZPoly& a) {
    while (!a.empty() && a.back().isZero()) a.pop_back();
}

int zpDeg_(const ZPoly& a) { return (int)a.size() - 1; }

ZPoly zpAdd_(const ZPoly& a, const ZPoly& b) {
    std::size_t n = std::max(a.size(), b.size());
    ZPoly r(n, BigInt(0));
    for (std::size_t i = 0; i < n; ++i) {
        if (i < a.size()) r[i] = r[i] + a[i];
        if (i < b.size()) r[i] = r[i] + b[i];
    }
    zpTrim_(r);
    return r;
}

ZPoly zpSub_(const ZPoly& a, const ZPoly& b) {
    std::size_t n = std::max(a.size(), b.size());
    ZPoly r(n, BigInt(0));
    for (std::size_t i = 0; i < n; ++i) {
        if (i < a.size()) r[i] = r[i] + a[i];
        if (i < b.size()) r[i] = r[i] - b[i];
    }
    zpTrim_(r);
    return r;
}

ZPoly zpMul_(const ZPoly& a, const ZPoly& b) {
    if (a.empty() || b.empty()) return {};
    ZPoly r(a.size() + b.size() - 1, BigInt(0));
    for (std::size_t i = 0; i < a.size(); ++i) {
        if (a[i].isZero()) continue;
        for (std::size_t j = 0; j < b.size(); ++j) {
            r[i + j] = r[i + j] + a[i] * b[j];
        }
    }
    zpTrim_(r);
    return r;
}

bool zpDivExact_(const ZPoly& a, const ZPoly& b, ZPoly& q, ZPoly& r) {
    if (b.empty()) return false;
    if (a.size() < b.size()) { q.clear(); r = a; return true; }
    r = a;
    int db = zpDeg_(b);
    q.assign((int)r.size() - db, BigInt(0));
    const BigInt& lcb = b.back();
    for (int k = (int)r.size() - 1; k >= db; --k) {

        BigInt rem = r[k] % lcb;
        if (!rem.isZero()) return false;
        BigInt c = r[k] / lcb;
        q[k - db] = c;
        for (int j = 0; j <= db; ++j) {
            r[k - db + j] = r[k - db + j] - c * b[j];
        }
    }
    r.resize(db);
    zpTrim_(r);
    zpTrim_(q);
    return true;
}

BigInt zpContent_(const ZPoly& a) {
    if (a.empty()) return BigInt(0);
    BigInt g(0);
    for (const auto& c : a) g = BigInt::gcd(g, c.abs());

    if (a.back().isNegative()) g = g.negate();
    return g;
}

ZPoly zpPrimitive_(const ZPoly& a, BigInt& contentOut) {
    if (a.empty()) { contentOut = BigInt(0); return {}; }
    BigInt c = zpContent_(a);
    if (c.isZero()) { contentOut = BigInt(0); return {}; }
    contentOut = c;
    ZPoly r(a.size());
    for (std::size_t i = 0; i < a.size(); ++i) r[i] = a[i] / c;
    return r;
}

ZPoly zpFromFraction_(const Polynomial<Fraction>& f, BigInt& denomLcmOut) {

    BigInt L(1);
    for (std::size_t i = 0; i < f.size(); ++i) {
        const BigInt& den = f[i].denominator();
        BigInt g = BigInt::gcd(L, den);
        L = L / g * den;
    }
    denomLcmOut = L;
    ZPoly r(f.size());
    for (std::size_t i = 0; i < f.size(); ++i) {
        BigInt num = f[i].numerator();
        BigInt den = f[i].denominator();

        r[i] = num * (L / den);
    }
    zpTrim_(r);
    return r;
}

Polynomial<Fraction> zpToFractionPoly_(const ZPoly& a) {
    if (a.empty()) return Polynomial<Fraction>();
    std::vector<Fraction> c(a.size());
    for (std::size_t i = 0; i < a.size(); ++i) c[i] = Fraction(a[i]);
    Polynomial<Fraction> r;
    for (int i = (int)c.size() - 1; i >= 0; --i) {
        r = r * Polynomial<Fraction>::x() + Polynomial<Fraction>(c[i]);
    }
    return r;
}

ZPoly zpModSym_(const ZPoly& a, const BigInt& m) {
    ZPoly r(a.size());
    BigInt half = m / BigInt(2);
    for (std::size_t i = 0; i < a.size(); ++i) {
        BigInt c = a[i] % m;
        if (c.isNegative()) c = c + m;
        if (c > half) c = c - m;
        r[i] = c;
    }
    zpTrim_(r);
    return r;
}

ZPoly zpModPos_(const ZPoly& a, const BigInt& m) {
    ZPoly r(a.size());
    for (std::size_t i = 0; i < a.size(); ++i) {
        BigInt c = a[i] % m;
        if (c.isNegative()) c = c + m;
        r[i] = c;
    }
    zpTrim_(r);
    return r;
}

PZ zpToPZ_(const ZPoly& a, PZ::Z p) {
    if (a.empty()) return PZ(p);
    std::vector<PZ::Z> c(a.size());
    BigInt P = BigInt((long long)p);
    for (std::size_t i = 0; i < a.size(); ++i) {
        BigInt m = a[i] % P;
        long long v = m.toLongLong();
        if (v < 0) v += p;
        c[i] = v;
    }
    return PZ(std::move(c), p);
}

ZPoly pzToZp_(const PZ& f) {
    ZPoly r(f.coeffs().size());
    for (std::size_t i = 0; i < f.coeffs().size(); ++i) r[i] = BigInt((long long)f.coeffs()[i]);
    zpTrim_(r);
    return r;
}

BigInt zpInfNorm_(const ZPoly& a) {
    BigInt m(0);
    for (const auto& c : a) {
        BigInt ac = c.abs();
        if (ac > m) m = ac;
    }
    return m;
}

BigInt mignotteBound_(const ZPoly& f) {
    int n = zpDeg_(f);
    if (n < 0) return BigInt(1);
    BigInt H = zpInfNorm_(f);

    BigInt two_n = BigInt::pow(BigInt(2), (unsigned)n);
    BigInt B = BigInt(n + 1) * two_n * H;
    if (B.isZero()) B = BigInt(1);
    return B;
}

struct FpEEA { PZ gcd, s, t; };
FpEEA fpEEA_(PZ a, PZ b) {
    PZ s0({1}, a.prime()), s1({}, a.prime());
    s1 = PZ({0}, a.prime());
    PZ t0({0}, a.prime()), t1({1}, a.prime());
    while (!b.isZero()) {
        auto dm = PZ::divmod(a, b);
        PZ s2 = PZ::sub(s0, PZ::mul(dm.q, s1));
        PZ t2 = PZ::sub(t0, PZ::mul(dm.q, t1));
        a = b; b = dm.r;
        s0 = s1; s1 = s2;
        t0 = t1; t1 = t2;
    }

    if (!a.isZero() && a.leading() != 1) {
        PZ::Z inv = PZ::invmod(a.leading(), a.prime());
        PZ invP({inv}, a.prime());
        a = PZ::mul(a, invP);
        s0 = PZ::mul(s0, invP);
        t0 = PZ::mul(t0, invP);
    }
    return {a, s0, t0};
}

ZPoly pzLiftedToZpoly_(const PZ& f, const BigInt& ) {

    return pzToZp_(f);
}

void henselStep_(
    const ZPoly& f,
    ZPoly& g_z, ZPoly& h_z,
    const PZ& s_p, const PZ& t_p,
    const BigInt& m , PZ::Z p) {

    ZPoly gh = zpMul_(g_z, h_z);
    ZPoly e  = zpSub_(f, gh);

    ZPoly e_div(e.size());
    for (std::size_t i = 0; i < e.size(); ++i) {
        BigInt q = e[i] / m;

        e_div[i] = q;
    }
    zpTrim_(e_div);
    PZ e_p = zpToPZ_(e_div, p);

    PZ h_p = zpToPZ_(h_z, p);
    PZ g_p = zpToPZ_(g_z, p);

    PZ se = PZ::mul(s_p, e_p);
    auto dm = PZ::divmod(se, h_p);
    PZ tau = dm.r;      
    PZ q_pz = dm.q;     
    PZ sigma = PZ::add(PZ::mul(t_p, e_p), PZ::mul(q_pz, g_p)); 

    ZPoly g_add = pzToZp_(sigma);
    ZPoly h_add = pzToZp_(tau);

    for (auto& c : g_add) c = c * m;
    for (auto& c : h_add) c = c * m;
    g_z = zpAdd_(g_z, g_add);
    h_z = zpAdd_(h_z, h_add);

    BigInt mp = m * BigInt((long long)p);
    g_z = zpModSym_(g_z, mp);
    h_z = zpModSym_(h_z, mp);
}

std::vector<ZPoly> zassenhausCombine_(
    ZPoly f, const std::vector<ZPoly>& factors, const BigInt& pk) {
    std::vector<ZPoly> results;
    std::vector<char> used(factors.size(), 0);
    int nActive = (int)factors.size();

    int s = 1;
    while (s * 2 <= nActive) {
        bool found = false;
        std::vector<int> avail;
        avail.reserve(factors.size());
        for (std::size_t i = 0; i < factors.size(); ++i) if (!used[i]) avail.push_back((int)i);
        int m = (int)avail.size();
        if (m < s) break;
        std::vector<int> idx(s);
        for (int i = 0; i < s; ++i) idx[i] = i;

        while (true) {

            BigInt lcF = f.back();
            ZPoly cand = {lcF};
            for (int i = 0; i < s; ++i) cand = zpMul_(cand, factors[avail[idx[i]]]);
            cand = zpModSym_(cand, pk);
            BigInt content;
            ZPoly prim = zpPrimitive_(cand, content);
            if (!prim.empty()) {
                ZPoly q, r;
                if (zpDivExact_(f, prim, q, r) && r.empty()) {
                    if (prim.back().isNegative()) {
                        for (auto& c : prim) c = c.negate();
                        for (auto& c : q) c = c.negate();
                    }
                    results.push_back(prim);
                    f = q;
                    for (int i = 0; i < s; ++i) used[avail[idx[i]]] = 1;
                    nActive -= s;
                    found = true;
                    break;
                }
            }

            int i = s - 1;
            while (i >= 0 && idx[i] == m - s + i) --i;
            if (i < 0) break;
            ++idx[i];
            for (int j = i + 1; j < s; ++j) idx[j] = idx[j-1] + 1;
        }
        if (!found) ++s;

    }
    if (zpDeg_(f) > 0) {
        if (f.back().isNegative()) for (auto& c : f) c = c.negate();
        results.push_back(f);
    }
    return results;
}

bool fpIsSquarefree_(const PZ& f) {
    if (f.degree() <= 0) return true;
    PZ d = PZ::gcd(f, f.derivative());
    return d.degree() == 0;
}

PZ::Z choosePrime_(const ZPoly& F) {
    static const PZ::Z candidates[] = {
        5, 7, 11, 13, 17, 19, 23, 29, 31, 37, 41, 43, 47,
        53, 59, 61, 67, 71, 73, 79, 83, 89, 97, 101, 103, 107, 109, 113
    };
    BigInt lc = F.back();
    for (PZ::Z p : candidates) {
        if ((lc % BigInt((long long)p)).isZero()) continue;
        PZ Fp = zpToPZ_(F, p);
        if ((int)Fp.degree() != zpDeg_(F)) continue;
        if (!fpIsSquarefree_(Fp)) continue;
        return p;
    }
    throw std::runtime_error("factorOverQ: no suitable prime in candidate set");
}

std::vector<ZPoly> factorSquarefreeZ_(const ZPoly& F) {
    if (zpDeg_(F) <= 0) return {};
    if (zpDeg_(F) == 1) return {F};

    PZ::Z p = choosePrime_(F);
    PZ Fp = zpToPZ_(F, p);

    auto fact = PZ::factor(Fp);
    std::vector<PZ> fpFactors;
    for (const auto& sf : fact.factors) {

        for (int k = 0; k < sf.e; ++k) fpFactors.push_back(sf.f);
    }
    if (fpFactors.size() == 1) {

        return {F};
    }

    BigInt B = mignotteBound_(F);
    BigInt bound = B * F.back().abs() * BigInt(2) + BigInt(1);

    BigInt pk(1);
    int K = 0;
    while (pk <= bound) { pk = pk * BigInt((long long)p); ++K; }

    BigInt P = BigInt((long long)p);
    std::vector<ZPoly> fz;
    for (const auto& g : fpFactors) fz.push_back(zpModSym_(pzToZp_(g), P));

    BigInt invLcPk;
    {

        BigInt a = F.back() % pk; if (a.isNegative()) a = a + pk;

        std::function<void(BigInt, BigInt, BigInt&, BigInt&, BigInt&)> extg =
            [&](BigInt A, BigInt B, BigInt& x, BigInt& y, BigInt& g) {
            if (B.isZero()) { x = BigInt(1); y = BigInt(0); g = A; return; }
            BigInt x1, y1;
            extg(B, A % B, x1, y1, g);
            x = y1;
            y = x1 - (A / B) * y1;
        };
        BigInt x, y, g;
        extg(a, pk, x, y, g);
        if (!g.isOne()) throw std::runtime_error("factorOverQ: lc not invertible mod p^K");
        invLcPk = x % pk; if (invLcPk.isNegative()) invLcPk = invLcPk + pk;
    }

    ZPoly F_monic(F.size());
    for (std::size_t i = 0; i < F.size(); ++i) F_monic[i] = F[i] * invLcPk;
    F_monic = zpModSym_(F_monic, pk);

    std::vector<ZPoly> lifted(fz.size());
    ZPoly target = F_monic;
    for (std::size_t i = 0; i + 1 < fz.size(); ++i) {

        ZPoly g_z = fz[i];

        PZ target_p = zpToPZ_(target, p);
        PZ g_p = zpToPZ_(g_z, p);
        auto dm = PZ::divmod(target_p, g_p);
        if (!dm.r.isZero()) {
            throw std::runtime_error("factorOverQ: hensel setup residue non-zero");
        }
        ZPoly h_z = pzToZp_(dm.q); 
        h_z = zpModSym_(h_z, P);   

        PZ h_p = dm.q;
        auto eea = fpEEA_(g_p, h_p);
        if (eea.gcd.degree() != 0 || eea.gcd.at(0) != 1) {
            throw std::runtime_error("factorOverQ: gcd(g,h) != 1 in F_p");
        }
        PZ s_p = eea.s, t_p = eea.t;

        BigInt m = P;
        for (int kk = 1; kk < K; ++kk) {
            henselStep_(target, g_z, h_z, s_p, t_p, m, p);
            m = m * P;
        }
        lifted[i] = g_z;
        target = h_z; 
    }
    lifted.back() = target;

    auto irreds = zassenhausCombine_(F, lifted, pk);
    return irreds;
}

} 

RationalFactorization factorOverQ(const Poly& f) {
    RationalFactorization out;
    if (f.isZero()) { out.leadingCoefficient = Fraction(0); return out; }
    if (f.degree() == 0) { out.leadingCoefficient = f[0]; return out; }

    auto sqf = squarefreeFactorization(f);
    out.leadingCoefficient = sqf.content;

    for (std::size_t k = 0; k < sqf.factors.size(); ++k) {
        const Poly& q = sqf.factors[k];
        int mult = (int)k + 1;
        if (q.isZero() || q.degree() == 0) continue;

        BigInt L;
        ZPoly Q = zpFromFraction_(q, L);

        BigInt cont;
        ZPoly prim = zpPrimitive_(Q, cont);
        if (prim.empty()) continue;

        if (prim.back().isNegative()) {
            for (auto& c : prim) c = c.negate();
        }

        auto zfacts = factorSquarefreeZ_(prim);

        for (auto& fi : zfacts) {

            BigInt lc = fi.back();
            Polynomial<Fraction> p_frac = zpToFractionPoly_(fi);

            Polynomial<Fraction> monic = p_frac * Polynomial<Fraction>(Fraction(BigInt(1), lc));

            Fraction lcFrac(lc);
            Fraction lcMult(1);
            for (int t = 0; t < mult; ++t) lcMult = lcMult * lcFrac;
            out.leadingCoefficient = out.leadingCoefficient * lcMult;
            out.factors.push_back({monic, mult});
        }

        Fraction adjust = Fraction(cont, L);
        Fraction adjPow(1);
        for (int t = 0; t < mult; ++t) adjPow = adjPow * adjust;
        out.leadingCoefficient = out.leadingCoefficient * adjPow;
    }

    return out;
}

} 
