#include "BigInt.h"

#include <algorithm>
#include <cassert>
#include <climits>
#include <ostream>
#include <stdexcept>
#include <utility>

namespace algemate::math {

namespace {
constexpr uint64_t kBase = 1ULL << 32;
}

BigInt::BigInt() = default;

BigInt::BigInt(long long v) {
    if (v < 0) {
        negative_ = true;
        setFromUnsigned_(static_cast<unsigned long long>(-(v + 1)) + 1ULL);
    } else {
        setFromUnsigned_(static_cast<unsigned long long>(v));
    }
    normalize_();
}

BigInt::BigInt(const std::string& s) { *this = fromString(s); }

void BigInt::setFromUnsigned_(unsigned long long v) {
    limbs_.clear();
    while (v != 0) {
        limbs_.push_back(static_cast<uint32_t>(v & 0xffffffffULL));
        v >>= 32;
    }
}

BigInt BigInt::fromString(const std::string& s) {
    if (s.empty()) throw std::invalid_argument("BigInt: empty string");
    BigInt result;
    std::size_t i = 0;
    bool neg = false;
    if (s[0] == '+' || s[0] == '-') {
        neg = (s[0] == '-');
        i = 1;
    }
    if (i >= s.size()) throw std::invalid_argument("BigInt: no digits");
    for (; i < s.size(); ++i) {
        char c = s[i];
        if (c < '0' || c > '9') throw std::invalid_argument("BigInt: invalid char");
        uint64_t carry = static_cast<uint64_t>(c - '0');
        for (auto& limb : result.limbs_) {
            uint64_t v = static_cast<uint64_t>(limb) * 10ULL + carry;
            limb = static_cast<uint32_t>(v & 0xffffffffULL);
            carry = v >> 32;
        }
        while (carry != 0) {
            result.limbs_.push_back(static_cast<uint32_t>(carry & 0xffffffffULL));
            carry >>= 32;
        }
    }
    result.negative_ = neg;
    result.normalize_();
    return result;
}

void BigInt::normalize_() {
    while (!limbs_.empty() && limbs_.back() == 0) limbs_.pop_back();
    if (limbs_.empty()) negative_ = false;
}

int BigInt::absCompare_(const BigInt& a, const BigInt& b) {
    if (a.limbs_.size() != b.limbs_.size())
        return a.limbs_.size() < b.limbs_.size() ? -1 : 1;
    for (std::size_t i = a.limbs_.size(); i-- > 0;) {
        if (a.limbs_[i] != b.limbs_[i]) return a.limbs_[i] < b.limbs_[i] ? -1 : 1;
    }
    return 0;
}

BigInt BigInt::absAdd_(const BigInt& a, const BigInt& b) {
    BigInt out;
    const std::size_t n = std::max(a.limbs_.size(), b.limbs_.size());
    out.limbs_.resize(n, 0);
    uint64_t carry = 0;
    for (std::size_t i = 0; i < n; ++i) {
        uint64_t x = i < a.limbs_.size() ? a.limbs_[i] : 0;
        uint64_t y = i < b.limbs_.size() ? b.limbs_[i] : 0;
        uint64_t sum = x + y + carry;
        out.limbs_[i] = static_cast<uint32_t>(sum & 0xffffffffULL);
        carry = sum >> 32;
    }
    if (carry) out.limbs_.push_back(static_cast<uint32_t>(carry));
    return out;
}

BigInt BigInt::absSub_(const BigInt& a, const BigInt& b) {
    BigInt out;
    out.limbs_.resize(a.limbs_.size(), 0);
    int64_t borrow = 0;
    for (std::size_t i = 0; i < a.limbs_.size(); ++i) {
        int64_t x = static_cast<int64_t>(a.limbs_[i]);
        int64_t y = i < b.limbs_.size() ? static_cast<int64_t>(b.limbs_[i]) : 0;
        int64_t diff = x - y - borrow;
        if (diff < 0) {
            diff += static_cast<int64_t>(kBase);
            borrow = 1;
        } else {
            borrow = 0;
        }
        out.limbs_[i] = static_cast<uint32_t>(diff & 0xffffffffULL);
    }
    out.normalize_();
    return out;
}

BigInt BigInt::absMul_(const BigInt& a, const BigInt& b) {
    BigInt out;
    if (a.limbs_.empty() || b.limbs_.empty()) return out;
    out.limbs_.assign(a.limbs_.size() + b.limbs_.size(), 0);
    for (std::size_t i = 0; i < a.limbs_.size(); ++i) {
        uint64_t carry = 0;
        uint64_t x = a.limbs_[i];
        for (std::size_t j = 0; j < b.limbs_.size(); ++j) {
            uint64_t cur = static_cast<uint64_t>(out.limbs_[i + j]) + x * b.limbs_[j] + carry;
            out.limbs_[i + j] = static_cast<uint32_t>(cur & 0xffffffffULL);
            carry = cur >> 32;
        }
        if (carry) out.limbs_[i + b.limbs_.size()] += static_cast<uint32_t>(carry);
    }
    out.normalize_();
    return out;
}

bool BigInt::bitAt_(std::size_t pos) const {
    std::size_t idx = pos >> 5;
    if (idx >= limbs_.size()) return false;
    return ((limbs_[idx] >> (pos & 31)) & 1u) != 0;
}

void BigInt::setBit_(std::size_t pos) {
    std::size_t idx = pos >> 5;
    if (idx >= limbs_.size()) limbs_.resize(idx + 1, 0);
    limbs_[idx] |= 1u << (pos & 31);
}

std::size_t BigInt::bitLength_() const {
    if (limbs_.empty()) return 0;
    uint32_t top = limbs_.back();
    std::size_t n = (limbs_.size() - 1) * 32;
    while (top) {
        ++n;
        top >>= 1;
    }
    return n;
}

void BigInt::shiftLeftOneBit_() {
    uint32_t carry = 0;
    for (auto& limb : limbs_) {
        uint32_t nc = limb >> 31;
        limb = (limb << 1) | carry;
        carry = nc;
    }
    if (carry) limbs_.push_back(carry);
}

void BigInt::absDivMod_(const BigInt& a, const BigInt& b, BigInt& q, BigInt& r) {
    assert(!b.limbs_.empty());
    q.limbs_.clear();
    r.limbs_.clear();
    q.negative_ = false;
    r.negative_ = false;
    if (absCompare_(a, b) < 0) {
        r = a;
        r.negative_ = false;
        return;
    }
    const std::size_t bits = a.bitLength_();
    for (std::size_t i = bits; i-- > 0;) {
        r.shiftLeftOneBit_();
        if (a.bitAt_(i)) {
            if (r.limbs_.empty()) r.limbs_.push_back(0);
            r.limbs_[0] |= 1u;
        }
        if (absCompare_(r, b) >= 0) {
            r = absSub_(r, b);
            q.setBit_(i);
        }
    }
    q.normalize_();
    r.normalize_();
}

BigInt BigInt::operator-() const {
    BigInt r = *this;
    if (!r.limbs_.empty()) r.negative_ = !r.negative_;
    return r;
}

BigInt BigInt::operator+(const BigInt& r) const {
    BigInt out;
    if (negative_ == r.negative_) {
        out = absAdd_(*this, r);
        out.negative_ = negative_;
    } else {
        int c = absCompare_(*this, r);
        if (c == 0) return BigInt();
        if (c > 0) {
            out = absSub_(*this, r);
            out.negative_ = negative_;
        } else {
            out = absSub_(r, *this);
            out.negative_ = r.negative_;
        }
    }
    out.normalize_();
    return out;
}

BigInt BigInt::operator-(const BigInt& r) const { return *this + (-r); }

BigInt BigInt::operator*(const BigInt& r) const {
    BigInt out = absMul_(*this, r);
    out.negative_ = (negative_ != r.negative_) && !out.limbs_.empty();
    return out;
}

BigInt BigInt::operator/(const BigInt& r) const {
    if (r.isZero()) throw std::domain_error("BigInt: divide by zero");
    BigInt q, rem;
    absDivMod_(*this, r, q, rem);
    q.negative_ = (negative_ != r.negative_) && !q.limbs_.empty();
    return q;
}

BigInt BigInt::operator%(const BigInt& r) const {
    if (r.isZero()) throw std::domain_error("BigInt: divide by zero");
    BigInt q, rem;
    absDivMod_(*this, r, q, rem);
    rem.negative_ = negative_ && !rem.limbs_.empty();
    return rem;
}

BigInt& BigInt::operator+=(const BigInt& r) { return *this = *this + r; }
BigInt& BigInt::operator-=(const BigInt& r) { return *this = *this - r; }
BigInt& BigInt::operator*=(const BigInt& r) { return *this = *this * r; }
BigInt& BigInt::operator/=(const BigInt& r) { return *this = *this / r; }
BigInt& BigInt::operator%=(const BigInt& r) { return *this = *this % r; }

BigInt& BigInt::operator++() { return *this += BigInt(1); }
BigInt  BigInt::operator++(int) { BigInt t = *this; ++*this; return t; }
BigInt& BigInt::operator--() { return *this -= BigInt(1); }
BigInt  BigInt::operator--(int) { BigInt t = *this; --*this; return t; }

bool BigInt::operator==(const BigInt& r) const {
    return negative_ == r.negative_ && limbs_ == r.limbs_;
}
bool BigInt::operator!=(const BigInt& r) const { return !(*this == r); }
bool BigInt::operator<(const BigInt& r) const {
    if (negative_ != r.negative_) return negative_;
    int c = absCompare_(*this, r);
    return negative_ ? c > 0 : c < 0;
}
bool BigInt::operator<=(const BigInt& r) const { return !(r < *this); }
bool BigInt::operator>(const BigInt& r) const { return r < *this; }
bool BigInt::operator>=(const BigInt& r) const { return !(*this < r); }

bool BigInt::isZero() const { return limbs_.empty(); }
bool BigInt::isOne() const { return !negative_ && limbs_.size() == 1 && limbs_[0] == 1; }
bool BigInt::isNegative() const { return negative_; }
bool BigInt::isPositive() const { return !negative_ && !limbs_.empty(); }
int  BigInt::sign() const { return limbs_.empty() ? 0 : (negative_ ? -1 : 1); }

BigInt BigInt::abs() const {
    BigInt r = *this;
    r.negative_ = false;
    return r;
}

BigInt BigInt::negate() const { return -*this; }

std::string BigInt::toString() const {
    if (limbs_.empty()) return "0";
    BigInt tmp = abs();
    std::string out;
    while (!tmp.limbs_.empty()) {
        uint64_t rem = 0;
        for (std::size_t i = tmp.limbs_.size(); i-- > 0;) {
            uint64_t cur = (rem << 32) | tmp.limbs_[i];
            tmp.limbs_[i] = static_cast<uint32_t>(cur / 10ULL);
            rem = cur % 10ULL;
        }
        out.push_back(static_cast<char>('0' + rem));
        tmp.normalize_();
    }
    if (negative_) out.push_back('-');
    std::reverse(out.begin(), out.end());
    return out;
}

long long BigInt::toLongLong() const {
    if (limbs_.empty()) return 0;
    uint64_t u = 0;
    if (limbs_.size() >= 1) u |= limbs_[0];
    if (limbs_.size() >= 2) u |= static_cast<uint64_t>(limbs_[1]) << 32;
    if (negative_) {
        if (limbs_.size() > 2 || u > static_cast<uint64_t>(LLONG_MAX) + 1ULL) return LLONG_MIN;
        if (u == static_cast<uint64_t>(LLONG_MAX) + 1ULL) return LLONG_MIN;
        return -static_cast<long long>(u);
    }
    if (limbs_.size() > 2 || u > static_cast<uint64_t>(LLONG_MAX)) return LLONG_MAX;
    return static_cast<long long>(u);
}

double BigInt::toDouble() const {
    double v = 0.0;
    for (std::size_t i = limbs_.size(); i-- > 0;) {
        v = v * static_cast<double>(kBase) + static_cast<double>(limbs_[i]);
    }
    return negative_ ? -v : v;
}

BigInt BigInt::gcd(BigInt a, BigInt b) {
    a.negative_ = false;
    b.negative_ = false;
    while (!b.isZero()) {
        BigInt t = a % b;
        a = std::move(b);
        b = std::move(t);
    }
    return a;
}

BigInt BigInt::pow(BigInt base, unsigned int exponent) {
    BigInt result(1);
    while (exponent != 0) {
        if (exponent & 1u) result *= base;
        exponent >>= 1u;
        if (exponent != 0) base *= base;
    }
    return result;
}

std::ostream& operator<<(std::ostream& os, const BigInt& v) { return os << v.toString(); }

}
