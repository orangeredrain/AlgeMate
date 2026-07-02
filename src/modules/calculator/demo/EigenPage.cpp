#include "EigenPage.h"
#include "DemoCommon.h"

#include "math/algorithm/ComplexEigen.h"
#include "math/algorithm/LinearAlgebra.h"
#include "math/algorithm/PolynomialAlg.h"
#include "math/core/Complex.h"
#include "math/core/Fraction.h"
#include "math/core/Matrix.h"
#include "math/core/Polynomial.h"
#include "math/trace/Step.h"
#include "math/trace/StepSequence.h"
#include "modules/calculator/interactive/expr/RenderSettings.h"

#include <QComboBox>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QScrollArea>
#include <QSpinBox>
#include <QTextBrowser>
#include <QVBoxLayout>

#include <algorithm>
#include <cmath>
#include <complex>
#include <cstddef>
#include <stdexcept>
#include <sstream>
#include <string>
#include <vector>

using namespace algemate::math;
using AlgeMate::Calculator::Interactive::RenderTheme;

namespace AlgeMate::Calculator::Demo {

static QString lambdaMinusALtx(const Matrix<Fraction>& A) {
    const auto n = A.rows();
    QString body;
    for (std::size_t i = 0; i < n; ++i) {
        for (std::size_t j = 0; j < n; ++j) {
            if (j) body += QStringLiteral(" & ");
            if (i == j) {
                Fraction d = A(i, j);
                if (d.isZero()) {
                    body += QStringLiteral("\\lambda");
                } else if (d.sign() < 0) {
                    body += QStringLiteral("\\lambda + %1").arg(fracLtx(-d));
                } else {
                    body += QStringLiteral("\\lambda - %1").arg(fracLtx(d));
                }
            } else {
                Fraction v = A(i, j);
                if (v.isZero()) body += QStringLiteral("0");
                else if (v.sign() < 0) body += fracLtx(-v);
                else body += QStringLiteral("-%1").arg(fracLtx(v));
            }
        }
        if (i + 1 < n) body += QStringLiteral(" \\\\\\\\ ");
    }
    QString cols(n, QLatin1Char('c'));
    return QStringLiteral("\\begin{pmatrix}%1\\end{pmatrix}").arg(body);
}

static QString charDetLtx(const Matrix<Fraction>& A) {
    const auto n = A.rows();
    QString body;
    for (std::size_t i = 0; i < n; ++i) {
        for (std::size_t j = 0; j < n; ++j) {
            if (j) body += QStringLiteral(" & ");
            if (i == j) {
                Fraction d = A(i, j);
                if (d.isZero()) body += QStringLiteral("\\lambda");
                else if (d.sign() < 0) body += QStringLiteral("\\lambda + %1").arg(fracLtx(-d));
                else body += QStringLiteral("\\lambda - %1").arg(fracLtx(d));
            } else {
                Fraction v = A(i, j);
                if (v.isZero()) body += QStringLiteral("0");
                else if (v.sign() < 0) body += fracLtx(-v);
                else body += QStringLiteral("-%1").arg(fracLtx(v));
            }
        }
        if (i + 1 < n) body += QStringLiteral(" \\\\\\\\ ");
    }
    QString cols(n, QLatin1Char('c'));
    return QStringLiteral("\\begin{vmatrix}%1\\end{vmatrix}").arg(body);
}

static Matrix<Fraction> companionMatrix(const Polynomial<Fraction>& p) {
    int n = p.degree();
    Matrix<Fraction> C(n, n);

    for (int i = 1; i < n; ++i)
        C(i, i - 1) = Fraction(1);

    const auto& c = p.coeffs();
    for (int i = 0; i < n; ++i)
        C(i, n - 1) = -c[i];
    return C;
}

static QString fmtDouble(double v, int decimals = 2) {
    if (std::abs(v) < 1e-12) v = 0.0;
    QString s = QString::number(v, 'f', decimals);

    if (s.contains(QLatin1Char('.'))) {
        while (s.endsWith(QLatin1Char('0'))) s.chop(1);
        if (s.endsWith(QLatin1Char('.'))) s.chop(1);
    }
    return s;
}

static QString complexLtx(double re, double im) {
    if (std::abs(im) < 1e-12)
        return fmtDouble(re);
    if (std::abs(re) < 1e-12) {
        if (std::abs(im - 1.0) < 1e-12) return QStringLiteral("i");
        if (std::abs(im + 1.0) < 1e-12) return QStringLiteral("-i");
        return fmtDouble(im) + QStringLiteral("i");
    }
    QString sign = (im > 0) ? QStringLiteral("+") : QStringLiteral("");
    return fmtDouble(re) + sign + fmtDouble(im) + QStringLiteral("i");
}

static QStringList quadraticRootsLtx(const Fraction& a, const Fraction& b, const Fraction& c) {
    Fraction delta = b * b - Fraction(4) * a * c;
    QStringList roots;
    if (delta.isZero()) {
        roots << fracLtx(Fraction(-1) * b / (Fraction(2) * a));
    } else if (delta.sign() > 0) {
        Fraction negB = Fraction(-1) * b;
        Fraction twoA = Fraction(2) * a;

        long long dNum = delta.numerator().toLongLong();
        long long dDen = delta.denominator().toLongLong();
        if (dNum > 0 && dDen > 0 && dNum < 100000000000000LL) {
            long long sn = extractSquareFactor(dNum);
            long long sd = extractSquareFactor(dDen);
            if (sn * sn == dNum && sd * sd == dDen) {
                Fraction sqrtD(sn, sd);
                roots << fracLtx((negB + sqrtD) / twoA);
                roots << fracLtx((negB - sqrtD) / twoA);
                return roots;
            }
        }
        roots << QStringLiteral("\\frac{%1 + \\sqrt{%2}}{%3}")
            .arg(fracLtx(negB), fracLtx(delta), fracLtx(twoA));
        roots << QStringLiteral("\\frac{%1 - \\sqrt{%2}}{%3}")
            .arg(fracLtx(negB), fracLtx(delta), fracLtx(twoA));
    } else {
        Fraction negDelta = Fraction(-1) * delta;
        roots << QStringLiteral("\\frac{%1 + i\\sqrt{%2}}{%3}")
            .arg(fracLtx(Fraction(-1) * b), fracLtx(negDelta), fracLtx(Fraction(2) * a));
        roots << QStringLiteral("\\frac{%1 - i\\sqrt{%2}}{%3}")
            .arg(fracLtx(Fraction(-1) * b), fracLtx(negDelta), fracLtx(Fraction(2) * a));
    }
    return roots;
}

using Complex = std::complex<double>;

static Complex evalPoly(const std::vector<Complex>& c, Complex x) {
    Complex r(0.0, 0.0);
    for (int i = static_cast<int>(c.size()) - 1; i >= 0; --i)
        r = r * x + c[i];
    return r;
}

static std::vector<Complex> durandKerner(const std::vector<Complex>& c) {
    int n = static_cast<int>(c.size()) - 1;
    if (n <= 0) return {};
    if (n == 1) return {-c[0] / c[1]};
    std::vector<Complex> roots(n);
    for (int i = 0; i < n; ++i) {
        double angle = 2.0 * 3.141592653589793 * i / n + 0.4;
        roots[i] = Complex(0.4 * std::cos(angle), 0.9 * std::sin(angle));
    }
    for (int iter = 0; iter < 200; ++iter) {
        double maxDelta = 0.0;
        for (int i = 0; i < n; ++i) {
            Complex p = evalPoly(c, roots[i]);
            Complex denom(1.0, 0.0);
            for (int j = 0; j < n; ++j)
                if (j != i) denom *= (roots[i] - roots[j]);
            if (std::abs(denom) < 1e-60) { roots[i] += Complex(1e-8, 1e-8); continue; }
            Complex delta = p / denom;
            if (std::abs(delta) > 1e6) delta = delta * (1e6 / std::abs(delta));
            roots[i] -= delta;
            maxDelta = std::max(maxDelta, std::abs(delta));
        }
        if (maxDelta < 1e-12) break;
    }
    return roots;
}

static std::vector<Complex> charpolyCoeffs(const Polynomial<Fraction>& cp) {
    std::vector<Complex> c;
    for (const auto& coeff : cp.coeffs())
        c.push_back(Complex(coeff.toDouble(), 0.0));
    return c;
}

static std::vector<std::pair<double, double>> numericalEigenvalues(const Matrix<Fraction>& A) {
    try {
        auto c = charpolyCoeffs(charpoly(A));
        auto roots = durandKerner(c);
        std::vector<std::pair<double, double>> evals;
        for (const auto& r : roots) {
            double re = r.real(), im = r.imag();
            if (std::isnan(re)) re = 0.0;
            if (std::isnan(im)) im = 0.0;
            if (std::isinf(re)) re = (re > 0 ? 1e6 : -1e6);
            if (std::isinf(im)) im = (im > 0 ? 1e6 : -1e6);
            evals.push_back({re, im});
        }
        return evals;
    } catch (...) { return {}; }
}

static std::vector<std::vector<std::pair<double,double>>>
complexNullspace(const Matrix<Fraction>& A, double re, double im) {
    const auto n = A.rows();
    std::vector<std::vector<std::pair<double,double>>> result;
    std::vector<std::vector<Complex>> M(n, std::vector<Complex>(n));
    for (std::size_t i = 0; i < n; ++i) {
        for (std::size_t j = 0; j < n; ++j)
            M[i][j] = Complex(-A(i, j).toDouble(), 0.0);
        M[i][i] = M[i][i] + Complex(re, im);
    }
    std::size_t r = 0;
    std::vector<std::size_t> pivotCols;
    std::vector<bool> isPivot(n, false);
    for (std::size_t c = 0; c < n && r < n; ++c) {
        std::size_t best = r;
        double bestAbs = std::abs(M[r][c]);
        for (std::size_t i = r + 1; i < n; ++i) {
            double a = std::abs(M[i][c]);
            if (a > bestAbs) { best = i; bestAbs = a; }
        }
        if (bestAbs < 1e-12) continue;
        if (best != r) std::swap(M[r], M[best]);
        Complex piv = M[r][c];
        for (std::size_t j = c; j < n; ++j) M[r][j] /= piv;
        for (std::size_t i = 0; i < n; ++i) {
            if (i == r) continue;
            Complex f = M[i][c];
            if (std::abs(f) < 1e-14) continue;
            for (std::size_t j = c; j < n; ++j) M[i][j] -= f * M[r][j];
        }
        pivotCols.push_back(c); isPivot[c] = true; ++r;
    }
    std::vector<std::size_t> freeCols;
    for (std::size_t c = 0; c < n; ++c)
        if (!isPivot[c]) freeCols.push_back(c);
    for (auto fc : freeCols) {
        std::vector<std::pair<double,double>> vec(n, {0.0, 0.0});
        vec[fc] = {1.0, 0.0};
        for (std::size_t ri = 0; ri < pivotCols.size(); ++ri) {
            std::size_t pc = pivotCols[ri];
            Complex val = -M[ri][fc];
            vec[pc] = {val.real(), val.imag()};
        }
        double norm = 0.0;
        for (const auto& v : vec) norm += v.first*v.first + v.second*v.second;
        if (norm > 1e-15) { norm = std::sqrt(norm); for (auto& v : vec) { v.first /= norm; v.second /= norm; } }
        result.push_back(vec);
    }
    return result;
}

static std::vector<std::pair<double,double>> inverseIter(const Matrix<Fraction>& A,
                                                          double re, double im) {
    auto ns = complexNullspace(A, re, im);
    if (!ns.empty()) return ns[0];
    return {};
}

EigenPage::EigenPage(QWidget* parent) : QWidget(parent) {
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    auto* topBar = new QWidget; topBar->setFixedHeight(48);
    auto* topLay = new QHBoxLayout(topBar);
    topLay->setContentsMargins(12, 0, 12, 0);
    auto* backBtn = new QPushButton(QStringLiteral("← 返回"));
    backBtn->setFlat(true); backBtn->setCursor(Qt::PointingHandCursor);
    backBtn->setStyleSheet(QStringLiteral("QPushButton { font-size:14px; color:#8A8FA3; } QPushButton:hover { color:#C0C4D6; }"));
    connect(backBtn, &QPushButton::clicked, this, &EigenPage::backRequested);
    topLay->addWidget(backBtn);
    auto* titleLbl = new QLabel(QStringLiteral("特征值与特征向量"));
    titleLbl->setStyleSheet(QStringLiteral("font-size:18px; font-weight:700;"));
    topLay->addWidget(titleLbl); topLay->addStretch(1);
    root->addWidget(topBar);

    auto* scroll = new QScrollArea; scroll->setWidgetResizable(true); scroll->setFrameShape(QFrame::NoFrame);
    auto* content = new QWidget;
    auto* cLay = new QVBoxLayout(content);
    cLay->setContentsMargins(24, 16, 24, 24); cLay->setSpacing(16);

    auto* paramW = new QWidget;
    auto* pLay = new QHBoxLayout(paramW);
    pLay->setContentsMargins(0, 0, 0, 0); pLay->setSpacing(10);
    pLay->addWidget(new QLabel(QStringLiteral("阶数 n =")));
    spinN_ = new QSpinBox; spinN_->setRange(2, 6); spinN_->setValue(2);
    pLay->addWidget(spinN_);
    pLay->addSpacing(12);
    pLay->addWidget(new QLabel(QStringLiteral("数域:")));
    domainCombo_ = new QComboBox;
    domainCombo_->addItem(QStringLiteral("实数域"));
    domainCombo_->addItem(QStringLiteral("复数域"));
    pLay->addWidget(domainCombo_);
    pLay->addSpacing(12);
    auto* genBtn = new QPushButton(QStringLiteral("生成矩阵"));
    genBtn->setCursor(Qt::PointingHandCursor);
    genBtn->setStyleSheet(QStringLiteral("QPushButton { background:#4A90D9; color:white; border-radius:6px; padding:6px 14px; font-size:13px; font-weight:600; } QPushButton:hover { background:#5BA0E9; }"));
    connect(genBtn, &QPushButton::clicked, this, &EigenPage::onGenerate);
    pLay->addWidget(genBtn); pLay->addStretch(1);
    cLay->addWidget(paramW);

    gridContainer_ = new QWidget;
    gridContainerLay_ = new QVBoxLayout(gridContainer_);
    gridContainerLay_->setContentsMargins(0, 0, 0, 0);
    cLay->addWidget(gridContainer_);

    solveBtn_ = new QPushButton(QStringLiteral("开始求解"));
    solveBtn_->setEnabled(false); solveBtn_->setCursor(Qt::PointingHandCursor); solveBtn_->setFixedWidth(160);
    solveBtn_->setStyleSheet(QStringLiteral("QPushButton { background:#4A90D9; color:white; border-radius:6px; padding:8px 16px; font-size:14px; font-weight:600; } QPushButton:hover { background:#5BA0E9; } QPushButton:disabled { background:#555; color:#888; }"));
    connect(solveBtn_, &QPushButton::clicked, this, &EigenPage::onSolve);
    cLay->addWidget(solveBtn_);
    auto* demoBtn = new QPushButton(QStringLiteral("演示例题"));
    demoBtn->setCursor(Qt::PointingHandCursor);
    demoBtn->setStyleSheet(QStringLiteral("QPushButton { background:#2196F3; color:white; border-radius:6px; padding:8px 16px; font-size:14px; font-weight:600; } QPushButton:hover { background:#1976D2; }"));
    connect(demoBtn, &QPushButton::clicked, this, &EigenPage::onDemo);
    cLay->addWidget(demoBtn);

    resultBrowser_ = new QTextBrowser; resultBrowser_->setOpenLinks(false); resultBrowser_->setMinimumHeight(400);
        attachLatexAutoPostProcess(resultBrowser_);
    resultBrowser_->setStyleSheet(QStringLiteral("QTextBrowser { border:1px solid #3A3D4A; border-radius:8px; padding:12px; }"));
    cLay->addWidget(resultBrowser_, 1);
    scroll->setWidget(content);
    root->addWidget(scroll, 1);
}

void EigenPage::onGenerate() {
    curN_ = spinN_->value(); cells_.clear();
    if (auto* old = gridContainer_->findChild<QWidget*>(QStringLiteral("inputGrid"))) delete old;
    auto* grid = new QWidget(gridContainer_); grid->setObjectName(QStringLiteral("inputGrid"));
    auto* lay = new QGridLayout(grid); lay->setSpacing(4); lay->setContentsMargins(0, 0, 0, 0);
    cells_.resize(static_cast<std::size_t>(curN_ * curN_));
    for (int i = 0; i < curN_; ++i)
        for (int j = 0; j < curN_; ++j) {
            auto* edit = new QLineEdit; edit->setFixedWidth(56); edit->setAlignment(Qt::AlignCenter);
            edit->setPlaceholderText(QStringLiteral("0"));
            cells_[static_cast<std::size_t>(i * curN_ + j)] = edit;
            edit->installEventFilter(this); lay->addWidget(edit, i, j);
        }
    gridContainerLay_->addWidget(grid); solveBtn_->setEnabled(true); resultBrowser_->clear();
    if (!cells_.empty()) cells_[0]->setFocus();
}

bool EigenPage::eventFilter(QObject* obj, QEvent* ev) {
    if (ev->type() == QEvent::KeyPress) {
        auto* ke = static_cast<QKeyEvent*>(ev);
        auto* edit = qobject_cast<QLineEdit*>(obj);
        if (edit && !cells_.empty()) {
            auto it = std::find(cells_.begin(), cells_.end(), edit);
            if (it != cells_.end()) {
                int idx = static_cast<int>(std::distance(cells_.begin(), it));
                int row = idx / curN_, col = idx % curN_;
                auto fc = [&](int r, int c) { if (r>=0&&r<curN_&&c>=0&&c<curN_) { cells_[r*curN_+c]->setFocus(); cells_[r*curN_+c]->selectAll(); } };
                if (ke->key() == Qt::Key_Right && edit->cursorPosition() == edit->text().length()) {
                    if (col+1<curN_) fc(row,col+1); else if (row+1<curN_) fc(row+1,0); return true;
                }
                if (ke->key() == Qt::Key_Left && edit->cursorPosition() == 0) {
                    if (col>0) fc(row,col-1); else if (row>0) fc(row-1,curN_-1); return true;
                }
                if (ke->key() == Qt::Key_Down) {
                    if (row+1<curN_) fc(row+1,col); else if (col+1<curN_) fc(0,col+1); return true;
                }
                if (ke->key() == Qt::Key_Up) {
                    if (row>0) fc(row-1,col); else if (col>0) fc(curN_-1,col-1); return true;
                }
                if (ke->key() == Qt::Key_Return || ke->key() == Qt::Key_Enter) {
                    if (idx+1 < static_cast<int>(cells_.size())) fc((idx+1)/curN_, (idx+1)%curN_);
                    else onSolve(); return true;
                }
            }
        }
    }
    return QWidget::eventFilter(obj, ev);
}

void EigenPage::onSolve() {
    if (curN_ == 0) return;

    Matrix<Fraction> A(static_cast<std::size_t>(curN_), static_cast<std::size_t>(curN_));
    for (int i = 0; i < curN_; ++i)
        for (int j = 0; j < curN_; ++j) {
            try { A(i, j) = parseFraction(cells_[static_cast<std::size_t>(i * curN_ + j)]->text()); }
            catch (...) {
                resultBrowser_->setHtml(QStringLiteral("<p style=\"color:#E74C3C; font-size:14px;\">输入错误：第 %1 行第 %2 列无法解析。</p>").arg(i+1).arg(j+1));
                return;
            }
        }

    bool complexDomain = (domainCombo_->currentIndex() == 1);
    auto th = RenderTheme::forCurrent();
    auto* doc = resultBrowser_->document(); doc->clear();
    QStringList parts;

    parts << titleHtml(QStringLiteral("求 A 的全部特征值和特征向量."), th);
    parts << formulaHtml(QStringLiteral("A = ") + matLtx(A), th, doc);
    parts << sectionHtml(QStringLiteral("解"), th);

    try {
    Polynomial<Fraction> cp = charpoly(A);
    RationalFactorization rf = factorOverQ(cp);

    parts << paraHtml(QStringLiteral("计算特征多项式："), th);
    parts << formulaHtml(
        QStringLiteral("|\\lambda I - A| = %1 = %2.")
            .arg(charDetLtx(A), polyLtx(cp)),
        th, doc, 15);

    std::vector<std::pair<Fraction, int>> exactEigenvals;
    std::vector<std::pair<std::pair<double, double>, int>> complexEigenvals; 
    bool allLinear = true;

    for (const auto& f : rf.factors) {
        int deg = f.first.degree(), mult = f.second;
        if (deg == 1) {
            Fraction root = Fraction(-1) * f.first.coeffs()[0] / f.first.coeffs()[1];
            exactEigenvals.push_back({root, mult});
        } else {
            allLinear = false;

            if (deg == 2) {
                auto roots = quadraticRootsLtx(f.first.coeffs()[2], f.first.coeffs()[1], f.first.coeffs()[0]);

                Fraction a = f.first.coeffs()[2], b = f.first.coeffs()[1], c = f.first.coeffs()[0];
                Fraction delta = b * b - Fraction(4) * a * c;
                if (delta.sign() >= 0) {

                    double sqrtDelta = std::sqrt(delta.toDouble());
                    double re1 = ((-b).toDouble() + sqrtDelta) / (2.0 * a.toDouble());
                    double re2 = ((-b).toDouble() - sqrtDelta) / (2.0 * a.toDouble());
                    for (int k = 0; k < mult; ++k) {
                        complexEigenvals.push_back({{re1, 0.0}, 1});
                        complexEigenvals.push_back({{re2, 0.0}, 1});
                    }
                } else {

                    double sqrtNegDelta = std::sqrt((-delta).toDouble());
                    double re = (-b).toDouble() / (2.0 * a.toDouble());
                    double im = sqrtNegDelta / (2.0 * a.toDouble());
                    for (int k = 0; k < mult; ++k) {
                        complexEigenvals.push_back({{re, im}, 1});
                        complexEigenvals.push_back({{re, -im}, 1});
                    }
                }
            } else {

                Matrix<Fraction> C = companionMatrix(f.first);
                auto cr = complexEigenvalues(C);
                for (const auto& ev : cr.eigenvalues) {
                    auto [re, im] = ev.value.toDouble();
                    for (int k = 0; k < mult; ++k)
                        complexEigenvals.push_back({{re, im}, 1});
                }
            }
        }
    }

    if (allLinear) {

        QString eigStr;
        for (std::size_t i = 0; i < exactEigenvals.size(); ++i) {
            if (i > 0) eigStr += QStringLiteral(", ");
            eigStr += QStringLiteral("$%1$").arg(fracLtx(exactEigenvals[i].first));
            if (exactEigenvals[i].second > 1)
                eigStr += QStringLiteral("（$%1$ 重）").arg(exactEigenvals[i].second);
        }

        {
            QString factLtx;
            for (const auto& ev : exactEigenvals)
                for (int k = 0; k < ev.second; ++k) {
                    Fraction r = ev.first;
                    if (r.sign() < 0)
                        factLtx += QStringLiteral("(\\lambda + %1)").arg(fracLtx(-r));
                    else if (r.isZero())
                        factLtx += QStringLiteral("\\lambda");
                    else
                        factLtx += QStringLiteral("(\\lambda - %1)").arg(fracLtx(r));
                }
            parts << formulaHtml(QStringLiteral("= %1.").arg(factLtx), th, doc, 15);
        }

        parts << paraHtml(QStringLiteral(
            "因此 $A$ 的全部特征值是 %1.").arg(eigStr), th, doc);

        int alphaIdx = 1;
        for (const auto& ev : exactEigenvals) {
            Fraction lambda = ev.first;
            int mult = ev.second;
            QString lamStr = fracLtx(lambda);

            Matrix<Fraction> lamI_minus_A(static_cast<std::size_t>(curN_), static_cast<std::size_t>(curN_));
            for (int i = 0; i < curN_; ++i) {
                for (int j = 0; j < curN_; ++j)
                    lamI_minus_A(i, j) = -A(i, j);
                lamI_minus_A(i, i) += lambda;
            }

            if (mult > 1)
                parts << paraHtml(QStringLiteral(
                    "对于特征值 $%1$（%2 重），解 $(%1 I - A)X = 0$：")
                    .arg(lamStr).arg(mult), th, doc);
            else
                parts << paraHtml(QStringLiteral(
                    "对于特征值 $%1$，解 $(%1 I - A)X = 0$：")
                    .arg(lamStr), th, doc);

            StepSequence trace;
            Matrix<Fraction> R = lamI_minus_A;
            improvedRref(R, trace);

            auto ms = trace.steps();
            QString chain = matLtx(ms[0].snapshot)
                + QStringLiteral("\\longrightarrow ")
                + matLtx(ms.back().snapshot)
                + QStringLiteral(",");
            parts << formulaHtml(chain, th, doc, 15);

            Matrix<Fraction> N = nullspace(lamI_minus_A);
            std::size_t geom = N.cols();
            if (geom == 0) {
                parts << paraHtml(QStringLiteral("它只有零解."), th);
                continue;
            }

            std::vector<std::size_t> pivotCols;
            for (std::size_t r = 0; r < R.rows(); ++r)
                for (std::size_t c = 0; c < R.cols(); ++c)
                    if (!R(r, c).isZero()) { pivotCols.push_back(c); break; }

            parts << paraHtml(QStringLiteral("它的一般解是"), th);
            QStringList genSolLines;
            for (std::size_t r = 0; r < pivotCols.size(); ++r) {
                std::size_t pc = pivotCols[r];
                QString line = QStringLiteral("x_{%1} = ").arg(pc + 1);
                bool first = true;
                for (std::size_t c = pc + 1; c < static_cast<std::size_t>(curN_); ++c) {
                    if (R(r, c).isZero()) continue;
                    Fraction coeff = -R(r, c);
                    bool neg = coeff.sign() < 0;
                    Fraction ac = coeff.abs();
                    if (first) { if (neg) line += QStringLiteral("-"); }
                    else { line += neg ? QStringLiteral(" - ") : QStringLiteral(" + "); }
                    if (!ac.isOne()) line += fracLtx(ac);
                    line += QStringLiteral("x_{%1}").arg(c + 1);
                    first = false;
                }
                if (first) line += QStringLiteral("0");
                genSolLines << line;
            }
            parts << formulaHtml(genSolLines.join(QStringLiteral(",\\ ")), th, doc, 15);

            std::vector<std::size_t> freeCols;
            for (std::size_t c = 0; c < static_cast<std::size_t>(curN_); ++c) {
                bool isPiv = false;
                for (auto pc : pivotCols) if (pc == c) { isPiv = true; break; }
                if (!isPiv) freeCols.push_back(c);
            }
            if (!freeCols.empty()) {
                QString freeStr;
                for (std::size_t k = 0; k < freeCols.size(); ++k) {
                    if (k > 0) freeStr += QStringLiteral(", ");
                    freeStr += QStringLiteral("$x_{%1}$").arg(freeCols[k] + 1);
                }
                parts << paraHtml(QStringLiteral(
                    "%1 是自由未知量，从而它的一个基础解系是").arg(freeStr), th, doc);
            }

            {
                QString basisStr;
                int startIdx = alphaIdx;
                for (std::size_t k = 0; k < geom; ++k) {
                    if (k > 0) basisStr += QStringLiteral(", \\quad ");
                    Matrix<Fraction> v(static_cast<std::size_t>(curN_), 1);
                    for (int i = 0; i < curN_; ++i) v(i, 0) = N(i, k);
                    basisStr += QStringLiteral("\\alpha_{%1} = %2").arg(alphaIdx++).arg(matLtx(v));
                }
                parts << formulaHtml(basisStr, th, doc, 15);

                if (geom == 1) {
                    parts << paraHtml(QStringLiteral(
                        "因此，$A$ 的属于 $%1$ 的全部特征向量是 "
                        "$\\{k_{%2}\\alpha_{%2} \\mid k_{%2} \\in K,\\ k_{%2} \\neq 0\\}$.")
                        .arg(lamStr).arg(startIdx), th, doc);
                } else {
                    QString cond = QStringLiteral("\\{");
                    for (std::size_t k = 0; k < geom; ++k) {
                        if (k > 0) cond += QStringLiteral(" + ");
                        cond += QStringLiteral("k_{%2}\\alpha_{%2}").arg(k + 1).arg(startIdx + static_cast<int>(k));
                    }
                    cond += QStringLiteral(" \\mid ");
                    for (std::size_t k = 0; k < geom; ++k) {
                        if (k > 0) cond += QStringLiteral(", ");
                        cond += QStringLiteral("k_{%1}").arg(k + 1);
                    }
                    cond += QStringLiteral(" \\in K,\\ (");
                    for (std::size_t k = 0; k < geom; ++k) {
                        if (k > 0) cond += QStringLiteral(", ");
                        cond += QStringLiteral("k_{%1}").arg(k + 1);
                    }
                    cond += QStringLiteral(") \\neq (0");
                    for (std::size_t k = 1; k < geom; ++k) cond += QStringLiteral(", 0");
                    cond += QStringLiteral(")\\}");

                    parts << paraHtml(QStringLiteral(
                        "因此，$A$ 的属于 $%1$ 的全部特征向量是 $%2$.")
                        .arg(lamStr, cond), th, doc);
                }
            }
        }
    } else {

        int alphaIdx = 1;

        {
            QString qFact;
            for (const auto& f : rf.factors) {
                if (f.first.degree() == 1) {
                    Fraction r = Fraction(-1) * f.first.coeffs()[0] / f.first.coeffs()[1];
                    if (r.sign() < 0) qFact += QStringLiteral("(\\lambda + %1)").arg(fracLtx(-r));
                    else if (r.isZero()) qFact += QStringLiteral("\\lambda");
                    else qFact += QStringLiteral("(\\lambda - %1)").arg(fracLtx(r));
                } else {
                    qFact += QStringLiteral("(") + polyLtx(f.first) + QStringLiteral(")");
                }
                if (f.second > 1) qFact += QStringLiteral("^{%1}").arg(f.second);
            }
            parts << formulaHtml(QStringLiteral("= %1.").arg(qFact), th, doc, 15);
        }

        std::vector<ComplexEigenPair> pairs;
        bool useNumerical = false;
        std::vector<std::pair<double, double>> numEvals;
        try {
            pairs = complexEigenPairs(A);
            if (pairs.empty()) throw std::runtime_error("complexEigenPairs returned empty");
        } catch (...) {
            useNumerical = true;
            try { numEvals = numericalEigenvalues(A); }
            catch (...) { numEvals.clear(); }
        }

        if (useNumerical) {

            if (!complexDomain) {
                std::vector<std::pair<double, double>> realEvals;
                for (const auto& ev : numEvals)
                    if (std::abs(ev.second) < 1e-10) realEvals.push_back(ev);
                if (realEvals.empty()) {
                    parts << paraHtml(QStringLiteral(
                        "特征多项式在 $\\mathbb{R}$ 上没有实根，"
                        "因此 $A$ 没有实特征值."), th, doc);
                } else {

                    {

                        std::vector<double> reals;
                        std::vector<std::pair<double,double>> cplx; 
                        std::vector<bool> used(numEvals.size(), false);
                        for (std::size_t i = 0; i < numEvals.size(); ++i) {
                            if (used[i]) continue;
                            double im = numEvals[i].second;
                            if (std::abs(im) < 1e-10) {
                                reals.push_back(numEvals[i].first);
                            } else {

                                for (std::size_t j = i + 1; j < numEvals.size(); ++j) {
                                    if (!used[j] && std::abs(numEvals[j].first - numEvals[i].first) < 1e-8
                                        && std::abs(numEvals[j].second + im) < 1e-8) {
                                        used[j] = true;
                                        cplx.push_back({numEvals[i].first, std::abs(im)});
                                        break;
                                    }
                                }
                            }
                            used[i] = true;
                        }
                        QString factLtx;
                        for (double r : reals) {
                            if (r < 0) factLtx += QStringLiteral("(\\lambda + %1)").arg(fmtDouble(-r));
                            else factLtx += QStringLiteral("(\\lambda - %1)").arg(fmtDouble(r));
                        }
                        for (const auto& cp : cplx) {
                            double r = cp.first, s = cp.second;
                            double b = -2.0 * r;        
                            double c = r*r + s*s;       
                            QString quad;
                            if (std::abs(b) < 1e-10) {
                                quad = QStringLiteral("\\lambda^{2} + %1").arg(fmtDouble(c));
                            } else if (b > 0) {
                                quad = QStringLiteral("\\lambda^{2} + %1\\lambda + %2").arg(fmtDouble(b)).arg(fmtDouble(c));
                            } else {
                                quad = QStringLiteral("\\lambda^{2} - %1\\lambda + %2").arg(fmtDouble(-b)).arg(fmtDouble(c));
                            }
                            factLtx += QStringLiteral("(%1)").arg(quad);
                        }
                        parts << formulaHtml(QStringLiteral("= %1.").arg(factLtx), th, doc, 15);
                    }

                    QString eigStr;
                    for (std::size_t i = 0; i < realEvals.size(); ++i) {
                        if (i > 0) eigStr += QStringLiteral(", ");
                        eigStr += QStringLiteral("$%1$").arg(fmtDouble(realEvals[i].first));
                    }
                    parts << paraHtml(QStringLiteral(
                        "因此 $A$ 的全部特征值是 %1.").arg(eigStr), th, doc);

                    for (const auto& ev : realEvals) {
                        double lam = ev.first;
                        QString lamStr = fmtDouble(lam);
                        parts << paraHtml(QStringLiteral(
                            "对于特征值 $%1$，解 $(%1 I - A)X = 0$：").arg(lamStr), th, doc);

                        auto vec = inverseIter(A, lam, 0.0);
                        if (!vec.empty() && std::abs(vec[0].first) + std::abs(vec[0].second) > 1e-10) {
                            QString body;
                            for (std::size_t i = 0; i < vec.size(); ++i) {
                                if (i) body += QStringLiteral(" \\\\\\\\ ");
                                body += complexLtx(vec[i].first, vec[i].second);
                                body += QStringLiteral(" & ");
                            }
                            int curAlpha = alphaIdx++;
                            QString vecLtx = QStringLiteral(
                                "\\alpha_{%1} = \\left(\\begin{array}{cr}%2\\end{array}\\right)")
                                .arg(curAlpha).arg(body);
                            parts << formulaHtml(vecLtx, th, doc, 13);
                            parts << paraHtml(QStringLiteral(
                                "因此，$A$ 的属于 $%1$ 的全部特征向量是 "
                                "$\\{k_{%2}\\alpha_{%2} \\mid k_{%2} \\in K,\\ k_{%2} \\neq 0\\}$.")
                                .arg(lamStr).arg(curAlpha), th, doc);
                        }
                    }
                }
            } else {

                {
                    QString factLtx;
                    for (const auto& ev : numEvals) {
                        auto [re, im] = ev;
                        if (std::abs(im) < 1e-10) {
                            if (re < 0) factLtx += QStringLiteral("(\\lambda + %1)").arg(fmtDouble(-re));
                            else factLtx += QStringLiteral("(\\lambda - %1)").arg(fmtDouble(re));
                        } else if (std::abs(re) < 1e-10) {
                            if (std::abs(im - 1.0) < 1e-10) factLtx += QStringLiteral("(\\lambda - i)");
                            else if (std::abs(im + 1.0) < 1e-10) factLtx += QStringLiteral("(\\lambda + i)");
                            else if (im > 0) factLtx += QStringLiteral("(\\lambda - %1i)").arg(fmtDouble(im));
                            else factLtx += QStringLiteral("(\\lambda + %1i)").arg(fmtDouble(-im));
                        } else {
                            factLtx += QStringLiteral("(\\lambda - (%1))").arg(complexLtx(re, im));
                        }
                    }
                    parts << formulaHtml(QStringLiteral("= %1.").arg(factLtx), th, doc, 14);
                }

                QString eigStr;
                for (std::size_t i = 0; i < numEvals.size(); ++i) {
                    if (i > 0) eigStr += QStringLiteral(", ");
                    eigStr += QStringLiteral("$%1$").arg(complexLtx(numEvals[i].first, numEvals[i].second));
                }
                parts << paraHtml(QStringLiteral(
                    "因此 $A$ 的全部特征值是 %1.").arg(eigStr), th, doc);

                for (const auto& ev : numEvals) {
                    if (std::abs(ev.second) > 1e-10) {
                        double lamRe = ev.first, lamIm = ev.second;
                        QString lamStr = complexLtx(lamRe, lamIm);
                        parts << paraHtml(QStringLiteral(
                            "对于特征值 $%1$，解 $(%1 I - A)X = 0$：").arg(lamStr), th, doc);
                        std::vector<std::vector<std::pair<double,double>>> ns;
                        try { ns = complexNullspace(A, lamRe, lamIm); } catch (...) {}
                        if (!ns.empty()) {
                            int curAlpha = alphaIdx;
                            QString basisStr;
                            for (std::size_t k = 0; k < ns.size(); ++k) {
                                if (k > 0) basisStr += QStringLiteral(", \\quad ");
                                QString body;
                                for (std::size_t i = 0; i < ns[k].size(); ++i) {
                                    if (i) body += QStringLiteral(" \\\\\\\\ ");
                                    body += complexLtx(ns[k][i].first, ns[k][i].second);
                                    body += QStringLiteral(" & ");
                                }
                                basisStr += QStringLiteral("\\alpha_{%1} = \\left(\\begin{array}{cr}%2\\end{array}\\right)")
                                    .arg(alphaIdx++).arg(body);
                            }
                            parts << formulaHtml(basisStr, th, doc, 13);
                            if (ns.size() == 1) {
                                parts << paraHtml(QStringLiteral(
                                    "因此，$A$ 的属于 $%1$ 的全部特征向量是 "
                                    "$\\{k_{%2}\\alpha_{%2} \\mid k_{%2} \\in K,\\ k_{%2} \\neq 0\\}$.")
                                    .arg(lamStr).arg(curAlpha), th, doc);
                            }
                        }
                    } else {
                        double lam = ev.first;
                        QString lamStr = fmtDouble(lam);
                        parts << paraHtml(QStringLiteral(
                            "对于特征值 $%1$，解 $(%1 I - A)X = 0$：").arg(lamStr), th, doc);
                        auto vec = inverseIter(A, lam, 0.0);
                        if (!vec.empty() && std::abs(vec[0].first) + std::abs(vec[0].second) > 1e-10) {
                            QString body;
                            for (std::size_t j = 0; j < vec.size(); ++j) {
                                if (j) body += QStringLiteral(" \\\\\\\\ ");
                                body += complexLtx(vec[j].first, vec[j].second);
                                body += QStringLiteral(" & ");
                            }
                            int curAlpha = alphaIdx++;
                            QString vecLtx = QStringLiteral(
                                "\\alpha_{%1} = \\left(\\begin{array}{cr}%2\\end{array}\\right)")
                                .arg(curAlpha).arg(body);
                            parts << formulaHtml(vecLtx, th, doc, 13);
                            parts << paraHtml(QStringLiteral(
                                "因此，$A$ 的属于 $%1$ 的全部特征向量是 "
                                "$\\{k_{%2}\\alpha_{%2} \\mid k_{%2} \\in K,\\ k_{%2} \\neq 0\\}$.")
                                .arg(lamStr).arg(curAlpha), th, doc);
                        }
                    }
                }
            }
        } else if (!complexDomain) {

            std::vector<ComplexEigenPair> realPairs;
            for (const auto& ev : pairs) {
                auto [re, im] = ev.value.toDouble();
                if (std::abs(im) < 1e-10)
                    realPairs.push_back(ev);
            }

            if (realPairs.empty()) {
                parts << paraHtml(QStringLiteral(
                    "特征多项式在 $\\mathbb{R}$ 上没有实根，"
                    "因此 $A$ 没有实特征值."), th, doc);
            } else {
                QString eigStr;
                for (std::size_t i = 0; i < realPairs.size(); ++i) {
                    if (i > 0) eigStr += QStringLiteral(", ");
                    eigStr += QStringLiteral("$%1$").arg(fmtDouble(realPairs[i].value.toDouble().first));
                    if (realPairs[i].multiplicity > 1)
                        eigStr += QStringLiteral("（$%1$ 重）").arg(realPairs[i].multiplicity);
                }
                parts << paraHtml(QStringLiteral(
                    "其实特征值为 %1.").arg(eigStr), th, doc);

                for (const auto& ev : realPairs) {
                    double lam = ev.value.toDouble().first;
                    QString lamStr = fmtDouble(lam);

                    parts << paraHtml(QStringLiteral(
                        "对于特征值 $%1$，解 $(%1 I - A)X = 0$（数值）：").arg(lamStr), th, doc);

                    const auto& basis = ev.eigenspaceBasis;
                    if (basis.cols() > 0) {
                        QString basisStr;
                        int startIdx = alphaIdx;
                        for (std::size_t k = 0; k < basis.cols(); ++k) {
                            if (k > 0) basisStr += QStringLiteral(", \\quad ");
                            QString body;
                            for (std::size_t i = 0; i < basis.rows(); ++i) {
                                if (i) body += QStringLiteral(" \\\\\\\\ ");
                                auto [vre, vim] = basis(i, k).toDouble();
                                body += complexLtx(vre, vim);
                                body += QStringLiteral(" & ");
                            }
                            basisStr += QStringLiteral("\\alpha_{%1} = \\left(\\begin{array}{cr}%2\\end{array}\\right)")
                                .arg(alphaIdx++).arg(body);
                        }
                        parts << formulaHtml(basisStr, th, doc, 13);

                        std::size_t geom = static_cast<std::size_t>(basis.cols());
                        if (geom == 1) {
                            parts << paraHtml(QStringLiteral(
                                "因此，$A$ 的属于 $%1$ 的全部特征向量是 "
                                "$\\{k_{%2}\\alpha_{%2} \\mid k_{%2} \\in K,\\ k_{%2} \\neq 0\\}$.")
                                .arg(lamStr).arg(startIdx), th, doc);
                        } else if (geom > 1) {
                            QString cond = QStringLiteral("\\{");
                            for (std::size_t k = 0; k < geom; ++k) {
                                if (k > 0) cond += QStringLiteral(" + ");
                                cond += QStringLiteral("k_{%2}\\alpha_{%2}").arg(k + 1).arg(startIdx + static_cast<int>(k));
                            }
                            cond += QStringLiteral(" \\mid ");
                            for (std::size_t k = 0; k < geom; ++k) {
                                if (k > 0) cond += QStringLiteral(", ");
                                cond += QStringLiteral("k_{%1}").arg(k + 1);
                            }
                            cond += QStringLiteral(" \\in K,\\ (");
                            for (std::size_t k = 0; k < geom; ++k) {
                                if (k > 0) cond += QStringLiteral(", ");
                                cond += QStringLiteral("k_{%1}").arg(k + 1);
                            }
                            cond += QStringLiteral(") \\neq (0");
                            for (std::size_t k = 1; k < geom; ++k) cond += QStringLiteral(", 0");
                            cond += QStringLiteral(")\\}");

                            parts << paraHtml(QStringLiteral(
                                "因此，$A$ 的属于 $%1$ 的全部特征向量是 $%2$.")
                                .arg(lamStr, cond), th, doc);
                        }
                    }
                }
            }
        } else {

            QString eigStr;
            for (std::size_t i = 0; i < pairs.size(); ++i) {
                if (i > 0) eigStr += QStringLiteral(", ");
                auto [re, im] = pairs[i].value.toDouble();
                eigStr += QStringLiteral("$%1$").arg(complexLtx(re, im));
                if (pairs[i].multiplicity > 1)
                    eigStr += QStringLiteral("（$%1$ 重）").arg(pairs[i].multiplicity);
            }

            {
                QString factLtx;
                for (const auto& ev : pairs) {
                    for (int k = 0; k < ev.multiplicity; ++k) {
                        auto [re, im] = ev.value.toDouble();
                        if (std::abs(im) < 1e-10) {
                            if (std::abs(re) < 1e-10) factLtx += QStringLiteral("\\lambda");
                            else if (re < 0) factLtx += QStringLiteral("(\\lambda + %1)").arg(fmtDouble(-re));
                            else factLtx += QStringLiteral("(\\lambda - %1)").arg(fmtDouble(re));
                        } else if (std::abs(re) < 1e-10) {

                            if (std::abs(im - 1.0) < 1e-10)
                                factLtx += QStringLiteral("(\\lambda - i)");
                            else if (std::abs(im + 1.0) < 1e-10)
                                factLtx += QStringLiteral("(\\lambda + i)");
                            else if (im > 0)
                                factLtx += QStringLiteral("(\\lambda - %1i)").arg(fmtDouble(im));
                            else
                                factLtx += QStringLiteral("(\\lambda + %1i)").arg(fmtDouble(-im));
                        } else {
                            factLtx += QStringLiteral("(\\lambda - (%1))").arg(complexLtx(re, im));
                        }
                    }
                }
                parts << formulaHtml(QStringLiteral("= %1.").arg(factLtx), th, doc, 14);
            }

            parts << paraHtml(QStringLiteral(
                "因此 $A$ 的全部特征值是 %1.").arg(eigStr), th, doc);

            for (const auto& ev : pairs) {
                auto [re, im] = ev.value.toDouble();
                QString lamStr = complexLtx(re, im);

                if (ev.multiplicity > 1)
                    parts << paraHtml(QStringLiteral(
                        "对于特征值 $%1$（%2 重），解 $(%1 I - A)X = 0$：")
                        .arg(lamStr).arg(ev.multiplicity), th, doc);
                else
                    parts << paraHtml(QStringLiteral(
                        "对于特征值 $%1$，解 $(%1 I - A)X = 0$：")
                        .arg(lamStr), th, doc);

                const auto& basis = ev.eigenspaceBasis;
                if (basis.cols() > 0) {
                    QString basisStr;
                    for (std::size_t k = 0; k < basis.cols(); ++k) {
                        if (k > 0) basisStr += QStringLiteral(", \\quad ");
                        QString body;
                        for (std::size_t i = 0; i < basis.rows(); ++i) {
                            if (i) body += QStringLiteral(" \\\\\\\\ ");
                            auto [vre, vim] = basis(i, k).toDouble();
                            body += complexLtx(vre, vim);
                            body += QStringLiteral(" & ");
                        }
                        basisStr += QStringLiteral("\\alpha_{%1} = \\left(\\begin{array}{cr}%2\\end{array}\\right)")
                            .arg(alphaIdx++).arg(body);
                    }
                    parts << formulaHtml(basisStr, th, doc, 13);

                    std::size_t geom = static_cast<std::size_t>(basis.cols());
                    int startIdx = alphaIdx - static_cast<int>(geom);
                    if (geom == 1) {
                        parts << paraHtml(QStringLiteral(
                            "因此，$A$ 的属于 $%1$ 的全部特征向量是 "
                            "$\\{k_{%2}\\alpha_{%2} \\mid k_{%2} \\in K,\\ k_{%2} \\neq 0\\}$.")
                            .arg(lamStr).arg(startIdx), th, doc);
                    } else if (geom > 1) {
                        QString cond = QStringLiteral("\\{");
                        for (std::size_t k = 0; k < geom; ++k) {
                            if (k > 0) cond += QStringLiteral(" + ");
                            cond += QStringLiteral("k_{%2}\\alpha_{%2}").arg(k + 1).arg(startIdx + static_cast<int>(k));
                        }
                        cond += QStringLiteral(" \\mid ");
                        for (std::size_t k = 0; k < geom; ++k) {
                            if (k > 0) cond += QStringLiteral(", ");
                            cond += QStringLiteral("k_{%1}").arg(k + 1);
                        }
                        cond += QStringLiteral(" \\in K,\\ (");
                        for (std::size_t k = 0; k < geom; ++k) {
                            if (k > 0) cond += QStringLiteral(", ");
                            cond += QStringLiteral("k_{%1}").arg(k + 1);
                        }
                        cond += QStringLiteral(") \\neq (0");
                        for (std::size_t k = 1; k < geom; ++k) cond += QStringLiteral(", 0");
                        cond += QStringLiteral(")\\}");

                        parts << paraHtml(QStringLiteral(
                            "因此，$A$ 的属于 $%1$ 的全部特征向量是 $%2$.")
                            .arg(lamStr, cond), th, doc);
                    }
                }
            }
        }
    }

    } catch (const std::exception& e) {
        parts << errorHtml(QStringLiteral(
            "计算出错：%1。"
            "可能是特征多项式含有高次不可约因式，"
            "当前暂不支持。").arg(QString::fromStdString(e.what())));
    } catch (...) {
        parts << errorHtml(QStringLiteral(
            "计算出错：未知异常。"
            "可能是特征多项式含有高次不可约因式。"));
    }

    resultBrowser_->setHtml(
        QStringLiteral("<div style=\"padding:4px; line-height:2.2;\">%1</div>")
        .arg(parts.join(QString())));
}

void EigenPage::onDemo() {
    spinN_->setValue(2);
    domainCombo_->setCurrentIndex(0);
    onGenerate();

    cells_[0]->setText(QStringLiteral("1"));
    cells_[1]->setText(QStringLiteral("2"));
    cells_[2]->setText(QStringLiteral("-1"));
    cells_[3]->setText(QStringLiteral("4"));
    onSolve();
}

} 
