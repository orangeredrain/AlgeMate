#include "algorithm/PolynomialAlg.h"

#include "core/BigInt.h"

#include <algorithm>
#include <cstdint>
#include <stdexcept>

namespace algemate::math {

namespace {

// Fraction 快速幂 (指数非负)
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

// 结式: 维护 sign 与系数缩放, 迭代 Euclidean
// Res(f, g) = lc(g)^(deg f - deg r) * (-1)^(deg f * deg g) * Res(g, r),  r = f mod g
Fraction resultantImpl_(Poly f, Poly g) {
    if (f.isZero() || g.isZero()) return Fraction(0);
    Fraction sign(1);
    Fraction factor(1);

    while (true) {
        int df = f.degree();
        int dg = g.degree();

        if (df < dg) {
            // Res(f, g) = (-1)^(df*dg) * Res(g, f)
            if (((df * dg) & 1) != 0) sign = -sign;
            std::swap(f, g);
            std::swap(df, dg);
        }

        if (dg == 0) {
            // Res(f, const) = const^df
            factor = factor * fracPow_(g.leading(), static_cast<unsigned int>(df));
            return sign * factor;
        }

        Poly r = f % g;
        if (r.isZero()) return Fraction(0);
        int dr = r.degree();

        // (-1)^(df * dg)
        if (((df * dg) & 1) != 0) sign = -sign;
        // lc(g)^(df - dr)
        factor = factor * fracPow_(g.leading(), static_cast<unsigned int>(df - dr));

        f = g;
        g = r;
    }
}

// 枚举 |value| 的正因子, 用 long long trial division
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

// 拉格朗日插值 由 n+1 个点 (x_s[i], y_s[i]) 构造度数次数不超过n的多项式
Poly lagrangeInterp_(const std::vector<Fraction>& xs, const std::vector<Fraction>& ys) {
    const std::size_t n = xs.size();
    Poly result;  // 0
    for (std::size_t i = 0; i < n; ++i) {
        Poly     Li(Fraction(1));
        Fraction denom(1);
        for (std::size_t j = 0; j < n; ++j) {
            if (j == i) continue;
            // (x - xs[j])
            Li = Li * Poly({-xs[j], Fraction(1)});
            denom = denom * (xs[i] - xs[j]);
        }
        result = result + Li * (ys[i] / denom);
    }
    return result;
}

}  // namespace

// squarefree

Poly squarefreePart(const Poly& f) {
    if (f.isZero()) return f;
    if (f.degree() == 0) return Poly(Fraction(1));
    Poly d = f.derivative();
    Poly g = gcd(f, d);        // monic
    Poly sp = f / g;
    if (!sp.isZero()) sp = sp.monic();
    return sp;
}

// Yun 算法 
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
    Poly a0 = gcd(fm, fp);         // monic
    Poly b  = fm / a0;             // b_1
    Poly c  = fp / a0;             // c_1
    Poly d  = c - b.derivative();  // d_1

    // 最多 deg(f)+1 轮迭代
    const int maxIter = fm.degree() + 1;
    for (int iter = 0; iter < maxIter && b.degree() > 0; ++iter) {
        Poly ai = gcd(b, d);                           // a_i = p_i (重数 i 因子)
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

// resultant and discriminant

Fraction resultant(const Poly& f, const Poly& g) {
    return resultantImpl_(f, g);
}

Fraction discriminant(const Poly& f) {
    if (f.degree() < 1) return Fraction(0);
    const int n = f.degree();
    Fraction res = resultantImpl_(f, f.derivative());
    // (-1)^(n(n-1)/2) / lc(f)
    if (((n * (n - 1) / 2) & 1) != 0) res = -res;
    res = res / f.leading();
    return res;
}

// Q 上多项式枚举有理根

std::vector<Fraction> rationalRoots(const Poly& f) {
    std::vector<Fraction> out;
    if (f.isZero() || f.degree() == 0) return out;

    // 将 f 转成本原多项式
    // g_i = f_i * D, 其中 D = lcm(denominators)
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
    // 约整系数公因子
    BigInt cont(0);
    for (const BigInt& v : gInt) cont = BigInt::gcd(cont, v.abs());
    if (cont.sign() == 0) return out;  // 全 0
    if (cont > BigInt(1)) {
        for (BigInt& v : gInt) v = v / cont;
    }

    // 若常数项为 0, 则 0 是一个根
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

    // 常数项 c0, 首项 cn
    BigInt c0 = gInt.front().abs();
    BigInt cn = gInt.back().abs();

    // 超过 long long 的系数不处理
    BigInt maxLL(static_cast<long long>(INT64_MAX));
    if (c0 > maxLL || cn > maxLL) {
        if (zeroIsRoot) out.push_back(Fraction(0));
        return out;
    }
    long long c0ll = c0.toLongLong();
    long long cnll = cn.toLongLong();

    std::vector<long long> ps = divisors_(c0ll);
    std::vector<long long> qs = divisors_(cnll);

    // 枚举候选 ±p/q
    std::vector<Fraction> candidates;
    for (long long p : ps) {
        for (long long q : qs) {
            // 要求 gcd(p, q) = 1, 避免重复
            long long a = p, b = q;
            while (b != 0) { long long t = a % b; a = b; b = t; }
            if (a != 1) continue;
            Fraction pos{BigInt(p), BigInt(q)};
            Fraction neg{BigInt(-p), BigInt(q)};
            candidates.push_back(pos);
            candidates.push_back(neg);
        }
    }

    // 检验 f(r) == 0
    std::vector<Fraction> roots;
    if (zeroIsRoot) roots.push_back(Fraction(0));
    for (const Fraction& r : candidates) {
        if (f(r).sign() == 0) roots.push_back(r);
    }

    // 去重、排序
    std::sort(roots.begin(), roots.end());
    roots.erase(std::unique(roots.begin(), roots.end()), roots.end());
    return roots;
}

// Sturm algorithm

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

// Cauchy 根界 / 实根隔离

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

// 递归 Sturm + 二分隔离, 假设 sp 无平方
// 不变量: A, B 都不是 sp 的根
void isolateImpl_(const Poly& sp, const std::vector<Poly>& seq,
                  const Fraction& A, const Fraction& B,
                  const Fraction& tol,
                  std::vector<std::pair<Fraction, Fraction>>& out,
                  int depth) {
    if (depth > 80) return;
    if (!(A < B)) return;
    int vA = sturmSignChanges(seq, A);
    int vB = sturmSignChanges(seq, B);
    int cnt = vA - vB;    // 开区间 (A, B] 内根数, 但 B 不是根故即 [A, B]
    if (cnt == 0) return;
    int sa = sp(A).sign();
    int sb = sp(B).sign();
    if (cnt == 1 && sa != 0 && sb != 0 && sa != sb) {
        // 二分精化到 tol
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
        // m 恰为有理根: 单独收入, 左右划分时切出邻域
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

}  // namespace

std::vector<std::pair<Fraction, Fraction>> isolateRealRoots(const Poly& f, const Fraction& tol) {
    std::vector<std::pair<Fraction, Fraction>> out;
    if (f.isZero() || f.degree() <= 0) return out;
    Poly sp = squarefreePart(f);
    if (sp.degree() <= 0) return out;
    // 严格超过 Cauchy 界 一点, 保证端点不是根
    Fraction R = cauchyBound(sp) + Fraction(1);
    Fraction A = Fraction(0) - R;
    Fraction B = R;
    std::vector<Poly> seq = sturmSequence(sp);
    // 总根数 = sigma(A) - sigma(B)
    isolateImpl_(sp, seq, A, B, tol, out, 0);
    std::sort(out.begin(), out.end(),
              [](const std::pair<Fraction, Fraction>& x,
                 const std::pair<Fraction, Fraction>& y) { return x.first < y.first; });
    return out;
}

// 代数数加法和乘法

// h(x) = Res_y(f(y), g(x - y)), N+1 点插值
// 度数上界: deg f * deg g
Poly sumPoly(const Poly& fAlpha, const Poly& gBeta) {
    if (fAlpha.isZero() || gBeta.isZero()) throw std::domain_error("sumPoly: zero polynomial");
    const int m = fAlpha.degree();
    const int n = gBeta.degree();
    const int N = m * n;
    if (N <= 0) return Poly(Fraction(1));  // 至少一方为常数, 退化

    std::vector<Fraction> xs;
    std::vector<Fraction> ys;
    xs.reserve(static_cast<std::size_t>(N + 1));
    ys.reserve(static_cast<std::size_t>(N + 1));

    for (int k = 0; k <= N; ++k) {
        Fraction xi(k);
        // h(y) = g(xi - y):  shift(xi) -> g(y + xi), scale(-1) -> g(-y + xi) = g(xi - y)
        Poly gi = gBeta.shift(xi).scale(Fraction(-1));
        Fraction v = resultantImpl_(fAlpha, gi);
        xs.push_back(xi);
        ys.push_back(v);
    }
    return lagrangeInterp_(xs, ys);
}


static Poly gOfXiOverY_timesYn_(const Poly& g, const Fraction& xi) {
    // h(y) = sum_{k=0..n} c_k * xi^k * y^{n-k}
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

// f(α) 的零化多项式: Res_x(g(x), y - f(x))
// 通过 (deg g) + 1 个 y 点 Lagrange 插值
// y 中的度数 = deg g (Res_x 的 y-度 = deg_x g)
Poly minPolyOfEval(const Poly& g, const Poly& f) {
    if (g.isZero() || g.degree() == 0) {
        throw std::domain_error("minPolyOfEval: g must have degree >= 1");
    }
    const int d = g.degree();
    // 极小情形: f = 0 -> β = 0 -> y 有根 y=0
    if (f.isZero()) return Poly::x();
    // f = const -> β = const -> (y - const)
    if (f.degree() == 0) {
        return Poly::x() - Poly(f[0]);
    }
    std::vector<Fraction> xs;
    std::vector<Fraction> ys;
    xs.reserve(static_cast<std::size_t>(d + 1));
    ys.reserve(static_cast<std::size_t>(d + 1));
    for (int k = 0; k <= d; ++k) {
        Fraction yi(k);
        // r(x) = yi - f(x)
        Poly r = Poly(yi) - f;
        Fraction v = resultantImpl_(g, r);
        xs.push_back(yi);
        ys.push_back(v);
    }
    return lagrangeInterp_(xs, ys);
}

}
