#pragma once

/*
* @file SymbolicExpr.h
* @brief 符号表达式 AST: 有理表达式 + 根式
*
* 节点: Num(Complex) / Sym(string) / Add / Mul / Pow / Func(abs|conj)
* sqrt/cbrt/root 在构造时折为 Pow(x, 1/n), 打印时识别回原形
* 每次构造自动规范化: 扁平化 Add/Mul, 合并同类项 / 同底幂, 吸收 0/1, 子项排序
*
* 不支持隐式乘法: "3x" 必须写 "3*x", "2sqrt(2)" 必须写 "2*sqrt(2)"
* 不做三角/指对函数, 不嵌入矩阵, 变量域默认为复数 (不自动 sqrt(x^2)->x)
*
* @example
*   auto x = SymbolicExpr::sym("x");
*   auto e = (x + 1) * (x - 1);           // -> x^2 + (-1)
*   auto d = e.diff("x");                 // 2*x
*   auto v = e.subs("x", SymbolicExpr(3));// 8
*   auto f = SymbolicExpr::fromString("x^2+2*x+1");
*/

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

    SymbolicExpr();                                        // 0
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
    const Complex&     asNum() const;                     // 仅 Num
    const std::string& asSym() const;                     // 仅 Sym
    const std::vector<SymbolicExpr>& children() const;
    FuncKind funcKind() const;                            // 仅 Func

    SymbolicExpr operator-() const;
    SymbolicExpr operator+(const SymbolicExpr& r) const;
    SymbolicExpr operator-(const SymbolicExpr& r) const;
    SymbolicExpr operator*(const SymbolicExpr& r) const;
    SymbolicExpr operator/(const SymbolicExpr& r) const;
    SymbolicExpr pow (const SymbolicExpr& e) const;

    static SymbolicExpr sqrt (const SymbolicExpr& x);
    static SymbolicExpr cbrt (const SymbolicExpr& x);
    static SymbolicExpr root (const SymbolicExpr& x, int n);
    static SymbolicExpr abs_ (const SymbolicExpr& x);     // 避免与 std::abs 冲突
    static SymbolicExpr conj (const SymbolicExpr& x);

    bool operator==(const SymbolicExpr& r) const;         // 结构相等 (规范形式后)
    bool operator!=(const SymbolicExpr& r) const { return !(*this == r); }

    SymbolicExpr subs    (const std::string& var, const SymbolicExpr& val) const;
    SymbolicExpr diff    (const std::string& var) const;
    Complex      evaluate() const;                        // 无自由变量时

    // 多项式桥 (以 var 为自变量, 其余部分作为系数)
    bool         isPolynomialIn(const std::string& var) const;        // 判定是否多项式 in var
    int          degree   (const std::string& var) const;             // 最高次, 零多项式返回 -1
    SymbolicExpr coefficient(const std::string& var, int k) const;    // x^k 的系数 (符号)
    Polynomial<SymbolicExpr> toPoly(const std::string& var) const;    // 非多项式抛 invalid_argument
    static SymbolicExpr fromPoly(const Polynomial<SymbolicExpr>& p,
                                 const std::string& var);

    SymbolicExpr expand() const;                                       // 乘法分配 + 幂展开
    SymbolicExpr factor(const std::string& var) const;                 // 有理根因式分解 (所有系数必须为实有理数)

    std::string toString() const;                         // 线性 CAS 风格
    std::string toLatex () const;

private:
    struct Node {
        Kind                      kind;
        Complex                   num;              // Num
        std::string               sym;              // Sym
        FuncKind                  func = FuncKind::Abs;
        std::vector<SymbolicExpr> args;             // Add/Mul/Pow/Func 子节点
    };
    std::shared_ptr<const Node> node_;
    explicit SymbolicExpr(std::shared_ptr<const Node> n) : node_(std::move(n)) {}

    // 工厂 (内部规范化)
    static SymbolicExpr makeNum_ (const Complex& c);
    static SymbolicExpr makeSym_ (const std::string& s);
    static SymbolicExpr makeAdd_ (std::vector<SymbolicExpr> ch);
    static SymbolicExpr makeMul_ (std::vector<SymbolicExpr> ch);
    static SymbolicExpr makePow_ (SymbolicExpr b, SymbolicExpr e);
    static SymbolicExpr makeFunc_(FuncKind k, SymbolicExpr arg);
};

std::ostream& operator<<(std::ostream& os, const SymbolicExpr& e);

}
