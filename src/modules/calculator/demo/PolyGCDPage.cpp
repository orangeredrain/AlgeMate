#include "PolyGCDPage.h"
#include "DemoCommon.h"

#include "math/core/Fraction.h"
#include "math/core/Polynomial.h"
#include "modules/calculator/interactive/expr/RenderSettings.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QScrollArea>
#include <QTextBrowser>
#include <QVBoxLayout>

#include <map>
#include <sstream>
#include <string>
#include <vector>

using namespace algemate::math;
using AlgeMate::Calculator::Interactive::RenderTheme;

namespace AlgeMate::Calculator::Demo {

using Poly = Polynomial<Fraction>;

// ---- Polynomial parser for strings like "x^3 + x^2 - 7x + 2" ----
static Poly parsePoly(const QString& s) {
    std::map<int, Fraction> coeffs;
    QString t = s;
    t.remove(QLatin1Char(' '));
    if (t.isEmpty()) return Poly();

    int i = 0, n = t.size();
    while (i < n) {
        // Read sign
        int sign = 1;
        if (t[i] == '+') { ++i; }
        else if (t[i] == '-') { ++i; sign = -1; }
        if (i >= n) break;

        // Read coefficient
        Fraction coef(sign);
        int start = i;
        while (i < n && (t[i].isDigit() || t[i] == '/')) ++i;
        bool hasDigits = (i > start);
        if (hasDigits) {
            QString numStr = t.mid(start, i - start);
            int slash = numStr.indexOf('/');
            if (slash >= 0) {
                BigInt num(numStr.left(slash).toStdString());
                BigInt den(numStr.mid(slash+1).toStdString());
                coef = Fraction(num, den);
            } else {
                coef = Fraction(BigInt(numStr.toLongLong()));
            }
            if (sign < 0) coef = -coef;
        }

        // Read x and exponent
        int deg = 0;
        if (i < n && (t[i] == 'x' || t[i] == 'X')) {
            ++i; deg = 1;
            if (i < n && t[i] == '^') {
                ++i; start = i;
                while (i < n && t[i].isDigit()) ++i;
                deg = t.mid(start, i - start).toInt();
            }
        } else if (!hasDigits) break;

        coeffs[deg] = coeffs[deg] + coef;
    }

    Poly result;
    for (const auto& [d, c] : coeffs)
        result = result + Poly::monomial(d, c);
    return result;
}

// Polynomial → LaTeX (variable = x)
static QString polyToLtx(const Poly& p) {
    const auto& c = p.coeffs();
    if (c.empty()) return QStringLiteral("0");
    QString out; bool first = true;
    for (int i = static_cast<int>(c.size()) - 1; i >= 0; --i) {
        if (c[i].isZero()) continue;
        bool neg = c[i].sign() < 0; Fraction ac = c[i].abs();
        if (first) { if (neg) out += QStringLiteral("-"); }
        else { out += neg ? QStringLiteral(" - ") : QStringLiteral(" + "); }
        if (i == 0) { out += fracLtx(ac); }
        else {
            if (!ac.isOne() || i == 0) out += fracLtx(ac);
            out += QStringLiteral("x");
            if (i > 1) out += QStringLiteral("^{%1}").arg(i);
        }
        first = false;
    }
    return first ? QStringLiteral("0") : out;
}

// Polynomial → LaTeX with scaled leading coef (e.g., 3f(x) = ...)
static QString polyScaledLtx(const Poly& p, const Fraction& scale) {
    if (scale.isOne()) return polyToLtx(p);
    Poly sp = p;
    for (auto& c : const_cast<std::vector<Fraction>&>(sp.coeffs())) c = c * scale;
    return polyToLtx(sp);
}

// =====================================================================
//  Constructor
// =====================================================================

PolyGCDPage::PolyGCDPage(QWidget* parent) : QWidget(parent) {
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0); root->setSpacing(0);

    auto* topBar = new QWidget; topBar->setFixedHeight(48);
    auto* topLay = new QHBoxLayout(topBar);
    topLay->setContentsMargins(12, 0, 12, 0);
    auto* backBtn = new QPushButton(QStringLiteral("← 返回"));
    backBtn->setFlat(true); backBtn->setCursor(Qt::PointingHandCursor);
    backBtn->setStyleSheet(QStringLiteral("QPushButton { font-size:14px; color:#8A8FA3; } QPushButton:hover { color:#C0C4D6; }"));
    connect(backBtn, &QPushButton::clicked, this, &PolyGCDPage::backRequested);
    topLay->addWidget(backBtn);
    auto* titleLbl = new QLabel(QStringLiteral("多项式最大公因式"));
    titleLbl->setStyleSheet(QStringLiteral("font-size:18px; font-weight:700;"));
    topLay->addWidget(titleLbl); topLay->addStretch(1);
    root->addWidget(topBar);

    auto* scroll = new QScrollArea; scroll->setWidgetResizable(true); scroll->setFrameShape(QFrame::NoFrame);
    auto* content = new QWidget;
    auto* cLay = new QVBoxLayout(content);
    cLay->setContentsMargins(24, 16, 24, 24); cLay->setSpacing(16);

    // Input area
    auto* inputW = new QWidget;
    auto* iLay = new QVBoxLayout(inputW);
    iLay->setSpacing(8);

    auto* fRow = new QHBoxLayout;
    fRow->addWidget(new QLabel(QStringLiteral("f(x) =")));
    inputF_ = new QLineEdit; inputF_->setPlaceholderText(QStringLiteral("例如: x^3 + x^2 - 7x + 2"));
    inputF_->setMinimumWidth(400); fRow->addWidget(inputF_, 1);
    iLay->addLayout(fRow);

    auto* gRow = new QHBoxLayout;
    gRow->addWidget(new QLabel(QStringLiteral("g(x) =")));
    inputG_ = new QLineEdit; inputG_->setPlaceholderText(QStringLiteral("例如: 3x^2 - 5x - 2"));
    inputG_->setMinimumWidth(400); gRow->addWidget(inputG_, 1);
    iLay->addLayout(gRow);
    cLay->addWidget(inputW);

    auto* btnRow = new QHBoxLayout;
    auto* solveBtn = new QPushButton(QStringLiteral("开始求解"));
    solveBtn->setCursor(Qt::PointingHandCursor); solveBtn->setFixedWidth(160);
    solveBtn->setStyleSheet(QStringLiteral("QPushButton { background:#4A90D9; color:white; border-radius:6px; padding:8px 16px; font-size:14px; font-weight:600; } QPushButton:hover { background:#5BA0E9; }"));
    connect(solveBtn, &QPushButton::clicked, this, &PolyGCDPage::onSolve);
    btnRow->addWidget(solveBtn);

    auto* demoBtn = new QPushButton(QStringLiteral("演示例题"));
    demoBtn->setCursor(Qt::PointingHandCursor);
    demoBtn->setStyleSheet(QStringLiteral("QPushButton { background:#2196F3; color:white; border-radius:6px; padding:8px 16px; font-size:14px; font-weight:600; } QPushButton:hover { background:#1976D2; }"));
    connect(demoBtn, &QPushButton::clicked, this, &PolyGCDPage::onDemo);
    btnRow->addWidget(demoBtn);
    btnRow->addStretch(1);
    cLay->addLayout(btnRow);

    resultBrowser_ = new QTextBrowser; resultBrowser_->setOpenLinks(false); resultBrowser_->setMinimumHeight(400);
    resultBrowser_->setStyleSheet(QStringLiteral("QTextBrowser { border:1px solid #3A3D4A; border-radius:8px; padding:12px; }"));
    cLay->addWidget(resultBrowser_, 1);
    scroll->setWidget(content);
    root->addWidget(scroll, 1);
}

// =====================================================================
//  Solve
// =====================================================================

void PolyGCDPage::onSolve() {
    QString sf = inputF_->text().trimmed();
    QString sg = inputG_->text().trimmed();
    if (sf.isEmpty() || sg.isEmpty()) return;

    Poly f = parsePoly(sf);
    Poly g = parsePoly(sg);
    if (f.isZero() && g.isZero()) return;
    if (f.degree() < g.degree()) std::swap(f, g);

    auto th = RenderTheme::forCurrent();
    auto* doc = resultBrowser_->document(); doc->clear();
    QStringList parts;

    parts << paraHtml(QStringLiteral("求 $(f(x), g(x))$，并表示为 $u(x)f(x)+v(x)g(x)$."), th, doc);
    parts << formulaHtml(QStringLiteral("f(x) = ") + polyToLtx(f) + QStringLiteral(", \\quad g(x) = ") + polyToLtx(g) + QStringLiteral("."), th, doc);
    parts << sectionHtml(QStringLiteral("解"), th);

    // ---- Euclidean algorithm ----
    if (f.degree() < g.degree()) std::swap(f, g);

    // Scale f by lc(g) to avoid fractions
    Fraction lcg = g.coeffs()[g.degree()];
    Poly A = f * Poly::monomial(0, lcg), B = g;

    // Table: 被除式 | 除式 | 商式 | 余式
    struct DivRow { Poly dividend, divisor, quotient, remainder; };
    std::vector<DivRow> divSteps;
    QString table;
    table += QStringLiteral("\\begin{array}{c|c|c|c}\n");
    table += QStringLiteral("\\mathrm{Dividend} & \\mathrm{Divisor} & \\mathrm{Quotient} & \\mathrm{Remainder} \\\\ \\hline\n");

    while (!B.isZero()) {
        Fraction lcB = B.coeffs()[B.degree()];
        Poly r = A, q;
        for (int d = A.degree(); d >= B.degree() && !r.isZero(); ) {
            Fraction qc = r.coeffs()[d] / lcB;
            q = q + Poly::monomial(d - B.degree(), qc);
            r = r - Poly::monomial(d - B.degree(), qc) * B;
            d = r.degree();
        }
        divSteps.push_back({A, B, q, r});
        table += polyToLtx(A) + QStringLiteral(" & ") + polyToLtx(B)
              + QStringLiteral(" & ") + polyToLtx(q)
              + QStringLiteral(" & ") + polyToLtx(r) + QStringLiteral(" \\\\ \\hline\n");

        A = B;
        if (!r.isZero())
            B = r * Poly::monomial(0, Fraction(1) / r.coeffs()[r.degree()]);
        else
            B = r;
    }
    table += QStringLiteral("\\end{array}");
    parts << formulaHtml(table, th, doc, 12);

    // ---- GCD ----
    Poly gcd = A;
    if (!gcd.isZero() && !gcd.coeffs()[gcd.degree()].isOne())
        gcd = gcd * Poly::monomial(0, Fraction(1) / gcd.coeffs()[gcd.degree()]);
    parts << paraHtml(QStringLiteral("因为最后一个不等于零的余式是 $%1$，所以").arg(polyToLtx(gcd)), th, doc);
    parts << formulaHtml(QStringLiteral("(f(x), g(x)) = %1.").arg(polyToLtx(gcd)), th, doc);

    // ---- Extended GCD ----
    Poly r0 = f, r1 = g;
    Poly s0(Fraction(1)), s1(Fraction(0));
    Poly t0(Fraction(0)), t1(Fraction(1));
    while (!r1.isZero()) {
        auto divRes = r0.divmod(r1);
        Poly q = divRes.quotient;
        Poly r2 = divRes.remainder;
        Poly s2 = s0 - q * s1;
        Poly t2 = t0 - q * t1;
        r0 = r1; r1 = r2;
        s0 = s1; s1 = s2;
        t0 = t1; t1 = t2;
    }
    if (!r0.isZero()) {
        Fraction lcg2 = r0.coeffs()[r0.degree()];
        r0 = r0 * Poly::monomial(0, Fraction(1) / lcg2);
        s0 = s0 * Poly::monomial(0, Fraction(1) / lcg2);
        t0 = t0 * Poly::monomial(0, Fraction(1) / lcg2);
    }

    parts << paraHtml(QStringLiteral("把上述辗转相除过程写出来就是"), th, doc);
    for (std::size_t k = 0; k < divSteps.size(); ++k) {
        auto& s = divSteps[k];
        QString eq = polyToLtx(s.dividend) + QStringLiteral(" = \\left(")
            + polyToLtx(s.quotient) + QStringLiteral("\\right)\\left(")
            + polyToLtx(s.divisor) + QStringLiteral("\\right)");
        if (!s.remainder.isZero())
            eq += QStringLiteral(" + \\left(") + polyToLtx(s.remainder) + QStringLiteral("\\right)");
        parts << formulaHtml(eq + QStringLiteral("."), th, doc, 14);
    }

    parts << paraHtml(QStringLiteral("于是"), th, doc);
    parts << formulaHtml(
        QStringLiteral("(f(x), g(x)) = %1 = \\left(%2\\right)f(x) + \\left(%3\\right)g(x).")
        .arg(polyToLtx(r0), polyToLtx(s0), polyToLtx(t0)), th, doc, 14);

    parts << paraHtml(QStringLiteral("故 $(f(x), g(x)) = u(x)f(x) + v(x)g(x)$，其中"), th, doc);
    parts << formulaHtml(QStringLiteral("u(x) = %1, \\quad v(x) = %2.").arg(polyToLtx(s0), polyToLtx(t0)), th, doc);

    resultBrowser_->setHtml(QStringLiteral("<div style=\"padding:4px; line-height:2.2;\">%1</div>").arg(parts.join(QString())));
}

void PolyGCDPage::onDemo() {
    inputF_->setText(QStringLiteral("x^3 + x^2 - 7x + 2"));
    inputG_->setText(QStringLiteral("3x^2 - 5x - 2"));
    onSolve();
}

} // namespace AlgeMate::Calculator::Demo
