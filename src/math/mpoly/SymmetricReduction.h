// SymmetricReduction.h — 对称多项式化为初等对称多项式的字典序降次法
#pragma once

#include "MPolynomial.h"
#include "core/Fraction.h"

#include <algorithm>
#include <functional>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <vector>

namespace algemate::math::mpoly {

using Fraction = algemate::math::Fraction;

// 解析器
inline MPolynomial parseSymmetricPoly(const std::string& s, int n) {
    MPolynomial result;
    std::string t = s;
    t.erase(std::remove(t.begin(), t.end(), ' '), t.end());
    if (t.empty()) return result;

    std::size_t i = 0;
    while (i < t.size()) {
        int sign = 1;
        if (t[i] == '+') ++i;
        else if (t[i] == '-') { ++i; sign = -1; }
        if (i >= t.size()) break;

        Fraction coef(sign);
        if (std::isdigit(t[i])) {
            std::size_t start = i;
            while (i < t.size() && (std::isdigit(t[i]) || t[i] == '/')) ++i;
            std::string ns = t.substr(start, i - start);
            auto sl = ns.find('/');
            if (sl != std::string::npos)
                coef = Fraction(BigInt(ns.substr(0,sl)), BigInt(ns.substr(sl+1)));
            else
                coef = Fraction(BigInt(std::stoll(ns)));
            if (sign < 0) coef = -coef;
        }

        std::vector<int> exps(n, 0);
        bool hasVar = false;
        while (i < t.size() && t[i] == 'x') {
            ++i;
            std::size_t start = i;
            while (i < t.size() && std::isdigit(t[i])) ++i;
            if (i == start) break;
            int idx = std::stoi(t.substr(start, i - start)) - 1;
            if (idx < 0 || idx >= n) break;
            int exp = 1;
            if (i < t.size() && t[i] == '^') {
                ++i; start = i;
                while (i < t.size() && std::isdigit(t[i])) ++i;
                exp = std::stoi(t.substr(start, i - start));
            }
            exps[idx] += exp;
            hasVar = true;
            if (i < t.size() && t[i] == '*') ++i;
        }

        if (!hasVar) {
            if (coef.isZero()) break;
            result = result + MPolynomial(coef, Monomial(std::vector<int>(n, 0)));
        } else {
            result = result + MPolynomial(coef, Monomial(std::move(exps)));
        }
        while (i < t.size() && t[i] != '+' && t[i] != '-') ++i;
    }
    return result;
}

// 对称性检测, 自动补全
// 返回: 0=不对称, 1=已对称, 2=已自动补全
inline int ensureSymmetric(MPolynomial& poly, int n, std::string& note) {
    // 收集所有项及其系数
    std::map<std::multiset<int>, Fraction> orbitCoef;

    for (const auto& [m, c] : poly.terms()) {
        std::multiset<int> key(m.exps_.begin(), m.exps_.end());
        // pad with zeros to size n
        while (key.size() < static_cast<std::size_t>(n)) key.insert(0);
        auto it = orbitCoef.find(key);
        if (it != orbitCoef.end()) {
            if (it->second != c) { note = "系数冲突: 轨道内出现不同系数"; return 0; }
        } else {
            orbitCoef[key] = c;
        }
    }

    // 检查每个轨道是否完整
    int added = 0;
    MPolynomial sym;
    for (auto& [key, c] : orbitCoef) {
        std::vector<int> pattern(key.begin(), key.end());
        // pattern is sorted ascending (multiset order). Generate all distinct permutations.
        std::set<std::vector<int>> seen;
        std::sort(pattern.begin(), pattern.end());
        do {
            if (seen.insert(pattern).second) {
                bool found = false;
                for (const auto& [m2, c2] : poly.terms()) {
                    if (std::vector<int>(m2.exps_.begin(), m2.exps_.end()) == pattern) {
                        found = true; break;
                    }
                }
                if (!found) ++added;
                sym = sym + MPolynomial(c, Monomial(pattern));
            }
        } while (std::next_permutation(pattern.begin(), pattern.end()));
    }

    if (added > 0) {
        poly = sym;
        note = "已自动补全 " + std::to_string(added) + " 项";
        return 2;
    }
    return 1;
}

// 字典序降次法: f(x) → g(σ)
struct ReductionStep {
    MPolynomial f;            // 当前多项式 (x_i 表示)
    std::vector<int> leadExp; // 首项指数组
    std::string phiStr;       // φ_k 的 LaTeX 表示
    std::string fNextStr;     // f_{k+1} 的 LaTeX 表示
};

inline std::vector<ReductionStep> reduceSymmetric(const MPolynomial& f, int n) {
    std::vector<ReductionStep> steps;
    MPolynomial current = f;

    while (!current.isZero()) {
        ReductionStep step;
        step.f = current;

        const auto& lm = current.leadingMonomial();
        step.leadExp = std::vector<int>(n, 0);
        for (std::size_t i = 0; i < lm.vars() && i < static_cast<std::size_t>(n); ++i)
            step.leadExp[i] = lm[i];
        std::vector<int> lead(step.leadExp);
        std::sort(lead.begin(), lead.end(), std::greater<int>());

        MPolynomial phi(Fraction(1));
        std::ostringstream phiLatex;
        bool first = true;
        for (int i = 0; i < n; ++i) {
            int exp = lead[i] - (i + 1 < n ? lead[i + 1] : 0);
            if (exp > 0) {
                if (!first) phiLatex << " ";
                phiLatex << "\\sigma_{" << (i + 1) << "}^{" << exp << "}";
                first = false;
                std::vector<int> sigmaExp(n, 0);
                sigmaExp[i] = 1;
                Monomial sigmaM(std::move(sigmaExp));
                for (int e = 0; e < exp; ++e)
                    phi = phi * MPolynomial(Fraction(1), sigmaM);
            }
        }
        MPolynomial phiX(Fraction(1));
        for (int i = 0; i < n; ++i) {
            int exp = lead[i] - (i + 1 < n ? lead[i + 1] : 0);
            if (exp > 0) {
                MPolynomial sigmaI;
                std::vector<int> mask(n, 0);
                for (int j = 0; j <= i; ++j) mask[n - 1 - j] = 1;
                do {
                    std::vector<int> exps(n, 0);
                    for (int j = 0; j < n; ++j)
                        if (mask[j]) exps[j] = 1;
                    sigmaI = sigmaI + MPolynomial(Fraction(1), Monomial(exps));
                } while (std::next_permutation(mask.begin(), mask.end()));

                for (int e = 0; e < exp; ++e)
                    phiX = phiX * sigmaI;
            }
        }

        Fraction lc = current.leadingCoefficient();
        std::ostringstream fullPhi;
        if (lc == Fraction(-1))
            fullPhi << "-";
        else if (!lc.isOne())
            fullPhi << lc.toLatex() << " ";
        fullPhi << phiLatex.str();
        step.phiStr = fullPhi.str();

        // f_{k+1} = f_k - lc · φ
        MPolynomial fNext = current - phiX * lc;
        step.fNextStr.clear();

        steps.push_back(step);
        current = fNext;
    }
    return steps;
}

// 把 "x1^2x2^2" 转为 LaTeX "x_{1}^{2}x_{2}^{2}"
inline std::string patternToLatex(const std::string& pattern) {
    std::string r;
    for (std::size_t i = 0; i < pattern.size(); ++i) {
        if (pattern[i] == 'x' && i+1 < pattern.size() && std::isdigit(pattern[i+1])) {
            r += "x_{";
            ++i;
            while (i < pattern.size() && std::isdigit(pattern[i])) { r += pattern[i]; ++i; }
            r += "}";
            --i; // adjust for loop increment
        } else if (pattern[i] == '^') {
            r += "^{";
            ++i;
            while (i < pattern.size() && std::isdigit(pattern[i])) { r += pattern[i]; ++i; }
            r += "}";
            --i;
        } else {
            r += pattern[i];
        }
    }
    return r;
}

// 待定系数法
struct GeneralResult {
    std::vector<std::vector<int>> patterns;  // 所有可能的指数组
    std::vector<Fraction> coefficients;       // 对应系数
    std::vector<std::string> sigmaExprs;      // σ 表达式 (LaTeX)
    std::string finalExpr;                    // 最终结果
};

// 从模式 "x1^2x2^2" 解析出指数并扩展为对称和
inline MPolynomial expandPattern(const std::string& pattern, int n) {
    auto poly = parseSymmetricPoly(pattern, n);
    if (poly.isZero()) return poly;

    const auto& lm = poly.leadingMonomial();
    std::vector<int> lead;
    for (std::size_t i = 0; i < static_cast<std::size_t>(n); ++i)
        lead.push_back(lm[i]);
    std::sort(lead.begin(), lead.end(), std::greater<int>());

    Fraction lc = poly.leadingCoefficient();
    MPolynomial result;
    std::vector<int> perm = lead;
    std::set<std::vector<int>> seen;
    do {
        if (seen.insert(perm).second)
            result = result + MPolynomial(lc, Monomial(perm));
    } while (std::next_permutation(perm.begin(), perm.end()));
    return result;
}


inline std::vector<std::vector<int>> enumPatterns(int n, int d, const std::vector<int>& lead) {
    std::vector<std::vector<int>> result;
    std::vector<int> cur(n, 0);
    std::function<void(int, int, int)> dfs = [&](int pos, int remaining, int prev) {
        if (pos == n) {
            if (remaining == 0 && cur <= lead) result.push_back(cur);
            return;
        }
        int maxVal = std::min(remaining, prev);
        if (pos == 0) maxVal = std::min(maxVal, lead[0]);
        for (int v = maxVal; v >= 0; --v) {
            // Early prune: if cur[0..pos] > lead[0..pos], skip
            cur[pos] = v;
            if (pos == 0 && v > lead[0]) continue;
            if (pos > 0) {
                bool skip = false;
                for (int k = 0; k <= pos; ++k) {
                    if (cur[k] > lead[k]) { skip = true; break; }
                    if (cur[k] < lead[k]) break; // cur is already behind, OK
                }
                if (skip) continue;
            }
            dfs(pos + 1, remaining - v, v);
        }
    };
    dfs(0, d, lead[0]);
    return result;
}

// 待定系数法
inline long long binom(int n, int k) {
    if (k < 0 || k > n) return 0;
    if (k == 0 || k == n) return 1;
    if (k > n - k) k = n - k;
    long long r = 1;
    for (int i = 0; i < k; ++i) r = r * (n - i) / (i + 1);
    return r;
}


inline Fraction evalSymmetricSum(const std::vector<int>& lead, int k) {
    int m = 0;
    for (int v : lead) if (v > 0) ++m;
    if (k < m) return Fraction(0);

    std::map<int, int> mult;
    for (int v : lead) if (v > 0) ++mult[v];

    long long num = 1;
    for (int i = 0; i < m; ++i) num *= (k - i);
    for (auto& [_, cnt] : mult)
        for (int i = 2; i <= cnt; ++i) num /= i;
    return Fraction(num);
}

inline GeneralResult generalSymmetricReduction(const std::string& pattern, int n) {
    GeneralResult r;
    auto poly = parseSymmetricPoly(pattern, n);
    if (poly.isZero()) return r;

    const auto& lm = poly.leadingMonomial();
    std::vector<int> lead;
    for (int i = 0; i < n; ++i) lead.push_back(lm[i]);
    std::sort(lead.begin(), lead.end(), std::greater<int>());

    int totalDeg = 0;
    for (int v : lead) totalDeg += v;

    auto patterns = enumPatterns(n, totalDeg, lead);
    int s = static_cast<int>(patterns.size());

    r.patterns = patterns;
    r.coefficients.resize(s);
    r.sigmaExprs.resize(s);

    for (int pi = 0; pi < s; ++pi) {
        std::ostringstream os;
        bool first = true;
        for (int i = 0; i < n; ++i) {
            int exp = patterns[pi][i] - (i + 1 < n ? patterns[pi][i + 1] : 0);
            if (exp > 0) {
                if (!first) os << " ";
                os << "\\sigma_{" << (i + 1) << "}^{" << exp << "}";
                first = false;
            }
        }
        r.sigmaExprs[pi] = os.str();
    }

    int m = 0;
    for (int v : lead) if (v > 0) ++m;
    std::vector<std::vector<Fraction>> A(s, std::vector<Fraction>(s));
    std::vector<Fraction> b(s);

    for (int eq = 0; eq < s; ++eq) {
        int ones = m + eq;
        b[eq] = evalSymmetricSum(lead, ones);

        for (int pi = 0; pi < s; ++pi) {
            Fraction val(1);
            for (int i = 0; i < n; ++i) {
                int exp = patterns[pi][i] - (i + 1 < n ? patterns[pi][i + 1] : 0);
                if (exp > 0) {
                    int kk = i + 1;
                    if (kk > ones) { val = Fraction(0); break; }
                    long long cb = binom(ones, kk);
                    for (int e = 0; e < exp; ++e) val = val * Fraction(cb);
                }
            }
            A[eq][pi] = val;
        }
    }

    for (int col = 0; col < s; ++col) {
        int pivot = col;
        while (pivot < s && A[pivot][col].isZero()) ++pivot;
        if (pivot == s) continue;
        if (pivot != col) { std::swap(A[col], A[pivot]); std::swap(b[col], b[pivot]); }
        Fraction piv = A[col][col];
        for (int j = col; j < s; ++j) A[col][j] = A[col][j] / piv;
        b[col] = b[col] / piv;
        for (int row = 0; row < s; ++row) {
            if (row == col) continue;
            Fraction f2 = A[row][col];
            if (f2.isZero()) continue;
            for (int j = col; j < s; ++j) A[row][j] = A[row][j] - f2 * A[col][j];
            b[row] = b[row] - f2 * b[col];
        }
    }
    for (int i = 0; i < s; ++i) r.coefficients[i] = b[i];

    std::ostringstream finalExpr;
    bool first = true;
    for (int pi = 0; pi < s; ++pi) {
        Fraction c = r.coefficients[pi];
        if (c.isZero()) continue;
        if (first) {
            if (c.sign() < 0) finalExpr << "-";
            first = false;
        } else {
            finalExpr << (c.sign() < 0 ? " - " : " + ");
        }
        Fraction ac = c.abs();
        if (!ac.isOne()) finalExpr << ac.toLatex() << " ";
        finalExpr << r.sigmaExprs[pi];
    }
    if (first) finalExpr << "0";
    r.finalExpr = finalExpr.str();
    return r;
}

} // namespace algemate::math::mpoly
