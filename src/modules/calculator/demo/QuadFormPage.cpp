#include "QuadFormPage.h"
#include "DemoCommon.h"

#include "math/core/Fraction.h"
#include "math/core/Matrix.h"
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
#include <cstddef>
#include <sstream>
#include <vector>

using namespace algemate::math;
using AlgeMate::Calculator::Interactive::RenderTheme;

namespace AlgeMate::Calculator::Demo {

static QString augMatLtx(const Matrix<Fraction>& A, const Matrix<Fraction>& P) {
    const auto n = A.rows();
    QString body;
    for (std::size_t i = 0; i < n; ++i) {
        for (std::size_t j = 0; j < n; ++j) {
            if (j) body += QStringLiteral(" & ");
            body += fracLtx(A(i, j));
        }
        body += QStringLiteral(" \\\\\\\\ ");
    }
    body += QStringLiteral("\\hline ");
    for (std::size_t i = 0; i < n; ++i) {
        for (std::size_t j = 0; j < n; ++j) {
            if (j) body += QStringLiteral(" & ");
            body += fracLtx(P(i, j));
        }
        if (i + 1 < n) body += QStringLiteral(" \\\\\\\\ ");
    }
    QString cols(n, QLatin1Char('c'));
    return QStringLiteral("\\left(\\begin{array}{%1}%2\\end{array}\\right)").arg(cols, body);
}

static QString opArrow(const QString& label) {
    return QStringLiteral("\\underset{\\text{%1}}{\\longrightarrow}").arg(label);
}

QuadFormPage::QuadFormPage(QWidget* parent) : QWidget(parent) {
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0); root->setSpacing(0);

    auto* topBar = new QWidget; topBar->setFixedHeight(48);
    auto* topLay = new QHBoxLayout(topBar);
    topLay->setContentsMargins(12, 0, 12, 0);
    auto* backBtn = new QPushButton(QStringLiteral("← 返回"));
    backBtn->setFlat(true); backBtn->setCursor(Qt::PointingHandCursor);
    backBtn->setStyleSheet(QStringLiteral("QPushButton { font-size:14px; color:#8A8FA3; } QPushButton:hover { color:#C0C4D6; }"));
    connect(backBtn, &QPushButton::clicked, this, &QuadFormPage::backRequested);
    topLay->addWidget(backBtn);
    auto* titleLbl = new QLabel(QStringLiteral("实二次型化标准形"));
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
    spinN_ = new QSpinBox; spinN_->setRange(2, 6); spinN_->setValue(3);
    pLay->addWidget(spinN_);
    pLay->addSpacing(12);
    auto* genBtn = new QPushButton(QStringLiteral("生成矩阵"));
    genBtn->setCursor(Qt::PointingHandCursor);
    genBtn->setStyleSheet(QStringLiteral("QPushButton { background:#4A90D9; color:white; border-radius:6px; padding:6px 14px; font-size:13px; font-weight:600; } QPushButton:hover { background:#5BA0E9; }"));
    connect(genBtn, &QPushButton::clicked, this, &QuadFormPage::onGenerate);
    pLay->addWidget(genBtn); pLay->addStretch(1);
    cLay->addWidget(paramW);

    gridContainer_ = new QWidget;
    gridContainerLay_ = new QVBoxLayout(gridContainer_);
    gridContainerLay_->setContentsMargins(0, 0, 0, 0);
    cLay->addWidget(gridContainer_);

    solveBtn_ = new QPushButton(QStringLiteral("开始求解"));
    solveBtn_->setEnabled(false); solveBtn_->setCursor(Qt::PointingHandCursor); solveBtn_->setFixedWidth(160);
    solveBtn_->setStyleSheet(QStringLiteral("QPushButton { background:#4A90D9; color:white; border-radius:6px; padding:8px 16px; font-size:14px; font-weight:600; } QPushButton:hover { background:#5BA0E9; } QPushButton:disabled { background:#555; color:#888; }"));
    connect(solveBtn_, &QPushButton::clicked, this, &QuadFormPage::onSolve);
    cLay->addWidget(solveBtn_);
    auto* demoBtn = new QPushButton(QStringLiteral("演示例题"));
    demoBtn->setCursor(Qt::PointingHandCursor);
    demoBtn->setStyleSheet(QStringLiteral("QPushButton { background:#2196F3; color:white; border-radius:6px; padding:8px 16px; font-size:14px; font-weight:600; } QPushButton:hover { background:#1976D2; }"));
    connect(demoBtn, &QPushButton::clicked, this, &QuadFormPage::onDemo);
    cLay->addWidget(demoBtn);

    resultBrowser_ = new QTextBrowser; resultBrowser_->setOpenLinks(false); resultBrowser_->setMinimumHeight(400);
        attachLatexAutoPostProcess(resultBrowser_);
    resultBrowser_->setStyleSheet(QStringLiteral("QTextBrowser { border:1px solid #3A3D4A; border-radius:8px; padding:12px; }"));
    cLay->addWidget(resultBrowser_, 1);
    scroll->setWidget(content);
    root->addWidget(scroll, 1);
}

void QuadFormPage::onGenerate() {
    curN_ = spinN_->value(); cells_.clear();
    if (auto* old = gridContainer_->findChild<QWidget*>(QStringLiteral("inputGrid"))) delete old;
    auto* grid = new QWidget(gridContainer_); grid->setObjectName(QStringLiteral("inputGrid"));
    auto* lay = new QGridLayout(grid); lay->setSpacing(4); lay->setContentsMargins(0, 0, 0, 0);
    cells_.resize(static_cast<std::size_t>(curN_ * curN_));
    for (int i = 0; i < curN_; ++i)
        for (int j = 0; j < curN_; ++j) {
            auto* edit = new QLineEdit; edit->setFixedWidth(56); edit->setAlignment(Qt::AlignCenter);
            edit->setPlaceholderText(QStringLiteral("0"));
            cells_[i*curN_+j] = edit; edit->installEventFilter(this);
            if (i != j) {
                int symIdx = j*curN_+i;
                connect(edit, &QLineEdit::textChanged, this, [this, symIdx, edit](const QString& t){
                    if (edit->property("syncing").toBool()) return;
                    cells_[symIdx]->setProperty("syncing", true);
                    cells_[symIdx]->setText(t);
                    cells_[symIdx]->setProperty("syncing", false);
                });
            }
            lay->addWidget(edit, i, j);
        }
    gridContainerLay_->addWidget(grid); solveBtn_->setEnabled(true); resultBrowser_->clear();
    if (!cells_.empty()) cells_[0]->setFocus();
}

static int nextUpper(int idx, int n) {
    int r = idx / n, c = idx % n;
    do {
        if (c + 1 < n) ++c; else { ++r; c = r; }
        if (r >= n) return -1;
    } while (r > c);
    return r * n + c;
}
static int prevUpper(int idx, int n) {
    int r = idx / n, c = idx % n;
    do {
        if (c > r) --c; else { --r; c = n - 1; }
        if (r < 0) return -1;
    } while (r > c);
    return r * n + c;
}
bool QuadFormPage::eventFilter(QObject* obj, QEvent* ev) {
    if (ev->type() == QEvent::KeyPress) {
        auto* ke = static_cast<QKeyEvent*>(ev);
        auto* edit = qobject_cast<QLineEdit*>(obj);
        if (edit && !cells_.empty()) {
            auto it = std::find(cells_.begin(), cells_.end(), edit);
            if (it != cells_.end()) {
                int idx = static_cast<int>(std::distance(cells_.begin(), it));
                auto focusByIdx = [&](int i) { if (i>=0 && i<static_cast<int>(cells_.size())) { cells_[i]->setFocus(); cells_[i]->selectAll(); } };
                if (ke->key() == Qt::Key_Right && edit->cursorPosition() == edit->text().length()) {
                    int nxt = nextUpper(idx, curN_); if (nxt >= 0) focusByIdx(nxt); return true;
                }
                if (ke->key() == Qt::Key_Left && edit->cursorPosition() == 0) {
                    int prv = prevUpper(idx, curN_); if (prv >= 0) focusByIdx(prv); return true;
                }
                int row = idx / curN_, col = idx % curN_;
                if (ke->key() == Qt::Key_Down) { if (row+1<curN_) focusByIdx((row+1)*curN_+col); return true; }
                if (ke->key() == Qt::Key_Up)   { if (row>0) focusByIdx((row-1)*curN_+col); return true; }
                if (ke->key() == Qt::Key_Return || ke->key() == Qt::Key_Enter) {
                    int nxt = nextUpper(idx, curN_); if (nxt >= 0) focusByIdx(nxt); else onSolve(); return true;
                }
            }
        }
    }
    return QWidget::eventFilter(obj, ev);
}

void QuadFormPage::onSolve() {
    if (curN_ == 0) return;

    Matrix<Fraction> A(static_cast<std::size_t>(curN_), static_cast<std::size_t>(curN_));
    for (int i = 0; i < curN_; ++i)
        for (int j = 0; j < curN_; ++j) {
            try { A(i, j) = parseFraction(cells_[i*curN_+j]->text()); }
            catch (...) {
                resultBrowser_->setHtml(QStringLiteral("<p style=\"color:#E74C3C; font-size:14px;\">输入错误：第 %1 行第 %2 列无法解析。</p>").arg(i+1).arg(j+1)); return;
            }
        }

    auto th = RenderTheme::forCurrent();
    auto* doc = resultBrowser_->document(); doc->clear();
    QStringList parts;

    QString quadExpr;
    bool first = true;
    for (std::size_t i = 0; i < A.rows(); ++i) {
        for (std::size_t j = i; j < A.cols(); ++j) {
            Fraction v = (i == j) ? A(i, i) : Fraction(2) * A(i, j);
            if (v.isZero()) continue;
            bool neg = v.sign() < 0; Fraction ac = v.abs();
            if (first) { if (neg) quadExpr += QStringLiteral("-"); }
            else { quadExpr += neg ? QStringLiteral(" - ") : QStringLiteral(" + "); }
            if (!ac.isOne()) quadExpr += fracLtx(ac);
            quadExpr += QStringLiteral("x_{%1}").arg(i+1);
            if (i != j) quadExpr += QStringLiteral("x_{%1}").arg(j+1);
            first = false;
        }
    }
    if (first) quadExpr = QStringLiteral("0");

    QString vars;
    for (int k = 1; k <= curN_; ++k) { if (k > 1) vars += QStringLiteral(","); vars += QStringLiteral("x_{%1}").arg(k); }

    parts << titleHtml(QStringLiteral("把下述二次型化成标准形，并写出所作的非退化线性替换："), th);
    parts << formulaHtml(QStringLiteral("g(%1) = %2.").arg(vars, quadExpr), th, doc);

    parts << sectionHtml(QStringLiteral("解"), th);
    parts << paraHtml(QStringLiteral("$g$ 的矩阵是"), th, doc);
    parts << formulaHtml(QStringLiteral("A = ") + matLtx(A) + QStringLiteral("."), th, doc);

    Matrix<Fraction> D = A;
    Matrix<Fraction> P(curN_, curN_);
    for (std::size_t i = 0; i < P.rows(); ++i) P(i, i) = Fraction(1);

    parts << paraHtml(QStringLiteral("对 $A$ 作成对的初等行、列变换："), th, doc);

    QStringList steps;
    steps << augMatLtx(D, P);

    auto rowAnnot = [](int i, int j, const Fraction& c) {
        if (c.isOne()) return QStringLiteral("(%1)+(%2)").arg(i+1).arg(j+1);
        if (c == Fraction(-1)) return QStringLiteral("(%1)-(%2)").arg(i+1).arg(j+1);
        if (c.sign() < 0) return QStringLiteral("(%1)-(%2)\\cdot %3").arg(i+1).arg(j+1).arg(fracLtx(-c));
        return QStringLiteral("(%1)+(%2)\\cdot %3").arg(i+1).arg(j+1).arg(fracLtx(c));
    };

    for (std::size_t i = 0; i < static_cast<std::size_t>(curN_); ++i) {

        if (D(i, i).isZero()) {
            for (std::size_t j = i + 1; j < static_cast<std::size_t>(curN_); ++j) {
                if (!D(j, i).isZero()) {
                    QString rOp = QStringLiteral("row: ") + rowAnnot(static_cast<int>(i), static_cast<int>(j), Fraction(1));
                    QString cOp = QStringLiteral("col: ") + rowAnnot(static_cast<int>(i), static_cast<int>(j), Fraction(1));
                    D.addMulRow(i, j, Fraction(1));
                    steps << opArrow(rOp) + QStringLiteral(" ") + augMatLtx(D, P);
                    for (std::size_t r = 0; r < D.rows(); ++r) D(r, i) += D(r, j);
                    for (std::size_t r = 0; r < P.rows(); ++r) P(r, i) += P(r, j);
                    steps << opArrow(cOp) + QStringLiteral(" ") + augMatLtx(D, P);
                    break;
                }
            }
        }
        if (D(i, i).isZero()) continue;

        for (std::size_t j = i + 1; j < static_cast<std::size_t>(curN_); ++j) {
            if (D(j, i).isZero()) continue;
            Fraction factor = -D(j, i) / D(i, i);
            QString rOp = QStringLiteral("row: ") + rowAnnot(static_cast<int>(j), static_cast<int>(i), factor);
            QString cOp = QStringLiteral("col: ") + rowAnnot(static_cast<int>(j), static_cast<int>(i), factor);
            D.addMulRow(j, i, factor);
            steps << opArrow(rOp) + QStringLiteral(" ") + augMatLtx(D, P);
            for (std::size_t r = 0; r < D.rows(); ++r) D(r, j) += factor * D(r, i);
            for (std::size_t r = 0; r < P.rows(); ++r) P(r, j) += factor * P(r, i);
            steps << opArrow(cOp) + QStringLiteral(" ") + augMatLtx(D, P);
        }
    }

    for (std::size_t s = 0; s < steps.size(); ++s)
        parts << formulaHtml(steps[s], th, doc, 12);

    parts << paraHtml(QStringLiteral("因此"), th, doc);
    parts << formulaHtml(QStringLiteral("D = ") + matLtx(D) + QStringLiteral(", \\quad C = ") + matLtx(P) + QStringLiteral("."), th, doc);

    QString stdForm;
    first = true;
    for (std::size_t i = 0; i < D.rows(); ++i) {
        Fraction d = D(i, i);
        if (d.isZero()) continue;
        bool neg = d.sign() < 0;
        if (first) { if (neg) stdForm += QStringLiteral("-"); }
        else { stdForm += neg ? QStringLiteral(" - ") : QStringLiteral(" + "); }
        if (!d.abs().isOne()) stdForm += fracLtx(d.abs());
        stdForm += QStringLiteral("y_{%1}^{2}").arg(i + 1);
        first = false;
    }
    if (first) stdForm = QStringLiteral("0");
    parts << paraHtml(QStringLiteral("令 $X = CY$，得"), th, doc);
    parts << formulaHtml(QStringLiteral("g(%1) = %2.").arg(vars, stdForm), th, doc);

    QString subst;
    for (std::size_t i = 0; i < P.rows(); ++i) {
        QString line = QStringLiteral("x_{%1} = ").arg(i + 1);
        bool fst = true;
        for (std::size_t j = 0; j < P.cols(); ++j) {
            Fraction c = P(i, j);
            if (c.isZero()) continue;
            bool neg = c.sign() < 0; Fraction ac = c.abs();
            if (fst) { if (neg) line += QStringLiteral("-"); }
            else { line += neg ? QStringLiteral(" - ") : QStringLiteral(" + "); }
            if (!ac.isOne()) line += fracLtx(ac);
            line += QStringLiteral("y_{%1}").arg(j + 1);
            fst = false;
        }
        if (fst) line += QStringLiteral("0");
        line += QStringLiteral(" & ");
        if (i + 1 < P.rows()) line += QStringLiteral(" \\\\\\\\ ");
        subst += line;
    }
    parts << paraHtml(QStringLiteral("所作的非退化线性替换 $X = CY$ 详细写出来就是"), th, doc);
    parts << formulaHtml(QStringLiteral("\\left\\{\\begin{array}{ll} %1 \\end{array}\\right.").arg(subst), th, doc);

    resultBrowser_->setHtml(QStringLiteral("<div style=\"padding:4px; line-height:2.2;\">%1</div>").arg(parts.join(QString())));
}

void QuadFormPage::onDemo() {
    spinN_->setValue(3); onGenerate();

    cells_[0*3+0]->setText(QStringLiteral("0"));
    cells_[0*3+1]->setText(QStringLiteral("1/2"));
    cells_[0*3+2]->setText(QStringLiteral("1/2"));
    cells_[1*3+0]->setText(QStringLiteral("1/2"));
    cells_[1*3+1]->setText(QStringLiteral("0"));
    cells_[1*3+2]->setText(QStringLiteral("-3/2"));
    cells_[2*3+0]->setText(QStringLiteral("1/2"));
    cells_[2*3+1]->setText(QStringLiteral("-3/2"));
    cells_[2*3+2]->setText(QStringLiteral("0"));
    onSolve();
}

} 
