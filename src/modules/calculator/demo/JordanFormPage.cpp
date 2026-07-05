#include "JordanFormPage.h"
#include "DemoCommon.h"

#include "math/algorithm/LambdaMatrix.h"
#include "math/core/Fraction.h"
#include "math/core/Matrix.h"
#include "math/core/Polynomial.h"
#include "modules/calculator/interactive/expr/RenderSettings.h"

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
#include <vector>

using namespace algemate::math;
using AlgeMate::Calculator::Interactive::RenderTheme;

namespace AlgeMate::Calculator::Demo {

namespace {

QString lambdaMatLtx(const LambdaMatrix& M) {
    const auto R = M.rows(), C = M.cols();
    if (R == 0 || C == 0) return QStringLiteral("()");
    const bool ghost = (C == 1);
    QString cols = ghost ? QStringLiteral("cr")
                         : QString(static_cast<int>(C), QLatin1Char('c'));
    QString body;
    for (std::size_t i = 0; i < R; ++i) {
        for (std::size_t j = 0; j < C; ++j) {
            if (j) body += QStringLiteral(" & ");
            body += polyLtx(M(i, j));
        }
        if (ghost) body += QStringLiteral(" & ");
        if (i + 1 < R) body += QStringLiteral(" \\\\\\\\ ");
    }
    return QStringLiteral("\\left(\\begin{array}{%1}%2\\end{array}\\right)")
        .arg(cols, body);
}

using Cmplx = std::complex<double>;

std::vector<Cmplx> polyToCoeffs(const Polynomial<Fraction>& p) {
    std::vector<Cmplx> c;
    for (const auto& coeff : p.coeffs())
        c.push_back(Cmplx(coeff.toDouble(), 0.0));
    return c;
}

Cmplx evalPoly(const std::vector<Cmplx>& c, Cmplx x) {
    Cmplx r(0.0, 0.0);
    for (int i = static_cast<int>(c.size()) - 1; i >= 0; --i)
        r = r * x + c[i];
    return r;
}

std::vector<Cmplx> durandKerner(const std::vector<Cmplx>& c) {
    int n = static_cast<int>(c.size()) - 1;
    if (n <= 0) return {};
    if (n == 1) return {-c[0] / c[1]};
    std::vector<Cmplx> roots(n);
    for (int i = 0; i < n; ++i) {
        double angle = 2.0 * 3.141592653589793 * i / n + 0.4;
        roots[i] = Cmplx(0.4 * std::cos(angle), 0.9 * std::sin(angle));
    }
    for (int iter = 0; iter < 200; ++iter) {
        double maxDelta = 0.0;
        for (int i = 0; i < n; ++i) {
            Cmplx p = evalPoly(c, roots[i]);
            Cmplx denom(1.0, 0.0);
            for (int j = 0; j < n; ++j)
                if (j != i) denom *= (roots[i] - roots[j]);
            if (std::abs(denom) < 1e-60) { roots[i] += Cmplx(1e-8, 1e-8); continue; }
            Cmplx delta = p / denom;
            if (std::abs(delta) > 1e6) delta = delta * (1e6 / std::abs(delta));
            roots[i] -= delta;
            maxDelta = std::max(maxDelta, std::abs(delta));
        }
        if (maxDelta < 1e-12) break;
    }
    return roots;
}

QString fmtDouble(double v) {
    if (std::abs(v) < 1e-12) v = 0.0;
    QString s = QString::number(v, 'f', 2);
    if (s.contains(QLatin1Char('.'))) {
        while (s.endsWith(QLatin1Char('0'))) s.chop(1);
        if (s.endsWith(QLatin1Char('.'))) s.chop(1);
    }
    return s;
}

QString complexNumLtx(double re, double im) {

    if (fmtDouble(std::abs(im)) == QStringLiteral("0"))
        return fmtDouble(re);
    if (fmtDouble(std::abs(re)) == QStringLiteral("0")) {
        if (std::abs(im - 1.0) < 1e-12) return QStringLiteral("i");
        if (std::abs(im + 1.0) < 1e-12) return QStringLiteral("-i");
        return fmtDouble(im) + QStringLiteral("i");
    }
    QString sign = (im > 0) ? QStringLiteral("+") : QStringLiteral("");
    return fmtDouble(re) + sign + fmtDouble(im) + QStringLiteral("i");
}

struct NumRoot { double re; double im; int mult; };
std::vector<NumRoot> numericalRoots(const Polynomial<Fraction>& p) {
    if (p.degree() <= 0) return {};
    auto coeffs = polyToCoeffs(p);
    auto roots = durandKerner(coeffs);

    for (auto& r : roots) {
        if (std::abs(r.imag()) < 1e-3) r = Cmplx(r.real(), 0.0);
    }

    std::vector<NumRoot> result;
    std::vector<bool> used(roots.size(), false);
    for (std::size_t i = 0; i < roots.size(); ++i) {
        if (used[i]) continue;
        double re = roots[i].real(), im = roots[i].imag();
        int mult = 1;
        for (std::size_t j = i + 1; j < roots.size(); ++j) {
            if (used[j]) continue;
            double dr = re - roots[j].real(), di = im - roots[j].imag();
            if (dr * dr + di * di < 1e-6) {
                ++mult;
                used[j] = true;
            }
        }
        used[i] = true;
        result.push_back({re, im, mult});
    }
    return result;
}

QString formatFactorLtx(double re, double im, int mult) {
    QString factor;
    if (fmtDouble(std::abs(im)) == QStringLiteral("0")) {

        if (fmtDouble(std::abs(re)) == QStringLiteral("0"))
            factor = QStringLiteral("\\lambda");
        else if (re > 0)
            factor = QStringLiteral("(\\lambda - %1)").arg(fmtDouble(re));
        else
            factor = QStringLiteral("(\\lambda + %1)").arg(fmtDouble(-re));
    } else if (fmtDouble(std::abs(re)) == QStringLiteral("0")) {

        if (im > 0)
            factor = QStringLiteral("(\\lambda - %1)").arg(complexNumLtx(0, im));
        else
            factor = QStringLiteral("(\\lambda + %1)").arg(complexNumLtx(0, -im));
    } else {

        factor = QStringLiteral("(\\lambda - (%1))").arg(complexNumLtx(re, im));
    }
    if (mult > 1) factor += QStringLiteral("^{%1}").arg(mult);
    return factor;
}

QString factorPolyNumLtx(const Polynomial<Fraction>& p) {
    if (p.degree() <= 0) return polyLtx(p);
    auto roots = numericalRoots(p);
    if (roots.empty()) return polyLtx(p);
    QStringList parts;
    for (const auto& r : roots)
        parts << formatFactorLtx(r.re, r.im, r.mult);
    return parts.join(QString());
}

} 

JordanFormPage::JordanFormPage(QWidget* parent)
    : QWidget(parent)
{
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    auto* topBar = new QWidget;
    topBar->setFixedHeight(48);
    auto* topLay = new QHBoxLayout(topBar);
    topLay->setContentsMargins(12, 0, 12, 0);

    auto* backBtn = new QPushButton(QStringLiteral("← 返回"));
    backBtn->setFlat(true);
    backBtn->setCursor(Qt::PointingHandCursor);
    backBtn->setStyleSheet(QStringLiteral(
        "QPushButton { font-size:14px; color:#8A8FA3; } "
        "QPushButton:hover { color:#C0C4D6; }"));
    connect(backBtn, &QPushButton::clicked, this, &JordanFormPage::backRequested);
    topLay->addWidget(backBtn);

    auto* titleLbl = new QLabel(QStringLiteral("Jordan 标准形"));
    titleLbl->setStyleSheet(QStringLiteral("font-size:18px; font-weight:700;"));
    topLay->addWidget(titleLbl);
    topLay->addStretch(1);

    root->addWidget(topBar);

    auto* scroll = new QScrollArea;
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);

    auto* content = new QWidget;
    auto* cLay = new QVBoxLayout(content);
    cLay->setContentsMargins(24, 16, 24, 24);
    cLay->setSpacing(16);

    auto* paramW = new QWidget;
    auto* pLay = new QHBoxLayout(paramW);
    pLay->setContentsMargins(0, 0, 0, 0);
    pLay->setSpacing(10);

    pLay->addWidget(new QLabel(QStringLiteral("阶数 n =")));
    spinN_ = new QSpinBox;
    spinN_->setRange(2, 6);
    spinN_->setValue(3);
    pLay->addWidget(spinN_);

    pLay->addSpacing(12);
    auto* genBtn = new QPushButton(QStringLiteral("生成矩阵"));
    genBtn->setCursor(Qt::PointingHandCursor);
    genBtn->setStyleSheet(QStringLiteral(
        "QPushButton { background:#4A90D9; color:white; border-radius:6px; "
        "padding:6px 14px; font-size:13px; font-weight:600; } "
        "QPushButton:hover { background:#5BA0E9; }"));
    connect(genBtn, &QPushButton::clicked, this, &JordanFormPage::onGenerate);
    pLay->addWidget(genBtn);
    pLay->addStretch(1);

    cLay->addWidget(paramW);

    gridContainer_ = new QWidget;
    gridContainerLay_ = new QVBoxLayout(gridContainer_);
    gridContainerLay_->setContentsMargins(0, 0, 0, 0);
    cLay->addWidget(gridContainer_);

    solveBtn_ = new QPushButton(QStringLiteral("开始求解"));
    solveBtn_->setEnabled(false);
    solveBtn_->setCursor(Qt::PointingHandCursor);
    solveBtn_->setFixedWidth(160);
    solveBtn_->setStyleSheet(QStringLiteral(
        "QPushButton { background:#4A90D9; color:white; border-radius:6px; "
        "padding:8px 16px; font-size:14px; font-weight:600; } "
        "QPushButton:hover { background:#5BA0E9; } "
        "QPushButton:disabled { background:#555; color:#888; }"));
    connect(solveBtn_, &QPushButton::clicked, this, &JordanFormPage::onSolve);
    cLay->addWidget(solveBtn_);

    auto* demoBtn = new QPushButton(QStringLiteral("演示例题"));
    demoBtn->setCursor(Qt::PointingHandCursor);
    demoBtn->setStyleSheet(QStringLiteral(
        "QPushButton { background:#2196F3; color:white; border-radius:6px; "
        "padding:8px 16px; font-size:14px; font-weight:600; } "
        "QPushButton:hover { background:#1976D2; }"));
    connect(demoBtn, &QPushButton::clicked, this, &JordanFormPage::onDemo);
    cLay->addWidget(demoBtn);

    resultBrowser_ = new QTextBrowser;
        attachLatexAutoPostProcess(resultBrowser_);
    resultBrowser_->setOpenLinks(false);
    resultBrowser_->setMinimumHeight(400);
    resultBrowser_->setStyleSheet(QStringLiteral(
        "QTextBrowser { border:1px solid #3A3D4A; border-radius:8px; padding:12px; }"));
    cLay->addWidget(resultBrowser_, 1);

    scroll->setWidget(content);
    root->addWidget(scroll, 1);
}

void JordanFormPage::onGenerate()
{
    curN_ = spinN_->value();
    cells_.clear();

    if (auto* old = gridContainer_->findChild<QWidget*>(
            QStringLiteral("inputGrid")))
        delete old;

    auto* grid = new QWidget(gridContainer_);
    grid->setObjectName(QStringLiteral("inputGrid"));
    auto* lay = new QGridLayout(grid);
    lay->setSpacing(4);
    lay->setContentsMargins(0, 0, 0, 0);

    cells_.resize(static_cast<std::size_t>(curN_ * curN_));

    for (int i = 0; i < curN_; ++i) {
        for (int j = 0; j < curN_; ++j) {
            auto* edit = new QLineEdit;
            edit->setFixedWidth(56);
            edit->setAlignment(Qt::AlignCenter);
            edit->setPlaceholderText(QStringLiteral("0"));
            cells_[static_cast<std::size_t>(i * curN_ + j)] = edit;
            edit->installEventFilter(this);
            lay->addWidget(edit, i, j);
        }
    }

    gridContainerLay_->addWidget(grid);
    solveBtn_->setEnabled(true);
    resultBrowser_->clear();

    if (!cells_.empty()) cells_[0]->setFocus();
}

bool JordanFormPage::eventFilter(QObject* obj, QEvent* ev)
{
    if (ev->type() == QEvent::KeyPress) {
        auto* ke = static_cast<QKeyEvent*>(ev);
        auto* edit = qobject_cast<QLineEdit*>(obj);
        if (edit && !cells_.empty()) {
            auto it = std::find(cells_.begin(), cells_.end(), edit);
            if (it != cells_.end()) {
                int idx = static_cast<int>(std::distance(cells_.begin(), it));
                int row = idx / curN_;
                int col = idx % curN_;

                auto focusCell = [&](int r, int c) {
                    if (r >= 0 && r < curN_ && c >= 0 && c < curN_) {
                        cells_[r * curN_ + c]->setFocus();
                        cells_[r * curN_ + c]->selectAll();
                    }
                };

                if (ke->key() == Qt::Key_Right
                    && edit->cursorPosition() == edit->text().length()) {
                    if (col + 1 < curN_) focusCell(row, col + 1);
                    else if (row + 1 < curN_) focusCell(row + 1, 0);
                    return true;
                }
                if (ke->key() == Qt::Key_Left
                    && edit->cursorPosition() == 0) {
                    if (col > 0) focusCell(row, col - 1);
                    else if (row > 0) focusCell(row - 1, curN_ - 1);
                    return true;
                }
                if (ke->key() == Qt::Key_Down) {
                    if (row + 1 < curN_) focusCell(row + 1, col);
                    else if (col + 1 < curN_) focusCell(0, col + 1);
                    return true;
                }
                if (ke->key() == Qt::Key_Up) {
                    if (row > 0) focusCell(row - 1, col);
                    else if (col > 0) focusCell(curN_ - 1, col - 1);
                    return true;
                }
                if (ke->key() == Qt::Key_Return
                    || ke->key() == Qt::Key_Enter) {
                    if (idx + 1 < static_cast<int>(cells_.size()))
                        focusCell((idx + 1) / curN_, (idx + 1) % curN_);
                    else
                        onSolve();
                    return true;
                }
            }
        }
    }
    return QWidget::eventFilter(obj, ev);
}

void JordanFormPage::onSolve()
{
    if (curN_ == 0) return;

    Matrix<Fraction> A(static_cast<std::size_t>(curN_),
                       static_cast<std::size_t>(curN_));
    for (int i = 0; i < curN_; ++i) {
        for (int j = 0; j < curN_; ++j) {
            try {
                A(i, j) = parseFraction(cells_[static_cast<std::size_t>(
                    i * curN_ + j)]->text());
            } catch (...) {
                resultBrowser_->setHtml(QStringLiteral(
                    "<p style=\"color:#E74C3C; font-size:14px;\">"
                    "输入错误：第 %1 行第 %2 "
                    "列无法解析。"
                    "请输入整数、分数 (3/4) "
                    "或小数。</p>")
                    .arg(i + 1).arg(j + 1));
                return;
            }
        }
    }

    auto th = RenderTheme::forCurrent();
    auto* doc = resultBrowser_->document();
    doc->clear();

    QStringList parts;

    parts << titleHtml(QStringLiteral("求下述矩阵的 Jordan 标准形："), th);
    parts << formulaHtml(QStringLiteral("A = ") + matLtx(A), th, doc);

    parts << sectionHtml(QStringLiteral("解"), th);

    LambdaMatrix lamM = lambdaMinus(A);

    parts << paraHtml(QStringLiteral(
        "写出特征矩阵 $\\lambda I - A$："), th, doc);
    parts << formulaHtml(QStringLiteral("\\lambda I - A = ") + lambdaMatLtx(lamM), th, doc, 14);

    std::vector<Polynomial<Fraction>> D = determinantalDivisors(lamM);
    const std::size_t n = static_cast<std::size_t>(curN_);

    parts << paraHtml(QStringLiteral(
        "行列式因子 $D_k(\\lambda)$"
        "（$k$ 阶子式的首一最大公因式，"
        "在复数域上完全分解，从大到小排列）："), th, doc);
    for (std::size_t k = n; k >= 1; --k) {
        const auto& dk = D[k - 1];
        parts << formulaHtml(
            QStringLiteral("D_{%1}(\\lambda) = %2")
                .arg(k).arg(factorPolyNumLtx(dk)),
            th, doc, 14);
    }

    std::vector<Polynomial<Fraction>> allInvs;
    std::vector<Polynomial<Fraction>> nonOneInvs;
    {
        Polynomial<Fraction> D_prev(Fraction(1));  
        for (std::size_t k = 0; k < D.size(); ++k) {
            auto qr = D[k].divmod(D_prev);
            allInvs.push_back(qr.quotient);
            D_prev = D[k];
            if (!(qr.quotient.degree() == 0
                  && qr.quotient.coeffs()[0] == Fraction(1)))
                nonOneInvs.push_back(qr.quotient);
        }
    }

    parts << paraHtml(QStringLiteral(
        "不变因子 $d_k(\\lambda) = D_k(\\lambda) / D_{k-1}(\\lambda)$"
        "（在复数域上完全分解）："), th, doc);
    for (std::size_t i = 0; i < allInvs.size(); ++i) {
        parts << formulaHtml(
            QStringLiteral("d_{%1}(\\lambda) = %2")
                .arg(i + 1).arg(factorPolyNumLtx(allInvs[i])),
            th, doc, 14);
    }

    parts << paraHtml(QStringLiteral(
        "初等因子"
        "（将每个非常数不变因子在复数域上分解为一次因式的幂）："), th, doc);

    struct BlockInfo { double re; double im; int size; };
    std::vector<std::pair<Complex, int>> jordanBlockSpecs;
    std::vector<BlockInfo> blockInfos;

    if (nonOneInvs.empty()) {
        parts << paraHtml(QStringLiteral(
            "所有不变因子均为 $1$，矩阵可对角化。"), th, doc);
    } else {
        for (const auto& inv : nonOneInvs) {
            auto roots = numericalRoots(inv);
            QStringList terms;
            for (const auto& r : roots) {
                terms << formatFactorLtx(r.re, r.im, r.mult);
                jordanBlockSpecs.push_back({Complex::fromDouble(r.re, r.im), r.mult});
                blockInfos.push_back({r.re, r.im, r.mult});
            }
            parts << formulaHtml(terms.join(QStringLiteral(",\\quad ")), th, doc, 14);
        }
    }

    auto jcf = jordanFromBlocks(jordanBlockSpecs);

    parts << paraHtml(QStringLiteral(
        "由初等因子写出 Jordan 块："), th, doc);
    for (const auto& bi : blockInfos) {
        parts << formulaHtml(
            QStringLiteral("J_{%1}(%2)")
                .arg(bi.size).arg(complexNumLtx(bi.re, bi.im)),
            th, doc, 14);
    }

    parts << paraHtml(QStringLiteral(
        "所以，矩阵 $A$ 的 Jordan 标准形为"), th, doc);
    {
        std::size_t total = 0;
        for (const auto& bi : blockInfos) total += static_cast<std::size_t>(bi.size);

        const bool ghost = (total == 1);
        QString cols = ghost ? QStringLiteral("cr")
                             : QString(static_cast<int>(total), QLatin1Char('c'));
        QString body;
        std::size_t off = 0;
        for (const auto& bi : blockInfos) {
            for (int i = 0; i < bi.size; ++i) {
                if (off + i > 0) body += QStringLiteral(" \\\\\\\\ ");
                for (std::size_t j = 0; j < total; ++j) {
                    if (j) body += QStringLiteral(" & ");
                    if (j == off + static_cast<std::size_t>(i))
                        body += complexNumLtx(bi.re, bi.im);
                    else if (j == off + static_cast<std::size_t>(i) + 1 && i + 1 < bi.size)
                        body += QStringLiteral("1");
                    else
                        body += QStringLiteral("0");
                }
                if (ghost) body += QStringLiteral(" & ");
            }
            off += static_cast<std::size_t>(bi.size);
        }
        QString jLtx = QStringLiteral("J = \\left(\\begin{array}{%1}%2\\end{array}\\right)")
            .arg(cols, body);
        parts << formulaHtml(jLtx, th, doc, 14);
    }

    parts << paraHtml(QStringLiteral(
        "即存在可逆矩阵 $P$ 使得 $P^{-1}AP = J$。"), th, doc);

    resultBrowser_->setHtml(
        QStringLiteral("<div style=\"padding:4px; line-height:2.2;\">%1</div>")
        .arg(parts.join(QString())));
}

void JordanFormPage::onDemo()
{
    spinN_->setValue(3);
    onGenerate();

    const std::vector<std::vector<int>> demo = {
        {2, 3, 2},
        {1, 8, 2},
        {-2, -14, -3}
    };
    for (int i = 0; i < 3; ++i)
        for (int j = 0; j < 3; ++j)
            cells_[i * 3 + j]->setText(QString::number(demo[i][j]));

    onSolve();
}

} 
