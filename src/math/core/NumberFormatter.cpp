#include "NumberFormatter.h"

#include "BigInt.h"

#include <algorithm>
#include <climits>
#include <cmath>
#include <cstdlib>
#include <stdexcept>
#include <unordered_map>

namespace algemate::math {

namespace {

BigInt pow10_(unsigned int e) {
    return BigInt::pow(BigInt(10), e);
}

bool allDigits_(const std::string& x) {
    for (char c : x) {
        if (c < '0' || c > '9') return false;
    }
    return true;
}

std::string trim_(const std::string& s) {
    size_t lo = s.find_first_not_of(" \t\n\r");
    size_t hi = s.find_last_not_of(" \t\n\r");
    if (lo == std::string::npos) return "";
    return s.substr(lo, hi - lo + 1);
}

}

std::string NumberFormatter::toDecimal(const Fraction& f, int precision, RoundMode mode) {
    if (precision < 0) precision = 0;

    const BigInt& num = f.numerator();
    const BigInt& den = f.denominator();
    bool neg = num.isNegative();
    BigInt absN = num.abs();

    BigInt intPart = absN / den;
    BigInt rem = absN % den;

    std::string frac;
    frac.reserve(static_cast<size_t>(precision) + 1);
    const BigInt ten(10);
    for (int i = 0; i < precision + 1; ++i) {
        rem = rem * ten;
        BigInt d = rem / den;
        rem = rem % den;
        frac.push_back(static_cast<char>('0' + d.toLongLong()));
    }

    int guard = frac.back() - '0';
    frac.pop_back();

    bool hasMoreTail = (guard > 0) || !rem.isZero();
    bool roundUp = false;
    switch (mode) {
        case RoundMode::Truncate: roundUp = false; break;
        case RoundMode::HalfUp:   roundUp = (guard >= 5); break;
        case RoundMode::Floor:    roundUp = neg && hasMoreTail; break;
        case RoundMode::Ceil:     roundUp = !neg && hasMoreTail; break;
    }

    if (roundUp) {
        int i = static_cast<int>(frac.size()) - 1;
        while (i >= 0 && frac[i] == '9') {
            frac[i] = '0';
            --i;
        }
        if (i >= 0) {
            frac[i] = static_cast<char>(frac[i] + 1);
        } else {
            intPart = intPart + BigInt(1);
        }
    }

    bool isZeroResult = intPart.isZero();
    if (isZeroResult) {
        for (char c : frac) {
            if (c != '0') { isZeroResult = false; break; }
        }
    }

    std::string out;
    if (neg && !isZeroResult) out.push_back('-');
    out += intPart.toString();
    if (precision > 0) {
        out.push_back('.');
        out += frac;
    }
    return out;
}

std::string NumberFormatter::toRepeatingDecimal(const Fraction& f, int maxPeriod) {
    const BigInt& num = f.numerator();
    const BigInt& den = f.denominator();
    bool neg = num.isNegative();
    BigInt absN = num.abs();

    BigInt intPart = absN / den;
    BigInt rem = absN % den;

    if (rem.isZero()) {
        std::string out;
        if (neg && !intPart.isZero()) out.push_back('-');
        out += intPart.toString();
        return out;
    }

    std::string digits;
    std::unordered_map<std::string, int> seen;
    int loopStart = -1;
    int i = 0;
    const BigInt ten(10);
    const int safetyLimit = std::max(maxPeriod * 4, 1024);

    while (!rem.isZero()) {
        std::string key = rem.toString();
        auto it = seen.find(key);
        if (it != seen.end()) {
            loopStart = it->second;
            break;
        }
        seen[key] = i;
        rem = rem * ten;
        BigInt d = rem / den;
        rem = rem % den;
        digits.push_back(static_cast<char>('0' + d.toLongLong()));
        ++i;
        if (i > safetyLimit) {
            return toDecimal(f, maxPeriod, RoundMode::HalfUp);
        }
    }

    std::string out;
    if (neg) out.push_back('-');
    out += intPart.toString();

    if (loopStart < 0) {
        out.push_back('.');
        out += digits;
        return out;
    }

    int periodLen = static_cast<int>(digits.size()) - loopStart;
    if (periodLen > maxPeriod) {
        return toDecimal(f, maxPeriod, RoundMode::HalfUp);
    }

    out.push_back('.');
    out.append(digits, 0, static_cast<size_t>(loopStart));
    out.push_back('(');
    out.append(digits, static_cast<size_t>(loopStart), std::string::npos);
    out.push_back(')');
    return out;
}

Fraction NumberFormatter::fromDecimal(const std::string& s) {
    std::string t = trim_(s);
    if (t.empty()) throw std::invalid_argument("NumberFormatter: empty string");

    bool neg = false;
    size_t p = 0;
    if (t[p] == '+' || t[p] == '-') {
        neg = (t[p] == '-');
        ++p;
    }
    if (p >= t.size()) throw std::invalid_argument("NumberFormatter: no digits");

    size_t eIdx = std::string::npos;
    for (size_t k = p; k < t.size(); ++k) {
        if (t[k] == 'e' || t[k] == 'E') { eIdx = k; break; }
    }

    long long exp = 0;
    std::string mant;
    if (eIdx == std::string::npos) {
        mant = t.substr(p);
    } else {
        mant = t.substr(p, eIdx - p);
        std::string es = t.substr(eIdx + 1);
        if (es.empty()) throw std::invalid_argument("NumberFormatter: bad exponent");
        try {
            exp = std::stoll(es);
        } catch (...) {
            throw std::invalid_argument("NumberFormatter: bad exponent");
        }
    }

    size_t lp = mant.find('(');
    size_t rp = mant.find(')');
    std::string mainPart = mant;
    std::string fracB;
    if (lp != std::string::npos) {
        if (rp == std::string::npos || rp < lp)
            throw std::invalid_argument("NumberFormatter: unmatched parenthesis");
        mainPart = mant.substr(0, lp);
        fracB = mant.substr(lp + 1, rp - lp - 1);
    }

    std::string intS;
    std::string fracA;
    size_t dot = mainPart.find('.');
    if (dot == std::string::npos) {
        intS = mainPart;
    } else {
        intS = mainPart.substr(0, dot);
        fracA = mainPart.substr(dot + 1);
    }
    if (intS.empty()) intS = "0";

    if (!allDigits_(intS) || !allDigits_(fracA) || !allDigits_(fracB)) {
        throw std::invalid_argument("NumberFormatter: invalid digit");
    }
    if (intS == "0" && fracA.empty() && fracB.empty() && eIdx == std::string::npos) {
        return Fraction(BigInt(0), BigInt(1));
    }

    int a = static_cast<int>(fracA.size());
    int b = static_cast<int>(fracB.size());

    BigInt num;
    BigInt den;
    if (b == 0) {
        num = BigInt::fromString(intS + fracA);
        den = pow10_(static_cast<unsigned>(a));
    } else {
        BigInt fullN = BigInt::fromString(intS + fracA + fracB);
        BigInt nonRepN = BigInt::fromString(intS + fracA);
        num = fullN - nonRepN;
        den = pow10_(static_cast<unsigned>(a + b)) - pow10_(static_cast<unsigned>(a));
    }

    if (exp > 0) {
        num = num * pow10_(static_cast<unsigned>(exp));
    } else if (exp < 0) {
        den = den * pow10_(static_cast<unsigned>(-exp));
    }

    if (neg) num = -num;
    return Fraction(num, den);
}

Fraction NumberFormatter::fromDouble(double v, long long maxDenominator) {
    if (std::isnan(v)) throw std::invalid_argument("NumberFormatter: NaN");
    if (std::isinf(v)) throw std::invalid_argument("NumberFormatter: Inf");
    if (maxDenominator < 1) maxDenominator = 1;

    bool neg = v < 0.0;
    double x = std::fabs(v);

    long long p0 = 0, q0 = 1;
    long long p1 = 1, q1 = 0;
    const int maxIter = 64;
    const double eps = 1e-15;

    for (int i = 0; i < maxIter; ++i) {
        double af = std::floor(x);
        if (af > static_cast<double>(LLONG_MAX / 2)) break;
        long long a = static_cast<long long>(af);

        long long p2, q2;
        if (a != 0 && (p1 > (LLONG_MAX - p0) / a || q1 > (LLONG_MAX - q0) / a)) break;
        p2 = a * p1 + p0;
        q2 = a * q1 + q0;
        if (q2 > maxDenominator) break;

        p0 = p1; p1 = p2;
        q0 = q1; q1 = q2;

        double frac = x - af;
        if (frac < eps) break;
        x = 1.0 / frac;
    }

    if (q1 == 0) q1 = 1;
    BigInt num(neg ? -p1 : p1);
    BigInt den(q1);
    return Fraction(num, den);
}

}
