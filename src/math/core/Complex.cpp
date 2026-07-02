#include "Complex.h"

#include <cctype>
#include <cmath>
#include <ostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace algemate::math {

Complex::Complex()
    : re_(Fraction(0)), im_(Fraction(0)) {}

Complex::Complex(const AlgReal& re)
    : re_(re), im_(Fraction(0)) {}

Complex::Complex(const AlgReal& re, const AlgReal& im)
    : re_(re), im_(im) {}

Complex::Complex(const Fraction& re)
    : re_(re), im_(Fraction(0)) {}

Complex::Complex(long long re)
    : re_(Fraction(re)), im_(Fraction(0)) {}

Complex Complex::i() {
    return Complex(AlgReal(Fraction(0)), AlgReal(Fraction(1)));
}

Complex Complex::fromDouble(double re, double im) {
    return Complex(AlgReal::fromDouble(re), AlgReal::fromDouble(im));
}

Complex Complex::operator-() const {
    return Complex(-re_, -im_);
}

Complex Complex::operator+(const Complex& r) const {
    return Complex(re_ + r.re_, im_ + r.im_);
}

Complex Complex::operator-(const Complex& r) const {
    return Complex(re_ - r.re_, im_ - r.im_);
}

Complex Complex::operator*(const Complex& r) const {
    AlgReal nre = re_ * r.re_ - im_ * r.im_;
    AlgReal nim = re_ * r.im_ + im_ * r.re_;
    return Complex(nre, nim);
}

Complex Complex::operator/(const Complex& r) const {
    if (r.isZero()) throw std::domain_error("Complex: division by zero");
    AlgReal denom = r.modulusSquared();          
    AlgReal nre   = (re_ * r.re_ + im_ * r.im_) / denom;
    AlgReal nim   = (im_ * r.re_ - re_ * r.im_) / denom;
    return Complex(nre, nim);
}

bool Complex::operator==(const Complex& r) const {
    return re_ == r.re_ && im_ == r.im_;
}

Complex Complex::conjugate() const {
    return Complex(re_, -im_);
}

AlgReal Complex::modulusSquared() const {
    return re_ * re_ + im_ * im_;
}

AlgReal Complex::modulus() const {
    return AlgReal::sqrt(modulusSquared());
}

std::pair<double, double> Complex::toDouble() const {
    return {re_.toDouble(), im_.toDouble()};
}

Complex Complex::sqrt(const Complex& z) {
    if (!z.isReal()) throw std::domain_error("Complex::sqrt: non-real z unsupported");
    const AlgReal& r = z.real();
    if (r.sign() >= 0) return Complex(AlgReal::sqrt(r));
    return Complex(AlgReal(Fraction(0)), AlgReal::sqrt(-r));
}

Complex Complex::cbrt(const Complex& z) {
    if (!z.isReal()) throw std::domain_error("Complex::cbrt: non-real z unsupported");
    return Complex(AlgReal::cbrt(z.real()));
}

Complex Complex::nthRoot(const Complex& z, int n) {
    if (n <= 0) throw std::invalid_argument("Complex::nthRoot: n must be positive");
    if (!z.isReal()) throw std::domain_error("Complex::nthRoot: non-real z unsupported");
    if (n == 1) return z;
    const AlgReal& r = z.real();
    if (n % 2 == 1) {
        return Complex(AlgReal::nthRoot(r, n));
    }
    if (r.sign() >= 0) return Complex(AlgReal::nthRoot(r, n));
    return Complex(AlgReal(Fraction(0)), AlgReal::nthRoot(-r, n));
}

namespace {

enum class TokKind {
    Number,      
    Ident,       
    Plus, Minus, Star, Slash,
    LParen, RParen, Comma,
    End
};

struct Token {
    TokKind     kind;
    std::string str;
    int         col;    
};

class Lexer {
public:
    explicit Lexer(const std::string& src) : s_(src) {}

    std::vector<Token> tokenize() {
        std::vector<Token> out;
        while (pos_ < s_.size()) {
            char c = s_[pos_];
            if (std::isspace(static_cast<unsigned char>(c))) { ++pos_; continue; }
            int col = static_cast<int>(pos_) + 1;
            if (std::isdigit(static_cast<unsigned char>(c))) {
                out.push_back(readNumber_(col));
            } else if (std::isalpha(static_cast<unsigned char>(c))) {
                out.push_back(readIdent_(col));
            } else {
                Token t; t.col = col; t.str = std::string(1, c);
                switch (c) {
                    case '+': t.kind = TokKind::Plus;   break;
                    case '-': t.kind = TokKind::Minus;  break;
                    case '*': t.kind = TokKind::Star;   break;
                    case '/': t.kind = TokKind::Slash;  break;
                    case '(': t.kind = TokKind::LParen; break;
                    case ')': t.kind = TokKind::RParen; break;
                    case ',': t.kind = TokKind::Comma;  break;
                    default: throw std::invalid_argument(
                        "Complex::fromString: unexpected char '" + std::string(1, c) +
                        "' at col " + std::to_string(col));
                }
                out.push_back(t);
                ++pos_;
            }
        }
        Token e; e.kind = TokKind::End; e.col = static_cast<int>(pos_) + 1;
        out.push_back(e);
        return out;
    }

private:
    Token readNumber_(int col) {
        std::size_t start = pos_;
        while (pos_ < s_.size() && std::isdigit(static_cast<unsigned char>(s_[pos_]))) ++pos_;
        bool hasDot = false, hasSlash = false;
        if (pos_ < s_.size() && s_[pos_] == '.') {
            hasDot = true; ++pos_;
            while (pos_ < s_.size() && std::isdigit(static_cast<unsigned char>(s_[pos_]))) ++pos_;
        } else if (pos_ < s_.size() && s_[pos_] == '/') {

            std::size_t save = pos_;
            ++pos_;
            if (pos_ < s_.size() && std::isdigit(static_cast<unsigned char>(s_[pos_]))) {
                hasSlash = true;
                while (pos_ < s_.size() && std::isdigit(static_cast<unsigned char>(s_[pos_]))) ++pos_;
            } else {
                pos_ = save;
            }
        }
        (void)hasDot; (void)hasSlash;
        Token t;
        t.kind = TokKind::Number;
        t.str  = s_.substr(start, pos_ - start);
        t.col  = col;
        return t;
    }

    Token readIdent_(int col) {
        std::size_t start = pos_;
        while (pos_ < s_.size() && std::isalpha(static_cast<unsigned char>(s_[pos_]))) ++pos_;
        Token t;
        t.kind = TokKind::Ident;
        t.str  = s_.substr(start, pos_ - start);
        t.col  = col;
        return t;
    }

    const std::string& s_;
    std::size_t        pos_ = 0;
};

class Parser {
public:
    explicit Parser(std::vector<Token> tokens) : toks_(std::move(tokens)) {}

    Complex parseFull() {
        Complex v = parseExpr_();
        expect_(TokKind::End, "end of input");
        return v;
    }

private:
    [[noreturn]] void fail_(const std::string& msg, int col) const {
        throw std::invalid_argument(
            "Complex::fromString: " + msg + " at col " + std::to_string(col));
    }
    const Token& peek_() const { return toks_[pos_]; }
    const Token& eat_()        { return toks_[pos_++]; }
    void expect_(TokKind k, const std::string& what) {
        if (peek_().kind != k) fail_("expected " + what, peek_().col);
        ++pos_;
    }

    Complex parseExpr_() {
        Complex lhs = parseTerm_();
        while (peek_().kind == TokKind::Plus || peek_().kind == TokKind::Minus) {
            TokKind op = eat_().kind;
            Complex rhs = parseTerm_();
            lhs = (op == TokKind::Plus) ? (lhs + rhs) : (lhs - rhs);
        }
        return lhs;
    }

    Complex parseTerm_() {
        Complex lhs = parseUnary_();
        while (peek_().kind == TokKind::Star || peek_().kind == TokKind::Slash) {
            TokKind op = eat_().kind;
            Complex rhs = parseUnary_();
            lhs = (op == TokKind::Star) ? (lhs * rhs) : (lhs / rhs);
        }
        return lhs;
    }

    Complex parseUnary_() {
        if (peek_().kind == TokKind::Plus)  { eat_(); return parseUnary_(); }
        if (peek_().kind == TokKind::Minus) { eat_(); return -parseUnary_(); }
        return parseAtom_();
    }

    Complex parseAtom_() {
        const Token& t = peek_();
        if (t.kind == TokKind::Number) {
            eat_();
            return parseNumberLiteral_(t);
        }
        if (t.kind == TokKind::Ident) {
            if (t.str == "i") { eat_(); return Complex::i(); }
            if (t.str == "sqrt" || t.str == "cbrt") {
                eat_();
                expect_(TokKind::LParen, "'('");
                Complex inner = parseExpr_();
                expect_(TokKind::RParen, "')'");
                return (t.str == "sqrt") ? Complex::sqrt(inner) : Complex::cbrt(inner);
            }
            if (t.str == "root") {
                eat_();
                expect_(TokKind::LParen, "'('");
                Complex inner = parseExpr_();
                expect_(TokKind::Comma, "','");
                if (peek_().kind != TokKind::Number)
                    fail_("expected integer literal as 2nd arg of root", peek_().col);
                Token nt = eat_();
                if (nt.str.find('.') != std::string::npos ||
                    nt.str.find('/') != std::string::npos)
                    fail_("root degree must be positive integer literal", nt.col);
                int n = std::stoi(nt.str);
                if (n <= 0) fail_("root degree must be positive", nt.col);
                expect_(TokKind::RParen, "')'");
                return Complex::nthRoot(inner, n);
            }
            fail_("unknown identifier '" + t.str + "'", t.col);
        }
        if (t.kind == TokKind::LParen) {
            eat_();
            Complex inner = parseExpr_();
            expect_(TokKind::RParen, "')'");
            return inner;
        }
        fail_("unexpected token '" + t.str + "'", t.col);
    }

    Complex parseNumberLiteral_(const Token& t) {
        const std::string& s = t.str;
        if (s.find('/') != std::string::npos) {
            return Complex(Fraction::fromString(s));
        }
        if (s.find('.') != std::string::npos) {
            return Complex(AlgReal::fromDouble(std::stod(s)));
        }
        return Complex(Fraction::fromString(s));
    }

    std::vector<Token> toks_;
    std::size_t        pos_ = 0;
};

} 

Complex Complex::fromString(const std::string& s) {
    Lexer lex(s);
    auto toks = lex.tokenize();
    Parser p(std::move(toks));
    return p.parseFull();
}

namespace {

std::string coefStr_(const AlgReal& x) {
    if (x.isRational()) return x.asRational().toString();
    return x.toString() + "*";
}

std::string coefLatex_(const AlgReal& x) {
    if (x.isRational()) return x.asRational().toLatex();
    return x.toLatex() + "\\cdot ";
}

} 

std::string Complex::toString() const {
    if (isZero()) return "0";
    if (im_.isZero()) return re_.toString();

    AlgReal plusOne(Fraction(1));
    AlgReal minusOne(Fraction(-1));

    if (re_.isZero()) {
        if (im_ == plusOne)  return "i";
        if (im_ == minusOne) return "-i";
        return coefStr_(im_) + "i";
    }
    std::ostringstream oss;
    oss << re_.toString();
    if (im_ == plusOne)        oss << "+i";
    else if (im_ == minusOne)  oss << "-i";
    else if (im_.sign() > 0)   oss << "+" << coefStr_(im_) << "i";
    else                       oss << "-" << coefStr_(-im_) << "i";
    return oss.str();
}

std::string Complex::toLatex() const {
    if (isZero()) return "0";
    if (im_.isZero()) return re_.toLatex();

    AlgReal plusOne(Fraction(1));
    AlgReal minusOne(Fraction(-1));

    if (re_.isZero()) {
        if (im_ == plusOne)  return "i";
        if (im_ == minusOne) return "-i";
        return coefLatex_(im_) + "i";
    }
    std::ostringstream oss;
    oss << re_.toLatex();
    if (im_ == plusOne)        oss << "+i";
    else if (im_ == minusOne)  oss << "-i";
    else if (im_.sign() > 0)   oss << "+" << coefLatex_(im_) << "i";
    else                       oss << "-" << coefLatex_(-im_) << "i";
    return oss.str();
}

std::ostream& operator<<(std::ostream& os, const Complex& z) {
    return os << z.toString();
}

}
