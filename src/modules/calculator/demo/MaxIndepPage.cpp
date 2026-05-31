#include "MaxIndepPage.h"
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
#include <vector>

using namespace algemate::math;
using AlgeMate::Calculator::Interactive::RenderTheme;

namespace AlgeMate::Calculator::Demo {

MaxIndepPage::MaxIndepPage(QWidget* parent)
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
    connect(backBtn, &QPushButton::clicked, this, &MaxIndepPage::backRequested);
    topLay->addWidget(backBtn);

    auto* titleLbl = new QLabel(QStringLiteral("求极大线性无关组"));
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

    pLay->addWidget(new QLabel(QStringLiteral("维数 n =")));
    spinN_ = new QSpinBox;
    spinN_->setRange(2, 10);
    spinN_->setValue(4);
    pLay->addWidget(spinN_);

    pLay->addSpacing(12);
    pLay->addWidget(new QLabel(QStringLiteral("向量个数 m =")));
    spinM_ = new QSpinBox;
    spinM_->setRange(1, 10);
    spinM_->setValue(4);
    pLay->addWidget(spinM_);

    pLay->addSpacing(12);
    auto* genBtn = new QPushButton(QStringLiteral("生成向量组"));
    genBtn->setCursor(Qt::PointingHandCursor);
    genBtn->setStyleSheet(QStringLiteral(
        "QPushButton { background:#4A90D9; color:white; border-radius:6px; "
        "padding:6px 14px; font-size:13px; font-weight:600; } "
        "QPushButton:hover { background:#5BA0E9; }"));
    connect(genBtn, &QPushButton::clicked, this, &MaxIndepPage::onGenerate);
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
    connect(solveBtn_, &QPushButton::clicked, this, &MaxIndepPage::onSolve);
    cLay->addWidget(solveBtn_);

    // -- demo button --
    auto* demoBtn = new QPushButton(QStringLiteral("演示例题"));
    demoBtn->setCursor(Qt::PointingHandCursor);
    demoBtn->setStyleSheet(QStringLiteral(
        "QPushButton { background:#2196F3; color:white; border-radius:6px; "
        "padding:8px 16px; font-size:14px; font-weight:600; } "
        "QPushButton:hover { background:#1976D2; }"));
    connect(demoBtn, &QPushButton::clicked, this, &MaxIndepPage::onDemo);
    cLay->addWidget(demoBtn);

    // -- result browser --
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

void MaxIndepPage::onGenerate()
{
    curN_ = spinN_->value();
    curM_ = spinM_->value();
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

    // Row 0: column headers α₁, α₂, ...
    for (int j = 0; j < curM_; ++j) {
        auto* hdr = new QLabel(
            QStringLiteral("α<sub>%1</sub>").arg(j + 1));
        hdr->setTextFormat(Qt::RichText);
        hdr->setAlignment(Qt::AlignCenter);
        hdr->setStyleSheet(QStringLiteral("font-size:13px; font-weight:600; padding:2px 8px;"));
        lay->addWidget(hdr, 0, j);
    }

    // Rows 1..n: input cells
    for (int i = 0; i < curN_; ++i) {
        for (int j = 0; j < curM_; ++j) {
            auto* edit = new QLineEdit;
            edit->setFixedWidth(56);
            edit->setAlignment(Qt::AlignCenter);
            edit->setPlaceholderText(QStringLiteral("0"));
            cells_[static_cast<std::size_t>(j * curN_ + i)] = edit;
            edit->installEventFilter(this);
            lay->addWidget(edit, i + 1, j);
        }
    }

    gridContainerLay_->addWidget(grid);
    solveBtn_->setEnabled(true);
    resultBrowser_->clear();

    if (!cells_.empty()) cells_[0]->setFocus();
}

bool MaxIndepPage::eventFilter(QObject* obj, QEvent* ev)
{
    if (ev->type() == QEvent::KeyPress) {
        auto* ke = static_cast<QKeyEvent*>(ev);
        auto* edit = qobject_cast<QLineEdit*>(obj);
        if (edit && !cells_.empty()) {
            auto it = std::find(cells_.begin(), cells_.end(), edit);
            if (it != cells_.end()) {
                int idx = static_cast<int>(std::distance(cells_.begin(), it));
                int col = idx / curN_;   // which vector
                int row = idx % curN_;   // which component

                auto focusCell = [&](int c, int r) {
                    if (c >= 0 && c < curM_ && r >= 0 && r < curN_) {
                        cells_[c * curN_ + r]->setFocus();
                        cells_[c * curN_ + r]->selectAll();
                    }
                };

                if (ke->key() == Qt::Key_Right
                    && edit->cursorPosition() == edit->text().length()) {
                    if (col + 1 < curM_) focusCell(col + 1, row);
                    else if (row + 1 < curN_) focusCell(0, row + 1);
                    return true;
                }
                if (ke->key() == Qt::Key_Left
                    && edit->cursorPosition() == 0) {
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
                if (ke->key() == Qt::Key_Return
                    || ke->key() == Qt::Key_Enter) {
                    if (idx + 1 < static_cast<int>(cells_.size()))
                        focusCell(col + ((row + 1) / curN_),
                                  (row + 1) % curN_);
                    else
                        onSolve();
                    return true;
                }
            }
        }
    }
    return QWidget::eventFilter(obj, ev);
}

void MaxIndepPage::onSolve()
{
    if (curN_ == 0 || curM_ == 0) return;

    // ---- 1. Parse input → Matrix<Fraction> A (n × m), columns = vectors ----
    Matrix<Fraction> A(static_cast<std::size_t>(curN_),
                       static_cast<std::size_t>(curM_));
    for (int j = 0; j < curM_; ++j) {
        for (int i = 0; i < curN_; ++i) {
            try {
                A(i, j) = parseFraction(cells_[static_cast<std::size_t>(
                    j * curN_ + i)]->text());
            } catch (...) {
                resultBrowser_->setHtml(QStringLiteral(
                    "<p style=\"color:#E74C3C; font-size:14px;\">"
                    "输入错误：α<sub>%1</sub>"
                    " 的第 %2 个分量无法解析。"
                    "请输入整数、分数 (3/4) "
                    "或小数。</p>")
                    .arg(j + 1).arg(i + 1));
                return;
            }
        }
    }

    // ---- 2. Row echelon form ----
    Matrix<Fraction> E = rowEchelon(A);

    // ---- 3. Rank and pivot columns from echelon ----
    std::size_t rk = 0;
    std::vector<std::size_t> pivotCols;
    for (std::size_t r = 0; r < E.rows(); ++r) {
        for (std::size_t c = 0; c < E.cols(); ++c) {
            if (!E(r, c).isZero()) {
                ++rk;
                pivotCols.push_back(c);
                break;
            }
        }
    }

    // ---- 4. Build result HTML (matches sample format) ----
    auto th = RenderTheme::forCurrent();
    auto* doc = resultBrowser_->document();
    doc->clear();

    QStringList parts;

    // Display vectors: α₁ = (col), α₂ = (col), ...
    {
        QString vecList;
        for (int j = 0; j < curM_; ++j) {
            if (j > 0) vecList += QStringLiteral(",\\ ");
            Matrix<Fraction> v(static_cast<std::size_t>(curN_), 1);
            for (int i = 0; i < curN_; ++i)
                v(i, 0) = A(i, j);
            vecList += QStringLiteral("\\alpha_{%1} = %2")
                .arg(j + 1).arg(matLtx(v));
        }
        parts << formulaHtml(vecList, th, doc, 16);
    }

    parts << titleHtml(QStringLiteral(
        "求这个向量组的秩和"
        "它的一个极大线性无关组."), th);

    parts << sectionHtml(QStringLiteral("解"), th);

    parts << paraHtml(QStringLiteral(
        "作初等行变换，"
        "把下述矩阵化成阶梯形矩阵:"), th);

    // Single step: A → row echelon form
    {
        QString chain = matLtx(A)
            + QStringLiteral("\\longrightarrow ")
            + matLtx(E)
            + QStringLiteral(".");
        parts << formulaHtml(chain, th, doc, 16);
    }

    // Conclusion with inline LaTeX
    {
        QString rankTerm;
        rankTerm += QStringLiteral("\\operatorname{rank}\\{");
        for (int j = 0; j < curM_; ++j) {
            if (j > 0) rankTerm += QStringLiteral(",");
            rankTerm += QStringLiteral("\\alpha_{%1}").arg(j + 1);
        }
        rankTerm += QStringLiteral("\\}=%1").arg(rk);

        QString indices;
        for (std::size_t k = 0; k < pivotCols.size(); ++k) {
            if (k > 0) indices += QStringLiteral(",");
            indices += QStringLiteral("\\alpha_{%1}").arg(pivotCols[k] + 1);
        }

        QString allVecs = QStringLiteral(
            "\\alpha_1,\\alpha_2,\\ldots,\\alpha_{%1}").arg(curM_);

        parts << paraHtml(QStringLiteral(
            "于是 $%1$，$%2$ 是向量组 "
            "$%3$ 的一个极大线性无关组.")
            .arg(rankTerm, indices, allVecs),
            th, doc);
    }

    resultBrowser_->setHtml(
        QStringLiteral("<div style=\"padding:4px; line-height:2.2;\">%1</div>")
        .arg(parts.join(QString())));
}

void MaxIndepPage::onDemo()
{
    spinN_->setValue(4);
    spinM_->setValue(4);
    onGenerate();

    // α₁ = (-1, 5, 3, -2)ᵀ  (column 0)
    // α₂ = (4, 1, -2, 9)ᵀ   (column 1)
    // α₃ = (2, 0, -1, 4)ᵀ   (column 2)
    // α₄ = (0, 3, 4, -5)ᵀ   (column 3)
    const std::vector<std::vector<int>> demo = {
        {-1, 5, 3, -2},   // α₁
        {4, 1, -2, 9},    // α₂
        {2, 0, -1, 4},    // α₃
        {0, 3, 4, -5},    // α₄
    };
    for (int j = 0; j < 4; ++j)
        for (int i = 0; i < 4; ++i)
            cells_[j * 4 + i]->setText(QString::number(demo[j][i]));

    onSolve();
}

} // namespace AlgeMate::Calculator::Demo
