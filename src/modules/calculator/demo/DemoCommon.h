
#ifndef ALGEMATE_DEMO_COMMON_H
#define ALGEMATE_DEMO_COMMON_H

#include "math/core/BigInt.h"
#include "math/core/Complex.h"
#include "math/core/Fraction.h"
#include "math/core/Matrix.h"
#include "math/core/Polynomial.h"
#include "math/trace/Step.h"
#include "math/trace/StepSequence.h"
#include "modules/calculator/interactive/expr/RenderSettings.h"
#include "modules/calculator/interactive/expr/Value.h"
#include "latex/LatexRenderer.h"

#include <jkqtmathtext/jkqtmathtext.h>

#include <QGuiApplication>
#include <QObject>
#include <QPainter>
#include <QPixmap>
#include <QScreen>
#include <QString>
#include <QTextBrowser>
#include <QTextDocument>
#include <QTimer>
#include <QUrl>

#include <cstddef>
#include <memory>
#include <string>
#include <vector>

namespace AlgeMate::Calculator::Demo {

inline QPixmap renderLatex(const QString& latex, const Interactive::RenderTheme& th,
                           int fontPt = 18)
{
    qreal dpr = 1.0;
    if (auto* scr = QGuiApplication::primaryScreen())
        dpr = scr->devicePixelRatio();
    if (dpr < 1.0) dpr = 1.0;

    const int ss = 3;
    JKQTMathText mt;
    mt.useXITS();
    mt.setFontSize(fontPt * ss);
    mt.setFontColor(QColor(th.text));
    mt.parse(QStringLiteral("$\\displaystyle ") + latex + QStringLiteral("$"));

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

    int wP = int((wSS / ss) * dpr);
    int hP = int((hSS / ss) * dpr);
    if (wP < 2) wP = 2;
    if (hP < 2) hP = 2;

    QPixmap scaled = big.scaled(wP, hP,
                                Qt::IgnoreAspectRatio,
                                Qt::SmoothTransformation);
    scaled.setDevicePixelRatio(dpr);
    return scaled;
}

inline QString embedImg(const QString& latex, const Interactive::RenderTheme& th,
                        QTextDocument* doc, int fontPt = 18)
{
    return Latex::LatexRenderer::embedAsImg(
        doc, latex, false, fontPt, QColor(th.text));
}

inline void attachLatexAutoPostProcess(QTextBrowser* browser)
{
    Latex::attachLatexAutoPostProcess(browser);
}

inline algemate::math::Fraction parseFraction(const QString& text)
{
    using algemate::math::BigInt;
    using algemate::math::Fraction;
    QString s = text.trimmed();
    if (s.isEmpty()) return Fraction(0);

    int slash = s.indexOf(QLatin1Char('/'));
    if (slash >= 0) {
        std::string num = s.left(slash).trimmed().toStdString();
        std::string den = s.mid(slash + 1).trimmed().toStdString();
        return Fraction(BigInt(num), BigInt(den));
    }

    if (s.contains(QLatin1Char('.'))) {
        int dot = s.indexOf(QLatin1Char('.'));
        int decPlaces = s.length() - dot - 1;
        QString numStr = s;
        numStr.remove(dot, 1);
        BigInt num(numStr.toStdString());
        BigInt den(1);
        for (int i = 0; i < decPlaces; ++i) den = den * BigInt(10);
        return Fraction(num, den);
    }

    return Fraction(BigInt(s.toStdString()));
}

inline QString fracLtx(const algemate::math::Fraction& f) {
    if (f.sign() < 0 && f.denominator() > algemate::math::BigInt(1)) {
        algemate::math::Fraction pos = algemate::math::Fraction(-1) * f;
        return QStringLiteral("-") + QString::fromStdString(pos.toLatex());
    }
    return QString::fromStdString(f.toLatex());
}

inline algemate::math::Fraction dotProd(const algemate::math::Matrix<algemate::math::Fraction>& a,
                                         const algemate::math::Matrix<algemate::math::Fraction>& b) {
    algemate::math::Fraction sum(0);
    for (std::size_t i = 0; i < a.rows(); ++i)
        sum += a(i, 0) * b(i, 0);
    return sum;
}

inline algemate::math::Fraction normSq(const algemate::math::Matrix<algemate::math::Fraction>& v) {
    return dotProd(v, v);
}

inline long long extractSquareFactor(long long n) {
    if (n <= 1) return 1;
    long long s = 1;
    for (long long p = 2; p * p <= n; ++p) {
        long long p2 = p * p;
        while (n % p2 == 0) {
            n /= p2;
            s *= p;
        }
    }
    return s;
}

inline QString fracDivSqrtLtx(const algemate::math::Fraction& a,
                               const algemate::math::Fraction& ns) {
    using algemate::math::BigInt;
    if (a.isZero()) return QStringLiteral("0");

    BigInt A = a.numerator().abs();
    BigInt B = a.denominator();
    BigInt P = ns.numerator();
    BigInt Q = ns.denominator();

    BigInt radicand = P * Q;
    BigInt denom = B * P;

    long long s = 1, sf = 1;
    long long R = radicand.toLongLong();
    if (R > 1 && R < 100000000000000LL) {
        s = extractSquareFactor(R);
        sf = R / (s * s);
    } else {
        sf = R;
    }

    BigInt numCoeff = A * BigInt(s);

    BigInt g = BigInt::gcd(numCoeff, denom);
    numCoeff = numCoeff / g;
    denom = denom / g;

    QString sign = (a.sign() < 0) ? QStringLiteral("-") : QString();

    bool hasSqrt = (sf > 1);
    bool denomOne = denom == BigInt(1);

    if (hasSqrt && denomOne) {

        if (numCoeff == BigInt(1))
            return sign + QStringLiteral("\\sqrt{%1}").arg(sf);
        return sign + QStringLiteral("%1\\sqrt{%2}")
            .arg(QString::fromStdString(numCoeff.toString())).arg(sf);
    }
    if (hasSqrt && !denomOne) {

        QString numStr;
        if (numCoeff == BigInt(1))
            numStr = QStringLiteral("\\sqrt{%1}").arg(sf);
        else
            numStr = QStringLiteral("%1\\sqrt{%2}")
                .arg(QString::fromStdString(numCoeff.toString())).arg(sf);
        return sign + QStringLiteral("\\frac{%1}{%2}")
            .arg(numStr, QString::fromStdString(denom.toString()));
    }

    if (denomOne)
        return sign + QString::fromStdString(numCoeff.toString());
    return sign + QStringLiteral("\\frac{%1}{%2}")
        .arg(QString::fromStdString(numCoeff.toString()),
             QString::fromStdString(denom.toString()));
}

inline QString polyLtx(const algemate::math::Polynomial<algemate::math::Fraction>& p) {
    using algemate::math::Fraction;
    if (p.isZero()) return QStringLiteral("0");
    if (p.degree() == 0) return fracLtx(p.coeffs()[0]);

    QString result;
    bool first = true;
    for (int d = p.degree(); d >= 0; --d) {
        Fraction c = p[static_cast<std::size_t>(d)];
        if (c.isZero()) continue;
        bool neg = c.sign() < 0;
        Fraction ac = c.abs();

        if (first) {
            if (neg) result += QStringLiteral("-");
        } else {
            result += neg ? QStringLiteral(" - ") : QStringLiteral(" + ");
        }

        if (d == 0) {
            result += fracLtx(ac);
        } else {
            if (!ac.isOne()) result += fracLtx(ac);
            result += QStringLiteral("\\lambda");
            if (d > 1) result += QStringLiteral("^{%1}").arg(d);
        }
        first = false;
    }
    return result;
}

inline QString complexLtx(const algemate::math::Complex& z) {
    if (z.isReal())
        return QString::fromStdString(z.real().toLatex());
    return QString::fromStdString(z.toLatex());
}

inline QString matComplexLtx(const algemate::math::Matrix<algemate::math::Complex>& M) {
    const auto R = M.rows(), C = M.cols();
    if (R == 0 || C == 0) return QStringLiteral("()");
    const bool ghost = (C == 1);
    QString cols = ghost ? QStringLiteral("cr")
                         : QString(static_cast<int>(C), QLatin1Char('c'));
    QString body;
    for (std::size_t i = 0; i < R; ++i) {
        for (std::size_t j = 0; j < C; ++j) {
            if (j) body += QStringLiteral(" & ");
            body += complexLtx(M(i, j));
        }
        if (ghost) body += QStringLiteral(" & ");
        if (i + 1 < R) body += QStringLiteral(" \\\\\\\\ ");
    }
    return QStringLiteral("\\left(\\begin{array}{%1}%2\\end{array}\\right)")
        .arg(cols, body);
}

inline QString matLtx(const algemate::math::Matrix<algemate::math::Fraction>& M)
{
    using algemate::math::Fraction;
    const auto R = M.rows(), C = M.cols();
    if (R == 0 || C == 0) return QStringLiteral("()");
    const bool ghost = (C == 1);
    QString cols = ghost ? QStringLiteral("rcl")
                         : QString(static_cast<int>(C), QLatin1Char('c'));
    QString body;
    for (std::size_t i = 0; i < R; ++i) {
        if (ghost) body += QStringLiteral(" & ");
        for (std::size_t j = 0; j < C; ++j) {
            if (j) body += QStringLiteral(" & ");
            body += fracLtx(M(i, j));
        }
        if (ghost) body += QStringLiteral(" & ");
        if (i + 1 < R) body += QStringLiteral(" \\\\\\\\ ");
    }
    return QStringLiteral("\\left(\\begin{array}{%1}%2\\end{array}\\right)")
        .arg(cols, body);
}

inline QString normVecLtx(const algemate::math::Matrix<algemate::math::Fraction>& v,
                          const algemate::math::Fraction& k) {
    const auto R = v.rows();
    if (R == 0) return QStringLiteral("()");
    QString body;
    for (std::size_t i = 0; i < R; ++i) {
        if (i) body += QStringLiteral(" \\\\\\\\ ");
        body += QStringLiteral(" & ");
        body += fracDivSqrtLtx(v(i, 0), k);
        body += QStringLiteral(" & ");
    }
    return QStringLiteral("\\left(\\begin{array}{rcl}%1\\end{array}\\right)").arg(body);
}

inline QString eqnSysLtx(const algemate::math::Matrix<algemate::math::Fraction>& A)
{
    using algemate::math::Fraction;
    const auto m = A.rows(), n = A.cols();
    QString body;
    for (std::size_t i = 0; i < m; ++i) {
        bool first = true;
        QString row;
        for (std::size_t j = 0; j < n; ++j) {
            const Fraction& c = A(i, j);
            if (c.isZero()) continue;
            bool neg = c.sign() < 0;
            Fraction ac = c.abs();
            if (first) {
                if (neg) row += QStringLiteral("-");
            } else {
                row += neg ? QStringLiteral(" - ") : QStringLiteral(" + ");
            }
            if (!ac.isOne())
                row += QString::fromStdString(ac.toLatex());
            row += QStringLiteral("x_{%1}").arg(j + 1);
            first = false;
        }
        if (first) row = QStringLiteral("0");
        row += QStringLiteral(" & = 0");
        if (i + 1 < m)
            row += QStringLiteral(", \\\\ ");
        else
            row += QStringLiteral(".");
        body += row;
    }
    return QStringLiteral("\\left\\{\\begin{array}{ll}%1\\end{array}\\right.").arg(body);
}

inline QString eqnSysLtx(const algemate::math::Matrix<algemate::math::Fraction>& A,
                         const algemate::math::Matrix<algemate::math::Fraction>& b)
{
    using algemate::math::Fraction;
    const auto m = A.rows(), n = A.cols();
    QString body;
    for (std::size_t i = 0; i < m; ++i) {
        bool first = true;
        QString row;
        for (std::size_t j = 0; j < n; ++j) {
            const Fraction& c = A(i, j);
            if (c.isZero()) continue;
            bool neg = c.sign() < 0;
            Fraction ac = c.abs();
            if (first) {
                if (neg) row += QStringLiteral("-");
            } else {
                row += neg ? QStringLiteral(" - ") : QStringLiteral(" + ");
            }
            if (!ac.isOne())
                row += QString::fromStdString(ac.toLatex());
            row += QStringLiteral("x_{%1}").arg(j + 1);
            first = false;
        }
        if (first) row = QStringLiteral("0");
        row += QStringLiteral(" & = ") + fracLtx(b(i, 0));
        if (i + 1 < m)
            row += QStringLiteral(", \\\\ ");
        else
            row += QStringLiteral(".");
        body += row;
    }
    return QStringLiteral("\\left\\{\\begin{array}{ll}%1\\end{array}\\right.").arg(body);
}

inline std::vector<algemate::math::Matrix<algemate::math::Fraction>>
milestones(const algemate::math::StepSequence& trace)
{
    using algemate::math::Fraction;
    std::vector<algemate::math::Matrix<Fraction>> ms;
    const auto& st = trace.steps();
    if (st.empty()) return ms;

    for (std::size_t i = 0; i < st.size(); ++i) {
        if (st[i].kind == algemate::math::StepKind::Initial) {
            ms.push_back(st[i].snapshot);
        } else if (st[i].kind == algemate::math::StepKind::SelectPivot && i >= 2) {
            ms.push_back(st[i - 1].snapshot);
        } else if (st[i].kind == algemate::math::StepKind::Conclude) {
            ms.push_back(st[i].snapshot);
        }
    }
    ms.erase(std::unique(ms.begin(), ms.end()), ms.end());
    return ms;
}

inline algemate::math::Matrix<algemate::math::Fraction>
scaleInt(const algemate::math::Matrix<algemate::math::Fraction>& v)
{
    using algemate::math::BigInt;
    using algemate::math::Fraction;
    BigInt lcm(1);
    for (std::size_t r = 0; r < v.rows(); ++r) {
        BigInt d = v(r, 0).denominator();
        BigInt g = BigInt::gcd(lcm, d);
        lcm = lcm * d / g;
    }
    auto out = v;
    Fraction s(lcm);
    for (std::size_t r = 0; r < out.rows(); ++r)
        out(r, 0) = out(r, 0) * s;
    return out;
}

inline algemate::math::Matrix<algemate::math::Fraction>
rowEchelon(algemate::math::Matrix<algemate::math::Fraction> M)
{
    std::size_t r = 0, c = 0;
    const auto rows = M.rows(), cols = M.cols();
    while (r < rows && c < cols) {

        bool hasOne = false;
        for (std::size_t i = r; i < rows && !hasOne; ++i)
            if (M(i, c).abs().isOne()) hasOne = true;

        if (!hasOne) {
            for (std::size_t i = r; i < rows; ++i) {
                if (M(i, c).isZero()) continue;
                for (std::size_t j = r; j < rows; ++j) {
                    if (i == j || M(j, c).isZero()) continue;
                    algemate::math::Fraction diff = M(i, c) - M(j, c);
                    if (diff.abs().isOne()) {
                        M.addMulRow(i, j, algemate::math::Fraction(-1));
                        break;
                    }
                }
            }
        }

        std::size_t best = r;
        while (best < rows && M(best, c).isZero()) ++best;
        if (best == rows) { ++c; continue; }

        algemate::math::Fraction bestVal = M(best, c).abs();
        for (std::size_t i = best + 1; i < rows; ++i) {
            if (M(i, c).isZero()) continue;
            algemate::math::Fraction absVal = M(i, c).abs();
            if (absVal.isOne()) { best = i; break; }
            if (absVal < bestVal && !bestVal.isOne()) { best = i; bestVal = absVal; }
        }

        if (best != r) M.swapRows(r, best);

        algemate::math::Fraction pivVal = M(r, c);
        for (std::size_t i = r + 1; i < rows; ++i) {
            if (M(i, c).isZero()) continue;
            M.addMulRow(i, r, -M(i, c) / pivVal);
        }

        ++r; ++c;
    }
    return M;
}

inline std::size_t improvedRref(algemate::math::Matrix<algemate::math::Fraction>& M,
                                algemate::math::StepSequence& trace)
{
    using algemate::math::Fraction;
    const auto rows = M.rows(), cols = M.cols();

    trace.pushInitial(M);

    std::size_t r = 0;
    for (std::size_t c = 0; c < cols && r < rows; ++c) {

        bool hasOne = false;
        for (std::size_t i = r; i < rows && !hasOne; ++i)
            if (M(i, c).abs().isOne()) hasOne = true;

        if (!hasOne) {
            for (std::size_t i = r; i < rows; ++i) {
                if (M(i, c).isZero()) continue;
                for (std::size_t j = r; j < rows; ++j) {
                    if (i == j || M(j, c).isZero()) continue;
                    Fraction diff = M(i, c) - M(j, c);
                    if (diff.abs().isOne()) {
                        M.addMulRow(i, j, Fraction(-1));
                        trace.pushAddMulRow(i, j, Fraction(-1), M);
                        break;
                    }
                }
            }
        }

        std::size_t best = r;
        while (best < rows && M(best, c).isZero()) ++best;
        if (best == rows) continue;  

        Fraction bestVal = M(best, c).abs();
        for (std::size_t i = best + 1; i < rows; ++i) {
            if (M(i, c).isZero()) continue;
            Fraction absVal = M(i, c).abs();
            if (absVal.isOne()) { best = i; break; }
            if (absVal < bestVal && !bestVal.isOne()) { best = i; bestVal = absVal; }
        }

        trace.pushSelectPivot(r, c, M);

        if (best != r) {
            M.swapRows(r, best);
            trace.pushSwapRows(r, best, M);
        }

        Fraction pivVal = M(r, c);
        for (std::size_t i = r + 1; i < rows; ++i) {
            if (M(i, c).isZero()) continue;
            Fraction factor = -M(i, c) / pivVal;
            M.addMulRow(i, r, factor);
            trace.pushAddMulRow(i, r, factor, M);
        }

        ++r;
    }

    for (std::size_t ri = r; ri > 0; --ri) {
        std::size_t crow = ri - 1;
        std::size_t pc = 0;
        while (pc < cols && M(crow, pc).isZero()) ++pc;
        if (pc == cols) continue;

        Fraction pivVal = M(crow, pc);
        if (!pivVal.isOne()) {
            M.scaleRow(crow, Fraction(1) / pivVal);
            trace.pushScaleRow(crow, Fraction(1) / pivVal, M);
        }

        for (std::size_t i = 0; i < crow; ++i) {
            if (M(i, pc).isZero()) continue;
            Fraction factor = -M(i, pc);
            M.addMulRow(i, crow, factor);
            trace.pushAddMulRow(i, crow, factor, M);
        }
    }

    trace.pushConclude("RREF", M);
    return r;
}

inline QString formulaHtml(const QString& latex, const Interactive::RenderTheme& th,
                           QTextDocument* doc, int fontPt = 18) {
    return QStringLiteral("<p align=\"center\">%1</p>")
        .arg(embedImg(latex, th, doc, fontPt));
}

inline QString titleHtml(const QString& text, const Interactive::RenderTheme& th) {
    return QStringLiteral(
        "<p style=\"color:%1; font-size:16px; font-weight:600;\">%2</p>")
        .arg(th.text, text);
}

inline QString sectionHtml(const QString& text, const Interactive::RenderTheme& th) {
    return QStringLiteral(
        "<p style=\"color:%1; font-size:16px; font-weight:700; margin-top:10px;\">%2</p>")
        .arg(th.text, text);
}

inline QString paraHtml(const QString& richText, const Interactive::RenderTheme& th,
                        QTextDocument* doc = nullptr) {
    QString body = doc ? Interactive::renderNoteWithLatex(richText, th, doc, 15)
                       : richText.toHtmlEscaped();
    return QStringLiteral(
        "<p style=\"color:%1; font-size:15px;\">%2</p>")
        .arg(th.text, body);
}

inline QString errorHtml(const QString& text) {
    return QStringLiteral(
        "<p style=\"color:#E74C3C; font-size:14px;\">%1</p>").arg(text);
}

} 

#endif
