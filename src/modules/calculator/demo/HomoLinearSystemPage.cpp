#include "HomoLinearSystemPage.h"
#include "DemoCommon.h"

#include "math/algorithm/LinearAlgebra.h"
#include "math/core/Fraction.h"
#include "math/core/Matrix.h"
#include "math/trace/Step.h"
#include "math/trace/StepSequence.h"
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
#include <vector>

using namespace algemate::math;
using AlgeMate::Calculator::Interactive::RenderTheme;

namespace AlgeMate::Calculator::Demo {

// =====================================================================
//  Constructor
// =====================================================================

HomoLinearSystemPage::HomoLinearSystemPage(QWidget* parent)
    : QWidget(parent)
{
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    // ---------- top bar ----------
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
    connect(backBtn, &QPushButton::clicked, this, &HomoLinearSystemPage::backRequested);
    topLay->addWidget(backBtn);

    auto* titleLbl = new QLabel(QStringLiteral("解齐次线性方程组"));
    titleLbl->setStyleSheet(QStringLiteral("font-size:18px; font-weight:700;"));
    topLay->addWidget(titleLbl);
    topLay->addStretch(1);

    root->addWidget(topBar);

    // ---------- scrollable content ----------
    auto* scroll = new QScrollArea;
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);

    auto* content = new QWidget;
    auto* cLay = new QVBoxLayout(content);
    cLay->setContentsMargins(24, 16, 24, 24);
    cLay->setSpacing(16);

    // -- parameter row --
    auto* paramW = new QWidget;
    auto* pLay = new QHBoxLayout(paramW);
    pLay->setContentsMargins(0, 0, 0, 0);
    pLay->setSpacing(10);

    pLay->addWidget(new QLabel(QStringLiteral("方程个数 m =")));
    spinM_ = new QSpinBox;
    spinM_->setRange(1, 10);
    spinM_->setValue(3);
    pLay->addWidget(spinM_);

    pLay->addSpacing(12);
    pLay->addWidget(new QLabel(QStringLiteral("未知数个数 n =")));
    spinN_ = new QSpinBox;
    spinN_->setRange(1, 10);
    spinN_->setValue(4);
    pLay->addWidget(spinN_);

    pLay->addSpacing(12);
    auto* genBtn = new QPushButton(QStringLiteral("生成方程组"));
    genBtn->setCursor(Qt::PointingHandCursor);
    genBtn->setStyleSheet(QStringLiteral(
        "QPushButton { background:#4A90D9; color:white; border-radius:6px; "
        "padding:6px 14px; font-size:13px; font-weight:600; } "
        "QPushButton:hover { background:#5BA0E9; }"));
    connect(genBtn, &QPushButton::clicked, this, &HomoLinearSystemPage::onGenerate);
    pLay->addWidget(genBtn);
    pLay->addStretch(1);

    cLay->addWidget(paramW);

    // -- grid container --
    gridContainer_ = new QWidget;
    gridContainerLay_ = new QVBoxLayout(gridContainer_);
    gridContainerLay_->setContentsMargins(0, 0, 0, 0);
    cLay->addWidget(gridContainer_);

    // -- solve button --
    solveBtn_ = new QPushButton(QStringLiteral("开始求解"));
    solveBtn_->setEnabled(false);
    solveBtn_->setCursor(Qt::PointingHandCursor);
    solveBtn_->setFixedWidth(160);
    solveBtn_->setStyleSheet(QStringLiteral(
        "QPushButton { background:#4A90D9; color:white; border-radius:6px; "
        "padding:8px 16px; font-size:14px; font-weight:600; } "
        "QPushButton:hover { background:#5BA0E9; } "
        "QPushButton:disabled { background:#555; color:#888; }"));
    connect(solveBtn_, &QPushButton::clicked, this, &HomoLinearSystemPage::onSolve);
    cLay->addWidget(solveBtn_);

    // -- demo button --
    auto* demoBtn = new QPushButton(QStringLiteral("演示例题"));
    demoBtn->setCursor(Qt::PointingHandCursor);
    demoBtn->setStyleSheet(QStringLiteral(
        "QPushButton { background:#2196F3; color:white; border-radius:6px; "
        "padding:8px 16px; font-size:14px; font-weight:600; } "
        "QPushButton:hover { background:#1976D2; }"));
    connect(demoBtn, &QPushButton::clicked, this, &HomoLinearSystemPage::onDemo);
    cLay->addWidget(demoBtn);

    // -- result browser --
    resultBrowser_ = new QTextBrowser;
    resultBrowser_->setOpenLinks(false);
    resultBrowser_->setMinimumHeight(400);
    resultBrowser_->setStyleSheet(QStringLiteral(
        "QTextBrowser { border:1px solid #3A3D4A; border-radius:8px; padding:12px; }"));
    cLay->addWidget(resultBrowser_, 1);

    scroll->setWidget(content);
    root->addWidget(scroll, 1);
}

// =====================================================================
//  Generate coefficient grid
// =====================================================================

void HomoLinearSystemPage::onGenerate()
{
    curM_ = spinM_->value();
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

    cells_.resize(static_cast<std::size_t>(curM_ * curN_));

    for (int i = 0; i < curM_; ++i) {
        int gc = 0;
        for (int j = 0; j < curN_; ++j) {
            if (j > 0) {
                auto* plus = new QLabel(QStringLiteral("+"));
                plus->setAlignment(Qt::AlignCenter);
                plus->setFixedWidth(14);
                lay->addWidget(plus, i, gc++);
            }
            auto* edit = new QLineEdit;
            edit->setFixedWidth(56);
            edit->setAlignment(Qt::AlignCenter);
            edit->setPlaceholderText(QStringLiteral("0"));
            cells_[static_cast<std::size_t>(i * curN_ + j)] = edit;
            edit->installEventFilter(this);
            lay->addWidget(edit, i, gc++);

            auto* var = new QLabel(
                QStringLiteral("x<sub>%1</sub>").arg(j + 1));
            var->setTextFormat(Qt::RichText);
            lay->addWidget(var, i, gc++);
        }
        auto* eq = new QLabel(QStringLiteral("= 0"));
        lay->addWidget(eq, i, gc);
    }

    gridContainerLay_->addWidget(grid);
    solveBtn_->setEnabled(true);
    resultBrowser_->clear();

    if (!cells_.empty()) cells_[0]->setFocus();
}

// =====================================================================
//  Arrow-key navigation between coefficient cells
// =====================================================================

bool HomoLinearSystemPage::eventFilter(QObject* obj, QEvent* ev)
{
    if (ev->type() == QEvent::KeyPress) {
        auto* ke = static_cast<QKeyEvent*>(ev);
        auto* edit = qobject_cast<QLineEdit*>(obj);
        if (edit && !cells_.empty()) {
            auto it = std::find(cells_.begin(), cells_.end(), edit);
            if (it != cells_.end()) {
                int idx = static_cast<int>(std::distance(cells_.begin(), it));
                int col = idx % curN_;
                int row = idx / curN_;

                auto focusCell = [&](int i) {
                    if (i >= 0 && i < static_cast<int>(cells_.size())) {
                        cells_[i]->setFocus();
                        cells_[i]->selectAll();
                    }
                };

                if (ke->key() == Qt::Key_Right
                    && edit->cursorPosition() == edit->text().length()) {
                    focusCell(idx + 1);
                    return true;
                }
                if (ke->key() == Qt::Key_Left
                    && edit->cursorPosition() == 0) {
                    focusCell(idx - 1);
                    return true;
                }
                if (ke->key() == Qt::Key_Down) {
                    focusCell((row + 1) * curN_ + col);
                    return true;
                }
                if (ke->key() == Qt::Key_Up) {
                    focusCell((row - 1) * curN_ + col);
                    return true;
                }
                if (ke->key() == Qt::Key_Return
                    || ke->key() == Qt::Key_Enter) {
                    if (idx + 1 < static_cast<int>(cells_.size()))
                        focusCell(idx + 1);
                    else
                        onSolve();
                    return true;
                }
            }
        }
    }
    return QWidget::eventFilter(obj, ev);
}

// =====================================================================
//  Solve & render
// =====================================================================

void HomoLinearSystemPage::onSolve()
{
    if (curM_ == 0 || curN_ == 0) return;

    // ---- 1. Parse input → Matrix<Fraction> A (m × n) ----
    Matrix<Fraction> A(static_cast<std::size_t>(curM_),
                       static_cast<std::size_t>(curN_));
    for (int i = 0; i < curM_; ++i) {
        for (int j = 0; j < curN_; ++j) {
            try {
                A(i, j) = parseFraction(cells_[static_cast<std::size_t>(
                    i * curN_ + j)]->text());
            } catch (...) {
                resultBrowser_->setHtml(QStringLiteral(
                    "<p style=\"color:#E74C3C; font-size:14px;\">"
                    "输入错误：第 %1 行第 %2 "
                    "列的系数无法解析。"
                    "请输入整数、分数 (3/4) "
                    "或小数。</p>")
                    .arg(i + 1).arg(j + 1));
                return;
            }
        }
    }

    // ---- 2. RREF with trace ----
    StepSequence trace;
    Matrix<Fraction> R = A;
    std::size_t rk = improvedRref(R, trace);

    // ---- 3. Nullspace ----
    Matrix<Fraction> N = nullspace(A);
    std::size_t nullDim = N.cols();

    // ---- 4. Pivot / free columns from RREF ----
    const std::size_t n = static_cast<std::size_t>(curN_);
    std::vector<std::size_t> pivotOfRow(R.rows(), n);
    std::vector<std::size_t> pivotCols;
    std::vector<bool> isPiv(n, false);
    for (std::size_t r = 0; r < R.rows(); ++r) {
        for (std::size_t c = 0; c < n; ++c) {
            if (!R(r, c).isZero()) {
                pivotOfRow[r] = c;
                pivotCols.push_back(c);
                isPiv[c] = true;
                break;
            }
        }
    }
    std::vector<std::size_t> freeCols;
    for (std::size_t c = 0; c < n; ++c)
        if (!isPiv[c]) freeCols.push_back(c);

    // ---- 5. Build result HTML ----
    auto th = RenderTheme::forCurrent();
    auto* doc = resultBrowser_->document();
    doc->clear();

    QStringList parts;
    parts << titleHtml(QStringLiteral(
        "求下述数域 K 上齐次线性"
        "方程组的一个基础解系，"
        "并且写出它的解集。"), th);

    parts << formulaHtml(eqnSysLtx(A), th, doc);

    parts << sectionHtml(QStringLiteral("解"), th);

    // RREF chain: group ≤ 3 matrices per line
    auto ms = milestones(trace);
    if (!ms.empty()) {
        const std::size_t perLine = (ms.size() <= 4) ? ms.size() : 3;
        for (std::size_t start = 0; start < ms.size(); ) {
            std::size_t end = std::min(start + perLine, ms.size());
            QString line;
            for (std::size_t i = start; i < end; ++i) {
                if (i > start) line += QStringLiteral("\\to ");
                line += matLtx(ms[i]);
            }
            if (end == ms.size()) line += QStringLiteral(".");
            int fpt = (ms.size() <= 3 && n <= 5) ? 16 : 14;
            parts << formulaHtml(line, th, doc, fpt);
            start = end;
        }
    }

    // Trivial-solution shortcut
    if (nullDim == 0) {
        parts << paraHtml(QStringLiteral(
            "方程组的秩等于未知量"
            "个数，因此方程组仅有"
            "零解。"), th);
        parts << formulaHtml(QStringLiteral("W = \\left\\{ \\mathbf{0} \\right\\}"), th, doc);
        resultBrowser_->setHtml(
            QStringLiteral("<div style=\"padding:4px; line-height:2.2;\">%1</div>")
            .arg(parts.join(QString())));
        return;
    }

    // General solution
    parts << paraHtml(QStringLiteral("于是方程组的一般解为"), th);

    {
        QString body;
        int written = 0;
        for (std::size_t ri = 0; ri < R.rows(); ++ri) {
            if (pivotOfRow[ri] >= n) continue;
            std::size_t pc = pivotOfRow[ri];
            QString row = QStringLiteral("x_{%1} = &").arg(pc + 1);
            bool first = true;
            for (auto fc : freeCols) {
                Fraction coeff = -R(ri, fc);
                if (coeff.isZero()) continue;
                bool neg = coeff.sign() < 0;
                Fraction ac = coeff.abs();
                if (first) {
                    if (neg) row += QStringLiteral("-");
                } else {
                    row += neg ? QStringLiteral(" - ") : QStringLiteral(" + ");
                }
                if (!ac.isOne())
                    row += QString::fromStdString(ac.toLatex());
                row += QStringLiteral("x_{%1}").arg(fc + 1);
                first = false;
            }
            if (first) row += QStringLiteral("0");

            ++written;
            if (written < static_cast<int>(pivotCols.size()))
                row += QStringLiteral(", \\\\ ");
            else
                row += QStringLiteral(",");
            body += row;
        }
        QString genSol = QStringLiteral(
            "\\left\\{\\begin{array}{rl}%1\\end{array}\\right.").arg(body);
        parts << formulaHtml(genSol, th, doc);
    }

    // Free-variable note + basis header
    {
        QString freeList;
        for (std::size_t i = 0; i < freeCols.size(); ++i) {
            if (i > 0) freeList += QStringLiteral(", ");
            freeList += QStringLiteral("x<sub>%1</sub>").arg(freeCols[i] + 1);
        }
        parts << paraHtml(QStringLiteral(
            "其中 %1 是自由未知量。"
            "因此方程组的一个基础"
            "解系为").arg(freeList), th);
    }

    // Basis vectors
    {
        QString basisLtx;
        for (std::size_t k = 0; k < nullDim; ++k) {
            Matrix<Fraction> v(n, 1);
            for (std::size_t r = 0; r < n; ++r) v(r, 0) = N(r, k);
            v = scaleInt(v);
            if (k > 0) basisLtx += QStringLiteral(", \\quad ");
            basisLtx += QStringLiteral("\\eta_{%1} = ").arg(k + 1);
            basisLtx += matLtx(v);
        }
        basisLtx += QStringLiteral(".");
        parts << formulaHtml(basisLtx, th, doc);
    }

    // Solution set W
    parts << paraHtml(QStringLiteral(
        "从而方程组的解集 "
        "<i>W</i> 为"), th);

    {
        QString setLtx = QStringLiteral("W = \\left\\{ ");
        for (std::size_t k = 0; k < nullDim; ++k) {
            if (k > 0) setLtx += QStringLiteral(" + ");
            setLtx += QStringLiteral("k_{%1}\\eta_{%1}").arg(k + 1);
        }
        setLtx += QStringLiteral(" \\mid ");
        for (std::size_t k = 0; k < nullDim; ++k) {
            if (k > 0) setLtx += QStringLiteral(", ");
            setLtx += QStringLiteral("k_{%1}").arg(k + 1);
        }
        setLtx += QStringLiteral(" \\in K \\right\\}");
        parts << formulaHtml(setLtx, th, doc);
    }

    resultBrowser_->setHtml(
        QStringLiteral("<div style=\"padding:4px; line-height:2.2;\">%1</div>")
        .arg(parts.join(QString())));
}

// =====================================================================
//  Demo button handler
// =====================================================================

void HomoLinearSystemPage::onDemo()
{
    spinM_->setValue(3);
    spinN_->setValue(4);
    onGenerate();

    const std::vector<std::vector<int>> demoA = {
        {1, -3, 5, -2},
        {-2, 1, -3, 1},
        {-1, -7, 9, -4}
    };
    for (int i = 0; i < 3; ++i)
        for (int j = 0; j < 4; ++j)
            cells_[i * 4 + j]->setText(QString::number(demoA[i][j]));

    onSolve();
}

} // namespace AlgeMate::Calculator::Demo
