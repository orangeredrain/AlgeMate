#include "Value.h"

#include <QStringList>
#include <QPainter>
#include <QPixmap>
#include <QFontMetrics>
#include <QHash>
#include <QTextDocument>
#include <QUrl>
#include <QtMath>
#include <QGuiApplication>
#include <QScreen>
#include <jkqtmathtext/jkqtmathtext.h>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>
#include <cstdlib>
#include <numeric>
#include <algorithm>

namespace AlgeMate::Calculator::Interactive {

using algemate::math::AlgReal;
using algemate::math::Fraction;
using algemate::math::Complex;

// ======================== 构造 ========================

Value::Value() : kind_(Kind::Scalar), s_(Scalar()) {}
Value::Value(const Scalar& s)  : kind_(Kind::Scalar), s_(s) {}
Value::Value(const ComplexC& c) {
    if (c.isReal()) { kind_ = Kind::Scalar; s_ = c.real(); }
    else            { kind_ = Kind::ComplexScalar; c_ = c; }
}
Value::Value(const MatrixA& m) : kind_(Kind::Matrix), m_(m) {}
Value::Value(const std::vector<Scalar>& coeffs, const QString& var)
    : kind_(Kind::Polynomial), polyCoeffs_(coeffs), polyVar_(var) {
    // 去掉高次末尾零
    while (!polyCoeffs_.empty() && polyCoeffs_.back().isZero())
        polyCoeffs_.pop_back();
}

Value Value::makeRootList(std::vector<std::pair<Scalar, Scalar>> roots, const QString& var) {
    Value v;
    v.kind_ = Kind::RootList;
    v.roots_ = std::move(roots);
    v.rootVar_ = var;
    return v;
}

Value Value::makeFactored(std::vector<Scalar> origCoeffs,
                          Scalar leading,
                          std::vector<std::pair<std::vector<Scalar>, int>> factors,
                          QString var) {
    Value v;
    v.kind_ = Kind::Factored;
    v.polyCoeffs_ = std::move(origCoeffs);
    v.s_ = std::move(leading);
    v.factors_ = std::move(factors);
    v.polyVar_ = std::move(var);
    // 去掉高次末尾零 (orig)
    while (!v.polyCoeffs_.empty() && v.polyCoeffs_.back().isZero())
        v.polyCoeffs_.pop_back();
    return v;
}

Value Value::makeVectorList(std::vector<MatrixA> vectors, const QString& var) {
    Value v;
    v.kind_ = Kind::VectorList;
    v.vectors_ = std::move(vectors);
    v.vecVar_ = var;
    return v;
}

Value Value::makeNamedMatrices(std::vector<std::pair<QString, MatrixA>> items) {
    Value v;
    v.kind_ = Kind::NamedMatrices;
    v.namedMats_ = std::move(items);
    return v;
}

Value Value::makeText(QString content) {
    Value v;
    v.kind_ = Kind::Text;
    v.textContent_ = std::move(content);
    return v;
}

ComplexC Value::toComplex() const {
    if (kind_ == Kind::Scalar)        return ComplexC(s_);
    if (kind_ == Kind::ComplexScalar) return c_;
    throw std::runtime_error("内部错误: 非标量无法提升为复数");
}

// 辅助: 将复标量降级尝试 (如果虚部为 0, 返实标量 Value; 否则原 Complex)
static Value demoteComplex(const ComplexC& z) {
    if (z.isReal()) return Value(z.real());
    return Value(z);
}

// ======================== 工具 ========================

static bool sameShape(const MatrixA& a, const MatrixA& b) {
    return a.rows() == b.rows() && a.cols() == b.cols();
}

// ======================== 加减 ========================

Value Value::add(const Value& r) const {
    if (isPolynomial() || r.isPolynomial())
        throw std::runtime_error("多项式目前不参与算术 (如需, 先提取系数)");
    if (isAnyScalar() && r.isAnyScalar()) {
        if (isComplexScalar() || r.isComplexScalar())
            return demoteComplex(toComplex() + r.toComplex());
        return Value(s_ + r.s_);
    }
    if (isMatrix() && r.isMatrix()) {
        if (!sameShape(m_, r.m_))
            throw std::runtime_error("矩阵相加要求相同形状");
        return Value(m_ + r.m_);
    }
    throw std::runtime_error("标量和矩阵不能相加");
}

Value Value::sub(const Value& r) const {
    if (isPolynomial() || r.isPolynomial())
        throw std::runtime_error("多项式目前不参与算术 (如需, 先提取系数)");
    if (isAnyScalar() && r.isAnyScalar()) {
        if (isComplexScalar() || r.isComplexScalar())
            return demoteComplex(toComplex() - r.toComplex());
        return Value(s_ - r.s_);
    }
    if (isMatrix() && r.isMatrix()) {
        if (!sameShape(m_, r.m_))
            throw std::runtime_error("矩阵相减要求相同形状");
        return Value(m_ - r.m_);
    }
    throw std::runtime_error("标量和矩阵不能相减");
}

// ======================== 乘 ========================

static MatrixA scalarTimesMatrix(const Scalar& k, const MatrixA& M) {
    MatrixA R(M.rows(), M.cols());
    for (std::size_t i = 0; i < M.rows(); ++i)
        for (std::size_t j = 0; j < M.cols(); ++j)
            R(i, j) = k * M(i, j);
    return R;
}

Value Value::mul(const Value& r) const {
    if (isPolynomial() || r.isPolynomial())
        throw std::runtime_error("多项式目前不参与算术 (如需, 先提取系数)");
    if (isAnyScalar() && r.isAnyScalar()) {
        if (isComplexScalar() || r.isComplexScalar())
            return demoteComplex(toComplex() * r.toComplex());
        return Value(s_ * r.s_);
    }
    if (isScalar() && r.isMatrix()) return Value(scalarTimesMatrix(s_, r.m_));
    if (isMatrix() && r.isScalar()) return Value(scalarTimesMatrix(r.s_, m_));
    if (isComplexScalar() || (r.isComplexScalar() && r.isMatrix()))
        throw std::runtime_error("复标量与矩阵的乘法暂不支持 (矩阵元素限实数)");
    // 矩阵 * 矩阵
    if (m_.cols() != r.m_.rows())
        throw std::runtime_error("矩阵乘法要求左矩阵列数等于右矩阵行数");
    // 稳定性优先: 任一矩阵含代数数 (非有理) 元素 → 转 double 计算,
    //   避免 AlgReal 累乘 squarefreePart/resultant 指数爆炸 (如 m*m*m*m).
    //   矩阵分解函数 (qr/svd/lu/jordan/...) 走内部层 AlgReal 算法, 不经此入口.
    auto hasAlgebraicElem = [](const MatrixA& M) {
        for (std::size_t i = 0; i < M.rows(); ++i)
            for (std::size_t j = 0; j < M.cols(); ++j)
                if (!M(i, j).isRational()) return true;
        return false;
    };
    if (hasAlgebraicElem(m_) || hasAlgebraicElem(r.m_)) {
        const std::size_t R = m_.rows(), K = m_.cols(), C = r.m_.cols();
        algemate::math::Matrix<double> A(R, K), B(K, C);
        for (std::size_t i = 0; i < R; ++i)
            for (std::size_t j = 0; j < K; ++j)
                A(i, j) = m_(i, j).toDouble();
        for (std::size_t i = 0; i < K; ++i)
            for (std::size_t j = 0; j < C; ++j)
                B(i, j) = r.m_(i, j).toDouble();
        algemate::math::Matrix<double> P = A * B;
        MatrixA out(R, C);
        for (std::size_t i = 0; i < R; ++i)
            for (std::size_t j = 0; j < C; ++j)
                out(i, j) = AlgReal::fromDouble(P(i, j));
        return Value(std::move(out));
    }
    return Value(m_ * r.m_);
}

// ======================== 除 ========================

Value Value::div(const Value& r) const {
    if (isPolynomial() || r.isPolynomial())
        throw std::runtime_error("多项式目前不参与算术");
    if (r.isAnyScalar()) {
        if (isAnyScalar()) {
            if (isComplexScalar() || r.isComplexScalar()) {
                ComplexC zr = r.toComplex();
                if (zr.isZero()) throw std::runtime_error("不能除以 0");
                return demoteComplex(toComplex() / zr);
            }
            if (r.s_.isZero()) throw std::runtime_error("不能除以 0");
            return Value(s_ / r.s_);
        }
        // 矩阵 / 实标量
        if (r.isComplexScalar())
            throw std::runtime_error("矩阵除以复标量暂不支持");
        if (r.s_.isZero()) throw std::runtime_error("不能除以 0");
        MatrixA R(m_.rows(), m_.cols());
        for (std::size_t i = 0; i < m_.rows(); ++i)
            for (std::size_t j = 0; j < m_.cols(); ++j)
                R(i, j) = m_(i, j) / r.s_;
        return Value(R);
    }
    // 矩阵除法: 暂不支持, 建议用 inv()
    throw std::runtime_error("不支持除以矩阵, 请使用 inv() 求逆后相乘");
}

// ======================== 幂 ========================

Value Value::pow(const Value& r) const {
    if (isPolynomial() || r.isPolynomial())
        throw std::runtime_error("多项式目前不参与幂运算");
    if (!r.isScalar())
        throw std::runtime_error("幂次必须是实标量整数");
    if (!r.s_.isRational())
        throw std::runtime_error("幂次暂仅支持整数");
    Fraction q = r.s_.asRational();
    if (q.denominator() != algemate::math::BigInt(1))
        throw std::runtime_error("幂次暂仅支持整数");
    long long n = q.numerator().toLongLong();

    if (isScalar()) {
        if (n == 0) return Value(Scalar((long long)1));
        bool neg = n < 0;
        if (neg) {
            if (s_.isZero()) throw std::runtime_error("0 不能取负幂");
            n = -n;
        }
        Scalar result((long long)1), base = s_;
        for (long long i = 0; i < n; ++i) result = result * base;
        if (neg) result = Scalar((long long)1) / result;
        return Value(result);
    }
    if (isComplexScalar()) {
        if (n == 0) return Value(Scalar((long long)1));
        bool neg = n < 0;
        if (neg) {
            if (c_.isZero()) throw std::runtime_error("0 不能取负幂");
            n = -n;
        }
        ComplexC result((long long)1), base = c_;
        for (long long i = 0; i < n; ++i) result = result * base;
        if (neg) result = ComplexC((long long)1) / result;
        return demoteComplex(result);
    }
    // 矩阵幂
    if (!m_.isSquare())
        throw std::runtime_error("只有方阵能取幂");
    if (n < 0)
        throw std::runtime_error("矩阵负整数幂暂未实现, 请先 inv() 再幂");
    // 边界保护: 防止高次幂导致 BigInt 爆炸或 bad_alloc
    if (n > 1000000)
        throw std::runtime_error("矩阵幂次过大 (上限 1000000)");
    const std::size_t R = m_.rows();

    // 数值降级策略 (稳定性优先):
    //   (1) 矩阵含代数数 (非有理) 元素 → 无条件走 double,
    //       AlgReal 累乘 squarefreePart/realRootsOf 会指数爆炸, 例如 [1,sqrt(2);2,3]^4 必崩.
    //   (2) 全有理 + 规模/次数偏大 → 走 double, 避免 BigInt 分数爆炸.
    //   其他 (小规模全有理) 走精确.
    bool hasAlgebraic = false;
    for (std::size_t i = 0; i < R && !hasAlgebraic; ++i)
        for (std::size_t j = 0; j < m_.cols() && !hasAlgebraic; ++j)
            if (!m_(i, j).isRational()) hasAlgebraic = true;

    auto isLargeEnough = [&]() {
        int bits = 0;
        for (long long v = n; v > 0; v >>= 1) ++bits;
        return (n >= 50) && (static_cast<long long>(R) * bits >= 30 || R >= 3);
    };
    const bool useNumeric = hasAlgebraic || isLargeEnough();
    if (useNumeric) {
        algemate::math::Matrix<double> D(R, R);
        for (std::size_t i = 0; i < R; ++i)
            for (std::size_t j = 0; j < R; ++j)
                D(i, j) = m_(i, j).toDouble();
        bool neg = n < 0;
        algemate::math::Matrix<double> Dp = D.power(static_cast<unsigned int>(std::llabs(n)));
        if (neg) throw std::runtime_error("矩阵负幂数值路径暂未实现");
        MatrixA out(R, R);
        for (std::size_t i = 0; i < R; ++i)
            for (std::size_t j = 0; j < R; ++j)
                out(i, j) = AlgReal::fromDouble(Dp(i, j));
        return Value(std::move(out));
    }

    // 精确路径: 大规模×高幂次的预防性抦截
    if (R >= 6 && n > 2000)
        throw std::runtime_error("阶数偏大的矩阵高次幂易导致内存爆炸, 请降低次数或矩阵阶数");
    try {
        return Value(m_.power(static_cast<unsigned int>(n)));
    } catch (const std::bad_alloc&) {
        throw std::runtime_error("矩阵幂运算内存不足, 请降低次数或矩阵规模");
    } catch (const std::length_error& e) {
        throw std::runtime_error(std::string("矩阵幂运算失败: ") + e.what());
    }
}

// ======================== 一元 ========================

Value Value::neg() const {
    if (isPolynomial()) {
        std::vector<Scalar> cs = polyCoeffs_;
        for (auto& c : cs) c = -c;
        return Value(cs, polyVar_);
    }
    if (isScalar())        return Value(-s_);
    if (isComplexScalar()) return demoteComplex(-c_);
    return Value(-m_);
}

Value Value::transpose() const {
    if (isPolynomial())
        throw std::runtime_error("多项式不支持转置");
    if (isScalar() || isComplexScalar()) return *this;
    return Value(m_.transpose());
}

// ======================== 格式化 ========================

QString Value::typeLabel() const {
    if (isScalar()) return QStringLiteral("标量");
    if (isComplexScalar()) return QStringLiteral("复标量");
    if (isPolynomial()) {
        int deg = (int)polyCoeffs_.size() - 1;
        if (deg < 0) deg = 0;
        return QStringLiteral("%1 的多项式 (次数 %2)").arg(polyVar_).arg(deg);
    }
    if (isRootList()) return QStringLiteral("%1 个根").arg(roots_.size());
    if (isFactored()) return QStringLiteral("多项式分解");
    if (isVectorList()) return vectors_.empty()
                               ? QStringLiteral("空向量列表")
                               : QStringLiteral("%1 个向量").arg(vectors_.size());
    if (isNamedMatrices()) return QStringLiteral("%1 个命名矩阵").arg(namedMats_.size());
    if (isText()) return QStringLiteral("文本结果");
    return QStringLiteral("%1 × %2 矩阵").arg(m_.rows()).arg(m_.cols());
}

// 纯文本 / HTML 共享: 单元格字符串表示 (按 format 决定精确/小数)
// compact=true: 用于矩阵/多项式内部元素.
//   简单符号形式 (sqrt(2), cbrt(5), root(7,4) 等) 保留; 只有 alg(...) 复杂形式回落小数,
//   避免代数运算 (sum/prod via 结式) 在大矩阵运算中爆炸 (bug 1).
static QString scalarToStr(const Scalar& s, const DisplayFormat& fmt, bool compact = false) {
    if (fmt.kind == DisplayFormat::Decimal) {
        // 有理整数直接显示整数, 避免 "1.0000" 这类尾随零
        if (s.isRational()) {
            const auto& q = s.asRational();
            if (q.denominator() == algemate::math::BigInt(1)) {
                return QString::fromStdString(q.numerator().toString());
            }
        }
        double d = s.toDouble();
        int n = std::max(0, std::min(fmt.decimals, 15));
        if (d == 0.0) d = 0.0;  // 消除 -0
        QString str = QString::number(d, 'f', n);
        // 小数: 去掉尾随 0 (和孤立的小数点)
        if (str.contains(QLatin1Char('.'))) {
            while (str.endsWith(QLatin1Char('0'))) str.chop(1);
            if (str.endsWith(QLatin1Char('.'))) str.chop(1);
        }
        if (str.isEmpty() || str == QStringLiteral("-")) str = QStringLiteral("0");
        return str;
    }
    // Exact 模式
    QString str = QString::fromStdString(s.toString());
    // AlgReal 非纯根式时会返回 "alg(p(x), [a, b])" 回落为小数近似 (无 ≈ 前缀).
    if (str.contains(QStringLiteral("alg("))) {
        double d = s.toDouble();
        if (d == 0.0) d = 0.0;
        return QString::number(d, 'f', 6);
    }
    // 纯符号形式 (sqrt(2), cbrt(5), root(7,4) 等) 在 compact 模式下也直接保留.
    // 有理数但分母过大也回落为小数, 避免显示庞大分数丑陋.
    if (s.isRational()) {
        const auto& den = s.asRational().denominator();
        if (den > algemate::math::BigInt(10000)) {
            double d = s.toDouble();
            if (d == 0.0) d = 0.0;
            return QString::number(d, 'f', 6);
        }
    }
    return str;
}

static std::vector<std::vector<QString>> matrixCellStrings(const MatrixA& M, const DisplayFormat& fmt) {
    std::vector<std::vector<QString>> g(M.rows(), std::vector<QString>(M.cols()));
    for (std::size_t i = 0; i < M.rows(); ++i)
        for (std::size_t j = 0; j < M.cols(); ++j)
            g[i][j] = scalarToStr(M(i, j), fmt, /*compact=*/true);
    return g;
}

// ---------------- 多项式 辅助: 系数的符号拆分 + 是否需要括号 ----------------

// 返回 (sign, absStr). sign = "+" 或 "-"
static std::pair<QString, QString> splitSign(const QString& s) {
    if (s.startsWith('-'))
        return { QStringLiteral("-"), s.mid(1).trimmed() };
    return { QStringLiteral("+"), s };
}

// 符号内部还含 + 或 - 时 (如 "3 - sqrt(2)"), 需要加括号以避免歧义
static bool needsParens(const QString& absStr) {
    // 简单数字 / 分数 / 单一 token 不需括号
    for (int i = 1; i < absStr.size(); ++i) {
        QChar c = absStr[i];
        if (c == '+' || c == '-') return true;
    }
    return false;
}

// 上标数字 (Unicode 上标): 0123456789
static QString toSuperscript(int n) {
    static const QChar sup[10] = {
        QChar(0x2070), QChar(0x00B9), QChar(0x00B2), QChar(0x00B3), QChar(0x2074),
        QChar(0x2075), QChar(0x2076), QChar(0x2077), QChar(0x2078), QChar(0x2079),
    };
    QString num = QString::number(n);
    QString out;
    for (QChar c : num) out += sup[c.digitValue()];
    return out;
}

static QString polyToPlain(const std::vector<Scalar>& coeffs, const QString& var,
                           const DisplayFormat& fmt) {
    if (coeffs.empty()) return QStringLiteral("0");
    QString out;
    bool first = true;
    for (int i = (int)coeffs.size() - 1; i >= 0; --i) {
        const Scalar& c = coeffs[i];
        if (c.isZero()) continue;
        // 多项式系数属于复合结构, 走 compact (代数数 → 6 位小数).
        QString s = scalarToStr(c, fmt, /*compact=*/true);
        auto [sign, abs] = splitSign(s);
        bool parens = needsParens(abs);
        QString absTok = parens ? (QStringLiteral("(") + abs + QStringLiteral(")")) : abs;

        QString term;
        if (i == 0) {
            term = absTok;
        } else if (i == 1) {
            term = (abs == QStringLiteral("1") && !parens) ? var : (absTok + QStringLiteral("\u00B7") + var);
        } else {
            QString powPart = var + toSuperscript(i);
            term = (abs == QStringLiteral("1") && !parens) ? powPart : (absTok + QStringLiteral("\u00B7") + powPart);
        }

        if (first) {
            out += (sign == QStringLiteral("-") ? QStringLiteral("-") : QString()) + term;
            first = false;
        } else {
            out += QStringLiteral(" ") + sign + QStringLiteral(" ") + term;
        }
    }
    if (first) return QStringLiteral("0");
    return out;
}

// 复数格式化: "a+bi" / "a-bi" / "bi" / "a"; 按 format 小数/精确统一
static QString complexToStr(const ComplexC& z, const DisplayFormat& fmt) {
    const Scalar& re = z.real();
    const Scalar& im = z.imag();
    const bool imZero = im.isZero();
    const bool reZero = re.isZero();
    if (reZero && imZero) return QStringLiteral("0");
    if (imZero) return scalarToStr(re, fmt);
    // 虚部存在
    QString imAbs = scalarToStr(im.sign() < 0 ? -im : im, fmt);
    QString imPart;
    // 去掉 "1i" → "i"; "-1i" → "-i"
    if (imAbs == QStringLiteral("1")) imPart = QStringLiteral("i");
    else                              imPart = imAbs + QStringLiteral("i");
    if (reZero) return (im.sign() < 0) ? QStringLiteral("-") + imPart : imPart;
    QString rePart = scalarToStr(re, fmt);
    QString sign = (im.sign() < 0) ? QStringLiteral(" - ") : QStringLiteral(" + ");
    return rePart + sign + imPart;
}

// 纯文本
QString Value::toPlain(const DisplayFormat& fmt) const {
    if (isScalar()) return scalarToStr(s_, fmt);
    if (isComplexScalar()) return complexToStr(c_, fmt);
    if (isPolynomial()) return polyToPlain(polyCoeffs_, polyVar_, fmt);
    if (isRootList()) {
        if (roots_.empty()) return QStringLiteral("无");
        QStringList lines;
        for (std::size_t i = 0; i < roots_.size(); ++i) {
            ComplexC z(roots_[i].first, roots_[i].second);
            lines << QStringLiteral("%1_%2 = %3")
                .arg(rootVar_).arg(i + 1).arg(complexToStr(z, fmt));
        }
        return lines.join('\n');
    }
    if (isFactored()) {
        QString lhs = polyToPlain(polyCoeffs_, polyVar_, fmt);
        QString out = lhs + QStringLiteral(" = ");
        QString lead = scalarToStr(s_, fmt);
        const bool leadOne    = (lead == QStringLiteral("1"));
        const bool leadNegOne = (lead == QStringLiteral("-1"));
        auto isPureVar = [](const std::vector<Scalar>& c) {
            return c.size() == 2 && c[0].isZero()
                && (c[1] - Scalar(Fraction(1))).isZero();
        };
        if (factors_.empty()) {
            out += lead;
        } else {
            if (leadNegOne) out += QStringLiteral("-");
            else if (!leadOne) out += lead;
            for (const auto& [coeffs, mult] : factors_) {
                QString inner = polyToPlain(coeffs, polyVar_, fmt);
                if (isPureVar(coeffs)) out += inner;
                else out += QStringLiteral("(") + inner + QStringLiteral(")");
                if (mult > 1) out += QStringLiteral("^%1").arg(mult);
            }
        }
        return out;
    }

    // ---- 向量列表 (零空间基等) ----
    if (isVectorList()) {
        if (vectors_.empty()) return QStringLiteral("无");
        QStringList lines;
        for (std::size_t i = 0; i < vectors_.size(); ++i) {
            const auto& vec = vectors_[i];
            QStringList comps;
            for (std::size_t r = 0; r < vec.rows(); ++r)
                comps << scalarToStr(vec(r, 0), fmt);
            lines << QStringLiteral("%1_%2 = (%3)^T")
                .arg(vecVar_).arg(i + 1).arg(comps.join(QStringLiteral(", ")));
        }
        return lines.join('\n');
    }

    // ---- 命名矩阵列表 (LU/QR/合同对角化等) ----
    if (isNamedMatrices()) {
        if (namedMats_.empty()) return QStringLiteral("无");
        QStringList lines;
        for (const auto& it : namedMats_) {
            lines << it.first;
            const auto& M = it.second;
            const std::size_t R = M.rows(), C = M.cols();
            if (R == 0 || C == 0) continue;
            for (std::size_t i = 0; i < R; ++i) {
                QStringList row;
                for (std::size_t j = 0; j < C; ++j)
                    row << scalarToStr(M(i, j), fmt);
                lines << QStringLiteral("  [ %1 ]").arg(row.join(QStringLiteral(", ")));
            }
        }
        return lines.join('\n');
    }

    // ---- 文本结果 ----
    if (isText()) {
        // 去掉 $...$ 的边界符 (仅在纯文本场景, HTML 则渲染为 LaTeX)
        QString out = textContent_;
        out.replace(QLatin1Char('$'), QString());
        return out;
    }

    auto g = matrixCellStrings(m_, fmt);
    const std::size_t R = m_.rows(), C = m_.cols();
    if (R == 0 || C == 0) return QStringLiteral("[]");

    std::vector<int> w(C, 0);
    for (std::size_t j = 0; j < C; ++j)
        for (std::size_t i = 0; i < R; ++i)
            w[j] = std::max<int>(w[j], g[i][j].size());

    QStringList lines;
    for (std::size_t i = 0; i < R; ++i) {
        QString row;
        for (std::size_t j = 0; j < C; ++j) {
            if (j) row += QStringLiteral("  ");
            row += QString(w[j] - g[i][j].size(), QChar(' ')) + g[i][j];
        }
        QChar l, r;
        if (R == 1) { l = QChar('['); r = QChar(']'); }
        else if (i == 0)       { l = QChar(0x23A1); r = QChar(0x23A4); }
        else if (i == R - 1)   { l = QChar(0x23A3); r = QChar(0x23A6); }
        else                   { l = QChar(0x23A2); r = QChar(0x23A5); }
        lines << QString("%1 %2 %3").arg(l).arg(row).arg(r);
    }
    return lines.join('\n');
}

// ---------------- LaTeX 生成 (用于 JKQTMathText) ----------------

// 实标量 → LaTeX. 分数转 \frac; 代数数优先调 toLatex() 以输出 \sqrt{} 等标准根号.
// 将非负整数 n 分解为 a² × sqfree, a 最大
// n 要求 >= 1 且 ≤ 10^14 较稳定
static std::pair<long long, long long> extractSquareFactorLL_(long long n) {
    if (n <= 0) return {1, std::max<long long>(n, 1)};
    long long a = 1, sf = n;
    for (long long p = 2; p * p <= sf; ++p) {
        long long p2 = p * p;
        while (sf % p2 == 0) {
            sf /= p2;
            a *= p;
        }
    }
    return {a, sf};
}

// 尝试把非负有理 q 的平方根 sqrt(q) 表达为 (num/den) × sqrt(r), r 无平方因子.
// 成功返回 true; 如果 q 的分子或分母 BigInt 过大无法分解，返回 false.
static bool tryExtractSqrtRational_(const Fraction& q,
                                    long long& num, long long& den, long long& r) {
    if (q.sign() < 0) return false;
    if (q.isZero()) { num = 0; den = 1; r = 1; return true; }
    const algemate::math::BigInt& P = q.numerator();
    const algemate::math::BigInt& D = q.denominator();
    // 10^14 级别的上限，防止试除过慢
    const algemate::math::BigInt limit(static_cast<long long>(100000000000000LL));
    if (P > limit || D > limit) return false;
    long long Pi = P.toLongLong();
    long long Di = D.toLongLong();
    if (Pi <= 0 || Di <= 0) return false;
    auto [aP, rP] = extractSquareFactorLL_(Pi);
    auto [aD, rD] = extractSquareFactorLL_(Di);
    // sqrt(q) = (aP/aD) * sqrt(rP/rD) = (aP/aD) * sqrt(rP*rD)/rD = (aP)/(aD*rD) * sqrt(rP*rD)
    // rP*rD 可能有残余平方因子，再抽一次
    long long RR = rP * rD;
    auto [aR, Rsf] = extractSquareFactorLL_(RR);
    num = aP * aR;
    den = aD * rD;
    if (den <= 0) return false;
    long long g = std::gcd(std::llabs(num), std::llabs(den));
    if (g > 1) { num /= g; den /= g; }
    r = Rsf;
    return true;
}

// 尝试将代数数 s 展开为  a + b·√r  闭式 (minPoly 为 2 次时适用).
//   s 的 minPoly 是首一化的 x^2 + p x + q (有理系数), 根 = (-p ± √Δ)/2, Δ = p^2 - 4q.
// 成功返回 true, 填充 latex (LaTeX 字符串).  不能展开返回 false.
static bool tryAlgToBinomialLatex_(const Scalar& s, QString& latex) {
    const auto& p = s.minPoly();
    if (p.degree() != 2) return false;
    const auto& cs = p.coeffs();                // low first: {c0, c1, c2}
    if (cs.size() != 3) return false;
    Fraction c2 = cs[2];
    if (c2.sign() == 0) return false;
    // 首一化
    Fraction bb = cs[1] / c2;                   // 对应 p
    Fraction cc = cs[0] / c2;                   // 对应 q
    Fraction disc = bb * bb - Fraction(4) * cc; // Δ
    if (disc.sign() < 0) return false;          // 虚根, 不应出现在 AlgReal
    // disc = 4 × (Δ/4),  根 = -bb/2 ± √(disc)/2
    // 用 tryExtractSqrtRational_ 提取 √disc = (sn/sd) √r
    long long sn = 0, sd = 1, r = 1;
    if (!tryExtractSqrtRational_(disc, sn, sd, r)) return false;
    // s = -bb/2 ± (sn/sd)/2 · √r = (-bb/2) ± (sn/(2 sd)) √r
    Fraction a = Fraction(-1) * bb / Fraction(2);
    // 正负号: 数值比较 sign
    double da = 0.0;
    try { da = a.toDouble(); } catch (...) { da = 0.0; }
    double ds = s.toDouble();
    bool plus = (ds >= da);                     // √r 部分系数正/负
    // b 的有理数绝对值 = sn / (2 sd)
    long long bNum = sn;
    long long bDen = 2 * sd;
    long long g = std::gcd(std::llabs(bNum), std::llabs(bDen));
    if (g > 1) { bNum /= g; bDen /= g; }
    // 组装 LaTeX
    // a 部分
    QString aTex;
    if (a.sign() == 0) {
        aTex.clear();
    } else if (a.denominator() == algemate::math::BigInt(1)) {
        aTex = QString::fromStdString(a.numerator().toString());
    } else {
        QString num = QString::fromStdString(a.numerator().toString());
        QString den = QString::fromStdString(a.denominator().toString());
        bool neg = num.startsWith('-');
        if (neg) num = num.mid(1);
        aTex = QStringLiteral("\\frac{%1}{%2}").arg(num, den);
        if (neg) aTex = QStringLiteral("-") + aTex;
    }
    // √r 部分
    if (r == 0 || bNum == 0) {
        // b = 0, s 纯有理 (理论上不应会, 但兼容)
        latex = aTex.isEmpty() ? QStringLiteral("0") : aTex;
        return true;
    }
    QString rtTex = (r == 1) ? QString() : QStringLiteral("\\sqrt{%1}").arg(r);
    QString bAbs;
    if (bNum == 1 && bDen == 1) {
        bAbs = rtTex.isEmpty() ? QStringLiteral("1") : rtTex;
    } else if (bDen == 1) {
        bAbs = rtTex.isEmpty()
             ? QString::number(bNum)
             : QStringLiteral("%1 %2").arg(bNum).arg(rtTex);
    } else {
        QString numerTex = rtTex.isEmpty()
            ? QString::number(bNum)
            : (bNum == 1 ? rtTex : QStringLiteral("%1 %2").arg(bNum).arg(rtTex));
        bAbs = QStringLiteral("\\frac{%1}{%2}").arg(numerTex).arg(bDen);
    }
    QString sign = plus ? QStringLiteral(" + ") : QStringLiteral(" - ");
    if (aTex.isEmpty()) {
        latex = plus ? bAbs : (QStringLiteral("-") + bAbs);
    } else {
        latex = aTex + sign + bAbs;
    }
    return true;
}

static QString scalarToLatex(const Scalar& s, const DisplayFormat& fmt, bool compact = false) {
    if (fmt.kind == DisplayFormat::Decimal) {
        return scalarToStr(s, fmt);
    }
    // Exact 模式
    if (s.isRational()) {
        const auto& q = s.asRational();
        const auto& den = q.denominator();
        if (den > algemate::math::BigInt(1) && den <= algemate::math::BigInt(10000)) {
            QString num = QString::fromStdString(q.numerator().toString());
            QString denStr = QString::fromStdString(den.toString());
            bool neg = num.startsWith('-');
            if (neg) num = num.mid(1);
            QString core = QStringLiteral("\\frac{%1}{%2}").arg(num, denStr);
            return neg ? (QStringLiteral("-") + core) : core;
        }
        return scalarToStr(s, fmt);
    }
    // compact 模式: 先试简单符号形式 (√ / cbrt / nthroot); 只有 alg(...) 回落小数.
    //   矩阵/多项式内部必须保持轻量, 禁止任何 AlgReal 二次运算 (如 s*s),
    //   因为高次代数数 (如 svd 结果 √(15±√221)) 的 s*s 会触发结式爆炸, 导致崩溃.
    if (compact) {
        QString texTry = QString::fromStdString(s.toLatex());
        const bool hasAlg = texTry.contains(QStringLiteral("\\mathrm{alg}"))
                         || texTry.contains(QStringLiteral("alg("));
        if (!hasAlg) return texTry;
        // 回退: 仅当 minPoly 度数 ≤ 2 时 尝试 s*s 有理性检验 (对 2 次代数数 s*s 很快),
        //   可救 qr/svd 结果中 AlgReal 算术产生的 non-minimal minPoly 情形
        //   (例如 1/√3 的 minPoly 未精化到 x²-1/3, 但 s*s = 1/3 仍可识别).
        //   degree ≥ 3 直接跳过, 避免高次 resultant 卡死.
        try {
            if (s.minPoly().degree() <= 2) {
                AlgReal sq = s * s;
                if (sq.isRational()) {
                    Fraction q = sq.asRational();
                    long long num = 0, den = 1, r = 1;
                    if (q.sign() >= 0 && tryExtractSqrtRational_(q, num, den, r)) {
                        const bool neg = (s.toDouble() < 0);
                        QString sign = neg ? QStringLiteral("-") : QString();
                        if (r == 1) {
                            if (num == 0) return QStringLiteral("0");
                            QString abs = (den == 1)
                                ? QString::number(num)
                                : QStringLiteral("\\frac{%1}{%2}").arg(num).arg(den);
                            return sign + abs;
                        }
                        QString rt = QStringLiteral("\\sqrt{%1}").arg(r);
                        if (num == 1 && den == 1) return sign + rt;
                        if (den == 1) return sign + QString::number(num) + QStringLiteral(" ") + rt;
                        QString numerTex = (num == 1) ? rt
                                                      : (QString::number(num) + QStringLiteral(" ") + rt);
                        return sign + QStringLiteral("\\frac{%1}{%2}").arg(numerTex).arg(den);
                    }
                }
            }
        } catch (...) {
            // 任何异常回落小数
        }
        double d = s.toDouble();
        if (d == 0.0) d = 0.0;
        return QString::number(d, 'f', 6);
    }
    // 代数数: 先用 AlgReal::toLatex() 试取 sqrt/nth-root 闭式
    QString tex = QString::fromStdString(s.toLatex());
    const bool isAlg = tex.contains(QStringLiteral("\\mathrm{alg}"))
                    || tex.contains(QStringLiteral("alg("));
    if (!isAlg) return tex;
    // 回退策略 1: s² 是否为非负有理数 q, 则 s = ±(num/den) sqrt(r)
    try {
        AlgReal sq = s * s;
        if (sq.isRational()) {
            Fraction q = sq.asRational();
            long long num = 0, den = 1, r = 1;
            if (q.sign() >= 0 && tryExtractSqrtRational_(q, num, den, r)) {
                const bool neg = (s.toDouble() < 0);
                QString sign = neg ? QStringLiteral("-") : QString();
                if (r == 1) {
                    if (num == 0) return QStringLiteral("0");
                    QString abs = (den == 1)
                        ? QString::number(num)
                        : QStringLiteral("\\frac{%1}{%2}").arg(num).arg(den);
                    return sign + abs;
                }
                QString rt = QStringLiteral("\\sqrt{%1}").arg(r);
                if (num == 1 && den == 1) return sign + rt;
                if (den == 1) return sign + QString::number(num) + QStringLiteral(" ") + rt;
                QString numerTex = (num == 1) ? rt
                                              : (QString::number(num) + QStringLiteral(" ") + rt);
                return sign + QStringLiteral("\\frac{%1}{%2}").arg(numerTex).arg(den);
            }
        }
    } catch (...) {
        // 任何异常回退到 \mathrm{alg} 原样
    }
    // 回退策略 2: minPoly 为 2 次 →  a + b√r  闭式 (如 1+√2, 2-√3, (1+√5)/2).
    {
        QString binom;
        if (tryAlgToBinomialLatex_(s, binom)) return binom;
    }
    // 无法简化: 保持 \mathrm{alg}(..) 精确表示，不转小数
    return tex;
}

static QString complexToLatex(const ComplexC& z, const DisplayFormat& fmt) {
    const Scalar& re = z.real();
    const Scalar& im = z.imag();
    const bool reZero = re.isZero();
    const bool imZero = im.isZero();
    if (reZero && imZero) return QStringLiteral("0");
    if (imZero) return scalarToLatex(re, fmt);
    Scalar imAbs = (im.sign() < 0) ? -im : im;
    QString imAbsTex = scalarToLatex(imAbs, fmt);
    QString imPart = (imAbsTex == QStringLiteral("1"))
                     ? QStringLiteral("\\mathrm{i}")
                     : imAbsTex + QStringLiteral("\\,\\mathrm{i}");
    if (reZero) return (im.sign() < 0) ? (QStringLiteral("-") + imPart) : imPart;
    QString rePart = scalarToLatex(re, fmt);
    QString sign = (im.sign() < 0) ? QStringLiteral("-") : QStringLiteral("+");
    return rePart + sign + imPart;
}

static QString matrixToLatex(const MatrixA& M, const DisplayFormat& fmt) {
    const std::size_t R = M.rows(), C = M.cols();
    if (R == 0 || C == 0) return QStringLiteral("[]");
    // JKQTMathText 对单列 \begin{array} 有渲染 bug, 添加 ghost 列绕过.
    const bool ghost = (C == 1);
    QString cols = ghost ? QStringLiteral("cr")
                         : QString(static_cast<int>(C), QLatin1Char('c'));
    QString body;
    for (std::size_t i = 0; i < R; ++i) {
        for (std::size_t j = 0; j < C; ++j) {
            if (j) body += QStringLiteral(" & ");
            body += scalarToLatex(M(i, j), fmt, /*compact=*/true);
        }
        if (ghost) body += QStringLiteral(" & ");
        if (i + 1 < R) body += QStringLiteral(" \\\\ ");
    }
    return QStringLiteral("\\left(\\begin{array}{%1}%2\\end{array}\\right)")
        .arg(cols, body);
}

static QString polyToLatex(const std::vector<Scalar>& coeffs, const QString& var,
                           const DisplayFormat& fmt) {
    if (coeffs.empty()) return QStringLiteral("0");
    QString varTex = var;
    if (var == QString::fromUtf8("\u03BB")) varTex = QStringLiteral("\\lambda ");
    QString out;
    bool first = true;
    for (int i = (int)coeffs.size() - 1; i >= 0; --i) {
        const Scalar& c = coeffs[i];
        if (c.isZero()) continue;
        QString s = scalarToLatex(c, fmt, /*compact=*/true);
        QString sign;
        QString absTex = s;
        if (absTex.startsWith('-')) { sign = QStringLiteral("-"); absTex = absTex.mid(1); }
        else sign = QStringLiteral("+");
        QString term;
        if (i == 0) term = absTex;
        else if (i == 1) {
            term = (absTex == QStringLiteral("1")) ? varTex : (absTex + varTex);
        } else {
            QString powPart = varTex + QStringLiteral("^{%1}").arg(i);
            term = (absTex == QStringLiteral("1")) ? powPart : (absTex + powPart);
        }
        if (first) {
            out = (sign == QStringLiteral("-") ? QStringLiteral("-") : QString()) + term;
            first = false;
        } else {
            out += QStringLiteral(" ") + sign + QStringLiteral(" ") + term;
        }
    }
    if (first) return QStringLiteral("0");
    return out;
}

// JKQTMathText 渲染: 3× 超采样 + 屏幕 dpr 自适应 + SmoothTransformation 缩放.
static QPixmap renderLatexPixmap(const QString& latex, const RenderTheme& th,
                                 int fontPt = 22) {
    qreal dpr = 1.0;
    if (QGuiApplication::primaryScreen())
        dpr = QGuiApplication::primaryScreen()->devicePixelRatio();
    if (dpr < 1.0) dpr = 1.0;

    const int ss = 3;
    JKQTMathText mt;
    mt.useXITS();
    mt.setFontSize(fontPt * ss);
    mt.setFontColor(QColor(th.text));
    mt.parse(QStringLiteral("$") + latex + QStringLiteral("$"));

    QPixmap probe(4, 4);
    QPainter pm(&probe);
    QSizeF sz = mt.getSize(pm);
    pm.end();

    const int marginSS = 6 * ss;
    int wSS = qCeil(sz.width())  + marginSS * 2;
    int hSS = qCeil(sz.height()) + marginSS * 2;
    if (wSS < 8) wSS = 8;
    if (hSS < 8) hSS = 8;

    QPixmap big(wSS, hSS);
    big.fill(Qt::transparent);
    {
        QPainter p(&big);
        p.setRenderHint(QPainter::Antialiasing, true);
        p.setRenderHint(QPainter::TextAntialiasing, true);
        p.setRenderHint(QPainter::SmoothPixmapTransform, true);
        mt.draw(p, Qt::AlignCenter, QRectF(0, 0, wSS, hSS), false);
    }

    int wPhysical = int((wSS / ss) * dpr);
    int hPhysical = int((hSS / ss) * dpr);
    if (wPhysical < 2) wPhysical = 2;
    if (hPhysical < 2) hPhysical = 2;

    QPixmap scaled = big.scaled(wPhysical, hPhysical,
                                Qt::IgnoreAspectRatio,
                                Qt::SmoothTransformation);
    scaled.setDevicePixelRatio(dpr);
    return scaled;
}

// 前置声明: URL -> LaTeX 映射表 (同一 namespace 下供 embedLatexAsImg 调用)
QHash<QString, QString>& g_latexByUrl_();

static QString embedLatexAsImg(const QString& latex, const RenderTheme& th,
                               QTextDocument* doc, int fontPt = 15) {
    QPixmap px = renderLatexPixmap(latex, th, fontPt);
    static long long counter = 0;
    const QString url = QStringLiteral("calc-tex://%1").arg(++counter);
    doc->addResource(QTextDocument::ImageResource, QUrl(url), px);
    // 保存 URL -> LaTeX 映射, 输出区复制时反查还原为源码
    g_latexByUrl_()[url] = latex;
    int logicalW = int(px.width()  / px.devicePixelRatio());
    int logicalH = int(px.height() / px.devicePixelRatio());
    return QStringLiteral("<img src=\"%1\" width=\"%2\" height=\"%3\" />")
        .arg(url).arg(logicalW).arg(logicalH);
}

QHash<QString, QString>& g_latexByUrl_() {
    static QHash<QString, QString> m;
    return m;
}

QString latexForImageUrl(const QString& url) {
    return g_latexByUrl_().value(url);
}

void clearLatexImageCache() {
    g_latexByUrl_().clear();
}

// ---------------- 兼容: 旧版手工方括号矩阵 pixmap (仅作为 fallback) ----------------

static QPixmap renderMatrixPixmap(const MatrixA& M, const RenderTheme& th,
                                  const DisplayFormat& fmt) {
    const std::size_t R = M.rows(), C = M.cols();
    if (R == 0 || C == 0) return QPixmap();

    auto g = matrixCellStrings(M, fmt);

    QFont font(QStringLiteral("Cascadia Mono"));
    font.setStyleHint(QFont::Monospace);
    font.setPointSize(13);
    QFontMetrics fm(font);

    const int cellPadX = 14;
    const int cellPadY = 6;
    const int rowH     = fm.height() + cellPadY * 2;
    const int bracketW = 12;
    const int bracketGap = 6;
    const int bracketThick = 2;
    const int hook = 8;
    const int topPad    = 6;
    const int bottomPad = 6;

    std::vector<int> colW(C, 0);
    for (std::size_t j = 0; j < C; ++j)
        for (std::size_t i = 0; i < R; ++i)
            colW[j] = std::max<int>(colW[j], fm.horizontalAdvance(g[i][j]));

    int innerW = 0;
    for (int w : colW) innerW += w + cellPadX * 2;
    const int totalW = bracketW + bracketGap + innerW + bracketGap + bracketW;
    const int totalH = topPad + rowH * (int)R + bottomPad;

    qreal dpr = 2.0;
    QPixmap px(int(totalW * dpr), int(totalH * dpr));
    px.setDevicePixelRatio(dpr);
    px.fill(Qt::transparent);

    QPainter p(&px);
    p.setRenderHint(QPainter::Antialiasing, true);
    p.setRenderHint(QPainter::TextAntialiasing, true);
    p.setFont(font);

    p.setPen(QColor(th.text));
    int yTop = topPad;
    int xInnerStart = bracketW + bracketGap;
    for (std::size_t i = 0; i < R; ++i) {
        int x = xInnerStart;
        for (std::size_t j = 0; j < C; ++j) {
            QRect cellRect(x, yTop + (int)i * rowH, colW[j] + cellPadX * 2, rowH);
            p.drawText(cellRect, Qt::AlignCenter, g[i][j]);
            x += colW[j] + cellPadX * 2;
        }
    }

    QPen pen(QColor(th.accent));
    pen.setWidth(bracketThick);
    pen.setCapStyle(Qt::FlatCap);
    p.setPen(pen);

    const int bracketTop    = 1;
    const int bracketBottom = totalH - 2;
    int xL = bracketW / 2;
    p.drawLine(xL, bracketTop, xL, bracketBottom);
    p.drawLine(xL, bracketTop,    xL + hook, bracketTop);
    p.drawLine(xL, bracketBottom, xL + hook, bracketBottom);
    int xR = totalW - bracketW / 2 - 1;
    p.drawLine(xR, bracketTop, xR, bracketBottom);
    p.drawLine(xR, bracketTop,    xR - hook, bracketTop);
    p.drawLine(xR, bracketBottom, xR - hook, bracketBottom);

    p.end();
    return px;
}

// ---------------- HTML 输出 ----------------

QString Value::toHtml(const RenderTheme& th, const DisplayFormat& fmt,
                      QTextDocument* doc) const {
    // ---- 实标量 ----
    if (isScalar()) {
        // 在 Exact 模式下, 包含分数 / 代数数 (非有理) 的标量走 LaTeX 路径,
        // 以触发 √ / tryAlgToBinomialLatex_ 等符号展开.
        bool useLatex = false;
        if (fmt.kind != DisplayFormat::Decimal) {
            if (s_.isRational()) {
                const auto& q = s_.asRational();
                const auto& den = q.denominator();
                if (den > algemate::math::BigInt(1) && den <= algemate::math::BigInt(10000))
                    useLatex = true;
            } else {
                // 代数数 (含 sqrt / 根式 / alg): 走 LaTeX 路径以保持符号形式
                useLatex = true;
            }
        }
        if (doc && useLatex) {
            return embedLatexAsImg(scalarToLatex(s_, fmt), th, doc);
        }
        QString txt = scalarToStr(s_, fmt).toHtmlEscaped();
        return QStringLiteral(
            "<span style=\"color:%1; font-family:'Cascadia Mono','Consolas',monospace; "
            "font-size:15px; font-weight:600;\">%2</span>").arg(th.text, txt);
    }

    // ---- 复标量 ----
    if (isComplexScalar()) {
        if (doc) return embedLatexAsImg(complexToLatex(c_, fmt), th, doc);
        QString txt = complexToStr(c_, fmt).toHtmlEscaped();
        return QStringLiteral(
            "<span style=\"color:%1; font-family:'Cascadia Mono','Consolas',monospace; "
            "font-size:15px; font-weight:600;\">%2</span>").arg(th.text, txt);
    }

    // ---- 多项式 ----
    if (isPolynomial()) {
        if (doc) return embedLatexAsImg(polyToLatex(polyCoeffs_, polyVar_, fmt), th, doc);
        QString raw = polyToPlain(polyCoeffs_, polyVar_, fmt);
        return QStringLiteral(
            "<span style=\"color:%1; font-family:'Cascadia Mono','Consolas',monospace; "
            "font-size:15px;\">%2</span>").arg(th.text, raw.toHtmlEscaped());
    }

    // ---- 根列表 (特征值 / 有理根) ----
    if (isRootList()) {
        if (roots_.empty()) {
            return QStringLiteral("<span style=\"color:%1;\">无</span>").arg(th.textMuted);
        }
        // LaTeX 符号: λ → \lambda, 其余原样 (如 x)
        auto varLatex = [](const QString& v) -> QString {
            if (v == QString::fromUtf8("\xce\xbb")) return QStringLiteral("\\lambda");
            return v;
        };
        QString html;
        for (std::size_t i = 0; i < roots_.size(); ++i) {
            ComplexC z(roots_[i].first, roots_[i].second);
            if (doc) {
                QString line = QStringLiteral("%1_{%2} = %3")
                    .arg(varLatex(rootVar_)).arg(i + 1).arg(complexToLatex(z, fmt));
                html += QStringLiteral("<div style=\"margin:3px 0;\">%1</div>")
                    .arg(embedLatexAsImg(line, th, doc));
            } else {
                html += QStringLiteral(
                    "<div style=\"color:%1; font-family:'Cascadia Mono','Consolas',monospace; "
                    "font-size:15px; font-weight:600; margin:3px 0;\">%2<sub>%3</sub> = %4</div>")
                    .arg(th.text)
                    .arg(rootVar_.toHtmlEscaped())
                    .arg(i + 1)
                    .arg(complexToStr(z, fmt).toHtmlEscaped());
            }
        }
        return html;
    }

    // ---- 多项式因式分解 (factor 函数的输出) ----
    if (isFactored()) {
        QString lhs = polyToLatex(polyCoeffs_, polyVar_, fmt);
        QString lead = scalarToLatex(s_, fmt);
        const bool leadOne    = (lead == QStringLiteral("1"));
        const bool leadNegOne = (lead == QStringLiteral("-1"));
        // 纯变量因式 (coeffs == [0, 1], 即 x 本身) 不加括号, 重数直接写 x^{n}.
        auto isPureVar = [](const std::vector<Scalar>& c) {
            return c.size() == 2 && c[0].isZero()
                && (c[1] - Scalar(Fraction(1))).isZero();
        };
        QString rhs;
        if (factors_.empty()) {
            rhs = lead;
        } else {
            if (leadNegOne) rhs = QStringLiteral("-");
            else if (!leadOne) rhs = lead;
            for (const auto& [coeffs, mult] : factors_) {
                QString inner = polyToLatex(coeffs, polyVar_, fmt);
                if (isPureVar(coeffs)) {
                    rhs += inner;
                } else {
                    rhs += QStringLiteral("\\left(") + inner + QStringLiteral("\\right)");
                }
                if (mult > 1) rhs += QStringLiteral("^{%1}").arg(mult);
            }
        }
        QString latex = lhs + QStringLiteral(" = ") + rhs;
        if (doc) return embedLatexAsImg(latex, th, doc);
        return QStringLiteral(
            "<span style=\"color:%1; font-family:'Cascadia Mono','Consolas',monospace; "
            "font-size:15px;\">%2</span>")
            .arg(th.text, toPlain(fmt).toHtmlEscaped());
    }

    // ---- 向量列表 (零空间基 / 特征向量组) ----
    if (isVectorList()) {
        if (vectors_.empty()) {
            return QStringLiteral("<span style=\"color:%1;\">无</span>").arg(th.textMuted);
        }
        // LaTeX 符号映射: η → \eta, ξ → \xi, λ → \lambda, 其余原样
        auto varLatex = [](const QString& v) -> QString {
            if (v == QString::fromUtf8("\xce\xb7")) return QStringLiteral("\\eta");
            if (v == QString::fromUtf8("\xce\xbe")) return QStringLiteral("\\xi");
            if (v == QString::fromUtf8("\xce\xbb")) return QStringLiteral("\\lambda");
            return v;
        };
        if (!doc) {
            return QStringLiteral(
                "<pre style=\"color:%1; font-family:'Cascadia Mono','Consolas',monospace; "
                "font-size:15px; margin:0;\">%2</pre>")
                .arg(th.text, toPlain(fmt).toHtmlEscaped());
        }
        QString html;
        for (std::size_t i = 0; i < vectors_.size(); ++i) {
            // 标签:  η_{i} =
            const QString label = QStringLiteral("%1_{%2} =")
                .arg(varLatex(vecVar_)).arg(i + 1);
            const QString labelImg = embedLatexAsImg(label, th, doc);
            // 向量本体: 走 LaTeX 路径 (matrixToLatex 含 ghost-column 绕过单列 bug)
            QString vecImg;
            try {
                QString latex = matrixToLatex(vectors_[i], fmt);
                vecImg = embedLatexAsImg(latex, th, doc);
            } catch (...) {
                QPixmap px = renderMatrixPixmap(vectors_[i], th, fmt);
                static long long vlCounter = 0;
                const QString url = QStringLiteral("calc-vec://%1").arg(++vlCounter);
                doc->addResource(QTextDocument::ImageResource, QUrl(url), px);
                int w = int(px.width()  / px.devicePixelRatio());
                int h = int(px.height() / px.devicePixelRatio());
                vecImg = QStringLiteral(
                    "<img src=\"%1\" width=\"%2\" height=\"%3\" />")
                    .arg(url).arg(w).arg(h);
            }
            html += QStringLiteral(
                "<div style=\"margin:4px 0;\">%1&nbsp;%2</div>")
                .arg(labelImg, vecImg);
        }
        return html;
    }

    // ---- 命名矩阵列表 (LU/QR/合同对角化等) ----
    if (isNamedMatrices()) {
        if (namedMats_.empty()) {
            return QStringLiteral("<span style=\"color:%1;\">无</span>").arg(th.textMuted);
        }
        if (!doc) {
            return QStringLiteral(
                "<pre style=\"color:%1; font-family:'Cascadia Mono','Consolas',monospace; "
                "font-size:15px; margin:0;\">%2</pre>")
                .arg(th.text, toPlain(fmt).toHtmlEscaped());
        }
        QString html;
        for (const auto& it : namedMats_) {
            const QString labelImg = embedLatexAsImg(it.first, th, doc);
            const auto& M = it.second;
            QString matImg;
            const std::size_t Rn = M.rows(), Cn = M.cols();
            if (Rn == 0 || Cn == 0) {
                // 纯标签行: 只渲染 LaTeX 内容
                html += QStringLiteral("<div style=\"margin:4px 0;\">%1</div>").arg(labelImg);
                continue;
            } else {
                // 统一走 LaTeX 路径 (matrixToLatex 含 ghost-column 绕过单列 bug)
                try {
                    QString latex = matrixToLatex(M, fmt);
                    matImg = embedLatexAsImg(latex, th, doc);
                } catch (...) {
                    QPixmap px = renderMatrixPixmap(M, th, fmt);
                    static long long nmCtr = 0;
                    const QString url = QStringLiteral("calc-nm://%1").arg(++nmCtr);
                    doc->addResource(QTextDocument::ImageResource, QUrl(url), px);
                    int w = int(px.width()  / px.devicePixelRatio());
                    int h = int(px.height() / px.devicePixelRatio());
                    matImg = QStringLiteral("<img src=\"%1\" width=\"%2\" height=\"%3\" />")
                                .arg(url).arg(w).arg(h);
                }
            }
            html += QStringLiteral(
                "<div style=\"margin:4px 0;\">%1&nbsp;%2</div>")
                .arg(labelImg, matImg);
        }
        return html;
    }

    // ---- 文本结果 (definiteness/signature 等) ----
    if (isText()) {
        if (!doc) {
            return QStringLiteral(
                "<span style=\"color:%1; font-size:16px; font-weight:600;\">%2</span>")
                .arg(th.text, textContent_.toHtmlEscaped());
        }
        // 主显示区用稍大字号渲染 (18pt)
        return QStringLiteral(
            "<div style=\"color:%1; font-size:16px; font-weight:600;\">%2</div>")
            .arg(th.text, renderNoteWithLatex(textContent_, th, doc, 18));
    }

    // ---- 矩阵 ----
    const std::size_t R = m_.rows(), C = m_.cols();
    if (R == 0 || C == 0) {
        return QStringLiteral("<span style=\"color:%1;\">[]</span>").arg(th.accent);
    }

    if (doc) {
        // 统一走 LaTeX 路径 (matrixToLatex 含 ghost-column 绕过单列 bug)
        try {
            QString latex = matrixToLatex(m_, fmt);
            return embedLatexAsImg(latex, th, doc);
        } catch (...) {
            QPixmap px = renderMatrixPixmap(m_, th, fmt);
            static long long counter = 0;
            const QString url = QStringLiteral("calc-mat://%1").arg(++counter);
            doc->addResource(QTextDocument::ImageResource, QUrl(url), px);
            int logicalW = int(px.width()  / px.devicePixelRatio());
            int logicalH = int(px.height() / px.devicePixelRatio());
            return QStringLiteral("<img src=\"%1\" width=\"%2\" height=\"%3\" />")
                .arg(url).arg(logicalW).arg(logicalH);
        }
    }

    // ------- 退化: 纯 HTML 版 (不带 doc 时) -------
    auto g = matrixCellStrings(m_, fmt);
    const QString accent  = th.accent;
    const QString textCol = th.text;
    QString html;
    html += QStringLiteral(
        "<table cellspacing=\"0\" cellpadding=\"0\" "
        "style=\"border-collapse:separate; border-spacing:0; margin:4px 0;\">");
    for (std::size_t i = 0; i < R; ++i) {
        html += QStringLiteral("<tr>");
        html += QStringLiteral(
            "<td style=\"border-left:2px solid %1;\">&nbsp;</td>").arg(accent);
        for (std::size_t j = 0; j < C; ++j) {
            html += QStringLiteral(
                "<td align=\"center\" style=\"color:%1; "
                "font-family:'Cascadia Mono','Consolas',monospace; "
                "font-size:14px; padding:5px 14px;\">%2</td>")
                .arg(textCol, g[i][j].toHtmlEscaped());
        }
        html += QStringLiteral(
            "<td style=\"border-right:2px solid %1;\">&nbsp;</td>").arg(accent);
        html += QStringLiteral("</tr>");
    }
    html += QStringLiteral("</table>");
    return html;
}

// 含 `$...$` 片段的说明文本渲染器.
//   处理规则: 配对的 $...$ 走 embedLatexAsImg; 未配对的 $ 视为字面; 其余文本 toHtmlEscaped.
QString renderNoteWithLatex(const QString& src, const RenderTheme& th,
                            QTextDocument* doc, int fontPt) {
    auto escapeWithNl = [](const QString& s) {
        QString r = s.toHtmlEscaped();
        r.replace(QLatin1Char('\n'), QStringLiteral("<br/>"));
        return r;
    };
    if (!doc) return escapeWithNl(src);
    QString out;
    const int n = src.size();
    int i = 0;
    while (i < n) {
        int lo = src.indexOf(QLatin1Char('$'), i);
        if (lo < 0) {
            out += escapeWithNl(src.mid(i));
            break;
        }
        if (lo > i) out += escapeWithNl(src.mid(i, lo - i));
        int hi = src.indexOf(QLatin1Char('$'), lo + 1);
        if (hi < 0) {
            // 未配对 $: 当作字面。
            out += QStringLiteral("$");
            out += escapeWithNl(src.mid(lo + 1));
            break;
        }
        QString latex = src.mid(lo + 1, hi - lo - 1);
        if (latex.isEmpty()) {
            // $$ 空片段: 输出字面 $$.
            out += QStringLiteral("$$");
        } else {
            out += embedLatexAsImg(latex, th, doc, fontPt);
        }
        i = hi + 1;
    }
    return out;
}

}
