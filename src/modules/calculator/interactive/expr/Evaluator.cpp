#include "Evaluator.h"

#include "math/algorithm/LinearAlgebra.h"
#include "math/algorithm/RealEigen.h"
#include "math/algorithm/ComplexEigen.h"
#include "math/algorithm/PolynomialAlg.h"
#include "math/algorithm/RationalCanonical.h"
#include "math/algorithm/BilinearForm.h"
#include "math/algorithm/OrthogonalDiag.h"
#include "math/algorithm/SVD.h"
#include "math/algorithm/JordanForm.h"
#include "math/algorithm/LambdaMatrix.h"
#include "math/core/AlgReal.h"
#include "math/core/Polynomial.h"
#include "math/core/PolynomialZp.h"
#include "math/mpoly/PowerSum.h"

#include <sstream>
#include <stdexcept>
#include <string>
#include <cmath>
#include <complex>
#include <algorithm>
#include <set>

#include <QStringList>

namespace AlgeMate::Calculator::Interactive {

using algemate::math::AlgReal;
using algemate::math::BigInt;
using algemate::math::Complex;
using algemate::math::Fraction;
using algemate::math::Matrix;
using algemate::math::Polynomial;
using algemate::math::PolynomialZp;

// 类型转换辅助

namespace {

// ================ Jordan / 复根数值化辅助 (参考 demo/JordanFormPage) ================
//
// 原 “jordan(A)” 走 algemate::math::jordanForm 的精确复数路径，在含重根或复特征
// 值时会崩溃。现改为：先走 λ-矩阵 → 行列式因子 → 不变因子（均精确），再在 ℂ 上
// 用 Durand–Kerner 数值求根得到初等因子 / Jordan 块。与 demo 页一致。

using JCmplx = std::complex<double>;

inline std::vector<JCmplx> jPolyToCoeffs(const Polynomial<Fraction>& p) {
    std::vector<JCmplx> c;
    for (const auto& coeff : p.coeffs())
        c.emplace_back(coeff.toDouble(), 0.0);
    return c;
}

inline JCmplx jEvalPoly(const std::vector<JCmplx>& c, JCmplx x) {
    JCmplx r(0.0, 0.0);
    for (int i = static_cast<int>(c.size()) - 1; i >= 0; --i)
        r = r * x + c[i];
    return r;
}

inline std::vector<JCmplx> jDurandKerner(const std::vector<JCmplx>& c) {
    int n = static_cast<int>(c.size()) - 1;
    if (n <= 0) return {};
    if (n == 1) return {-c[0] / c[1]};
    std::vector<JCmplx> roots(n);
    for (int i = 0; i < n; ++i) {
        double angle = 2.0 * 3.141592653589793 * i / n + 0.4;
        roots[i] = JCmplx(0.4 * std::cos(angle), 0.9 * std::sin(angle));
    }
    for (int iter = 0; iter < 200; ++iter) {
        double maxDelta = 0.0;
        for (int i = 0; i < n; ++i) {
            JCmplx p = jEvalPoly(c, roots[i]);
            JCmplx denom(1.0, 0.0);
            for (int j = 0; j < n; ++j)
                if (j != i) denom *= (roots[i] - roots[j]);
            if (std::abs(denom) < 1e-60) { roots[i] += JCmplx(1e-8, 1e-8); continue; }
            JCmplx delta = p / denom;
            if (std::abs(delta) > 1e6) delta = delta * (1e6 / std::abs(delta));
            roots[i] -= delta;
            maxDelta = std::max(maxDelta, std::abs(delta));
        }
        if (maxDelta < 1e-12) break;
    }
    return roots;
}

struct JNumRoot { double re; double im; int mult; };

inline std::vector<JNumRoot> jNumericalRoots(const Polynomial<Fraction>& p) {
    if (p.degree() <= 0) return {};
    auto coeffs = jPolyToCoeffs(p);
    auto roots  = jDurandKerner(coeffs);
    for (auto& r : roots)
        if (std::abs(r.imag()) < 1e-3) r = JCmplx(r.real(), 0.0);
    std::vector<JNumRoot> result;
    std::vector<bool> used(roots.size(), false);
    for (std::size_t i = 0; i < roots.size(); ++i) {
        if (used[i]) continue;
        double re = roots[i].real(), im = roots[i].imag();
        int mult = 1;
        for (std::size_t j = i + 1; j < roots.size(); ++j) {
            if (used[j]) continue;
            double dr = re - roots[j].real(), di = im - roots[j].imag();
            if (dr * dr + di * di < 1e-6) { ++mult; used[j] = true; }
        }
        used[i] = true;
        result.push_back({re, im, mult});
    }
    return result;
}

inline QString jFmtDouble(double v) {
    if (std::abs(v) < 1e-12) v = 0.0;
    QString s = QString::number(v, 'f', 2);
    if (s.contains(QLatin1Char('.'))) {
        while (s.endsWith(QLatin1Char('0'))) s.chop(1);
        if (s.endsWith(QLatin1Char('.'))) s.chop(1);
    }
    return s;
}

// 复数 (re, im) → LaTeX. 纯实 / 纯虚 / 一般复数 三种格式。
inline QString jComplexNumLtx(double re, double im) {
    if (jFmtDouble(std::abs(im)) == QStringLiteral("0"))
        return jFmtDouble(re);
    if (jFmtDouble(std::abs(re)) == QStringLiteral("0")) {
        if (std::abs(im - 1.0) < 1e-12) return QStringLiteral("i");
        if (std::abs(im + 1.0) < 1e-12) return QStringLiteral("-i");
        return jFmtDouble(im) + QStringLiteral("i");
    }
    QString sign = (im > 0) ? QStringLiteral("+") : QStringLiteral("");
    return jFmtDouble(re) + sign + jFmtDouble(im) + QStringLiteral("i");
}

// 初等因子的单个一次因式 LaTeX, 例如 (\lambda - 3) / (\lambda + 2) / (\lambda - 2i) / (\lambda - (1+2i))
inline QString jFormatFactorLtx(double re, double im, int mult) {
    QString factor;
    if (jFmtDouble(std::abs(im)) == QStringLiteral("0")) {
        if (jFmtDouble(std::abs(re)) == QStringLiteral("0"))
            factor = QStringLiteral("\\lambda");
        else if (re > 0)
            factor = QStringLiteral("(\\lambda - %1)").arg(jFmtDouble(re));
        else
            factor = QStringLiteral("(\\lambda + %1)").arg(jFmtDouble(-re));
    } else if (jFmtDouble(std::abs(re)) == QStringLiteral("0")) {
        if (im > 0)
            factor = QStringLiteral("(\\lambda - %1)").arg(jComplexNumLtx(0, im));
        else
            factor = QStringLiteral("(\\lambda + %1)").arg(jComplexNumLtx(0, -im));
    } else {
        factor = QStringLiteral("(\\lambda - (%1))").arg(jComplexNumLtx(re, im));
    }
    if (mult > 1) factor += QStringLiteral("^{%1}").arg(mult);
    return factor;
}

} // anonymous namespace

static Matrix<Fraction> toFractionMatrix(const MatrixA& M, const char* op) {
    Matrix<Fraction> F(M.rows(), M.cols());
    for (std::size_t i = 0; i < M.rows(); ++i) {
        for (std::size_t j = 0; j < M.cols(); ++j) {
            if (!M(i, j).isRational())
                throw std::runtime_error(
                    std::string(op) + " 暂仅支持有理数矩阵 (当前含代数数元素)");
            F(i, j) = M(i, j).asRational();
        }
    }
    return F;
}

static MatrixA toAlgRealMatrix(const Matrix<Fraction>& F) {
    MatrixA M(F.rows(), F.cols());
    for (std::size_t i = 0; i < F.rows(); ++i)
        for (std::size_t j = 0; j < F.cols(); ++j)
            M(i, j) = AlgReal(F(i, j));
    return M;
}


static bool hasAlgebraicElem_(const MatrixA& M) {
    for (std::size_t i = 0; i < M.rows(); ++i)
        for (std::size_t j = 0; j < M.cols(); ++j)
            if (!M(i, j).isRational()) return true;
    return false;
}

static algemate::math::Matrix<double> toDoubleMatrix_(const MatrixA& M) {
    algemate::math::Matrix<double> D(M.rows(), M.cols());
    for (std::size_t i = 0; i < M.rows(); ++i)
        for (std::size_t j = 0; j < M.cols(); ++j)
            D(i, j) = M(i, j).toDouble();
    return D;
}

static MatrixA fromDoubleMatrix_(const algemate::math::Matrix<double>& D) {
    MatrixA M(D.rows(), D.cols());
    for (std::size_t i = 0; i < D.rows(); ++i)
        for (std::size_t j = 0; j < D.cols(); ++j)
            M(i, j) = AlgReal::fromDouble(D(i, j));
    return M;
}

// Faddeev-LeVerrier: 数值计算 double 矩阵的特征多项式系数
// 返回 c[0]..c[n], 其中 c[n]=1, 多项式为 λ^n + c[n-1]λ^{n-1} + ... + c[0]
static std::vector<double> charpolyDouble(const algemate::math::Matrix<double>& A) {
    std::size_t n = A.rows();
    algemate::math::Matrix<double> M = A;
    std::vector<double> c(n + 1);
    c[n] = 1.0;
    for (std::size_t k = 1; k <= n; ++k) {
        double trace = 0.0;
        for (std::size_t i = 0; i < n; ++i)
            trace += M(i, i);
        c[n - k] = -trace / static_cast<double>(k);
        if (k < n) {
            for (std::size_t i = 0; i < n; ++i)
                M(i, i) += c[n - k];
            M = A * M;
        }
    }
    return c;
}

// double 版 Gauss 消元
static algemate::math::Matrix<double> gaussEliminateDouble_(algemate::math::Matrix<double> M,
                                                           bool fullReduce, int* swaps = nullptr) {
    const std::size_t R = M.rows();
    const std::size_t C = M.cols();
    const double eps = 1e-12;
    std::size_t lead = 0;
    int sw = 0;
    for (std::size_t r = 0; r < R; ++r) {
        if (lead >= C) break;
        // 部分主元: 选 |pivot| 最大的行
        std::size_t best = r;
        double bestAbs = std::abs(M(r, lead));
        for (std::size_t i = r + 1; i < R; ++i) {
            double a = std::abs(M(i, lead));
            if (a > bestAbs) { bestAbs = a; best = i; }
        }
        if (bestAbs < eps) {
            ++lead;
            --r;
            continue;
        }
        if (best != r) { M.swapRows(best, r); ++sw; }
        double pivot = M(r, lead);
        if (fullReduce) {
            for (std::size_t c = 0; c < C; ++c) {
                if (c == lead) M(r, c) = 1.0;
                else M(r, c) = M(r, c) / pivot;
            }
            for (std::size_t k = 0; k < R; ++k) {
                if (k == r) continue;
                double f = M(k, lead);
                if (std::abs(f) < eps) continue;
                for (std::size_t c = 0; c < C; ++c)
                    M(k, c) = M(k, c) - f * M(r, c);
            }
        } else {
            for (std::size_t k = r + 1; k < R; ++k) {
                double f = M(k, lead);
                if (std::abs(f) < eps) continue;
                f = f / pivot;
                for (std::size_t c = 0; c < C; ++c)
                    M(k, c) = M(k, c) - f * M(r, c);
            }
        }
        ++lead;
    }
    if (swaps) *swaps = sw;
    return M;
}

// RREF: 返回行最简阶梯形, swaps 记录行交换次数
static MatrixA gaussEliminateAlg(MatrixA M, bool fullReduce, int* swaps = nullptr) {
    const std::size_t R = M.rows();
    const std::size_t C = M.cols();
    std::size_t lead = 0;
    int sw = 0;
    for (std::size_t r = 0; r < R; ++r) {
        if (lead >= C) break;
        std::size_t i = r;
        while (i < R && M(i, lead).isZero()) ++i;
        if (i == R) {
            ++lead;
            --r;
            continue;
        }
        if (i != r) { M.swapRows(i, r); ++sw; }
        // 归一 (满秩阶梯形) 或 保留主元 (仅 REF)
        Scalar pivot = M(r, lead);
        if (fullReduce) {
            // M(r, *) /= pivot
            for (std::size_t c = 0; c < C; ++c) {
                if (c == lead) M(r, c) = Scalar((long long)1);
                else M(r, c) = M(r, c) / pivot;
            }
            // 消去其他行的 lead 列
            for (std::size_t k = 0; k < R; ++k) {
                if (k == r) continue;
                Scalar f = M(k, lead);
                if (f.isZero()) continue;
                for (std::size_t c = 0; c < C; ++c) {
                    M(k, c) = M(k, c) - f * M(r, c);
                }
            }
        } else {
            // 只消交下方
            for (std::size_t k = r + 1; k < R; ++k) {
                Scalar f = M(k, lead);
                if (f.isZero()) continue;
                f = f / pivot;
                for (std::size_t c = 0; c < C; ++c) {
                    M(k, c) = M(k, c) - f * M(r, c);
                }
            }
        }
        ++lead;
    }
    if (swaps) *swaps = sw;
    return M;
}

static Scalar detAlg(const MatrixA& M) {
    if (!M.isSquare()) throw std::runtime_error("det 要求方阵");
    if (hasAlgebraicElem_(M)) {
        // 代数数矩阵 → double 降级 (稳定优先)
        int sw = 0;
        auto U = gaussEliminateDouble_(toDoubleMatrix_(M), false, &sw);
        double d = 1.0;
        for (std::size_t i = 0; i < U.rows(); ++i) d *= U(i, i);
        if (sw % 2) d = -d;
        return Scalar(AlgReal::fromDouble(d));
    }
    int sw = 0;
    MatrixA U = gaussEliminateAlg(M, false, &sw);
    Scalar d((long long)1);
    for (std::size_t i = 0; i < U.rows(); ++i) d = d * U(i, i);
    if (sw % 2) d = -d;
    return d;
}

static std::size_t rankAlg(const MatrixA& M) {
    if (hasAlgebraicElem_(M)) {
        auto U = gaussEliminateDouble_(toDoubleMatrix_(M), false);
        std::size_t r = 0;
        const double eps = 1e-9;
        for (std::size_t i = 0; i < U.rows(); ++i) {
            bool nz = false;
            for (std::size_t j = 0; j < U.cols(); ++j) {
                if (std::abs(U(i, j)) > eps) { nz = true; break; }
            }
            if (nz) ++r;
        }
        return r;
    }
    MatrixA U = gaussEliminateAlg(M, false);
    std::size_t r = 0;
    for (std::size_t i = 0; i < U.rows(); ++i) {
        bool nz = false;
        for (std::size_t j = 0; j < U.cols(); ++j) {
            if (!U(i, j).isZero()) { nz = true; break; }
        }
        if (nz) ++r;
    }
    return r;
}

static MatrixA rrefAlg(const MatrixA& M) {
    if (hasAlgebraicElem_(M)) {
        return fromDoubleMatrix_(gaussEliminateDouble_(toDoubleMatrix_(M), true));
    }
    return gaussEliminateAlg(M, true);
}

static MatrixA invAlg(const MatrixA& M) {
    if (!M.isSquare()) throw std::runtime_error("inv 要求方阵");
    const std::size_t N = M.rows();
    if (hasAlgebraicElem_(M)) {
        // 代数数矩阵 → double 降级
        algemate::math::Matrix<double> aug(N, 2 * N);
        auto D = toDoubleMatrix_(M);
        for (std::size_t i = 0; i < N; ++i) {
            for (std::size_t j = 0; j < N; ++j) aug(i, j) = D(i, j);
            aug(i, N + i) = 1.0;
        }
        auto R = gaussEliminateDouble_(aug, true);
        const double eps = 1e-9;
        for (std::size_t i = 0; i < N; ++i) {
            for (std::size_t j = 0; j < N; ++j) {
                double expect = (i == j) ? 1.0 : 0.0;
                if (std::abs(R(i, j) - expect) > eps)
                    throw std::runtime_error("矩阵奇异, 不可逆");
            }
        }
        algemate::math::Matrix<double> Inv(N, N);
        for (std::size_t i = 0; i < N; ++i)
            for (std::size_t j = 0; j < N; ++j)
                Inv(i, j) = R(i, N + j);
        return fromDoubleMatrix_(Inv);
    }
    MatrixA aug(N, 2 * N);
    for (std::size_t i = 0; i < N; ++i) {
        for (std::size_t j = 0; j < N; ++j) aug(i, j) = M(i, j);
        for (std::size_t j = 0; j < N; ++j)
            aug(i, N + j) = (i == j) ? Scalar((long long)1) : Scalar();
    }
    MatrixA R = gaussEliminateAlg(aug, true);
    // 左半必须是单位矩阵
    for (std::size_t i = 0; i < N; ++i) {
        for (std::size_t j = 0; j < N; ++j) {
            bool ok = (i == j) ? (R(i, j) - Scalar((long long)1)).isZero()
                               : R(i, j).isZero();
            if (!ok) throw std::runtime_error("矩阵奇异, 不可逆");
        }
    }
    MatrixA Inv(N, N);
    for (std::size_t i = 0; i < N; ++i)
        for (std::size_t j = 0; j < N; ++j)
            Inv(i, j) = R(i, N + j);
    return Inv;
}

// 判断 AST 子树是否只含纯代数结构 (Number/Ident/Unary/Binary),
// 用于赋值分派: 只有纯多项式表达式才走 astToPolyCoeffs_ 路径,
// 含 Call/Matrix 等节点的 RHS 走普通 eval_ (避免 bug 2:
// m = squarefree(x^3-x^2) 因 astToPolyCoeffs_ 不支持 K::Call 而抛错).
static bool isPureArithmeticAst_(const NodePtr& n) {
    using K = Node::Kind;
    if (!n) return false;
    switch (n->kind) {
        case K::Number: case K::Ident: return true;
        case K::Unary:  return isPureArithmeticAst_(n->child);
        case K::Binary: return isPureArithmeticAst_(n->lhs)
                            && isPureArithmeticAst_(n->rhs);
        default: return false; // Call / Matrix / Assign
    }
}

static int algRealToInt(const Scalar& x, const char* ctx) {
    if (!x.isRational())
        throw std::runtime_error(std::string(ctx) + " 需要整数参数");
    Fraction q = x.asRational();
    if (q.denominator() != BigInt(1))
        throw std::runtime_error(std::string(ctx) + " 需要整数参数");
    long long n = q.numerator().toLongLong();
    if (n > 1000000000 || n < -1000000000)
        throw std::runtime_error(std::string(ctx) + " 整数参数越界");
    return (int)n;
}

// ---------------- 多项式辅助 ----------------

// 从 Value(Polynomial) 提取 Polynomial<Fraction>. 系数必须全为有理数.
static Polynomial<Fraction> valueToPoly(const Value& v, const char* op) {
    if (!v.isPolynomial())
        throw std::runtime_error(std::string(op) + " 需要多项式参数");
    const auto& cs = v.asPolyCoeffs();
    Polynomial<Fraction> out;
    for (std::size_t i = 0; i < cs.size(); ++i) {
        if (!cs[i].isRational())
            throw std::runtime_error(std::string(op) + " 需要有理系数多项式");
        Fraction q = cs[i].asRational();
        if (q.sign() != 0)
            out = out + Polynomial<Fraction>::monomial(i, q);
    }
    return out;
}

static Value polyToValue(const Polynomial<Fraction>& p, const QString& var) {
    const auto& cs = p.coeffs();
    std::vector<Scalar> out;
    out.reserve(cs.size());
    for (const auto& q : cs) out.emplace_back(q);
    return Value(out, var);
}

// 将 Polynomial<Fraction> 转为 LaTeX 片段 (不包 $), 用于 note 内联显示.
// var 为变量的 LaTeX 形式 (如 "x" / "\\lambda ").
static QString polyFractionToLatex_(const Polynomial<Fraction>& p, const QString& var) {
    const auto& coeffs = p.coeffs();
    if (coeffs.empty()) return QStringLiteral("0");
    QString out;
    bool first = true;
    for (int i = (int)coeffs.size() - 1; i >= 0; --i) {
        const Fraction& c = coeffs[i];
        if (c.isZero()) continue;
        bool neg = (c.sign() < 0);
        Fraction absC = c.abs();
        QString sign;
        if (first) sign = neg ? QStringLiteral("-") : QString();
        else       sign = neg ? QStringLiteral(" - ") : QStringLiteral(" + ");
        QString absTex = QString::fromStdString(absC.toLatex());
        QString term;
        if (i == 0) {
            term = absTex;
        } else {
            QString coefPart = absC.isOne() ? QString() : absTex;
            QString varPart = (i == 1) ? var
                                       : QStringLiteral("%1^{%2}").arg(var).arg(i);
            term = coefPart + varPart;
        }
        out += sign + term;
        first = false;
    }
    if (out.isEmpty()) return QStringLiteral("0");
    return out;
}

// 多项式函数集: 这些函数可以接受含符号变量的表达式作为参数.
// 名称统一为全小写 (调用点已在 K::Call 处 toLower), 别名在此列出.
static bool isPolyFn_(const std::string& fn) {
    return fn == "gcd" || fn == "polygcd"
        || fn == "factor"
        || fn == "resultant" || fn == "res"
        || fn == "discriminant" || fn == "disc"
        || fn == "rationalroots" || fn == "rroots"
        || fn == "irreducible" || fn == "irred"
        || fn == "squarefree" || fn == "sqfree"
        || fn == "rfactor"
        || fn == "roots";
}

// Evaluator

std::vector<QString> Evaluator::supportedFunctions() {
    return {
        // 数值
        QStringLiteral("sqrt(x)"),  QStringLiteral("root(n, x)"),  QStringLiteral("abs(x)"),
        // 复数 (精确模式下仅 Q[i]; 参数含 sqrt 等按数值近似)
        QStringLiteral("re(z)"),  QStringLiteral("im(z)"),
        QStringLiteral("conj(z)"),  QStringLiteral("arg(z)"),
        // 矩阵构造
        QStringLiteral("Identity(n)"),  QStringLiteral("zeros(m, n)"),  QStringLiteral("ones(m, n)"),
        // 矩阵基础
        QStringLiteral("tr(M)"),  QStringLiteral("transpose(M)"),
        QStringLiteral("det(M)"),  QStringLiteral("rank(M)"),
        QStringLiteral("inv(M)"),  QStringLiteral("rref(M)"),
        // 线性方程
        QStringLiteral("solve(A, b)"),  QStringLiteral("nullspace(M)"),
        // 特征结构
        QStringLiteral("charpoly(M)"),  QStringLiteral("eigs(M)"),
        QStringLiteral("ceigs(M)"),
        // 二次型 / 对称矩阵
        QStringLiteral("issym(A)"),   QStringLiteral("signature(A)"),
        QStringLiteral("definiteness(A)"),
        QStringLiteral("congdiag(A)"),
        // 矩阵分解
        QStringLiteral("lu(A)"),  QStringLiteral("qr(A)"),
        QStringLiteral("svd(A)"),
        QStringLiteral("gramschmidt(V)"),
        // 标准形
        QStringLiteral("jordan(A)"),  QStringLiteral("rcf(A)"),
        // 多项式
        QStringLiteral("polygcd(p, q)"),  QStringLiteral("factor(p)"),
        QStringLiteral("resultant(f, g)"),  QStringLiteral("discriminant(f)"),
        QStringLiteral("rationalroots(p)"),
        QStringLiteral("squarefree(f)"),
        QStringLiteral("rfactor(p)"),
        QStringLiteral("minpoly(a)"),  QStringLiteral("minpoly(M)"),
        QStringLiteral("irreducible(f)"),  QStringLiteral("irreducible(f, p)"),
        QStringLiteral("roots(p)"),
        QStringLiteral("irredcnt(n, q)"),
        QStringLiteral("powerSumToSym(k, n)"),
        QStringLiteral("symToPowerSum(k, n)"),
    };
}

EvalResult Evaluator::evaluate(const QString& source) {
    EvalResult r;
    auto pr = parse(source.toStdString());
    if (!pr.ok) {
        r.ok = false;
        r.error = QString::fromStdString(pr.error);
        r.errorPos = pr.errorPos;
        return r;
    }
    try {
        lastCallNote_.clear();
        if (pr.root->kind == Node::Kind::Assign) {
            // i 保留作为虚数单位, 不允许被用户赋值覆盖.
            if (pr.root->name == "i")
                throw std::runtime_error("i 保留作为虚数单位, 不允许赋值");
            // 赋值 RHS 含唯一自由变量 → 构造多项式 Value (支持  f = x^2+1  /  f(x) = x^2+1).
            //   多变量或无自由变量的 RHS 仍走普通 eval_ 路径.
            Value v;
            std::set<std::string> fvs;
            collectFreeVars_(pr.root->child, fvs);
            if (fvs.size() == 1 && isPureArithmeticAst_(pr.root->child)) {
                const std::string& var = *fvs.begin();
                auto coeffs = astToPolyCoeffs_(pr.root->child, var);
                v = Value(coeffs, QString::fromStdString(var));
            } else {
                // RHS 含 Call (如 squarefree(...)) / Matrix / 多变量 → 普通求值.
                v = eval_(pr.root->child);
            }
            env_[pr.root->name] = v;
            r.ok = true;
            r.assignedName = QString::fromStdString(pr.root->name);
            r.value = v;
            r.typeDesc = v.typeLabel();
        } else {
            Value v = eval_(pr.root);
            env_["ans"] = v;
            r.ok = true;
            r.value = v;
            r.typeDesc = v.typeLabel();
        }
        r.extraNote = lastCallNote_;
    } catch (const std::exception& e) {
        r.ok = false;
        r.error = QString::fromUtf8(e.what());
        r.errorPos = -1;
    }
    return r;
}

Value Evaluator::eval_(const NodePtr& n) {
    using K = Node::Kind;
    switch (n->kind) {
    case K::Number: {
        // 字符串 -> Fraction -> AlgReal
        const std::string& s = n->name;
        if (s.find('.') == std::string::npos) {
            return Value(Scalar(Fraction(BigInt(s))));
        }
        // 小数: 把 a.bcd 转 (abcd) / 10^3
        auto dot = s.find('.');
        std::string intPart = s.substr(0, dot);
        std::string fracPart = s.substr(dot + 1);
        if (intPart.empty()) intPart = "0";
        if (fracPart.empty()) return Value(Scalar(Fraction(BigInt(intPart))));
        // 合并
        std::string whole = intPart + fracPart;
        // 除去负号处理: 若 intPart 负, 小数部分按绝对值处理
        bool neg = (!intPart.empty() && intPart[0] == '-');
        if (neg) whole = "-" + intPart.substr(1) + fracPart;
        BigInt num(whole);
        BigInt den(1);
        for (std::size_t i = 0; i < fracPart.size(); ++i) den = den * BigInt(10);
        return Value(Scalar(Fraction(num, den)));
    }
    case K::Ident: {
        // 虚数单位 i 保留, 不受环境变量覆盖 (Assign 路径已拦截 i 的赋值).
        if (n->name == "i") return Value(Complex::i());
        auto it = env_.find(n->name);
        if (it == env_.end())
            throw std::runtime_error("未定义的变量: " + n->name);
        return it->second;
    }
    case K::Matrix: {
        if (n->rows.empty() || n->rows.front().empty())
            throw std::runtime_error("不支持空矩阵");
        const std::size_t R = n->rows.size();
        const std::size_t C = n->rows.front().size();
        MatrixA M(R, C);
        for (std::size_t i = 0; i < R; ++i) {
            for (std::size_t j = 0; j < C; ++j) {
                Value v = eval_(n->rows[i][j]);
                if (!v.isScalar())
                    throw std::runtime_error("矩阵字面量的元素必须是标量");
                M(i, j) = v.asScalar();
            }
        }
        return Value(M);
    }
    case K::Unary: {
        Value a = eval_(n->child);
        if (n->name == "-") return a.neg();
        if (n->name == "'") return a.transpose();
        throw std::runtime_error("未知一元运算: " + n->name);
    }
    case K::Binary: {
        Value a = eval_(n->lhs);
        Value b = eval_(n->rhs);
        if (n->name == "+") return a.add(b);
        if (n->name == "-") return a.sub(b);
        if (n->name == "*") return a.mul(b);
        if (n->name == "/") return a.div(b);
        if (n->name == "^") return a.pow(b);
        throw std::runtime_error("未知二元运算: " + n->name);
    }
    case K::Call: {
        // 函数名大小写不敏感: 统一转小写再分发; 变量名 (env 查找) 仍保持原始大小写.
        std::string fn = n->name;
        std::transform(fn.begin(), fn.end(), fn.begin(),
                       [](unsigned char c) { return std::tolower(c); });
        // 用户自定义多项式函数调用: env 里 name 是多项式 → 代入求值.
        //   f = x^2 + 1  之后  f(2) → 5,  f(y) → 换元后的多项式(y),  f(2x+1) → 展开新多项式.
        //   为避免和 i 这类特殊标识混淆, 此处过滤 name == "i".
        if (n->name != "i") {
            auto it = env_.find(n->name);
            if (it != env_.end() && it->second.isPolynomial()) {
                if (n->args.size() != 1)
                    throw std::runtime_error(
                        "多项式 " + n->name + " 调用应有恰好 1 个参数");
                const Value& poly = it->second;
                const auto& cs = poly.asPolyCoeffs();
                Value argv = eval_(n->args[0]);
                // 标量求值. 若系数全为有理数, 用 AlgReal::evaluatePoly
                // (内部通过结式, 度数不膨胀) 一次求值, 避免 Horner 循环中
                // AlgReal 的 minPoly 度数指数爆炸 (4→16→64→256 导致崩溃, bug 3).
                if (argv.isScalar()) {
                    if (cs.empty()) return Value(Scalar(Fraction(0)));
                    bool allRational = true;
                    for (const auto& c : cs) {
                        if (!c.isRational()) { allRational = false; break; }
                    }
                    if (allRational) {
                        Polynomial<Fraction> fp;
                        for (std::size_t i = 0; i < cs.size(); ++i) {
                            Fraction q = cs[i].asRational();
                            if (q.sign() != 0)
                                fp = fp + Polynomial<Fraction>::monomial(i, q);
                        }
                        return Value(AlgReal::evaluatePoly(fp, argv.asScalar()));
                    }
                    // 系数含代数数 (罕见): 退化为 Horner (可能仍有爆炸风险, 但至少不是默认路径)
                    Scalar acc = cs.back();
                    for (int i = (int)cs.size() - 2; i >= 0; --i)
                        acc = acc * argv.asScalar() + cs[i];
                    return Value(acc);
                }
                // 复标量: 复数 Horner
                if (argv.isComplexScalar()) {
                    if (cs.empty()) return Value(ComplexC());
                    ComplexC acc(cs.back());
                    for (int i = (int)cs.size() - 2; i >= 0; --i)
                        acc = acc * argv.asComplex() + ComplexC(cs[i]);
                    return Value(acc);
                }
                // 多项式: 代入展开得到新多项式
                if (argv.isPolynomial()) {
                    const auto& ac = argv.asPolyCoeffs();
                    std::vector<Scalar> acc;
                    if (!cs.empty()) acc = { cs.back() };
                    for (int i = (int)cs.size() - 2; i >= 0; --i) {
                        // tmp = acc * ac
                        std::vector<Scalar> tmp;
                        if (acc.empty() || ac.empty()) tmp = {};
                        else {
                            tmp.assign(acc.size() + ac.size() - 1, Scalar(Fraction(0)));
                            for (std::size_t a = 0; a < acc.size(); ++a)
                                for (std::size_t b = 0; b < ac.size(); ++b)
                                    tmp[a + b] = tmp[a + b] + acc[a] * ac[b];
                        }
                        // + cs[i]
                        if (tmp.empty()) tmp.push_back(cs[i]);
                        else tmp[0] = tmp[0] + cs[i];
                        acc = std::move(tmp);
                    }
                    while (acc.size() > 1 && acc.back().isZero()) acc.pop_back();
                    if (acc.empty()) acc.push_back(Scalar(Fraction(0)));
                    return Value(acc, argv.polyVar());
                }
                throw std::runtime_error(
                    "多项式 " + n->name + " 调用参数类型不支持");
            }
        }
        // 多项式函数 + 参数含符号变量: 走 AST→多项式路径, 避开对未定义变量的 eval.
        if (isPolyFn_(fn)) {
            std::set<std::string> fvs;
            for (auto& a : n->args) collectFreeVars_(a, fvs);
            if (!fvs.empty()) {
                if (fvs.size() > 1) {
                    std::string msg = "多项式函数暂不支持多个未知数, 发现: ";
                    bool first = true;
                    for (const auto& v : fvs) {
                        if (!first) msg += ", ";
                        msg += v;
                        first = false;
                    }
                    throw std::runtime_error(msg);
                }
                std::string var = *fvs.begin();
                std::vector<Value> pargs;
                pargs.reserve(n->args.size());
                for (auto& a : n->args) {
                    auto coeffs = astToPolyCoeffs_(a, var);
                    pargs.push_back(Value(coeffs, QString::fromStdString(var)));
                }
                return callFn_(fn, pargs);
            }
        }
        std::vector<Value> args;
        args.reserve(n->args.size());
        for (auto& a : n->args) args.push_back(eval_(a));
        return callFn_(fn, args);
    }
    case K::Assign:
        throw std::runtime_error("赋值语句不能嵌套在表达式中");
    }
    throw std::runtime_error("内部错误: 未知 AST 类型");
}

// Durand-Kerner: 求首一多项式 c[0..n] (c[n]=1) 的全部复根
static std::vector<std::complex<double>> durandKernerRoots(std::vector<std::complex<double>> c) {
    std::size_t deg = c.size() - 1;
    if (deg == 0) return {};
    std::vector<std::complex<double>> roots(deg);
    {
        std::complex<double> seed(0.4, 0.9), cur(1.0, 0.0);
        for (std::size_t k = 0; k < deg; ++k) { roots[k] = cur; cur *= seed; }
    }
    for (int it = 0; it < 400; ++it) {
        double maxDelta = 0.0;
        for (std::size_t k = 0; k < deg; ++k) {
            std::complex<double> y = c[deg];
            for (int j = (int)deg - 1; j >= 0; --j) y = y * roots[k] + c[j];
            std::complex<double> den(1.0, 0.0);
            for (std::size_t j = 0; j < deg; ++j)
                if (j != k) den *= (roots[k] - roots[j]);
            if (std::abs(den) < 1e-300) continue;
            std::complex<double> delta = y / den;
            if (std::abs(delta) > 1e6) delta = delta * (1e6 / std::abs(delta));
            roots[k] -= delta;
            if (std::abs(delta) > maxDelta) maxDelta = std::abs(delta);
        }
        if (maxDelta < 1e-14) break;
    }
    std::sort(roots.begin(), roots.end(),
        [](const std::complex<double>& a, const std::complex<double>& b){
            if (std::abs(a.real() - b.real()) > 1e-10) return a.real() < b.real();
            return a.imag() < b.imag();
        });
    return roots;
}

Value Evaluator::callFn_(const std::string& fn, const std::vector<Value>& args) {
    auto need = [&](std::size_t k) {
        if (args.size() != k)
            throw std::runtime_error(fn + " 需要 " + std::to_string(k) + " 个参数");
    };

    // 标量函数
    // sqrt / root 走精确代数数路径: sqrt(2) 返回真正的 √2 (而非有理近似),
    //   保证 minpoly(sqrt(2)+sqrt(3)) = x^4 - 10x^2 + 1 精确正确.
    //   复数参数仍走数值路径 (Complex 未实现符号开方).
    if (fn == "sqrt") {
        need(1);
        if (args[0].isComplexScalar()) {
            auto ab = args[0].asComplex().toDouble();
            std::complex<double> z(ab.first, ab.second);
            std::complex<double> r = std::sqrt(z);
            return Value(Complex(AlgReal::fromDouble(r.real()),
                                 AlgReal::fromDouble(r.imag())));
        }
        if (!args[0].isScalar()) throw std::runtime_error("sqrt 需要标量");
        const Scalar& x = args[0].asScalar();
        int sg = x.sign();
        if (sg == 0) return Value(Scalar());
        if (sg < 0) {
            // sqrt(-k) = i * sqrt(k)
            Scalar pos = -x;
            Scalar r = pos.isRational()
                ? AlgReal::sqrt(pos.asRational())
                : AlgReal::sqrt(pos);
            return Value(Complex(AlgReal(), r));
        }
        return Value(x.isRational()
            ? AlgReal::sqrt(x.asRational())
            : AlgReal::sqrt(x));
    }
    if (fn == "root") {
        need(2);
        if (!args[0].isScalar() || !args[1].isScalar())
            throw std::runtime_error("root 需要标量参数");
        int nIdx = algRealToInt(args[0].asScalar(), "root 的第一个参数 (次数)");
        if (nIdx < 2)
            throw std::runtime_error("root 的第一个参数 (次数) 必须 >= 2");
        const Scalar& x = args[1].asScalar();
        int sg = x.sign();
        if (sg == 0) return Value(Scalar());
        if ((nIdx % 2 == 0) && sg < 0)
            throw std::runtime_error("root 偶次幂要求参数非负");
        if (sg < 0) {
            // 奇次负根: -root(n, -x)
            Scalar pos = -x;
            Scalar r = pos.isRational()
                ? AlgReal::nthRoot(pos.asRational(), nIdx)
                : AlgReal::nthRoot(pos, nIdx);
            return Value(-r);
        }
        return Value(x.isRational()
            ? AlgReal::nthRoot(x.asRational(), nIdx)
            : AlgReal::nthRoot(x, nIdx));
    }
    if (fn == "abs") {
        need(1);
        if (args[0].isComplexScalar()) {
            auto ab = args[0].asComplex().toDouble();
            double m = std::sqrt(ab.first * ab.first + ab.second * ab.second);
            return Value(AlgReal::fromDouble(m));
        }
        if (!args[0].isScalar())
            throw std::runtime_error("abs 暂仅支持标量/复标量参数");
        const Scalar& x = args[0].asScalar();
        return Value(x.sign() < 0 ? -x : x);
    }
    if (fn == "re" || fn == "real") {
        need(1);
        if (args[0].isComplexScalar()) return Value(args[0].asComplex().real());
        if (args[0].isScalar())        return args[0];
        throw std::runtime_error("re 需要标量/复标量");
    }
    if (fn == "im" || fn == "imag") {
        need(1);
        if (args[0].isComplexScalar()) return Value(args[0].asComplex().imag());
        if (args[0].isScalar())        return Value(Scalar());  // 实数的虚部 = 0
        throw std::runtime_error("im 需要标量/复标量");
    }
    if (fn == "conj" || fn == "conjugate") {
        need(1);
        if (args[0].isComplexScalar()) {
            ComplexC z = args[0].asComplex();
            return Value(z.conjugate());
        }
        if (args[0].isScalar()) return args[0];
        throw std::runtime_error("conj 需要标量/复标量");
    }
    if (fn == "arg") {
        need(1);
        double re, im;
        if (args[0].isComplexScalar()) {
            auto ab = args[0].asComplex().toDouble();
            re = ab.first; im = ab.second;
        } else if (args[0].isScalar()) {
            re = args[0].asScalar().toDouble(); im = 0.0;
        } else throw std::runtime_error("arg 需要标量/复标量");
        return Value(AlgReal::fromDouble(std::atan2(im, re)));
    }

    // ---- 矩阵构造 ----
    if (fn == "Identity" || fn == "identity") {
        need(1);
        int k = algRealToInt(args[0].asScalar(), fn.c_str());
        if (k <= 0) throw std::runtime_error(fn + " 维数必须是正整数");
        return Value(MatrixA::identity((std::size_t)k));
    }
    if (fn == "zeros" || fn == "ones") {
        need(2);
        int m = algRealToInt(args[0].asScalar(), fn.c_str());
        int p = algRealToInt(args[1].asScalar(), fn.c_str());
        if (m <= 0 || p <= 0) throw std::runtime_error(fn + " 尺寸必须为正整数");
        return Value(fn == "zeros"
            ? MatrixA::zeros((std::size_t)m, (std::size_t)p)
            : MatrixA::ones ((std::size_t)m, (std::size_t)p));
    }

    // ---- 矩阵查询 / 运算 ----
    if (fn == "tr" || fn == "trace") {
        need(1);
        if (!args[0].isMatrix()) throw std::runtime_error(fn + " 需要矩阵参数");
        const MatrixA& M = args[0].asMatrix();
        if (!M.isSquare()) throw std::runtime_error(fn + " 要求方阵");
        Scalar sum;
        for (std::size_t i = 0; i < M.rows(); ++i) sum = sum + M(i, i);
        return Value(sum);
    }
    if (fn == "transpose") {
        need(1);
        return args[0].transpose();
    }

    // ---- 代数数矩阵支持的函数 ----
    if (fn == "det") {
        need(1);
        if (!args[0].isMatrix()) throw std::runtime_error("det 需要矩阵参数");
        return Value(detAlg(args[0].asMatrix()));
    }
    if (fn == "rank") {
        need(1);
        if (!args[0].isMatrix()) throw std::runtime_error("rank 需要矩阵参数");
        return Value(Scalar((long long)rankAlg(args[0].asMatrix())));
    }
    if (fn == "rref") {
        need(1);
        if (!args[0].isMatrix()) throw std::runtime_error("rref 需要矩阵参数");
        return Value(rrefAlg(args[0].asMatrix()));
    }
    if (fn == "inv") {
        need(1);
        if (!args[0].isMatrix()) throw std::runtime_error("inv 需要矩阵参数");
        return Value(invAlg(args[0].asMatrix()));
    }

    // ---- ceigs/complexeigs: 支持代数数矩阵 (自动转 double 数值计算) ----
    if (fn == "ceigs" || fn == "complexeigs") {
        need(1);
        if (!args[0].isMatrix())
            throw std::runtime_error(fn + " 需要矩阵参数");
        const MatrixA& M = args[0].asMatrix();
        if (!M.isSquare()) throw std::runtime_error(fn + " 要求方阵");

        auto numericalize = [](double x) {
            return Scalar(AlgReal::fromDouble(x, 1000000000));
        };

        if (hasAlgebraicElem_(M)) {
            // 代数数矩阵: 转 double → charpoly → Durand-Kerner 求全部复根
            auto D = toDoubleMatrix_(M);
            auto coeffs = charpolyDouble(D);
            std::size_t deg = coeffs.size() - 1;
            if (deg == 0)
                return Value::makeRootList({}, QString::fromUtf8("\xce\xbb"));
            double lc = coeffs[deg];
            if (std::abs(lc) < 1e-300)
                throw std::runtime_error(fn + ": 特征多项式首项为零");
            std::vector<std::complex<double>> c(deg + 1);
            for (std::size_t i = 0; i <= deg; ++i)
                c[i] = std::complex<double>(coeffs[i] / lc, 0.0);
            auto roots = durandKernerRoots(std::move(c));
            std::vector<std::pair<Scalar, Scalar>> out;
            out.reserve(deg);
            for (auto& r : roots) {
                double thRe = std::max(1e-7, 1e-10 * std::abs(r.real()));
                double thIm = std::max(1e-7, 1e-10 * std::abs(r.imag()));
                Scalar reS = (std::abs(r.real()) < thRe) ? Scalar() : numericalize(r.real());
                Scalar imS = (std::abs(r.imag()) < thIm) ? Scalar() : numericalize(r.imag());
                out.emplace_back(reS, imS);
            }
            lastCallNote_ = QStringLiteral("数值解");
            return Value::makeRootList(std::move(out), QString::fromUtf8("\xce\xbb"));
        } else {
            // 有理数矩阵: 精确复特征值算法
            Matrix<Fraction> F = toFractionMatrix(M, fn.c_str());
            auto res = algemate::math::complexEigenvalues(F);
            std::vector<std::pair<Scalar, Scalar>> out;
            for (const auto& ev : res.eigenvalues) {
                double re = ev.value.real().toDouble();
                double im = ev.value.imag().toDouble();
                double thRe = std::max(1e-7, 1e-10 * std::abs(re));
                double thIm = std::max(1e-7, 1e-10 * std::abs(im));
                Scalar reS = (std::abs(re) < thRe) ? Scalar() : numericalize(re);
                Scalar imS = (std::abs(im) < thIm) ? Scalar() : numericalize(im);
                for (int k = 0; k < ev.multiplicity; ++k)
                    out.emplace_back(reS, imS);
            }
            if (!res.unsolvedFactors.empty()) {
                lastCallNote_ = QStringLiteral("数值解, 已展开重根; 未解出因式 %1 个")
                    .arg(res.unsolvedFactors.size());
            } else {
                lastCallNote_ = QStringLiteral("数值解, 已展开重根");
            }
            return Value::makeRootList(std::move(out), QString::fromUtf8("\xce\xbb"));
        }
    }

    // ---- 以下函数仍需有理数矩阵 (特征多项式 / 实特征值求解) ----
    if (fn == "charpoly" || fn == "eigs" || fn == "eigenvalues"
        || fn == "nullspace") {
        need(1);
        if (!args[0].isMatrix())
            throw std::runtime_error(fn + " 需要矩阵参数");
        const MatrixA& M = args[0].asMatrix();
        Matrix<Fraction> F = toFractionMatrix(M, fn.c_str());

        if (fn == "charpoly") {
            if (!F.isSquare()) throw std::runtime_error("charpoly 要求方阵");
            auto p = algemate::math::charpoly(F);
            const auto& c = p.coeffs();
            std::vector<Scalar> cs;
            cs.reserve(c.size());
            for (const auto& q : c) cs.emplace_back(q);
            return Value(cs, QStringLiteral("\u03BB"));
        }
        if (fn == "eigs" || fn == "eigenvalues") {
            if (!F.isSquare()) throw std::runtime_error(fn + " 要求方阵");
            // charpoly + Durand-Kerner: 返回 RootList (按 λ_i = ... 逐行显示, 含重根).
            auto cp = algemate::math::charpoly(F);
            const auto& cs = cp.coeffs();
            std::size_t deg = cs.empty() ? 0 : cs.size() - 1;
            if (deg == 0)
                return Value::makeRootList({}, QString::fromUtf8("\xce\xbb"));
            double lc = cs[deg].toDouble();
            if (std::abs(lc) < 1e-300)
                throw std::runtime_error("eigs: 特征多项式首项为零");
            std::vector<std::complex<double>> c(deg + 1);
            for (std::size_t i = 0; i <= deg; ++i)
                c[i] = std::complex<double>(cs[i].toDouble() / lc, 0.0);
            auto roots = durandKernerRoots(std::move(c));
            // 混合阈: 给定根, 虚部 Θ(|re|*1e-10) 以下 或 绝对 1e-7 以下, 都视为 0.
            // eigs 仅返回实特征值: 虚部大于阈值的根跳过; 全复根时返回空列表 (渲染为 "无").
            auto toScalar = [](double x){
                return Scalar(algemate::math::AlgReal::fromDouble(x, 1000000000));
            };
            std::vector<std::pair<Scalar, Scalar>> out;
            out.reserve(deg);
            for (auto& r : roots) {
                double th = std::max(1e-7, 1e-10 * std::abs(r.real()));
                if (std::abs(r.imag()) >= th) continue;  // 非实根 → 丢弃
                out.emplace_back(toScalar(r.real()), Scalar());
            }
            if (out.empty())
                lastCallNote_ = QStringLiteral("无实特征值; 如需全部复特征值请用 ceigs");
            else
                lastCallNote_ = QStringLiteral("实特征值 (Durand-Kerner), 重根会被重复列出");
            return Value::makeRootList(std::move(out), QString::fromUtf8("\xce\xbb"));
        }
        if (fn == "nullspace") {
            auto ns = algemate::math::nullspace(F);
            auto nsA = toAlgRealMatrix(ns);
            // 拆列 → 向量列表 (空则渲染为 "无", 即平凡零空间).
            std::vector<MatrixA> vecs;
            vecs.reserve(nsA.cols());
            for (std::size_t j = 0; j < nsA.cols(); ++j) {
                MatrixA col(nsA.rows(), 1);
                for (std::size_t i = 0; i < nsA.rows(); ++i)
                    col(i, 0) = nsA(i, j);
                vecs.push_back(std::move(col));
            }
            if (vecs.empty())
                lastCallNote_ = QStringLiteral("平凡零空间 (仅零向量)");
            else
                lastCallNote_ = QStringLiteral("零空间维数 %1").arg(vecs.size());
            return Value::makeVectorList(std::move(vecs), QString::fromUtf8("\xce\xb7"));
        }
    }

    // ---- 线性方程: solve(A, b) ----
    if (fn == "solve") {
        need(2);
        if (!args[0].isMatrix() || !args[1].isMatrix())
            throw std::runtime_error("solve 需要矩阵 A 和右端向量 b");
        Matrix<Fraction> A = toFractionMatrix(args[0].asMatrix(), "solve");
        Matrix<Fraction> b = toFractionMatrix(args[1].asMatrix(), "solve");
        auto res = algemate::math::solve(A, b);
        if (!res.hasSolution) {
            lastCallNote_ = QStringLiteral("方程无解");
            return Value(MatrixA(0, 0));
        }
        if (res.nullspaceBasis.cols() == 0) {
            lastCallNote_ = QStringLiteral("唯一解");
            return Value(toAlgRealMatrix(res.particular));
        }
        lastCallNote_ = QStringLiteral(
            "无穷多解, 返回特解; 零空间维 %1, 请用 nullspace(A) 查看基")
            .arg(res.nullspaceBasis.cols());
        return Value(toAlgRealMatrix(res.particular));
    }

    // ---- 多项式上的函数 ----
    if (fn == "polygcd" || fn == "gcd") {
        need(2);
        auto a = valueToPoly(args[0], fn.c_str());
        auto b = valueToPoly(args[1], fn.c_str());
        auto g = algemate::math::gcd(a, b);
        QString var = args[0].polyVar();
        if (var.isEmpty()) var = args[1].polyVar();
        if (var.isEmpty()) var = QStringLiteral("x");
        return polyToValue(g, var);
    }
    if (fn == "factor") {
        need(1);
        auto p = valueToPoly(args[0], "factor");
        auto res = algemate::math::factorOverQ(p);
        QString var = args[0].polyVar();
        if (var.isEmpty()) var = QStringLiteral("x");
        // Fraction → Scalar 转换
        std::vector<Scalar> origCoeffs;
        origCoeffs.reserve(p.coeffs().size());
        for (const auto& q : p.coeffs()) origCoeffs.emplace_back(q);
        Scalar leading(res.leadingCoefficient);
        std::vector<std::pair<std::vector<Scalar>, int>> facs;
        facs.reserve(res.factors.size());
        for (const auto& fe : res.factors) {
            std::vector<Scalar> fc;
            fc.reserve(fe.first.coeffs().size());
            for (const auto& q : fe.first.coeffs()) fc.emplace_back(q);
            facs.emplace_back(std::move(fc), fe.second);
        }
        return Value::makeFactored(std::move(origCoeffs), std::move(leading),
                                   std::move(facs), var);
    }
    if (fn == "resultant" || fn == "res") {
        need(2);
        auto f = valueToPoly(args[0], "resultant");
        auto g = valueToPoly(args[1], "resultant");
        return Value(Scalar(algemate::math::resultant(f, g)));
    }
    if (fn == "discriminant" || fn == "disc") {
        need(1);
        auto f = valueToPoly(args[0], "discriminant");
        return Value(Scalar(algemate::math::discriminant(f)));
    }
    if (fn == "rationalroots" || fn == "rroots") {
        need(1);
        auto f = valueToPoly(args[0], fn.c_str());
        auto rs = algemate::math::rationalRoots(f);
        if (rs.empty()) {
            lastCallNote_ = QStringLiteral("无有理根");
            return Value::makeRootList({}, QStringLiteral("x"));
        }
        std::vector<std::pair<Scalar, Scalar>> out;
        out.reserve(rs.size());
        for (const auto& r : rs) out.emplace_back(Scalar(r), Scalar());
        return Value::makeRootList(std::move(out), QStringLiteral("x"));
    }
    if (fn == "roots") {
        need(1);
        auto f = valueToPoly(args[0], fn.c_str());
        const auto& cs = f.coeffs();
        std::size_t deg = cs.empty() ? 0 : cs.size() - 1;
        if (deg == 0) {
            lastCallNote_ = QStringLiteral("多项式为常数函数，无根");
            return Value::makeRootList({}, QStringLiteral("x"));
        }
        double lc = cs[deg].toDouble();
        std::vector<std::complex<double>> c(deg + 1);
        for (std::size_t i = 0; i <= deg; ++i)
            c[i] = std::complex<double>(cs[i].toDouble() / lc, 0.0);
        auto roots = durandKernerRoots(std::move(c));
        auto toScalar = [](double x){
            // 用大分母分数强制显示为小数（denom > 10000 时 scalarToStr 输出小数）
            long long num = static_cast<long long>(std::round(x * 1e8));
            return Scalar(algemate::math::Fraction(
                algemate::math::BigInt(num), algemate::math::BigInt(100000000LL)));
        };
        std::vector<std::pair<Scalar, Scalar>> out;
        out.reserve(roots.size());
        // roots 返回全部复根（实+虚），虚部接近于 0 的视为实根
        for (auto& r : roots) {
            double th = std::max(1e-7, 1e-10 * std::abs(r.real()));
            double im = (std::abs(r.imag()) < th) ? 0.0 : r.imag();
            out.emplace_back(toScalar(r.real()), toScalar(im));
        }
        return Value::makeRootList(std::move(out), args[0].polyVar().isEmpty()
            ? QStringLiteral("x") : args[0].polyVar());
    }
    if (fn == "rfactor") {
        need(1);
        auto p = valueToPoly(args[0], "rfactor");
        auto res = algemate::math::factorOverQ(p);
        QString var = args[0].polyVar();
        if (var.isEmpty()) var = QStringLiteral("x");

        std::vector<Scalar> origCoeffs;
        origCoeffs.reserve(p.coeffs().size());
        for (const auto& q : p.coeffs()) origCoeffs.emplace_back(q);
        Scalar leading(res.leadingCoefficient);

        std::vector<std::pair<std::vector<Scalar>, int>> facs;
        for (const auto& fe : res.factors) {
            int deg = fe.first.degree();
            if (deg == 1) {
                std::vector<Scalar> fc;
                for (const auto& q : fe.first.coeffs()) fc.emplace_back(q);
                facs.emplace_back(std::move(fc), fe.second);
            } else {
                // Irreducible factor of degree ≥ 2 over ℚ
                // Use Durand-Kerner to find all complex roots, pair conjugates
                const auto& fcs = fe.first.coeffs();
                double lc = fcs[deg].toDouble();
                std::vector<std::complex<double>> c(deg + 1);
                for (int i = 0; i <= deg; ++i)
                    c[i] = std::complex<double>(fcs[i].toDouble() / lc, 0.0);
                auto roots = durandKernerRoots(std::move(c));

                // Separate real roots from complex conjugate pairs
                std::vector<double> reals;
                std::vector<std::pair<double,double>> cplx;
                std::vector<bool> used(roots.size(), false);
                for (std::size_t i = 0; i < roots.size(); ++i) {
                    if (used[i]) continue;
                    double im = roots[i].imag();
                    double th = std::max(1e-8, 1e-10 * std::abs(roots[i].real()));
                    if (std::abs(im) < th) {
                        for (int k = 0; k < fe.second; ++k) reals.push_back(roots[i].real());
                    } else {
                        for (std::size_t j = i + 1; j < roots.size(); ++j) {
                            if (!used[j] && std::abs(roots[j].real() - roots[i].real()) < 1e-8
                                && std::abs(roots[j].imag() + im) < 1e-8) {
                                used[j] = true;
                                for (int k = 0; k < fe.second; ++k)
                                    cplx.push_back({roots[i].real(), std::abs(im)});
                                break;
                            }
                        }
                    }
                    used[i] = true;
                }

                // Build linear factors from real roots
                for (double r : reals) {
                    long long num = static_cast<long long>(std::round(r * 1e8));
                    Fraction fr(BigInt(num), BigInt(100000000LL));
                    facs.emplace_back(std::vector<Scalar>{Scalar(-fr), Scalar(Fraction(1))}, 1);
                }
                // Build quadratic factors from complex pairs
                for (const auto& cp : cplx) {
                    double r = cp.first, s = cp.second;
                    double b = -2.0 * r;
                    double c2 = r*r + s*s;
                    long long nb = static_cast<long long>(std::round(b * 1e8));
                    long long nc = static_cast<long long>(std::round(c2 * 1e8));
                    Fraction fb(BigInt(nb), BigInt(100000000LL));
                    Fraction fc2(BigInt(nc), BigInt(100000000LL));
                    facs.emplace_back(std::vector<Scalar>{Scalar(fc2), Scalar(fb), Scalar(Fraction(1))}, 1);
                }
            }
        }
        return Value::makeFactored(std::move(origCoeffs), std::move(leading),
                                   std::move(facs), var);
    }

    if (fn == "irredcnt") {
        need(2);
        auto getInt = [](const Value& v, const char* name) -> long long {
            const Scalar& s = v.asScalar();
            if (!s.isRational()) throw std::runtime_error(std::string(name) + " 必须是整数");
            Fraction f = s.asRational();
            if (!f.denominator().isOne()) throw std::runtime_error(std::string(name) + " 必须是整数");
            return f.numerator().toLongLong();
        };
        long long n = getInt(args[0], "irredcnt: n");
        long long q = getInt(args[1], "irredcnt: q");
        if (n < 1) throw std::runtime_error("irredcnt: n 必须是正整数");
        if (q < 2) throw std::runtime_error("irredcnt: q 必须是大于 1 的整数");
        // 验证 q 是素数的方幂
        {
            long long qq = q;
            long long p = 0; int k = 0;
            for (long long d = 2; d * d <= qq; ++d) {
                if (qq % d == 0) {
                    p = d; while (qq % d == 0) { qq /= d; ++k; }
                    break;
                }
            }
            if (p == 0) {
                // qq 本身是素数 (包含 q=2,3,5,7 等一次幂情况): p^1
                p = qq; k = 1; qq = 1;
            }
            if (qq != 1) throw std::runtime_error(
                "irredcnt: 有限域的阶一定是素数的方幂, q=" + std::to_string(q) + " 不合法");
        }
        // Möbius function μ(d)
        auto mobius = [](long long d) -> int {
            if (d == 1) return 1;
            int cnt = 0;
            for (long long p = 2; p * p <= d; ++p) {
                if (d % p == 0) {
                    ++cnt;
                    int c2 = 0;
                    while (d % p == 0) { d /= p; ++c2; }
                    if (c2 > 1) return 0;
                }
            }
            if (d > 1) ++cnt;
            return (cnt % 2) ? -1 : 1;
        };
        // Compute (1/n) Σ_{d|n} μ(d) · q^{n/d}
        BigInt result(0);
        for (long long d = 1; d * d <= n; ++d) {
            if (n % d == 0) {
                int mu = mobius(d);
                if (mu != 0) {
                    BigInt q_pow = BigInt(1);
                    long long exp = n / d;
                    BigInt qb(q);
                    for (long long e = exp; e > 0; e >>= 1) {
                        if (e & 1) q_pow = q_pow * qb;
                        qb = qb * qb;
                    }
                    if (mu == 1) result = result + q_pow;
                    else result = result - q_pow;
                }
                if (d * d != n) {
                    long long d2 = n / d;
                    int mu2 = mobius(d2);
                    if (mu2 != 0) {
                        BigInt q_pow = BigInt(1);
                        long long exp = n / d2;
                        BigInt qb(q);
                        for (long long e = exp; e > 0; e >>= 1) {
                            if (e & 1) q_pow = q_pow * qb;
                            qb = qb * qb;
                        }
                        if (mu2 == 1) result = result + q_pow;
                        else result = result - q_pow;
                    }
                }
            }
        }
        BigInt nn(n);
        // result / n → Fraction
        Fraction answer(result, nn);
        if (answer.denominator().isOne())
            return Value(Scalar(answer.numerator()));
        return Value(Scalar(answer));
    }

    if (fn == "powersumtosym") {
        need(2);
        auto getInt = [](const Value& v, const char* name) -> long long {
            const Scalar& s = v.asScalar();
            if (!s.isRational()) throw std::runtime_error(std::string(name) + " 必须是整数");
            Fraction f = s.asRational();
            if (!f.denominator().isOne()) throw std::runtime_error(std::string(name) + " 必须是整数");
            return f.numerator().toLongLong();
        };
        long long k = getInt(args[0], "powerSumToSym: k");
        long long n = getInt(args[1], "powerSumToSym: n");
        if (k < 1) throw std::runtime_error("powerSumToSym: k 必须是正整数");
        if (n < 1) throw std::runtime_error("powerSumToSym: n 必须是正整数");
        auto poly = algemate::math::mpoly::powerSumToSym(static_cast<int>(k), static_cast<int>(n));
        int m = std::min(static_cast<int>(k), static_cast<int>(n));
        std::vector<std::string> names;
        for (int i = 1; i <= m; ++i)
            names.push_back("\\sigma_{" + std::to_string(i) + "}");
        std::string result = "$s_{" + std::to_string(k) + "} = " + poly.toString(names) + "$";
        return Value::makeText(QString::fromStdString(result));
    }
    if (fn == "symtopowersum") {
        need(2);
        auto getInt = [](const Value& v, const char* name) -> long long {
            const Scalar& s = v.asScalar();
            if (!s.isRational()) throw std::runtime_error(std::string(name) + " 必须是整数");
            Fraction f = s.asRational();
            if (!f.denominator().isOne()) throw std::runtime_error(std::string(name) + " 必须是整数");
            return f.numerator().toLongLong();
        };
        long long k = getInt(args[0], "symToPowerSum: k");
        long long n = getInt(args[1], "symToPowerSum: n");
        if (k < 1) throw std::runtime_error("symToPowerSum: k 必须是正整数");
        if (n < 1) throw std::runtime_error("symToPowerSum: n 必须是正整数");
        if (k > n) throw std::runtime_error("symToPowerSum: k 不能大于 n (不存在 σ_{" + std::to_string(k) + "} in " + std::to_string(n) + " variables)");
        auto poly = algemate::math::mpoly::symToPowerSum(static_cast<int>(k), static_cast<int>(n));
        std::vector<std::string> names;
        for (int i = 1; i <= static_cast<int>(k); ++i)
            names.push_back("s_{" + std::to_string(i) + "}");
        std::string result = "$\\sigma_{" + std::to_string(k) + "} = " + poly.toString(names) + "$";
        return Value::makeText(QString::fromStdString(result));
    }

    // 最小多项式
    //   minpoly(α): 代数数 α 在 Q 上的最小多项式 (AlgReal::minPoly)
    //   minpoly(A): 方阵 A 的最小多项式 (= 最大不变因子, Frobenius)
    if (fn == "minpoly" || fn == "minimalpolynomial") {
        need(1);
        if (args[0].isScalar()) {
            const auto& mp = args[0].asScalar().minPoly();
            return polyToValue(mp, QStringLiteral("x"));
        }
        if (args[0].isMatrix()) {
            const MatrixA& M = args[0].asMatrix();
            if (!M.isSquare()) throw std::runtime_error("minpoly 要求方阵");
            Matrix<Fraction> F = toFractionMatrix(M, "minpoly");
            auto mp = algemate::math::minimalPolynomial(F);
            return polyToValue(mp, QStringLiteral("x"));
        }
        throw std::runtime_error("minpoly 需要代数数或方阵参数");
    }

    // ---- 不可约判定 ----
    //   irreducible(f)   : f 在 Q 上是否不可约 (调 factorOverQ, 因式数 == 1 且重数 == 1)
    //   irreducible(f, p): f 在 F_p 上是否不可约 (p 为素数)
    if (fn == "irreducible" || fn == "irred") {
        if (args.size() != 1 && args.size() != 2)
            throw std::runtime_error(fn + " 需要 1 或 2 个参数");
        auto f = valueToPoly(args[0], fn.c_str());
        if (f.degree() < 1)
            throw std::runtime_error(fn + " 要求多项式次数 >= 1");
        if (args.size() == 1) {
            auto rf = algemate::math::factorOverQ(f);
            bool irr = (rf.factors.size() == 1 && rf.factors[0].second == 1);
            lastCallNote_ = irr ? QStringLiteral("$\\mathbb{Q}[x]$ 中不可约")
                                : QStringLiteral("$\\mathbb{Q}[x]$ 中可约");
            return Value(Scalar(Fraction(irr ? 1 : 0)));
        }
        // F_p 分支
        // polyFn 路径下 p 会被包成常数多项式, 需提取标量.
        Scalar pScalar;
        if (args[1].isScalar()) {
            pScalar = args[1].asScalar();
        } else if (args[1].isPolynomial()
                   && args[1].asPolyCoeffs().size() == 1) {
            pScalar = args[1].asPolyCoeffs()[0];
        } else {
            throw std::runtime_error(fn + " 第二参数 p 必须是素数");
        }
        if (!pScalar.isRational())
            throw std::runtime_error(fn + " 第二参数 p 必须是素数");
        Fraction qp = pScalar.asRational();
        if (qp.denominator() != BigInt(1))
            throw std::runtime_error(fn + " 第二参数 p 必须是素数");
        long long p = qp.numerator().toLongLong();
        if (p < 2)
            throw std::runtime_error(fn + " 第二参数 p 必须是 >= 2 的素数");
        auto isPrime = [](long long k) {
            if (k < 2) return false;
            for (long long i = 2; i * i <= k; ++i)
                if (k % i == 0) return false;
            return true;
        };
        if (!isPrime(p))
            throw std::runtime_error(fn + QStringLiteral(" 第二参数 %1 不是素数")
                                     .arg(p).toStdString());
        auto fp = PolynomialZp::fromPoly(f, p);
        if (fp.degree() < 1)
            throw std::runtime_error(fn + QStringLiteral(" 模 %1 后多项式退化为常数")
                                     .arg(p).toStdString());
        bool irr = PolynomialZp::isIrreducible(fp);
        lastCallNote_ = QStringLiteral("$\\mathbb{Z}_{%1}[x]$ 中 %2")
                            .arg(p).arg(irr ? QStringLiteral("不可约") : QStringLiteral("可约"));
        return Value(Scalar(Fraction(irr ? 1 : 0)));
    }

    // 无平方部分
    if (fn == "squarefree" || fn == "sqfree") {
        need(1);
        auto f = valueToPoly(args[0], fn.c_str());
        auto sp = algemate::math::squarefreePart(f);
        QString var = args[0].polyVar();
        if (var.isEmpty()) var = QStringLiteral("x");
        return polyToValue(sp, var);
    }

    // ================ 二次型 / 对称矩阵 ================
    if (fn == "issym" || fn == "issymmetric") {
        need(1);
        if (!args[0].isMatrix()) throw std::runtime_error(fn + " 需要矩阵参数");
        Matrix<Fraction> A = toFractionMatrix(args[0].asMatrix(), fn.c_str());
        bool sym = algemate::math::isSymmetric(A);
        lastCallNote_ = sym ? QStringLiteral("对称矩阵")
                            : QStringLiteral("非对称矩阵");
        return Value(Scalar(Fraction(sym ? 1 : 0)));
    }
    if (fn == "signature") {
        need(1);
        if (!args[0].isMatrix()) throw std::runtime_error("signature 需要矩阵参数");
        Matrix<Fraction> A = toFractionMatrix(args[0].asMatrix(), "signature");
        if (!algemate::math::isSymmetric(A))
            throw std::runtime_error("signature 要求对称矩阵");
        auto sig = algemate::math::quadraticSignature(A);
        QString text = QStringLiteral(
            "惯性指数 $(p^+, p^-, p^0) = (%1, %2, %3)$")
            .arg(sig.positive).arg(sig.negative).arg(sig.zero);
        lastCallNote_.clear();
        return Value::makeText(std::move(text));
    }
    if (fn == "definiteness" || fn == "definite") {
        need(1);
        if (!args[0].isMatrix()) throw std::runtime_error(fn + " 需要矩阵参数");
        Matrix<Fraction> A = toFractionMatrix(args[0].asMatrix(), fn.c_str());
        if (!algemate::math::isSymmetric(A))
            throw std::runtime_error(fn + " 要求对称矩阵");
        auto cls = algemate::math::classifyQuadraticForm(A);
        auto sig = algemate::math::quadraticSignature(A);
        using algemate::math::DefiniteClass;
        QString tag;
        switch (cls) {
            case DefiniteClass::PositiveDefinite:
                tag = QStringLiteral("正定 ($A \\succ 0$)"); break;
            case DefiniteClass::PositiveSemidefinite:
                tag = QStringLiteral("半正定 ($A \\succeq 0$)"); break;
            case DefiniteClass::NegativeDefinite:
                tag = QStringLiteral("负定 ($A \\prec 0$)"); break;
            case DefiniteClass::NegativeSemidefinite:
                tag = QStringLiteral("半负定 ($A \\preceq 0$)"); break;
            case DefiniteClass::Indefinite:
                tag = QStringLiteral("不定"); break;
            case DefiniteClass::Zero:
                tag = QStringLiteral("零型"); break;
        }
        lastCallNote_ = QStringLiteral("惯性指数 $(p^+, p^-, p^0) = (%1, %2, %3)$")
                            .arg(sig.positive).arg(sig.negative).arg(sig.zero);
        return Value::makeText(std::move(tag));
    }

    // ================ 正交化 / 合同对角化 / 矩阵分解 ================
    if (fn == "gramschmidt" || fn == "gso") {
        need(1);
        if (!args[0].isMatrix()) throw std::runtime_error(fn + " 需要矩阵参数");
        Matrix<Fraction> V = toFractionMatrix(args[0].asMatrix(), fn.c_str());
        auto Q = algemate::math::gramSchmidtOrthonormal(V);
        if (Q.cols() < V.cols())
            lastCallNote_ = QStringLiteral("列向量组线性相关, 已自动丢弃冗余列 (输出 %1 列)")
                                .arg(Q.cols());
        return Value(Q);
    }
    if (fn == "congdiag" || fn == "congruence") {
        need(1);
        if (!args[0].isMatrix()) throw std::runtime_error(fn + " 需要矩阵参数");
        Matrix<Fraction> A = toFractionMatrix(args[0].asMatrix(), fn.c_str());
        if (!algemate::math::isSymmetric(A))
            throw std::runtime_error(fn + " 要求对称矩阵");
        auto res = algemate::math::congruenceDiagonalize(A);
        std::vector<std::pair<QString, MatrixA>> items;
        items.emplace_back(QStringLiteral("P ^{T} A P = D, \\quad D ="),
                           toAlgRealMatrix(res.D));
        items.emplace_back(QStringLiteral("P ="), toAlgRealMatrix(res.P));
        lastCallNote_ = QStringLiteral("合同对角化: $P^{T} A P = D$");
        return Value::makeNamedMatrices(std::move(items));
    }
    if (fn == "lu") {
        need(1);
        if (!args[0].isMatrix()) throw std::runtime_error("lu 需要矩阵参数");
        Matrix<Fraction> A = toFractionMatrix(args[0].asMatrix(), "lu");
        auto res = algemate::math::luDecompose(A);
        std::vector<std::pair<QString, MatrixA>> items;
        items.emplace_back(QStringLiteral("L ="), toAlgRealMatrix(res.L));
        items.emplace_back(QStringLiteral("U ="), toAlgRealMatrix(res.U));
        lastCallNote_ = QStringLiteral("Doolittle $LU$ 分解: $A = L U$");
        return Value::makeNamedMatrices(std::move(items));
    }
    if (fn == "qr") {
        need(1);
        if (!args[0].isMatrix()) throw std::runtime_error("qr 需要矩阵参数");
        Matrix<Fraction> A = toFractionMatrix(args[0].asMatrix(), "qr");
        auto res = algemate::math::qrDecompose(A);
        if (res.Q.cols() < A.cols())
            throw std::runtime_error("qr 要求 A 列满秩 (Gram-Schmidt 如列相关无法形成正交基)");
        std::vector<std::pair<QString, MatrixA>> items;
        items.emplace_back(QStringLiteral("Q ="), res.Q);
        items.emplace_back(QStringLiteral("R ="), res.R);
        lastCallNote_ = QStringLiteral("$QR$ 分解: $A = Q R$, $Q$ 列正交单位, $R$ 上三角");
        return Value::makeNamedMatrices(std::move(items));
    }
    if (fn == "svd") {
        need(1);
        if (!args[0].isMatrix()) throw std::runtime_error("svd 需要矩阵参数");
        Matrix<Fraction> A = toFractionMatrix(args[0].asMatrix(), "svd");
        auto res = algemate::math::svdDecompose(A);
        std::vector<std::pair<QString, MatrixA>> items;
        items.emplace_back(QStringLiteral("U ="), res.U);
        items.emplace_back(QStringLiteral("\\Sigma ="), res.Sigma);
        items.emplace_back(QStringLiteral("V ="), res.V);
        lastCallNote_ = QStringLiteral("$SVD$ 分解: $A = U \\Sigma V^{T}$, 奇异值降序");
        return Value::makeNamedMatrices(std::move(items));
    }

    // ================ Jordan 标准形 (复数域数值化; 参考 demo/JordanFormPage) ================
    //
    // 不再调用 algemate::math::jordanForm (含重根 / 复特征值时会崩溃), 改为:
    //   1) 精确: λI - A → 行列式因子 D_k(λ) → 不变因子 d_k = D_k / D_{k-1}
    //   2) 数值: 对每个非常量不变因子用 Durand–Kerner 在 ℂ 上求复根 → 初等因子 → Jordan 块
    //   3) 输出: J 以 LaTeX 矩阵的形式返回 (元素可为复数), 不再提供似实矩阵 Q.
    if (fn == "jordan") {
        need(1);
        if (!args[0].isMatrix()) throw std::runtime_error("jordan 需要矩阵参数");
        const MatrixA& Ma = args[0].asMatrix();
        if (!Ma.isSquare()) throw std::runtime_error("jordan 要求方阵");
        Matrix<Fraction> A = toFractionMatrix(Ma, "jordan");

        // ---- 1. 行列式因子 D_k (精确) ----
        auto lamM = algemate::math::lambdaMinus(A);
        std::vector<Polynomial<Fraction>> D = algemate::math::determinantalDivisors(lamM);
        const std::size_t n = A.rows();

        // ---- 2. 不变因子 d_k = D_k / D_{k-1} (含连续的 1) ----
        std::vector<Polynomial<Fraction>> allInvs;
        std::vector<Polynomial<Fraction>> nonOneInvs;
        {
            Polynomial<Fraction> Dprev(Fraction(1));
            for (std::size_t k = 0; k < D.size(); ++k) {
                auto qr = D[k].divmod(Dprev);
                allInvs.push_back(qr.quotient);
                Dprev = D[k];
                if (!(qr.quotient.degree() == 0
                      && qr.quotient.coeffs()[0] == Fraction(1)))
                    nonOneInvs.push_back(qr.quotient);
            }
        }

        // ---- 3. 初等因子 + Jordan 块 (复数域数值) ----
        struct JBlockInfo { double re; double im; int size; };
        std::vector<JBlockInfo> blockInfos;
        std::vector<QString>    elemFactorLtx;   // 按不变因子分组拼成一个乘积字符串
        for (const auto& inv : nonOneInvs) {
            auto roots = jNumericalRoots(inv);
            QStringList terms;
            for (const auto& r : roots) {
                terms << jFormatFactorLtx(r.re, r.im, r.mult);
                blockInfos.push_back({r.re, r.im, r.mult});
            }
            elemFactorLtx.push_back(terms.join(QStringLiteral(",\\quad ")));
        }

        // ---- 4. 拼出 Jordan 矩阵的 LaTeX (让完整于不变因子乘积为 1 的退化情况也能走) ----
        QString jLtx;
        if (blockInfos.empty()) {
            // 所有不变因子都是 1 → 只能是 n=0; 多举步逻辑会报 "要求方阵", 这里避免除零.
            jLtx = QStringLiteral("J = ()");
        } else {
            std::size_t total = 0;
            for (const auto& bi : blockInfos)
                total += static_cast<std::size_t>(bi.size);
            const bool ghost = (total == 1);
            QString cols = ghost ? QStringLiteral("cr")
                                 : QString(static_cast<int>(total), QLatin1Char('c'));
            QString body;
            std::size_t off = 0;
            for (const auto& bi : blockInfos) {
                for (int i = 0; i < bi.size; ++i) {
                    if (off + static_cast<std::size_t>(i) > 0)
                        body += QStringLiteral(" \\\\ ");
                    for (std::size_t j = 0; j < total; ++j) {
                        if (j) body += QStringLiteral(" & ");
                        if (j == off + static_cast<std::size_t>(i))
                            body += jComplexNumLtx(bi.re, bi.im);
                        else if (j == off + static_cast<std::size_t>(i) + 1
                                 && i + 1 < bi.size)
                            body += QStringLiteral("1");
                        else
                            body += QStringLiteral("0");
                    }
                    if (ghost) body += QStringLiteral(" & ");
                }
                off += static_cast<std::size_t>(bi.size);
            }
            jLtx = QStringLiteral(
                "J = \\left(\\begin{array}{%1}%2\\end{array}\\right)")
                .arg(cols, body);
        }

        // ---- 5. 说明垃圈 (行列式因子 / 不变因子 / 初等因子 / Jordan 块) ----
        QString note;
        note += QStringLiteral("Jordan 标准形 (复数域, 数值解); 特征值含复数时已以 $a+bi$ 形式呈现.");
        note += QStringLiteral("\n行列式因子:");
        for (std::size_t k = 0; k < D.size(); ++k) {
            note += QStringLiteral("\n  $D_{%1}(\\lambda) = %2$")
                        .arg(k + 1)
                        .arg(polyFractionToLatex_(D[k], QStringLiteral("\\lambda ")));
        }
        note += QStringLiteral("\n不变因子:");
        for (std::size_t i = 0; i < allInvs.size(); ++i) {
            note += QStringLiteral("\n  $d_{%1}(\\lambda) = %2$")
                        .arg(i + 1)
                        .arg(polyFractionToLatex_(allInvs[i], QStringLiteral("\\lambda ")));
        }
        note += QStringLiteral("\n初等因子 (复数域完全分解):");
        if (elemFactorLtx.empty()) {
            note += QStringLiteral("\n  无 (所有不变因子均为 1)");
        } else {
            for (const auto& s : elemFactorLtx)
                note += QStringLiteral("\n  $%1$").arg(s);
        }
        note += QStringLiteral("\nJordan 块:");
        if (blockInfos.empty()) {
            note += QStringLiteral("\n  无");
        } else {
            for (const auto& bi : blockInfos)
                note += QStringLiteral("\n  $J_{%1}(%2)$")
                            .arg(bi.size)
                            .arg(jComplexNumLtx(bi.re, bi.im));
        }
        lastCallNote_ = note;
        // makeText 走 renderNoteWithLatex, 需以 $...$ 包裹才会被当作 LaTeX 渲染.
        return Value::makeText(QStringLiteral("$") + jLtx + QStringLiteral("$"));
    }
    if (fn == "rcf" || fn == "frobenius" || fn == "rationalcanonical") {
        need(1);
        if (!args[0].isMatrix()) throw std::runtime_error(fn + " 需要矩阵参数");
        const MatrixA& Ma = args[0].asMatrix();
        if (!Ma.isSquare()) throw std::runtime_error(fn + " 要求方阵");
        Matrix<Fraction> A = toFractionMatrix(Ma, fn.c_str());
        auto res = algemate::math::rationalCanonicalForm(A);
        auto cp = algemate::math::charpoly(A);
        auto fact = algemate::math::factorOverQ(cp);
        QString note;
        // 特征多项式 Q 不可约分解
        note += QStringLiteral("$\\det(\\lambda I - A) = ");
        bool firstTerm = true;
        if (!(fact.leadingCoefficient == Fraction(1))) {
            note += QString::fromStdString(fact.leadingCoefficient.toLatex());
            firstTerm = false;
        }
        for (const auto& pe : fact.factors) {
            QString pTex = polyFractionToLatex_(pe.first, QStringLiteral("\\lambda "));
            QString term;
            if (pe.second == 1) term = QStringLiteral("(%1)").arg(pTex);
            else                term = QStringLiteral("(%1)^{%2}").arg(pTex).arg(pe.second);
            if (!firstTerm) note += QStringLiteral(" \\cdot ");
            note += term;
            firstTerm = false;
        }
        if (firstTerm) note += QStringLiteral("1");
        note += QStringLiteral("$");
        // 不变因子
        note += QStringLiteral("\n不变因子:");
        {
            const std::size_t n = A.rows();
            const auto& invs = res.invariantFactors;
            const std::size_t pad = (invs.size() < n) ? (n - invs.size()) : 0;
            for (std::size_t i = 0; i < pad; ++i) {
                note += QStringLiteral("\n  $d_{%1}(\\lambda) = 1$").arg(i + 1);
            }
            for (std::size_t i = 0; i < invs.size(); ++i) {
                note += QStringLiteral("\n  $d_{%1}(\\lambda) = %2$")
                            .arg(pad + i + 1)
                            .arg(polyFractionToLatex_(invs[i], QStringLiteral("\\lambda ")));
            }
        }
        note += QStringLiteral("\n最小多项式:\n  $m(\\lambda) = %1$")
                    .arg(polyFractionToLatex_(res.minimalPolynomial, QStringLiteral("\\lambda ")));
        lastCallNote_ = note;
        return Value(toAlgRealMatrix(res.F));
    }

    throw std::runtime_error("未定义的函数: " + fn);
}

// ========== AST -> 多项式 辅助 (仅为 gcd/factor/... 等多项式函数服务) ==========

void Evaluator::collectFreeVars_(const NodePtr& n, std::set<std::string>& out) {
    using K = Node::Kind;
    if (!n) return;
    if (n->kind == K::Ident) {
        if (n->name != "i" && env_.find(n->name) == env_.end())
            out.insert(n->name);
    }
    if (n->child) collectFreeVars_(n->child, out);
    if (n->lhs)   collectFreeVars_(n->lhs, out);
    if (n->rhs)   collectFreeVars_(n->rhs, out);
    for (auto& a : n->args) collectFreeVars_(a, out);
    for (auto& r : n->rows) for (auto& c : r) collectFreeVars_(c, out);
}

bool Evaluator::dependsOnVar_(const NodePtr& n, const std::string& var) {
    using K = Node::Kind;
    if (!n) return false;
    if (n->kind == K::Ident && n->name == var) return true;
    if (n->child && dependsOnVar_(n->child, var)) return true;
    if (n->lhs   && dependsOnVar_(n->lhs,   var)) return true;
    if (n->rhs   && dependsOnVar_(n->rhs,   var)) return true;
    for (auto& a : n->args) if (dependsOnVar_(a, var)) return true;
    for (auto& r : n->rows) for (auto& c : r) if (dependsOnVar_(c, var)) return true;
    return false;
}

std::vector<Scalar> Evaluator::astToPolyCoeffs_(const NodePtr& n, const std::string& var) {
    using K = Node::Kind;

    auto trim = [](std::vector<Scalar>& r){
        while (r.size() > 1 && r.back().isZero()) r.pop_back();
    };
    auto polyAdd = [&](const std::vector<Scalar>& a, const std::vector<Scalar>& b){
        std::vector<Scalar> r(std::max(a.size(), b.size()), Scalar(Fraction(0)));
        for (std::size_t i = 0; i < a.size(); ++i) r[i] = r[i] + a[i];
        for (std::size_t i = 0; i < b.size(); ++i) r[i] = r[i] + b[i];
        trim(r); return r;
    };
    auto polyMul = [&](const std::vector<Scalar>& a, const std::vector<Scalar>& b){
        if (a.empty() || b.empty()) return std::vector<Scalar>{ Scalar(Fraction(0)) };
        std::vector<Scalar> r(a.size() + b.size() - 1, Scalar(Fraction(0)));
        for (std::size_t i = 0; i < a.size(); ++i)
            for (std::size_t j = 0; j < b.size(); ++j)
                r[i+j] = r[i+j] + a[i] * b[j];
        trim(r); return r;
    };

    // 子树不含 var -> 转常数项
    if (!dependsOnVar_(n, var)) {
        Value v = eval_(n);
        if (!v.isScalar())
            throw std::runtime_error("多项式系数必须是标量");
        return { v.asScalar() };
    }

    switch (n->kind) {
    case K::Ident:
        // 必定是 var (其他会在 !dependsOnVar 分支里 fallback 或抛错)
        return { Scalar(Fraction(0)), Scalar(Fraction(1)) };
    case K::Unary: {
        if (n->name == "-") {
            auto a = astToPolyCoeffs_(n->child, var);
            for (auto& x : a) x = -x;
            return a;
        }
        throw std::runtime_error("多项式中不支持一元运算: " + n->name);
    }
    case K::Binary: {
        if (n->name == "+" || n->name == "-") {
            auto a = astToPolyCoeffs_(n->lhs, var);
            auto b = astToPolyCoeffs_(n->rhs, var);
            if (n->name == "-") for (auto& x : b) x = -x;
            return polyAdd(a, b);
        }
        if (n->name == "*") {
            return polyMul(astToPolyCoeffs_(n->lhs, var),
                           astToPolyCoeffs_(n->rhs, var));
        }
        if (n->name == "^") {
            if (dependsOnVar_(n->rhs, var))
                throw std::runtime_error("多项式的幂次不能含 " + var);
            Value e = eval_(n->rhs);
            if (!e.isScalar() || !e.asScalar().isRational())
                throw std::runtime_error("多项式幂次必须是非负整数");
            Fraction q = e.asScalar().asRational();
            if (q.denominator() != algemate::math::BigInt(1))
                throw std::runtime_error("多项式幂次必须是整数");
            long long p = q.numerator().toLongLong();
            if (p < 0) throw std::runtime_error("多项式幂次必须 >= 0");
            auto base = astToPolyCoeffs_(n->lhs, var);
            std::vector<Scalar> r = { Scalar(Fraction(1)) };
            // 快速幂
            while (p > 0) {
                if (p & 1) r = polyMul(r, base);
                p >>= 1;
                if (p > 0) base = polyMul(base, base);
            }
            return r;
        }
        if (n->name == "/") {
            // 只允许除以不含 var 的标量
            if (dependsOnVar_(n->rhs, var))
                throw std::runtime_error("多项式不支持除以含 " + var + " 的表达式");
            Value e = eval_(n->rhs);
            if (!e.isScalar()) throw std::runtime_error("多项式除以非标量");
            Scalar d = e.asScalar();
            if (d.isZero()) throw std::runtime_error("除零");
            auto a = astToPolyCoeffs_(n->lhs, var);
            for (auto& x : a) x = x / d;
            return a;
        }
        throw std::runtime_error("多项式不支持运算: " + n->name);
    }
    default:
        throw std::runtime_error("多项式表达式中有不支持的节点类型");
    }
}

}
