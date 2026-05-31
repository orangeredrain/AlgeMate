#include "GSOPage.h"
#include "DemoCommon.h"

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

// ---- helpers ----

// 缩放列向量: v * k
static Matrix<Fraction> scaleVec(const Matrix<Fraction>& v, const Fraction& k) {
    Matrix<Fraction> r(v.rows(), 1);
    for (std::size_t i = 0; i < v.rows(); ++i)
        r(i, 0) = v(i, 0) * k;
    return r;
}

// 列向量加法: a + b
static Matrix<Fraction> addVec(const Matrix<Fraction>& a, const Matrix<Fraction>& b) {
    Matrix<Fraction> r(a.rows(), 1);
    for (std::size_t i = 0; i < a.rows(); ++i)
        r(i, 0) = a(i, 0) + b(i, 0);
    return r;
}

// 找出线性无关向量索引 (0-based)
static std::vector<std::size_t> indepIndices(const Matrix<Fraction>& A) {
    Matrix<Fraction> E = rowEchelon(A);
    std::vector<std::size_t> piv;
    for (std::size_t r = 0; r < E.rows(); ++r) {
        for (std::size_t c = 0; c < E.cols(); ++c) {
            if (!E(r, c).isZero()) { piv.push_back(c); break; }
        }
    }
    return piv;
}

// =====================================================================
//  Constructor
// =====================================================================

GSOPage::GSOPage(QWidget* parent) : QWidget(parent) {
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
    connect(backBtn, &QPushButton::clicked, this, &GSOPage::backRequested);
    topLay->addWidget(backBtn);

    auto* titleLbl = new QLabel(QStringLiteral("Schmidt 正交化"));
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

    pLay->addWidget(new QLabel(QStringLiteral("维数 n =")));
    spinN_ = new QSpinBox;
    spinN_->setRange(2, 10);
    spinN_->setValue(3);
    pLay->addWidget(spinN_);

    pLay->addSpacing(12);
    pLay->addWidget(new QLabel(QStringLiteral("向量个数 m =")));
    spinM_ = new QSpinBox;
    spinM_->setRange(1, 10);
    spinM_->setValue(2);
    pLay->addWidget(spinM_);

    pLay->addSpacing(12);
    auto* genBtn = new QPushButton(QStringLiteral("生成向量组"));
    genBtn->setCursor(Qt::PointingHandCursor);
    genBtn->setStyleSheet(QStringLiteral(
        "QPushButton { background:#4A90D9; color:white; border-radius:6px; "
        "padding:6px 14px; font-size:13px; font-weight:600; } "
        "QPushButton:hover { background:#5BA0E9; }"));
    connect(genBtn, &QPushButton::clicked, this, &GSOPage::onGenerate);
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
    connect(solveBtn_, &QPushButton::clicked, this, &GSOPage::onSolve);
    cLay->addWidget(solveBtn_);

    auto* demoBtn = new QPushButton(QStringLiteral("演示例题"));
    demoBtn->setCursor(Qt::PointingHandCursor);
    demoBtn->setStyleSheet(QStringLiteral(
        "QPushButton { background:#2196F3; color:white; border-radius:6px; "
        "padding:8px 16px; font-size:14px; font-weight:600; } "
        "QPushButton:hover { background:#1976D2; }"));
    connect(demoBtn, &QPushButton::clicked, this, &GSOPage::onDemo);
    cLay->addWidget(demoBtn);

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
//  Generate / Event filter (same pattern as MaxIndepPage)
// =====================================================================

void GSOPage::onGenerate() {
    curN_ = spinN_->value();
    curM_ = spinM_->value();
    cells_.clear();

    if (auto* old = gridContainer_->findChild<QWidget*>(QStringLiteral("inputGrid")))
        delete old;

    auto* grid = new QWidget(gridContainer_);
    grid->setObjectName(QStringLiteral("inputGrid"));
    auto* lay = new QGridLayout(grid);
    lay->setSpacing(4);
    lay->setContentsMargins(0, 0, 0, 0);

    cells_.resize(static_cast<std::size_t>(curM_ * curN_));

    // Row 0: column headers
    for (int j = 0; j < curM_; ++j) {
        auto* hdr = new QLabel(QStringLiteral("α<sub>%1</sub>").arg(j + 1));
        hdr->setTextFormat(Qt::RichText);
        hdr->setAlignment(Qt::AlignCenter);
        hdr->setStyleSheet(QStringLiteral("font-size:13px; font-weight:600; padding:2px 8px;"));
        lay->addWidget(hdr, 0, j);
    }

    for (int i = 0; i < curN_; ++i)
        for (int j = 0; j < curM_; ++j) {
            auto* edit = new QLineEdit;
            edit->setFixedWidth(56);
            edit->setAlignment(Qt::AlignCenter);
            edit->setPlaceholderText(QStringLiteral("0"));
            cells_[static_cast<std::size_t>(j * curN_ + i)] = edit;
            edit->installEventFilter(this);
            lay->addWidget(edit, i + 1, j);
        }

    gridContainerLay_->addWidget(grid);
    solveBtn_->setEnabled(true);
    resultBrowser_->clear();
    if (!cells_.empty()) cells_[0]->setFocus();
}

bool GSOPage::eventFilter(QObject* obj, QEvent* ev) {
    if (ev->type() == QEvent::KeyPress) {
        auto* ke = static_cast<QKeyEvent*>(ev);
        auto* edit = qobject_cast<QLineEdit*>(obj);
        if (edit && !cells_.empty()) {
            auto it = std::find(cells_.begin(), cells_.end(), edit);
            if (it != cells_.end()) {
                int idx = static_cast<int>(std::distance(cells_.begin(), it));
                int col = idx / curN_, row = idx % curN_;

                auto focusCell = [&](int c, int r) {
                    if (c >= 0 && c < curM_ && r >= 0 && r < curN_) {
                        cells_[c * curN_ + r]->setFocus();
                        cells_[c * curN_ + r]->selectAll();
                    }
                };

                if (ke->key() == Qt::Key_Right && edit->cursorPosition() == edit->text().length()) {
                    if (col + 1 < curM_) focusCell(col + 1, row);
                    else if (row + 1 < curN_) focusCell(0, row + 1);
                    return true;
                }
                if (ke->key() == Qt::Key_Left && edit->cursorPosition() == 0) {
                    if (col > 0) focusCell(col - 1, row);
                    else if (row > 0) focusCell(curM_ - 1, row - 1);
                    return true;
                }
                if (ke->key() == Qt::Key_Down) {
                    if (row + 1 < curN_) focusCell(col, row + 1);
                    else if (col + 1 < curM_) focusCell(col + 1, 0);
                    return true;
                }
                if (ke->key() == Qt::Key_Up) {
                    if (row > 0) focusCell(col, row - 1);
                    else if (col > 0) focusCell(col - 1, curN_ - 1);
                    return true;
                }
                if (ke->key() == Qt::Key_Return || ke->key() == Qt::Key_Enter) {
                    if (idx + 1 < static_cast<int>(cells_.size()))
                        focusCell(col + ((row + 1) / curN_), (row + 1) % curN_);
                    else onSolve();
                    return true;
                }
            }
        }
    }
    return QWidget::eventFilter(obj, ev);
}

// =====================================================================
//  Solve
// =====================================================================

void GSOPage::onSolve() {
    if (curN_ == 0 || curM_ == 0) return;

    // ---- 1. Parse input ----
    Matrix<Fraction> A(static_cast<std::size_t>(curN_),
                       static_cast<std::size_t>(curM_));
    for (int j = 0; j < curM_; ++j)
        for (int i = 0; i < curN_; ++i) {
            try {
                A(i, j) = parseFraction(cells_[static_cast<std::size_t>(j * curN_ + i)]->text());
            } catch (...) {
                resultBrowser_->setHtml(QStringLiteral(
                    "<p style=\"color:#E74C3C; font-size:14px;\">"
                    "输入错误：α<sub>%1</sub> 的第 %2 个分量无法解析。</p>")
                    .arg(j + 1).arg(i + 1));
                return;
            }
        }

    // ---- 2. Find maximal independent subset ----
    auto piv = indepIndices(A);
    std::size_t rk = piv.size();

    // Collect independent vectors
    std::vector<Matrix<Fraction>> alphas;
    std::vector<int> origIdx; // original indices (1-based)
    for (auto c : piv) {
        Matrix<Fraction> v(static_cast<std::size_t>(curN_), 1);
        for (int i = 0; i < curN_; ++i) v(i, 0) = A(i, c);
        alphas.push_back(v);
        origIdx.push_back(static_cast<int>(c) + 1);
    }

    auto th = RenderTheme::forCurrent();
    auto* doc = resultBrowser_->document();
    doc->clear();

    QStringList parts;

    // ---- Display vectors ----
    {
        QString vecList;
        for (int j = 0; j < curM_; ++j) {
            if (j > 0) vecList += QStringLiteral(",\\quad ");
            Matrix<Fraction> v(static_cast<std::size_t>(curN_), 1);
            for (int i = 0; i < curN_; ++i) v(i, 0) = A(i, j);
            vecList += QStringLiteral("\\alpha_{%1} = %2").arg(j + 1).arg(matLtx(v));
        }
        parts << formulaHtml(vecList, th, doc, 14);
    }

    parts << titleHtml(QStringLiteral(
        "求与上述向量组等价的正交单位向量组."), th);

    parts << sectionHtml(QStringLiteral("解"), th);

    // State which vectors are independent / dependent
    if (rk < static_cast<std::size_t>(curM_)) {
        QString indepList;
        for (std::size_t k = 0; k < piv.size(); ++k) {
            if (k > 0) indepList += QStringLiteral(",");
            indepList += QStringLiteral("\\alpha_{%1}").arg(piv[k] + 1);
        }
        parts << paraHtml(QStringLiteral(
            "先求向量组的极大线性无关组."
            " 作初等行变换可知 $\\operatorname{rank}=%1$，"
            "$%2$ 是一个极大线性无关组，其余向量可由它线性表出."
            " 因此只需对 $%2$ 正交单位化.").arg(rk).arg(indepList), th, doc);
    }

    // ---- 3. Gram-Schmidt orthogonalization ----
    parts << paraHtml(QStringLiteral("首先正交化，令"), th);

    std::vector<Matrix<Fraction>> betas(rk);
    for (std::size_t i = 0; i < rk; ++i) {
        betas[i] = alphas[i];
        QString betaName = QStringLiteral("\\beta_{%1}").arg(i + 1);

        if (i == 0) {
            // β₁ = α₁
            parts << formulaHtml(betaName + QStringLiteral(" = \\alpha_{%1},")
                .arg(origIdx[i]), th, doc);
        } else {
            // β_i = α_i - Σ (α_i,β_j)/(β_j,β_j) · β_j
            QString rhs = QStringLiteral("\\alpha_{%1}").arg(origIdx[i]);
            for (std::size_t j = 0; j < i; ++j) {
                Fraction num = dotProd(alphas[i], betas[j]);
                Fraction den = dotProd(betas[j], betas[j]);
                rhs += QStringLiteral(" - \\frac{%1}{%2}\\beta_{%3}")
                    .arg(fracLtx(num), fracLtx(den)).arg(j + 1);
                betas[i] = addVec(betas[i], scaleVec(betas[j], -num / den));
            }

            // Show formula with numbers
            QString detail = betaName + QStringLiteral(" = ") + rhs;

            // Also show the numeric substitution
            if (i == 1 && rk == 2) {
                // Full detail for 2-vector case (most common)
                Fraction num = dotProd(alphas[1], betas[0]);
                Fraction den = dotProd(betas[0], betas[0]);
                detail += QStringLiteral(" = %1 - \\frac{%2}{%3}%4")
                    .arg(matLtx(alphas[1]), fracLtx(num), fracLtx(den), matLtx(betas[0]));
                detail += QStringLiteral(" = %1.").arg(matLtx(betas[i]));
            } else {
                detail += QStringLiteral(" = %1.").arg(matLtx(betas[i]));
            }

            parts << formulaHtml(detail, th, doc, 14);
        }
    }

    // ---- 4. Normalization ----
    parts << paraHtml(QStringLiteral("然后单位化，令"), th);

    for (std::size_t i = 0; i < rk; ++i) {
        Fraction ns = normSq(betas[i]);
        QString etaName = QStringLiteral("\\eta_{%1}").arg(i + 1);
        QString normStr = QStringLiteral("\\sqrt{%1}").arg(fracLtx(ns));

        // η_i = (1/|β_i|) β_i = (1/√ns) · β_i = (simplified)
        QString detail = etaName + QStringLiteral(" = \\frac{1}{|\\beta_{%1}|}\\beta_{%1}")
            .arg(i + 1);
        detail += QStringLiteral(" = \\frac{1}{%1}%2")
            .arg(normStr, matLtx(betas[i]));
        detail += QStringLiteral(" = %1.").arg(normVecLtx(betas[i], ns));

        parts << formulaHtml(detail, th, doc, 14);
    }

    // Final conclusion
    {
        QString etaList;
        for (std::size_t i = 0; i < rk; ++i) {
            if (i > 0) etaList += QStringLiteral(",");
            etaList += QStringLiteral("\\eta_{%1}").arg(i + 1);
        }
        QString alphaList;
        for (int j = 0; j < curM_; ++j) {
            if (j > 0) alphaList += QStringLiteral(",");
            alphaList += QStringLiteral("\\alpha_{%1}").arg(j + 1);
        }
        parts << paraHtml(QStringLiteral(
            "则 $%1$ 是与 $%2$ 等价的正交单位向量组.")
            .arg(etaList, alphaList), th, doc);
    }

    resultBrowser_->setHtml(
        QStringLiteral("<div style=\"padding:4px; line-height:2.2;\">%1</div>")
        .arg(parts.join(QString())));
}

// =====================================================================
//  Demo
// =====================================================================

void GSOPage::onDemo() {
    spinN_->setValue(3);
    spinM_->setValue(2);
    onGenerate();

    // α₁ = (2, -1, 0)ᵀ, α₂ = (2, 0, 1)ᵀ
    cells_[0]->setText(QStringLiteral("2"));   // α₁[0]
    cells_[1]->setText(QStringLiteral("-1"));  // α₁[1]
    cells_[2]->setText(QStringLiteral("0"));   // α₁[2]
    cells_[3]->setText(QStringLiteral("2"));   // α₂[0]
    cells_[4]->setText(QStringLiteral("0"));   // α₂[1]
    cells_[5]->setText(QStringLiteral("1"));   // α₂[2]

    onSolve();
}

} // namespace AlgeMate::Calculator::Demo
