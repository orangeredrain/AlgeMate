#include "algorithm/PolynomialAlg.h"
#include "core/BigInt.h"
#include "core/Fraction.h"
#include "core/PolynomialZp.h"

#include <algorithm>
#include <cstdint>
#include <functional>
#include <stdexcept>
#include <vector>

/*
* @file FactorOverQ.cpp
* @brief Q[x] 不可约因式分解: Hensel + Zassenhaus
*
* 主流程 factorOverQ(f):
* 1) Yun 无平方分解: f = c · ∏ q_i^{e_i} (q_i ∈ Q[x] squarefree monic)
* 2) 对每个 q_i: 转 primitive Z[x] 多项式 F_i (不除 content, 保留首项系数)
* 3) 选素数 p 使 p \nmid lc(F_i) 且 F_i mod p 依旧 squarefree
* 4) 在 F_p[x] 做 Cantor-Zassenhaus 分解
* 5) Hensel 提升到 mod p^k, k 满足 p^k > 2 · Mignotte(F_i)
* 6) Zassenhaus 组合搜索在 Z[x] 上试除, 得到不可约 Z[x] 因子
* 7) 将因子首一化得到 Q[x] 不可约因子
*/

namespace algemate::math {

using ZPoly = std::vector<BigInt>; // 低位优先, 末尾非零 (除零多项式)
using PZ    = PolynomialZp;

namespace {

//  ZPoly 基础工具
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

// 返回 q, r 使 a = q*b + r, deg r < deg b; b 首项系数整除 r[k] 的精确除法
// 若不能精确除, 返回 r 非零多项式表示失败.
// 这里专用于 Z[x] 精确带余除: 要求 lc(b) | 每次减去后的首项.
bool zpDivExact_(const ZPoly& a, const ZPoly& b, ZPoly& q, ZPoly& r) {
    if (b.empty()) return false;
    if (a.size() < b.size()) { q.clear(); r = a; return true; }
    r = a;
    int db = zpDeg_(b);
    q.assign((int)r.size() - db, BigInt(0));
    const BigInt& lcb = b.back();
    for (int k = (int)r.size() - 1; k >= db; --k) {
        // r[k] 必须能整除 lc(b)
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

// 整系数内容 (gcd of |coefficients|)
BigInt zpContent_(const ZPoly& a) {
    if (a.empty()) return BigInt(0);
    BigInt g(0);
    for (const auto& c : a) g = BigInt::gcd(g, c.abs());
    // 保留与首项系数相同的符号 (为了 primitive 首项 ≥ 0 规范)
    if (a.back().isNegative()) g = g.negate();
    return g;
}

// 把 a 除以其 content, 返回 primitive 多项式 (首项 ≥ 0)
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
    // 令 L = lcm(分母), 返回 L * f \in Z[x]
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
        // r[i] = num * L / den
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

// 系数映射: a 的每个系数 对 m 做中心化/归约
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

// a mod m (结果 \in [0, m))
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

// ZPoly ↔ PolynomialZp (prime p)
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

// 计算 max |coeff|
BigInt zpInfNorm_(const ZPoly& a) {
    BigInt m(0);
    for (const auto& c : a) {
        BigInt ac = c.abs();
        if (ac > m) m = ac;
    }
    return m;
}

// Mignotte/Landau bound: B = (n+1) * 2^n * ||f||_∞
// 任何 Z[x] 因子 g (deg g ≤ n) 满足 ||g||_{\infty} < B
BigInt mignotteBound_(const ZPoly& f) {
    int n = zpDeg_(f);
    if (n < 0) return BigInt(1);
    BigInt H = zpInfNorm_(f);
    // B = (n+1) * 2^n * H
    BigInt two_n = BigInt::pow(BigInt(2), (unsigned)n);
    BigInt B = BigInt(n + 1) * two_n * H;
    if (B.isZero()) B = BigInt(1);
    return B;
}


//  Hensel 提升 (两因子, 倍增模数)
// F_p 上 EEA: 返回 (gcd, s, t) 满足 s*a + t*b = gcd
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
    // 归一化 gcd 为 monic
    if (!a.isZero() && a.leading() != 1) {
        PZ::Z inv = PZ::invmod(a.leading(), a.prime());
        PZ invP({inv}, a.prime());
        a = PZ::mul(a, invP);
        s0 = PZ::mul(s0, invP);
        t0 = PZ::mul(t0, invP);
    }
    return {a, s0, t0};
}

// 对系数做 mod p^k 后中心化 + 转 ZPoly
ZPoly pzLiftedToZpoly_(const PZ& f, const BigInt& /*mod*/) {
    // PZ 系数 ∈ [0, p), 直接转即可 (外层会再 mod p^k)
    return pzToZp_(f);
}

// 单步 Hensel: 把 mod m=p^k 的 (g, h, s, t) 提升到 mod m·p=p^{k+1}.
// 输入 f ∈ Z[x], g_z, h_z ∈ Z[x] 满足 f ≡ g·h (mod m), s_z·g + t_z·h ≡ 1 (mod p).
// 输出新的 g', h' (mod m·p). s, t 保持 mod p 不变.
void henselStep_(
    const ZPoly& f,
    ZPoly& g_z, ZPoly& h_z,
    const PZ& s_p, const PZ& t_p,
    const BigInt& m /*= p^k */, PZ::Z p) {
    // e = f - g·h, 其系数必被 m 整除
    ZPoly gh = zpMul_(g_z, h_z);
    ZPoly e  = zpSub_(f, gh);
    // e / m 的系数再 mod p
    ZPoly e_div(e.size());
    for (std::size_t i = 0; i < e.size(); ++i) {
        BigInt q = e[i] / m;
        // 这里假设 e[i] % m == 0 (Hensel 不变量)
        e_div[i] = q;
    }
    zpTrim_(e_div);
    PZ e_p = zpToPZ_(e_div, p);
    // 计算 t·e mod h (在 F_p 下)
    PZ h_p = zpToPZ_(h_z, p);
    PZ g_p = zpToPZ_(g_z, p);
    // sg + th = 1 mod p  ⇔  gs + ht = 1 mod p.
    // Hensel step: divide s·ε by h (not t·ε), because the identity
    //   g·τ + h·σ = g(sε - qh) + h(tε + qg) = (gs + ht)ε = ε
    // requires τ paired with g and σ paired with h.
    PZ se = PZ::mul(s_p, e_p);
    auto dm = PZ::divmod(se, h_p);
    PZ tau = dm.r;      // τ = s·ε mod h (deg τ < deg h)
    PZ q_pz = dm.q;     // q = s·ε div h
    PZ sigma = PZ::add(PZ::mul(t_p, e_p), PZ::mul(q_pz, g_p)); // σ = t·ε + q·g
    // g_new = g + m·σ,  h_new = h + m·τ
    ZPoly g_add = pzToZp_(sigma);
    ZPoly h_add = pzToZp_(tau);
    // 把 g_add, h_add 每个系数 · m
    for (auto& c : g_add) c = c * m;
    for (auto& c : h_add) c = c * m;
    g_z = zpAdd_(g_z, g_add);
    h_z = zpAdd_(h_z, h_add);
    // mod (m·p) 对称化
    BigInt mp = m * BigInt((long long)p);
    g_z = zpModSym_(g_z, mp);
    h_z = zpModSym_(h_z, mp);
}

//  Zassenhaus 组合: 从 lifted mod p^K 的因子列表中, 试除 f ∈ Z[x]
// f 是 primitive Z[x], squarefree. factors 是 f_i mod p^K 的列表 (ZPoly, 中心化).
// 返回 f 的不可约 Z[x] 因子列表 (primitive, 首项系数正).
std::vector<ZPoly> zassenhausCombine_(
    ZPoly f, const std::vector<ZPoly>& factors, const BigInt& pk) {
    std::vector<ZPoly> results;
    std::vector<char> used(factors.size(), 0);
    int nActive = (int)factors.size();

    // 枚举子集大小 s = 1, 2, ..., nActive/2.
    // 每次找到一个因子后, 重建 avail 继续当前 s 从头开始.
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
        // 枚举所有 size=s 的组合
        while (true) {
            // 构造 candidate
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
            // next combination
            int i = s - 1;
            while (i >= 0 && idx[i] == m - s + i) --i;
            if (i < 0) break;
            ++idx[i];
            for (int j = i + 1; j < s; ++j) idx[j] = idx[j-1] + 1;
        }
        if (!found) ++s;
        // if found, keep s unchanged and rebuild avail in next iteration
    }
    if (zpDeg_(f) > 0) {
        if (f.back().isNegative()) for (auto& c : f) c = c.negate();
        results.push_back(f);
    }
    return results;
}

//  选素数 p: p \nmid lc(F) 且 F mod p squarefree
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

//  核心: squarefree primitive F ∈ Z[x] 的 Z[x] 不可约因式分解
std::vector<ZPoly> factorSquarefreeZ_(const ZPoly& F) {
    if (zpDeg_(F) <= 0) return {};
    if (zpDeg_(F) == 1) return {F};

    PZ::Z p = choosePrime_(F);
    PZ Fp = zpToPZ_(F, p);

    // F_p 上分解
    auto fact = PZ::factor(Fp);
    std::vector<PZ> fpFactors;
    for (const auto& sf : fact.factors) {
        // squarefree 输入
        for (int k = 0; k < sf.e; ++k) fpFactors.push_back(sf.f);
    }
    if (fpFactors.size() == 1) {
        // 在 F_p 上不可约 ⇒ 在 Z[x] 上不可约
        return {F};
    }

    // 目标: mod p^K > 2 · lc(F) · B
    BigInt B = mignotteBound_(F);
    BigInt bound = B * F.back().abs() * BigInt(2) + BigInt(1);
    // 找最小 K
    BigInt pk(1);
    int K = 0;
    while (pk <= bound) { pk = pk * BigInt((long long)p); ++K; }

    // Hensel lift: 多因子倍增策略 — 每次把当前 factors 列表的一个 (g = factors[0], h = ∏ rest) 做 step, 提升整体 mod 从 p^k 到 p^{k+1}.
    // 先转为 ZPoly 形式 (中心化 mod p).
    BigInt P = BigInt((long long)p);
    std::vector<ZPoly> fz;
    for (const auto& g : fpFactors) fz.push_back(zpModSym_(pzToZp_(g), P));

    BigInt invLcPk;
    {
        // F.back() mod p^K 的逆: 用扩展欧几里得 in ℤ
        BigInt a = F.back() % pk; if (a.isNegative()) a = a + pk;
        // ext gcd
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

    // F_monic = F * invLc mod p^K (中心化)
    ZPoly F_monic(F.size());
    for (std::size_t i = 0; i < F.size(); ++i) F_monic[i] = F[i] * invLcPk;
    F_monic = zpModSym_(F_monic, pk);

    std::vector<ZPoly> lifted(fz.size());
    ZPoly target = F_monic;
    for (std::size_t i = 0; i + 1 < fz.size(); ++i) {
        // g_z = lift 目标之一 (monic)
        ZPoly g_z = fz[i];
        // h_p = target mod p 除以 g mod p, 得余其他因子积 (monic)
        PZ target_p = zpToPZ_(target, p);
        PZ g_p = zpToPZ_(g_z, p);
        auto dm = PZ::divmod(target_p, g_p);
        if (!dm.r.isZero()) {
            throw std::runtime_error("factorOverQ: hensel setup residue non-zero");
        }
        ZPoly h_z = pzToZp_(dm.q); // 在 [0,p) 系数
        h_z = zpModSym_(h_z, P);   // 中心化

        // EEA in F_p: s·g + t·h = 1
        PZ h_p = dm.q;
        auto eea = fpEEA_(g_p, h_p);
        if (eea.gcd.degree() != 0 || eea.gcd.at(0) != 1) {
            throw std::runtime_error("factorOverQ: gcd(g,h) != 1 in F_p");
        }
        PZ s_p = eea.s, t_p = eea.t;

        // 线性 lift: mod p^1 → mod p^K
        BigInt m = P;
        for (int kk = 1; kk < K; ++kk) {
            henselStep_(target, g_z, h_z, s_p, t_p, m, p);
            m = m * P;
        }
        lifted[i] = g_z;
        target = h_z; // 继续用作下一轮的 target
    }
    lifted.back() = target;

    // Zassenhaus 组合
    auto irreds = zassenhausCombine_(F, lifted, pk);
    return irreds;
}

} // anonymous namespace


//  公开 API: factorOverQ
RationalFactorization factorOverQ(const Poly& f) {
    RationalFactorization out;
    if (f.isZero()) { out.leadingCoefficient = Fraction(0); return out; }
    if (f.degree() == 0) { out.leadingCoefficient = f[0]; return out; }

    // 先做 Yun squarefree 分解
    auto sqf = squarefreeFactorization(f);
    out.leadingCoefficient = sqf.content;

    // 对每个 squarefree 因子 q_i, 对 Z[x] 分解
    for (std::size_t k = 0; k < sqf.factors.size(); ++k) {
        const Poly& q = sqf.factors[k];
        int mult = (int)k + 1;
        if (q.isZero() || q.degree() == 0) continue;

        // q ∈ Q[x] monic squarefree → 转 ZPoly
        BigInt L;
        ZPoly Q = zpFromFraction_(q, L);
        // 提取 content
        BigInt cont;
        ZPoly prim = zpPrimitive_(Q, cont);
        if (prim.empty()) continue;
        // 符号规范化
        if (prim.back().isNegative()) {
            for (auto& c : prim) c = c.negate();
        }

        // 调用内部 squarefree Z[x] 分解
        auto zfacts = factorSquarefreeZ_(prim);

        // 转回 Q[x] monic 因子
        for (auto& fi : zfacts) {
            // primitive 因子: 归一化 lc
            BigInt lc = fi.back();
            Polynomial<Fraction> p_frac = zpToFractionPoly_(fi);
            // 除以 lc 使 monic
            Polynomial<Fraction> monic = p_frac * Polynomial<Fraction>(Fraction(BigInt(1), lc));
            // leadingCoefficient 需吸收每个 lc^mult
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

} // namespace algemate::math
