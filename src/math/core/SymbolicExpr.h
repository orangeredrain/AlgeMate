#pragma once

#include "Complex.h"
#include "Fraction.h"
#include "Polynomial.h"

#include <iosfwd>
#include <memory>
#include <string>
#include <vector>

namespace algemate::math {

class SymbolicExpr {
public:
    enum class Kind { Num, Sym, Add, Mul, Pow, Func };
    enum class FuncKind { Abs, Conj };

    SymbolicExpr();                                        
    SymbolicExpr(long long n);
    SymbolicExpr(const Fraction& q);
    SymbolicExpr(const AlgReal& a);
    SymbolicExpr(const Complex& c);

    static SymbolicExpr num(const Complex& c);
    static SymbolicExpr sym(const std::string& name);
    static SymbolicExpr fromString(const std::string& s);

    Kind kind() const;
    bool isNum()  const;
    bool isSym()  const;
    bool isZero() const;
    bool isOne()  const;
    const Complex&     asNum() const;                     
    const std::string& asSym() const;                     
    const std::vector<SymbolicExpr>& children() const;
    FuncKind funcKind() const;                            

    SymbolicExpr operator-() const;
    SymbolicExpr operator+(const SymbolicExpr& r) const;
    SymbolicExpr operator-(const SymbolicExpr& r) const;
    SymbolicExpr operator*(const SymbolicExpr& r) const;
    SymbolicExpr operator/(const SymbolicExpr& r) const;
    SymbolicExpr pow (const SymbolicExpr& e) const;

    static SymbolicExpr sqrt (const SymbolicExpr& x);
    static SymbolicExpr cbrt (const SymbolicExpr& x);
    static SymbolicExpr root (const SymbolicExpr& x, int n);
    static SymbolicExpr abs_ (const SymbolicExpr& x);     
    static SymbolicExpr conj (const SymbolicExpr& x);

    bool operator==(const SymbolicExpr& r) const;         
    bool operator!=(const SymbolicExpr& r) const { return !(*this == r); }

    SymbolicExpr subs    (const std::string& var, const SymbolicExpr& val) const;
    SymbolicExpr diff    (const std::string& var) const;
    Complex      evaluate() const;                        

    bool         isPolynomialIn(const std::string& var) const;        
    int          degree   (const std::string& var) const;             
    SymbolicExpr coefficient(const std::string& var, int k) const;    
    Polynomial<SymbolicExpr> toPoly(const std::string& var) const;    
    static SymbolicExpr fromPoly(const Polynomial<SymbolicExpr>& p,
                                 const std::string& var);

    SymbolicExpr expand() const;                                       
    SymbolicExpr factor(const std::string& var) const;                 

    std::string toString() const;                         
    std::string toLatex () const;

private:
    struct Node {
        Kind                      kind;
        Complex                   num;              
        std::string               sym;              
        FuncKind                  func = FuncKind::Abs;
        std::vector<SymbolicExpr> args;             
    };
    std::shared_ptr<const Node> node_;
    explicit SymbolicExpr(std::shared_ptr<const Node> n) : node_(std::move(n)) {}

    static SymbolicExpr makeNum_ (const Complex& c);
    static SymbolicExpr makeSym_ (const std::string& s);
    static SymbolicExpr makeAdd_ (std::vector<SymbolicExpr> ch);
    static SymbolicExpr makeMul_ (std::vector<SymbolicExpr> ch);
    static SymbolicExpr makePow_ (SymbolicExpr b, SymbolicExpr e);
    static SymbolicExpr makeFunc_(FuncKind k, SymbolicExpr arg);
};

std::ostream& operator<<(std::ostream& os, const SymbolicExpr& e);

}
