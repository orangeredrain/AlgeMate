#include "InversePage.h"
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

static Matrix<Fraction> gaussJordanInverse(const Matrix<Fraction>& A, StepSequence& trace)
{
    const auto n = A.rows();

    Matrix<Fraction> M(n, 2 * n);
    for (std::size_t i = 0; i < n; ++i) {
        for (std::size_t j = 0; j < n; ++j)
            M(i, j) = A(i, j);
        M(i, n + i) = Fraction(1);
    }

    trace.pushInitial(M);

    for (std::size_t c = 0; c < n; ++c) {

        bool hasOne = false;
        for (std::size_t i = c; i < n && !hasOne; ++i)
            if (M(i, c).abs().isOne()) hasOne = true;

        if (!hasOne) {
            for (std::size_t i = c; i < n; ++i) {
                if (M(i, c).isZero()) continue;
                for (std::size_t j = c; j < n; ++j) {
                    if (i == j || M(j, c).isZero()) continue;
                    Fraction diff = M(i, c) - M(j, c);
                    if (diff.abs().isOne()) {
                        M.addMulRow(i, j, Fraction(-1));
                        break;
                    }
                }
            }
        }

        std::size_t best = c;
        while (best < n && M(best, c).isZero()) ++best;
        if (best == n) continue;

        Fraction bestVal = M(best, c).abs();
        for (std::size_t i = best + 1; i < n; ++i) {
            if (M(i, c).isZero()) continue;
            Fraction absVal = M(i, c).abs();
            if (absVal.isOne()) { best = i; break; }
            if (absVal < bestVal && !bestVal.isOne()) { best = i; bestVal = absVal; }
        }

        if (best != c) M.swapRows(c, best);

        Fraction piv = M(c, c);
        if (!piv.isOne()) {
            Fraction inv = Fraction(1) / piv;
            M.scaleRow(c, inv);
            trace.pushScaleRow(c, inv, M);
        }

        trace.pushSelectPivot(c, c, M);

        for (std::size_t r = 0; r < n; ++r) {
            if (r == c || M(r, c).isZero()) continue;
            Fraction factor = -M(r, c);
            M.addMulRow(r, c, factor);
            trace.pushAddMulRow(r, c, factor, M);
        }
    }

    trace.pushConclude("Gauss-Jordan complete", M);

    Matrix<Fraction> inv(n, n);
    for (std::size_t i = 0; i < n; ++i)
        for (std::size_t j = 0; j < n; ++j)
            inv(i, j) = M(i, n + j);
    return inv;
}

static QString augMatLtx(const Matrix<Fraction>& M, std::size_t n)
{
    if (M.rows() == 0) return QStringLiteral("()");
    const auto R = M.rows();

    QString body;
    for (std::size_t i = 0; i < R; ++i) {
        for (std::size_t j = 0; j < n; ++j) {
            if (j) body += QStringLiteral(" & ");
            body += fracLtx(M(i, j));
        }
        body += QStringLiteral(" & \\vline & ");
        for (std::size_t j = n; j < 2 * n; ++j) {
            if (j > n) body += QStringLiteral(" & ");
            body += fracLtx(M(i, j));
        }
        if (i + 1 < R) body += QStringLiteral(" \\\\\\\\ ");
    }

    QString colSpec;
    for (std::size_t j = 0; j < n; ++j) colSpec += QLatin1Char('c');
    colSpec += QStringLiteral("|");
    for (std::size_t j = 0; j < n; ++j) colSpec += QLatin1Char('c');
    return QStringLiteral("\\left(\\begin{array}{%1}%2\\end{array}\\right)")
        .arg(colSpec, body);
}

InversePage::InversePage(QWidget* parent)
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
    connect(backBtn, &QPushButton::clicked, this, &InversePage::backRequested);
    topLay->addWidget(backBtn);

    auto* titleLbl = new QLabel(QStringLiteral("矩阵求逆"));
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
    connect(genBtn, &QPushButton::clicked, this, &InversePage::onGenerate);
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
    connect(solveBtn_, &QPushButton::clicked, this, &InversePage::onSolve);
    cLay->addWidget(solveBtn_);

    auto* demoBtn = new QPushButton(QStringLiteral("演示例题"));
    demoBtn->setCursor(Qt::PointingHandCursor);
    demoBtn->setStyleSheet(QStringLiteral(
        "QPushButton { background:#2196F3; color:white; border-radius:6px; "
        "padding:8px 16px; font-size:14px; font-weight:600; } "
        "QPushButton:hover { background:#1976D2; }"));
    connect(demoBtn, &QPushButton::clicked, this, &InversePage::onDemo);
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

void InversePage::onGenerate()
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

bool InversePage::eventFilter(QObject* obj, QEvent* ev)
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

void InversePage::onSolve()
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

    Fraction detA = det(A);

    auto th = RenderTheme::forCurrent();
    auto* doc = resultBrowser_->document();
    doc->clear();

    if (detA.isZero()) {
        QStringList parts;
        parts << titleHtml(QStringLiteral("求下述矩阵的逆矩阵："), th);
        parts << formulaHtml(QStringLiteral("A = ") + matLtx(A), th, doc);
        parts << paraHtml(QStringLiteral(
            "因为 $|A| = 0$，所以 $A$ 不可逆."), th, doc);
        resultBrowser_->setHtml(
            QStringLiteral("<div style=\"padding:4px; line-height:2.2;\">%1</div>")
            .arg(parts.join(QString())));
        return;
    }

    StepSequence trace;
    Matrix<Fraction> inv = gaussJordanInverse(A, trace);

    QStringList parts;

    parts << titleHtml(QStringLiteral("求下述矩阵的逆矩阵："), th);
    parts << formulaHtml(QStringLiteral("A = ") + matLtx(A), th, doc);

    parts << sectionHtml(QStringLiteral("解"), th);

    parts << paraHtml(QStringLiteral(
        "因为 $|A| = %1 \\neq 0$，所以 $A$ 可逆."
        " 作初等行变换，把增广矩阵 $(A,\\,I)$"
        " 化成 $(I,\\,A^{-1})$：").arg(fracLtx(detA)), th, doc);

    const auto& steps = trace.steps();
    {
        std::vector<Matrix<Fraction>> snapshots;
        snapshots.push_back(steps[0].snapshot);  
        for (std::size_t i = 1; i < steps.size(); ++i) {
            if (steps[i].kind == StepKind::SelectPivot)
                snapshots.push_back(steps[i].snapshot);
        }
        auto& last = steps.back();
        if (last.kind == StepKind::Conclude)
            snapshots.push_back(last.snapshot);

        for (std::size_t i = 0; i < snapshots.size(); ++i) {
            QString line;
            if (i > 0) line += QStringLiteral("\\longrightarrow ");
            line += augMatLtx(snapshots[i], static_cast<std::size_t>(curN_));
            if (i + 1 == snapshots.size()) line += QStringLiteral(".");
            parts << formulaHtml(line, th, doc, 13);
        }
    }

    parts << paraHtml(QStringLiteral("因此"), th);
    parts << formulaHtml(QStringLiteral("A^{-1} = ") + matLtx(inv), th, doc);

    resultBrowser_->setHtml(
        QStringLiteral("<div style=\"padding:4px; line-height:2.2;\">%1</div>")
        .arg(parts.join(QString())));
}

void InversePage::onDemo()
{
    spinN_->setValue(3);
    onGenerate();

    const std::vector<std::vector<int>> demo = {
        {4, 1, 2},
        {3, 2, 1},
        {5, -3, 2}
    };
    for (int i = 0; i < 3; ++i)
        for (int j = 0; j < 3; ++j)
            cells_[i * 3 + j]->setText(QString::number(demo[i][j]));

    onSolve();
}

} 
