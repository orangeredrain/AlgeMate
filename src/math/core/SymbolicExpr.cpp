#include "SymbolicExpr.h"

#include "AlgReal.h"
#include "BigInt.h"
#include "Complex.h"
#include "Fraction.h"
#include "algorithm/PolynomialAlg.h"

#include <algorithm>
#include <cctype>
#include <map>
#include <ostream>
#include <sstream>
#include <stdexcept>
#include <unordered_map>
#include <utility>

namespace algemate::math {

SymbolicExpr::SymbolicExpr() : node_(makeNum_(Complex()).node_) {}
SymbolicExpr::SymbolicExpr(long long n)      : node_(makeNum_(Complex(n)).node_) {}
SymbolicExpr::SymbolicExpr(const Fraction& q): node_(makeNum_(Complex(q)).node_) {}
SymbolicExpr::SymbolicExpr(const AlgReal& a) : node_(makeNum_(Complex(a)).node_) {}
SymbolicExpr::SymbolicExpr(const Complex& c) : node_(makeNum_(c).node_) {}

SymbolicExpr SymbolicExpr::num(const Complex& c)      { return makeNum_(c); }
SymbolicExpr SymbolicExpr::sym(const std::string& s)  { return makeSym_(s); }

SymbolicExpr::Kind SymbolicExpr::kind() const { return node_->kind; }
bool SymbolicExpr::isNum()  const { return node_->kind == Kind::Num; }
bool SymbolicExpr::isSym()  const { return node_->kind == Kind::Sym; }
bool SymbolicExpr::isZero() const { return isNum() && node_->num.isZero(); }
bool SymbolicExpr::isOne()  const {
    return isNum() && node_->num.isReal() && node_->num.real().isRational()
        && node_->num.real().asRational() == Fraction(1);
}
const Complex&     SymbolicExpr::asNum() const { return node_->num; }
const std::string& SymbolicExpr::asSym() const { return node_->sym; }
const std::vector<SymbolicExpr>& SymbolicExpr::children() const { return node_->args; }
SymbolicExpr::FuncKind SymbolicExpr::funcKind() const { return node_->func; }

bool SymbolicExpr::operator==(const SymbolicExpr& r) const {
    if (node_.get() == r.node_.get()) return true;
    if (node_->kind != r.node_->kind) return false;
    switch (node_->kind) {
    case Kind::Num:  return node_->num == r.node_->num;
    case Kind::Sym:  return node_->sym == r.node_->sym;
    case Kind::Func: if (node_->func != r.node_->func) return false; [[fallthrough]];
    case Kind::Add:
    case Kind::Mul:
    case Kind::Pow: {
        if (node_->args.size() != r.node_->args.size()) return false;
        for (std::size_t i = 0; i < node_->args.size(); ++i)
            if (!(node_->args[i] == r.node_->args[i])) return false;
        return true;
    }
    }
    return false;
}

SymbolicExpr SymbolicExpr::makeNum_(const Complex& c) {
    auto n = std::make_shared<Node>();
    n->kind = Kind::Num;
    n->num  = c;
    return SymbolicExpr(n);
}

SymbolicExpr SymbolicExpr::makeSym_(const std::string& s) {
    auto n = std::make_shared<Node>();
    n->kind = Kind::Sym;
    n->sym  = s;
    return SymbolicExpr(n);
}

namespace {

bool extractIntNum(const SymbolicExpr& e, long long& out) {
    if (!e.isNum()) return false;
    const Complex& c = e.asNum();
    if (!c.isReal()) return false;
    const AlgReal& r = c.real();
    if (!r.isRational()) return false;
    Fraction f = r.asRational();
    if (!f.isInteger()) return false;
    out = f.numerator().toLongLong();
    return true;
}

bool extractRatNum(const SymbolicExpr& e, Fraction& out) {
    if (!e.isNum()) return false;
    const Complex& c = e.asNum();
    if (!c.isReal()) return false;
    const AlgReal& r = c.real();
    if (!r.isRational()) return false;
    out = r.asRational();
    return true;
}

Complex complexIntPow(const Complex& c, long long k) {
    if (k < 0) throw std::domain_error("complexIntPow: k must be non-negative");
    Complex result(1LL);
    Complex base = c;
    while (k > 0) {
        if (k & 1) result = result * base;
        base = base * base;
        k >>= 1;
    }
    return result;
}

} 

SymbolicExpr SymbolicExpr::makeAdd_(std::vector<SymbolicExpr> ch) {

    std::vector<SymbolicExpr> flat;
    flat.reserve(ch.size());
    for (auto& c : ch) {
        if (c.kind() == Kind::Add) {
            for (auto& sub : c.children()) flat.push_back(sub);
        } else {
            flat.push_back(std::move(c));
        }
    }

    Complex konst(0LL);
    std::vector<SymbolicExpr> rest;
    rest.reserve(flat.size());
    for (auto& c : flat) {
        if (c.isNum()) konst = konst + c.asNum();
        else           rest.push_back(std::move(c));
    }

    std::vector<std::pair<std::string, std::pair<Complex, SymbolicExpr>>> groups;

    std::unordered_map<std::string, std::size_t> index;

    auto splitCoef = [](const SymbolicExpr& e) -> std::pair<Complex, SymbolicExpr> {
        if (e.kind() == Kind::Mul && !e.children().empty() && e.children()[0].isNum()) {
            std::vector<SymbolicExpr> rest2(e.children().begin() + 1, e.children().end());
            Complex coef = e.children()[0].asNum();
            if (rest2.size() == 1) return {coef, rest2[0]};
            return {coef, SymbolicExpr::makeMul_(std::move(rest2))};
        }
        return {Complex(1LL), e};
    };

    for (auto& c : rest) {
        auto pair   = splitCoef(c);
        std::string key = pair.second.toString();
        auto it = index.find(key);
        if (it == index.end()) {
            index[key] = groups.size();
            groups.push_back({key, pair});
        } else {
            groups[it->second].second.first = groups[it->second].second.first + pair.first;
        }
    }

    std::vector<SymbolicExpr> out;
    out.reserve(groups.size() + 1);
    for (auto& g : groups) {
        const Complex& coef = g.second.first;
        const SymbolicExpr& body = g.second.second;
        if (coef.isZero()) continue;
        if (coef == Complex(1LL)) { out.push_back(body); continue; }
        out.push_back(SymbolicExpr::makeMul_({makeNum_(coef), body}));
    }

    if (!konst.isZero()) out.push_back(makeNum_(konst));

    if (out.empty()) return makeNum_(Complex(0LL));
    if (out.size() == 1) return out[0];

    std::sort(out.begin(), out.end(),
              [](const SymbolicExpr& a, const SymbolicExpr& b) {
                  return a.toString() < b.toString();
              });

    auto n = std::make_shared<Node>();
    n->kind = Kind::Add;
    n->args = std::move(out);
    return SymbolicExpr(n);
}

SymbolicExpr SymbolicExpr::makeMul_(std::vector<SymbolicExpr> ch) {

    std::vector<SymbolicExpr> flat;
    flat.reserve(ch.size());
    for (auto& c : ch) {
        if (c.kind() == Kind::Mul) {
            for (auto& sub : c.children()) flat.push_back(sub);
        } else {
            flat.push_back(std::move(c));
        }
    }

    Complex konst(1LL);
    std::vector<SymbolicExpr> rest;
    rest.reserve(flat.size());
    for (auto& c : flat) {
        if (c.isNum()) konst = konst * c.asNum();
        else           rest.push_back(std::move(c));
    }

    if (konst.isZero()) return makeNum_(Complex(0LL));

    struct Grp { SymbolicExpr base; SymbolicExpr exp; };
    std::vector<Grp> groups;
    std::unordered_map<std::string, std::size_t> index;

    auto splitPow = [](const SymbolicExpr& e) -> std::pair<SymbolicExpr, SymbolicExpr> {
        if (e.kind() == Kind::Pow) return {e.children()[0], e.children()[1]};
        return {e, SymbolicExpr(Complex(1LL))};
    };

    for (auto& c : rest) {
        auto pair   = splitPow(c);
        std::string key = pair.first.toString();
        auto it = index.find(key);
        if (it == index.end()) {
            index[key] = groups.size();
            groups.push_back({pair.first, pair.second});
        } else {
            groups[it->second].exp = groups[it->second].exp + pair.second;
        }
    }

    std::vector<SymbolicExpr> out;
    out.reserve(groups.size() + 1);
    for (auto& g : groups) {
        if (g.exp.isZero()) continue;
        if (g.exp.isOne())  { out.push_back(g.base); continue; }
        out.push_back(makePow_(g.base, g.exp));
    }

    if (!(konst == Complex(1LL))) out.insert(out.begin(), makeNum_(konst));

    if (out.empty()) return makeNum_(Complex(1LL));
    if (out.size() == 1) return out[0];

    auto begin = out.begin();
    if (!out.empty() && out.front().isNum()) ++begin;
    std::sort(begin, out.end(),
              [](const SymbolicExpr& a, const SymbolicExpr& b) {
                  return a.toString() < b.toString();
              });

    auto n = std::make_shared<Node>();
    n->kind = Kind::Mul;
    n->args = std::move(out);
    return SymbolicExpr(n);
}

SymbolicExpr SymbolicExpr::makePow_(SymbolicExpr b, SymbolicExpr e) {

    if (e.isZero()) {
        if (b.isZero()) throw std::domain_error("SymbolicExpr: 0^0 is undefined");
        return makeNum_(Complex(1LL));
    }
    if (e.isOne())  return b;
    if (b.isZero()) {

        long long k;
        Fraction q;
        if (extractIntNum(e, k)) {
            if (k < 0) throw std::domain_error("SymbolicExpr: 0^negative is undefined");
        } else if (extractRatNum(e, q)) {
            if (q.numerator().toLongLong() <= 0)
                throw std::domain_error("SymbolicExpr: 0^non-positive is undefined");
        }
        return makeNum_(Complex(0LL));
    }
    if (b.isOne()) return makeNum_(Complex(1LL));

    if (b.isNum() && e.isNum()) {
        auto isRatCplx = [](const Complex& c) {
            return c.real().isRational() && c.imag().isRational();
        };
        const Complex& bc = b.asNum();
        long long k;
        if (extractIntNum(e, k)) {
            if (k >= 0) return makeNum_(complexIntPow(bc, k));
            Complex inv = Complex(1LL) / bc;
            return makeNum_(complexIntPow(inv, -k));
        }
        Fraction q;
        if (extractRatNum(e, q) && isRatCplx(bc)) {
            long long num = q.numerator().toLongLong();
            long long den = q.denominator().toLongLong();
            if (den >= 2) {
                try {
                    Complex r = Complex::nthRoot(bc, static_cast<int>(den));
                    if (isRatCplx(r)) {
                        if (num >= 0) return makeNum_(complexIntPow(r, num));
                        return makeNum_(complexIntPow(Complex(1LL) / r, -num));
                    }
                } catch (const std::exception&) {  }
            }
        }

    }

    if (b.kind() == Kind::Pow) {
        SymbolicExpr b2 = b.children()[0];
        SymbolicExpr e2 = b.children()[1];
        return makePow_(b2, e2 * e);
    }

    auto n = std::make_shared<Node>();
    n->kind = Kind::Pow;
    n->args = {b, e};
    return SymbolicExpr(n);
}

SymbolicExpr SymbolicExpr::makeFunc_(FuncKind k, SymbolicExpr arg) {
    if (k == FuncKind::Abs && arg.isNum()) {
        AlgReal m = arg.asNum().modulus();
        return makeNum_(Complex(m));
    }
    if (k == FuncKind::Conj && arg.isNum()) {
        return makeNum_(arg.asNum().conjugate());
    }
    auto n = std::make_shared<Node>();
    n->kind = Kind::Func;
    n->func = k;
    n->args = {std::move(arg)};
    return SymbolicExpr(n);
}

SymbolicExpr SymbolicExpr::operator-() const {
    return makeMul_({makeNum_(Complex(-1LL)), *this});
}
SymbolicExpr SymbolicExpr::operator+(const SymbolicExpr& r) const {
    return makeAdd_({*this, r});
}
SymbolicExpr SymbolicExpr::operator-(const SymbolicExpr& r) const {
    return makeAdd_({*this, -r});
}
SymbolicExpr SymbolicExpr::operator*(const SymbolicExpr& r) const {
    return makeMul_({*this, r});
}
SymbolicExpr SymbolicExpr::operator/(const SymbolicExpr& r) const {
    if (r.isZero()) throw std::domain_error("SymbolicExpr: division by zero");
    return makeMul_({*this, makePow_(r, makeNum_(Complex(-1LL)))});
}
SymbolicExpr SymbolicExpr::pow(const SymbolicExpr& e) const {
    return makePow_(*this, e);
}

SymbolicExpr SymbolicExpr::sqrt(const SymbolicExpr& x) {
    return makePow_(x, makeNum_(Complex(Fraction(1, 2))));
}
SymbolicExpr SymbolicExpr::cbrt(const SymbolicExpr& x) {
    return makePow_(x, makeNum_(Complex(Fraction(1, 3))));
}
SymbolicExpr SymbolicExpr::root(const SymbolicExpr& x, int n) {
    if (n == 0) throw std::invalid_argument("SymbolicExpr::root: n must be non-zero");
    if (n == 1) return x;
    return makePow_(x, makeNum_(Complex(Fraction(1, n))));
}
SymbolicExpr SymbolicExpr::abs_ (const SymbolicExpr& x) { return makeFunc_(FuncKind::Abs,  x); }
SymbolicExpr SymbolicExpr::conj (const SymbolicExpr& x) { return makeFunc_(FuncKind::Conj, x); }

SymbolicExpr SymbolicExpr::subs(const std::string& var, const SymbolicExpr& val) const {
    switch (node_->kind) {
    case Kind::Num:  return *this;
    case Kind::Sym:  return (node_->sym == var) ? val : *this;
    case Kind::Add: {
        std::vector<SymbolicExpr> ch;
        ch.reserve(node_->args.size());
        for (auto& c : node_->args) ch.push_back(c.subs(var, val));
        return makeAdd_(std::move(ch));
    }
    case Kind::Mul: {
        std::vector<SymbolicExpr> ch;
        ch.reserve(node_->args.size());
        for (auto& c : node_->args) ch.push_back(c.subs(var, val));
        return makeMul_(std::move(ch));
    }
    case Kind::Pow:
        return makePow_(node_->args[0].subs(var, val),
                        node_->args[1].subs(var, val));
    case Kind::Func:
        return makeFunc_(node_->func, node_->args[0].subs(var, val));
    }
    return *this;
}

SymbolicExpr SymbolicExpr::diff(const std::string& var) const {
    switch (node_->kind) {
    case Kind::Num: return makeNum_(Complex(0LL));
    case Kind::Sym: return makeNum_(Complex(node_->sym == var ? 1LL : 0LL));
    case Kind::Add: {
        std::vector<SymbolicExpr> ch;
        ch.reserve(node_->args.size());
        for (auto& c : node_->args) ch.push_back(c.diff(var));
        return makeAdd_(std::move(ch));
    }
    case Kind::Mul: {
        std::vector<SymbolicExpr> terms;
        terms.reserve(node_->args.size());
        for (std::size_t i = 0; i < node_->args.size(); ++i) {
            std::vector<SymbolicExpr> factors;
            factors.reserve(node_->args.size());
            for (std::size_t j = 0; j < node_->args.size(); ++j) {
                factors.push_back(i == j ? node_->args[j].diff(var) : node_->args[j]);
            }
            terms.push_back(makeMul_(std::move(factors)));
        }
        return makeAdd_(std::move(terms));
    }
    case Kind::Pow: {
        const SymbolicExpr& b = node_->args[0];
        const SymbolicExpr& e = node_->args[1];
        if (!e.isNum()) {
            throw std::domain_error("SymbolicExpr::diff: variable exponent unsupported");
        }
        SymbolicExpr em1 = e - makeNum_(Complex(1LL));
        return e * makePow_(b, em1) * b.diff(var);
    }
    case Kind::Func:
        throw std::domain_error("SymbolicExpr::diff: abs/conj non-differentiable");
    }
    return makeNum_(Complex(0LL));
}

Complex SymbolicExpr::evaluate() const {
    switch (node_->kind) {
    case Kind::Num: return node_->num;
    case Kind::Sym:
        throw std::domain_error("SymbolicExpr::evaluate: free variable '" + node_->sym + "'");
    case Kind::Add: {
        Complex s(0LL);
        for (auto& c : node_->args) s = s + c.evaluate();
        return s;
    }
    case Kind::Mul: {
        Complex p(1LL);
        for (auto& c : node_->args) p = p * c.evaluate();
        return p;
    }
    case Kind::Pow: {
        Complex b = node_->args[0].evaluate();
        const SymbolicExpr& e = node_->args[1];
        long long k;
        if (extractIntNum(e, k)) {
            if (k >= 0) return complexIntPow(b, k);
            return complexIntPow(Complex(1LL) / b, -k);
        }
        Fraction q;
        if (extractRatNum(e, q)) {
            long long n = q.numerator().toLongLong();
            long long d = q.denominator().toLongLong();
            if (d >= 2) {
                Complex r = Complex::nthRoot(b, static_cast<int>(d));
                if (n >= 0) return complexIntPow(r, n);
                return complexIntPow(Complex(1LL) / r, -n);
            }
        }
        throw std::domain_error("SymbolicExpr::evaluate: non-rational exponent unsupported");
    }
    case Kind::Func: {
        Complex a = node_->args[0].evaluate();
        if (node_->func == FuncKind::Abs)  return Complex(a.modulus());
        if (node_->func == FuncKind::Conj) return a.conjugate();
    }
    }
    throw std::logic_error("SymbolicExpr::evaluate: unreachable");
}

namespace {

int precOf(const SymbolicExpr& e) {
    if (e.isNum()) {
        std::string s = e.asNum().toString();
        if (s.empty()) return 5;
        if (s[0] == '-') return 0;
        bool has_slash = false;
        for (std::size_t i = 0; i < s.size(); ++i) {
            char c = s[i];
            if (c == '+' || c == '*') return 0;
            if (c == '-' && i > 0) return 0;
            if (c == '/') has_slash = true;
        }
        return has_slash ? 2 : 5;
    }
    switch (e.kind()) {
    case SymbolicExpr::Kind::Sym:  return 5;
    case SymbolicExpr::Kind::Func: return 5;
    case SymbolicExpr::Kind::Pow:  return 4;
    case SymbolicExpr::Kind::Mul:  return 2;
    case SymbolicExpr::Kind::Add:  return 1;
    default: return 0;
    }
}

std::string wrap(const std::string& s, int p, int minP) {
    return p < minP ? "(" + s + ")" : s;
}

std::string tryRadicalStr(const SymbolicExpr& base, const SymbolicExpr& exp) {
    if (!exp.isNum()) return {};
    const Complex& c = exp.asNum();
    if (!c.isReal() || !c.real().isRational()) return {};
    Fraction q = c.real().asRational();
    if (q.numerator().toLongLong() != 1) return {};
    long long d = q.denominator().toLongLong();
    if (d < 2) return {};
    std::string b = base.toString();
    if (d == 2) return "sqrt(" + b + ")";
    if (d == 3) return "cbrt(" + b + ")";
    return "root(" + b + ", " + std::to_string(d) + ")";
}

std::string tryRadicalLatex(const SymbolicExpr& base, const SymbolicExpr& exp) {
    if (!exp.isNum()) return {};
    const Complex& c = exp.asNum();
    if (!c.isReal() || !c.real().isRational()) return {};
    Fraction q = c.real().asRational();
    if (q.numerator().toLongLong() != 1) return {};
    long long d = q.denominator().toLongLong();
    if (d < 2) return {};
    std::string b = base.toLatex();
    if (d == 2) return "\\sqrt{" + b + "}";
    return "\\sqrt[" + std::to_string(d) + "]{" + b + "}";
}

std::pair<bool, SymbolicExpr> splitSign(const SymbolicExpr& c) {
    if (c.isNum() && c.asNum().isReal() && c.asNum().real().sign() < 0) {
        return {true, SymbolicExpr::num(-c.asNum())};
    }
    if (c.kind() == SymbolicExpr::Kind::Mul && !c.children().empty()
        && c.children()[0].isNum()
        && c.children()[0].asNum().isReal()
        && c.children()[0].asNum().real().sign() < 0) {
        Complex negCoef = -c.children()[0].asNum();
        if (negCoef == Complex(1LL)) {
            if (c.children().size() == 2) return {true, c.children()[1]};
            SymbolicExpr body = c.children()[1];
            for (std::size_t i = 2; i < c.children().size(); ++i) body = body * c.children()[i];
            return {true, body};
        }
        SymbolicExpr body = SymbolicExpr::num(negCoef);
        for (std::size_t i = 1; i < c.children().size(); ++i) body = body * c.children()[i];
        return {true, body};
    }
    return {false, c};
}

} 

std::string SymbolicExpr::toString() const {
    switch (node_->kind) {
    case Kind::Num: return node_->num.toString();
    case Kind::Sym: return node_->sym;
    case Kind::Add: {
        std::string out;
        for (std::size_t i = 0; i < node_->args.size(); ++i) {
            auto sp = splitSign(node_->args[i]);
            std::string s = wrap(sp.second.toString(), precOf(sp.second), 1);
            if (i == 0) out += sp.first ? "-" + s : s;
            else        out += (sp.first ? " - " : " + ") + s;
        }
        return out;
    }
    case Kind::Mul: {
        std::string out;
        std::size_t i = 0;
        bool first = true;
        if (!node_->args.empty() && node_->args[0].isNum()
            && node_->args[0].asNum().isReal()
            && node_->args[0].asNum().real().sign() < 0) {
            Complex neg = -node_->args[0].asNum();
            out = "-";
            if (!(neg == Complex(1LL))) {
                SymbolicExpr t = SymbolicExpr::num(neg);
                out += wrap(t.toString(), precOf(t), 2);
                first = false;
            }
            i = 1;
        }
        for (; i < node_->args.size(); ++i) {
            const SymbolicExpr& c = node_->args[i];
            std::string s = wrap(c.toString(), precOf(c), 2);
            if (!first) out += "*";
            out += s;
            first = false;
        }
        return out;
    }
    case Kind::Pow: {
        const SymbolicExpr& b = node_->args[0];
        const SymbolicExpr& e = node_->args[1];
        std::string rad = tryRadicalStr(b, e);
        if (!rad.empty()) return rad;
        return wrap(b.toString(), precOf(b), 5)
             + "^"
             + wrap(e.toString(), precOf(e), 5);
    }
    case Kind::Func: {
        const char* fn = (node_->func == FuncKind::Abs) ? "abs" : "conj";
        return std::string(fn) + "(" + node_->args[0].toString() + ")";
    }
    }
    return {};
}

std::string SymbolicExpr::toLatex() const {
    switch (node_->kind) {
    case Kind::Num: return node_->num.toLatex();
    case Kind::Sym: return node_->sym;
    case Kind::Add: {
        std::string out;
        for (std::size_t i = 0; i < node_->args.size(); ++i) {
            auto sp = splitSign(node_->args[i]);
            std::string s = wrap(sp.second.toLatex(), precOf(sp.second), 1);
            if (i == 0) out += sp.first ? "-" + s : s;
            else        out += (sp.first ? " - " : " + ") + s;
        }
        return out;
    }
    case Kind::Mul: {
        std::string out;
        std::size_t i = 0;
        bool first = true;
        if (!node_->args.empty() && node_->args[0].isNum()
            && node_->args[0].asNum().isReal()
            && node_->args[0].asNum().real().sign() < 0) {
            Complex neg = -node_->args[0].asNum();
            out = "-";
            if (!(neg == Complex(1LL))) {
                SymbolicExpr t = SymbolicExpr::num(neg);
                out += wrap(t.toLatex(), precOf(t), 2);
                first = false;
            }
            i = 1;
        }
        for (; i < node_->args.size(); ++i) {
            const SymbolicExpr& c = node_->args[i];
            std::string s = wrap(c.toLatex(), precOf(c), 2);
            if (!first) out += " \\cdot ";
            out += s;
            first = false;
        }
        return out;
    }
    case Kind::Pow: {
        const SymbolicExpr& b = node_->args[0];
        const SymbolicExpr& e = node_->args[1];
        std::string rad = tryRadicalLatex(b, e);
        if (!rad.empty()) return rad;
        return wrap(b.toLatex(), precOf(b), 5)
             + "^{" + e.toLatex() + "}";
    }
    case Kind::Func: {
        if (node_->func == FuncKind::Abs)
            return "\\left|" + node_->args[0].toLatex() + "\\right|";
        return "\\overline{" + node_->args[0].toLatex() + "}";
    }
    }
    return {};
}

std::ostream& operator<<(std::ostream& os, const SymbolicExpr& e) {
    return os << e.toString();
}

namespace {

enum class TokKind { Num, Ident, Plus, Minus, Star, Slash, Caret, LParen, RParen, Comma, End };

struct Tok {
    TokKind     kind;
    std::string text;
    std::size_t col;
};

class Lexer {
public:
    explicit Lexer(const std::string& s) : s_(s) {}

    Tok next() {
        while (pos_ < s_.size() && std::isspace(static_cast<unsigned char>(s_[pos_]))) ++pos_;
        if (pos_ >= s_.size()) return {TokKind::End, {}, pos_};
        std::size_t col = pos_;
        char ch = s_[pos_];

        if (std::isdigit(static_cast<unsigned char>(ch))) {
            std::size_t start = pos_;
            while (pos_ < s_.size() && std::isdigit(static_cast<unsigned char>(s_[pos_]))) ++pos_;
            if (pos_ < s_.size() && s_[pos_] == '.') {
                ++pos_;
                while (pos_ < s_.size() && std::isdigit(static_cast<unsigned char>(s_[pos_]))) ++pos_;
            }
            return {TokKind::Num, s_.substr(start, pos_ - start), col};
        }
        if (std::isalpha(static_cast<unsigned char>(ch)) || ch == '_') {
            std::size_t start = pos_;
            while (pos_ < s_.size()
                   && (std::isalnum(static_cast<unsigned char>(s_[pos_])) || s_[pos_] == '_')) ++pos_;
            return {TokKind::Ident, s_.substr(start, pos_ - start), col};
        }
        ++pos_;
        switch (ch) {
        case '+': return {TokKind::Plus,   "+", col};
        case '-': return {TokKind::Minus,  "-", col};
        case '*': return {TokKind::Star,   "*", col};
        case '/': return {TokKind::Slash,  "/", col};
        case '^': return {TokKind::Caret,  "^", col};
        case '(': return {TokKind::LParen, "(", col};
        case ')': return {TokKind::RParen, ")", col};
        case ',': return {TokKind::Comma,  ",", col};
        default:
            throw std::invalid_argument("SymbolicExpr::fromString: unexpected char '" + std::string(1, ch)
                                        + "' at col " + std::to_string(col));
        }
    }

private:
    const std::string& s_;
    std::size_t pos_ = 0;
};

class Parser {
public:
    explicit Parser(const std::string& s) : lex_(s) { cur_ = lex_.next(); }

    SymbolicExpr parse() {
        SymbolicExpr e = parseExpr();
        if (cur_.kind != TokKind::End)
            throw std::invalid_argument("SymbolicExpr::fromString: trailing token at col " + std::to_string(cur_.col));
        return e;
    }

private:
    Lexer lex_;
    Tok   cur_;

    void advance() { cur_ = lex_.next(); }

    [[noreturn]] void fail(const std::string& msg) {
        throw std::invalid_argument("SymbolicExpr::fromString: " + msg + " at col " + std::to_string(cur_.col));
    }

    SymbolicExpr parseExpr() {
        SymbolicExpr lhs = parseTerm();
        while (cur_.kind == TokKind::Plus || cur_.kind == TokKind::Minus) {
            TokKind op = cur_.kind; advance();
            SymbolicExpr rhs = parseTerm();
            lhs = (op == TokKind::Plus) ? (lhs + rhs) : (lhs - rhs);
        }
        return lhs;
    }

    SymbolicExpr parseTerm() {
        SymbolicExpr lhs = parseUnary();
        while (cur_.kind == TokKind::Star || cur_.kind == TokKind::Slash) {
            TokKind op = cur_.kind; advance();
            SymbolicExpr rhs = parseUnary();
            lhs = (op == TokKind::Star) ? (lhs * rhs) : (lhs / rhs);
        }
        return lhs;
    }

    SymbolicExpr parseUnary() {
        if (cur_.kind == TokKind::Plus)  { advance(); return parseUnary(); }
        if (cur_.kind == TokKind::Minus) { advance(); return -parseUnary(); }
        return parsePower();
    }

    SymbolicExpr parsePower() {
        SymbolicExpr a = parseAtom();
        if (cur_.kind == TokKind::Caret) {
            advance();
            SymbolicExpr e = parseUnary();
            return a.pow(e);
        }
        return a;
    }

    SymbolicExpr parseAtom() {
        if (cur_.kind == TokKind::LParen) {
            advance();
            SymbolicExpr e = parseExpr();
            if (cur_.kind != TokKind::RParen) fail("expected ')'");
            advance();
            return e;
        }
        if (cur_.kind == TokKind::Num) {
            const std::string& t = cur_.text;
            SymbolicExpr r = (t.find('.') != std::string::npos)
                ? SymbolicExpr(Complex(AlgReal::fromDouble(std::stod(t))))
                : SymbolicExpr(Complex(std::stoll(t)));
            advance();
            return r;
        }
        if (cur_.kind == TokKind::Ident) {
            std::string name = cur_.text; std::size_t col = cur_.col;
            advance();

            if (cur_.kind == TokKind::LParen) {
                advance();
                std::vector<SymbolicExpr> args;
                if (cur_.kind != TokKind::RParen) {
                    args.push_back(parseExpr());
                    while (cur_.kind == TokKind::Comma) { advance(); args.push_back(parseExpr()); }
                }
                if (cur_.kind != TokKind::RParen) fail("expected ')'");
                advance();
                if (name == "sqrt" && args.size() == 1) return SymbolicExpr::sqrt(args[0]);
                if (name == "cbrt" && args.size() == 1) return SymbolicExpr::cbrt(args[0]);
                if (name == "root" && args.size() == 2) {
                    long long n;
                    if (!extractIntNum(args[1], n) || n < 2) {
                        throw std::invalid_argument("SymbolicExpr::fromString: root(x, n) requires integer n >= 2 at col " + std::to_string(col));
                    }
                    return SymbolicExpr::root(args[0], static_cast<int>(n));
                }
                if (name == "abs"  && args.size() == 1) return SymbolicExpr::abs_(args[0]);
                if (name == "conj" && args.size() == 1) return SymbolicExpr::conj(args[0]);
                throw std::invalid_argument("SymbolicExpr::fromString: unknown function '" + name
                                            + "' or bad arity at col " + std::to_string(col));
            }

            if (name == "i") return SymbolicExpr(Complex::i());

            if (name == "sqrt" || name == "cbrt" || name == "root"
                || name == "abs"  || name == "conj") {
                throw std::invalid_argument("SymbolicExpr::fromString: '" + name
                                            + "' is a reserved function, need '(' at col " + std::to_string(col));
            }
            return SymbolicExpr::sym(name);
        }
        fail("unexpected token");
    }
};

} 

SymbolicExpr SymbolicExpr::fromString(const std::string& s) {
    Parser p(s);
    return p.parse();
}

namespace {

bool collectPoly_(const SymbolicExpr& e,
                  const std::string& var,
                  std::map<int, SymbolicExpr>& out);

void mulPolyMaps_(const std::map<int, SymbolicExpr>& a,
                  const std::map<int, SymbolicExpr>& b,
                  std::map<int, SymbolicExpr>& out) {
    for (const auto& [ia, ca] : a) {
        for (const auto& [ib, cb] : b) {
            int k = ia + ib;
            SymbolicExpr term = ca * cb;
            auto it = out.find(k);
            if (it == out.end()) out.emplace(k, term);
            else it->second = it->second + term;
        }
    }
}

bool collectPoly_(const SymbolicExpr& e,
                  const std::string& var,
                  std::map<int, SymbolicExpr>& out) {
    using Kind = SymbolicExpr::Kind;
    switch (e.kind()) {
    case Kind::Num:
        if (!e.isZero()) out[0] = e;
        return true;
    case Kind::Sym:
        if (e.asSym() == var) out[1] = SymbolicExpr(1LL);
        else                  out[0] = e;
        return true;
    case Kind::Add: {
        for (const auto& c : e.children()) {
            std::map<int, SymbolicExpr> sub;
            if (!collectPoly_(c, var, sub)) return false;
            for (auto& [k, v] : sub) {
                auto it = out.find(k);
                if (it == out.end()) out.emplace(k, v);
                else it->second = it->second + v;
            }
        }
        return true;
    }
    case Kind::Mul: {
        std::map<int, SymbolicExpr> acc;
        acc[0] = SymbolicExpr(1LL);
        for (const auto& c : e.children()) {
            std::map<int, SymbolicExpr> sub;
            if (!collectPoly_(c, var, sub)) return false;
            std::map<int, SymbolicExpr> next;
            mulPolyMaps_(acc, sub, next);
            acc.swap(next);
        }
        for (auto& [k, v] : acc) {
            if (v.isZero()) continue;
            auto it = out.find(k);
            if (it == out.end()) out.emplace(k, v);
            else it->second = it->second + v;
        }
        return true;
    }
    case Kind::Pow: {
        const SymbolicExpr& base = e.children()[0];
        const SymbolicExpr& exp  = e.children()[1];
        long long k;

        if (!extractIntNum(exp, k) || k < 0) {
            if (base.kind() != Kind::Sym || base.asSym() != var) {
                std::map<int, SymbolicExpr> sub;
                if (!collectPoly_(base, var, sub)) return false;
                if (sub.size() == 1 && sub.count(0)) { out[0] = e; return true; }
                return false;
            }
            return false;
        }

        if (base.kind() == Kind::Sym && base.asSym() == var) {
            out[static_cast<int>(k)] = SymbolicExpr(1LL);
            return true;
        }

        std::map<int, SymbolicExpr> sub;
        if (!collectPoly_(base, var, sub)) return false;
        std::map<int, SymbolicExpr> acc;
        acc[0] = SymbolicExpr(1LL);
        for (long long i = 0; i < k; ++i) {
            std::map<int, SymbolicExpr> next;
            mulPolyMaps_(acc, sub, next);
            acc.swap(next);
        }
        for (auto& [kk, v] : acc) {
            if (v.isZero()) continue;
            auto it = out.find(kk);
            if (it == out.end()) out.emplace(kk, v);
            else it->second = it->second + v;
        }
        return true;
    }
    case Kind::Func: {

        std::map<int, SymbolicExpr> sub;
        if (!collectPoly_(e.children()[0], var, sub)) return false;
        if (sub.size() == 1 && sub.count(0)) { out[0] = e; return true; }
        return false;
    }
    }
    return false;
}

bool symToRational_(const SymbolicExpr& e, Fraction& out) {
    if (!e.isNum()) return false;
    const Complex& c = e.asNum();
    if (!c.isReal()) return false;
    if (!c.real().isRational()) return false;
    out = c.real().asRational();
    return true;
}

} 

bool SymbolicExpr::isPolynomialIn(const std::string& var) const {
    std::map<int, SymbolicExpr> m;
    return collectPoly_(*this, var, m);
}

int SymbolicExpr::degree(const std::string& var) const {
    std::map<int, SymbolicExpr> m;
    if (!collectPoly_(*this, var, m))
        throw std::invalid_argument("SymbolicExpr::degree: not a polynomial in '" + var + "'");
    int d = -1;
    for (auto& [k, v] : m) if (!v.isZero() && k > d) d = k;
    return d;
}

SymbolicExpr SymbolicExpr::coefficient(const std::string& var, int k) const {
    std::map<int, SymbolicExpr> m;
    if (!collectPoly_(*this, var, m))
        throw std::invalid_argument("SymbolicExpr::coefficient: not a polynomial in '" + var + "'");
    auto it = m.find(k);
    if (it == m.end()) return SymbolicExpr(0LL);
    return it->second;
}

Polynomial<SymbolicExpr> SymbolicExpr::toPoly(const std::string& var) const {
    std::map<int, SymbolicExpr> m;
    if (!collectPoly_(*this, var, m))
        throw std::invalid_argument("SymbolicExpr::toPoly: not a polynomial in '" + var + "'");
    int d = -1;
    for (auto& [k, v] : m) if (!v.isZero() && k > d) d = k;
    if (d < 0) return Polynomial<SymbolicExpr>();
    Polynomial<SymbolicExpr> p;
    for (int i = 0; i <= d; ++i) {
        auto it = m.find(i);
        SymbolicExpr c = (it == m.end()) ? SymbolicExpr(0LL) : it->second;
        p = p + Polynomial<SymbolicExpr>::monomial(static_cast<std::size_t>(i), c);
    }
    return p;
}

SymbolicExpr SymbolicExpr::fromPoly(const Polynomial<SymbolicExpr>& p, const std::string& var) {
    if (p.isZero()) return SymbolicExpr(0LL);
    SymbolicExpr x = SymbolicExpr::sym(var);
    std::vector<SymbolicExpr> terms;
    for (std::size_t i = 0; i < p.coeffs().size(); ++i) {
        const SymbolicExpr& c = p.coeffs()[i];
        if (c.isZero()) continue;
        SymbolicExpr term;
        if (i == 0)      term = c;
        else if (i == 1) term = c * x;
        else             term = c * x.pow(SymbolicExpr(static_cast<long long>(i)));
        terms.push_back(term);
    }
    if (terms.empty()) return SymbolicExpr(0LL);
    if (terms.size() == 1) return terms[0];
    SymbolicExpr r = terms[0];
    for (std::size_t i = 1; i < terms.size(); ++i) r = r + terms[i];
    return r;
}

namespace {

SymbolicExpr distribMul_(const SymbolicExpr& a, const SymbolicExpr& b) {
    using Kind = SymbolicExpr::Kind;
    const bool aIsAdd = (a.kind() == Kind::Add);
    const bool bIsAdd = (b.kind() == Kind::Add);
    if (!aIsAdd && !bIsAdd) return a * b;
    std::vector<SymbolicExpr> as = aIsAdd ? a.children() : std::vector<SymbolicExpr>{a};
    std::vector<SymbolicExpr> bs = bIsAdd ? b.children() : std::vector<SymbolicExpr>{b};
    SymbolicExpr acc = SymbolicExpr(0LL);
    for (const auto& ai : as)
        for (const auto& bi : bs)
            acc = acc + (ai * bi);
    return acc;
}

} 

SymbolicExpr SymbolicExpr::expand() const {
    switch (node_->kind) {
    case Kind::Num:
    case Kind::Sym:
        return *this;
    case Kind::Func: {

        return *this;
    }
    case Kind::Add: {
        std::vector<SymbolicExpr> ch;
        ch.reserve(node_->args.size());
        for (const auto& c : node_->args) ch.push_back(c.expand());
        return makeAdd_(std::move(ch));
    }
    case Kind::Mul: {
        std::vector<SymbolicExpr> ch;
        ch.reserve(node_->args.size());
        for (const auto& c : node_->args) ch.push_back(c.expand());
        SymbolicExpr acc = ch[0];
        for (std::size_t i = 1; i < ch.size(); ++i) acc = distribMul_(acc, ch[i]);
        return acc;
    }
    case Kind::Pow: {
        SymbolicExpr b = node_->args[0].expand();
        const SymbolicExpr& e = node_->args[1];
        long long k;
        if (b.kind() == Kind::Add && extractIntNum(e, k) && k >= 2) {
            SymbolicExpr acc = b;
            for (long long i = 1; i < k; ++i) acc = distribMul_(acc, b);
            return acc;
        }
        return makePow_(b, e);
    }
    }
    return *this;
}

SymbolicExpr SymbolicExpr::factor(const std::string& var) const {
    SymbolicExpr self = this->expand();
    Polynomial<SymbolicExpr> ps = self.toPoly(var);
    if (ps.degree() <= 0) return self;

    std::vector<Fraction> fcoeffs(ps.coeffs().size());
    for (std::size_t i = 0; i < ps.coeffs().size(); ++i) {
        if (!symToRational_(ps.coeffs()[i], fcoeffs[i]))
            throw std::invalid_argument("SymbolicExpr::factor: requires real rational coefficients");
    }
    Polynomial<Fraction> pf;
    for (std::size_t i = 0; i < fcoeffs.size(); ++i)
        pf = pf + Polynomial<Fraction>::monomial(i, fcoeffs[i]);

    std::vector<Fraction> roots = rationalRoots(pf);
    if (roots.empty()) return self; 

    std::vector<Fraction> linRoots;
    Polynomial<Fraction> remain = pf;
    for (const Fraction& r : roots) {
        Polynomial<Fraction> linear = {-r, Fraction(1)}; 
        while (true) {
            auto dm = remain.divmod(linear);
            if (dm.remainder.isZero()) {
                linRoots.push_back(r);
                remain = dm.quotient;
            } else break;
        }
    }

    if (linRoots.empty()) return self;

    SymbolicExpr x = SymbolicExpr::sym(var);
    SymbolicExpr result;
    bool first = true;

    if (!(remain.degree() == 0 && remain.constant() == Fraction(1))) {
        SymbolicExpr remainExpr = SymbolicExpr(0LL);
        for (std::size_t i = 0; i < remain.coeffs().size(); ++i) {
            const Fraction& c = remain[i];
            if (c == Fraction(0)) continue;
            SymbolicExpr term = SymbolicExpr(c);
            if (i == 1)      term = term * x;
            else if (i >= 2) term = term * x.pow(SymbolicExpr(static_cast<long long>(i)));
            remainExpr = remainExpr + term;
        }
        result = remainExpr;
        first = false;
    }

    for (const Fraction& r : linRoots) {
        SymbolicExpr lin = x - SymbolicExpr(r);
        if (first) { result = lin; first = false; }
        else       { result = result * lin; }
    }
    return result;
}

}
