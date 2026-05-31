#include "SymDiagPage.h"
#include "DemoCommon.h"

#include "math/algorithm/LinearAlgebra.h"
#include "math/algorithm/PolynomialAlg.h"
#include "math/core/Fraction.h"
#include "math/core/Matrix.h"
#include "math/core/Polynomial.h"
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
#include <complex>
#include <cstddef>
#include <vector>

using namespace algemate::math;
using AlgeMate::Calculator::Interactive::RenderTheme;

namespace AlgeMate::Calculator::Demo {

// ---- helpers ----

// λI - A determinant (vmatrix)
static QString detLtx(const Matrix<Fraction>& A) {
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

// Numerical nullspace: Gaussian elimination on double matrix with relative threshold
static std::vector<std::vector<double>> numNullspace(const Matrix<Fraction>& A, double lambda) {
    auto n = A.rows();
    std::vector<std::vector<double>> M(n, std::vector<double>(n));
    double maxAbs = 0.0;
    for (std::size_t i = 0; i < n; ++i) {
        for (std::size_t j = 0; j < n; ++j) {
            M[i][j] = -A(i, j).toDouble();
            maxAbs = std::max(maxAbs, std::abs(M[i][j]));
        }
        M[i][i] += lambda;
        maxAbs = std::max(maxAbs, std::abs(M[i][i]));
    }
    double thresh = std::max(1e-12, maxAbs * 1e-10);
    std::size_t r = 0;
    std::vector<std::size_t> pivots;
    std::vector<bool> isPiv(n, false);
    for (std::size_t c = 0; c < n && r < n; ++c) {
        std::size_t best = r;
        for (std::size_t i = r + 1; i < n; ++i)
            if (std::abs(M[i][c]) > std::abs(M[best][c])) best = i;
        if (std::abs(M[best][c]) < thresh) continue;
        if (best != r) std::swap(M[r], M[best]);
        double piv = M[r][c];
        for (std::size_t j = c; j < n; ++j) M[r][j] /= piv;
        for (std::size_t i = 0; i < n; ++i) {
            if (i == r) continue;
            double f = M[i][c];
            if (std::abs(f) < 1e-14) continue;
            for (std::size_t j = c; j < n; ++j) M[i][j] -= f * M[r][j];
        }
        pivots.push_back(c); isPiv[c] = true; ++r;
    }
    std::vector<std::size_t> freeCols;
    for (std::size_t c = 0; c < n; ++c) if (!isPiv[c]) freeCols.push_back(c);
    std::vector<std::vector<double>> result;
    for (auto fc : freeCols) {
        std::vector<double> v(n, 0.0); v[fc] = 1.0;
        for (std::size_t ri = 0; ri < pivots.size(); ++ri)
            v[pivots[ri]] = -M[ri][fc];
        double norm = 0.0;
        for (double x : v) norm += x * x;
        if (norm > 1e-15) { norm = std::sqrt(norm); for (auto& x : v) x /= norm; }
        result.push_back(v);
    }
    return result;
}

// Format a column vector as decimal LaTeX (no normalization, ghost column)
static QString decColLtx(const Matrix<Fraction>& v) {
    QString body;
    for (std::size_t i = 0; i < v.rows(); ++i) {
        if (i) body += QStringLiteral(" \\\\\\\\ ");
        double val = v(i, 0).toDouble();
        if (std::abs(val) < 1e-10) val = 0.0;
        QString s = QString::number(val, 'f', 4);
        while (s.endsWith('0')) s.chop(1);
        if (s.endsWith('.')) s.chop(1);
        body += s + QStringLiteral(" & ");
    }
    return QStringLiteral("\\left(\\begin{array}{cr}%1\\end{array}\\right)").arg(body);
}

// Format a column vector as decimal LaTeX (normalized by √ns, ghost column)
static QString decVecLtx(const Matrix<Fraction>& v, const Fraction& ns) {
    QString body;
    for (std::size_t i = 0; i < v.rows(); ++i) {
        if (i) body += QStringLiteral(" \\\\\\\\ ");
        double val = v(i, 0).toDouble() / std::sqrt(ns.toDouble());
        if (std::abs(val) < 1e-10) val = 0.0;
        QString s = QString::number(val, 'f', 4);
        while (s.endsWith('0')) s.chop(1);
        if (s.endsWith('.')) s.chop(1);
        body += s + QStringLiteral(" & ");
    }
    return QStringLiteral("\\left(\\begin{array}{cr}%1\\end{array}\\right)").arg(body);
}

// Gram-Schmidt on a set of column vectors (in-place, returns orthogonalized)
static std::vector<Matrix<Fraction>> gramSchmidt(std::vector<Matrix<Fraction>> vecs) {
    std::vector<Matrix<Fraction>> result;
    for (auto& v : vecs) {
        for (const auto& q : result) {
            Fraction num = dotProd(v, q);
            Fraction den = dotProd(q, q);
            for (std::size_t i = 0; i < v.rows(); ++i)
                v(i, 0) -= num * q(i, 0) / den;
        }
        if (!normSq(v).isZero()) result.push_back(v);
    }
    return result;
}

// =====================================================================
//  Constructor
// =====================================================================

SymDiagPage::SymDiagPage(QWidget* parent) : QWidget(parent) {
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0); root->setSpacing(0);

    auto* topBar = new QWidget; topBar->setFixedHeight(48);
    auto* topLay = new QHBoxLayout(topBar);
    topLay->setContentsMargins(12, 0, 12, 0);
    auto* backBtn = new QPushButton(QStringLiteral("← 返回"));
    backBtn->setFlat(true); backBtn->setCursor(Qt::PointingHandCursor);
    backBtn->setStyleSheet(QStringLiteral("QPushButton { font-size:14px; color:#8A8FA3; } QPushButton:hover { color:#C0C4D6; }"));
    connect(backBtn, &QPushButton::clicked, this, &SymDiagPage::backRequested);
    topLay->addWidget(backBtn);
    auto* titleLbl = new QLabel(QStringLiteral("实对称矩阵对角化"));
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
    connect(genBtn, &QPushButton::clicked, this, &SymDiagPage::onGenerate);
    pLay->addWidget(genBtn); pLay->addStretch(1);
    cLay->addWidget(paramW);

    gridContainer_ = new QWidget;
    gridContainerLay_ = new QVBoxLayout(gridContainer_);
    gridContainerLay_->setContentsMargins(0, 0, 0, 0);
    cLay->addWidget(gridContainer_);

    solveBtn_ = new QPushButton(QStringLiteral("开始求解"));
    solveBtn_->setEnabled(false); solveBtn_->setCursor(Qt::PointingHandCursor); solveBtn_->setFixedWidth(160);
    solveBtn_->setStyleSheet(QStringLiteral("QPushButton { background:#4A90D9; color:white; border-radius:6px; padding:8px 16px; font-size:14px; font-weight:600; } QPushButton:hover { background:#5BA0E9; } QPushButton:disabled { background:#555; color:#888; }"));
    connect(solveBtn_, &QPushButton::clicked, this, &SymDiagPage::onSolve);
    cLay->addWidget(solveBtn_);
    auto* demoBtn = new QPushButton(QStringLiteral("演示例题"));
    demoBtn->setCursor(Qt::PointingHandCursor);
    demoBtn->setStyleSheet(QStringLiteral("QPushButton { background:#2196F3; color:white; border-radius:6px; padding:8px 16px; font-size:14px; font-weight:600; } QPushButton:hover { background:#1976D2; }"));
    connect(demoBtn, &QPushButton::clicked, this, &SymDiagPage::onDemo);
    cLay->addWidget(demoBtn);

    resultBrowser_ = new QTextBrowser; resultBrowser_->setOpenLinks(false); resultBrowser_->setMinimumHeight(400);
    resultBrowser_->setStyleSheet(QStringLiteral("QTextBrowser { border:1px solid #3A3D4A; border-radius:8px; padding:12px; }"));
    cLay->addWidget(resultBrowser_, 1);
    scroll->setWidget(content);
    root->addWidget(scroll, 1);
}

void SymDiagPage::onGenerate() {
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
            edit->installEventFilter(this);
            if (i != j) {
                int symIdx = j * curN_ + i;
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

bool SymDiagPage::eventFilter(QObject* obj, QEvent* ev) {
    if (ev->type() == QEvent::KeyPress) {
        auto* ke = static_cast<QKeyEvent*>(ev);
        auto* edit = qobject_cast<QLineEdit*>(obj);
        if (edit && !cells_.empty()) {
            auto it = std::find(cells_.begin(), cells_.end(), edit);
            if (it != cells_.end()) {
                int idx = static_cast<int>(std::distance(cells_.begin(), it));
                int row = idx / curN_, col = idx % curN_;
                auto focusByIdx = [&](int i) { if (i>=0 && i<static_cast<int>(cells_.size())) { cells_[i]->setFocus(); cells_[i]->selectAll(); } };

                if (ke->key() == Qt::Key_Right && edit->cursorPosition() == edit->text().length()) {
                    int nxt = nextUpper(idx, curN_); if (nxt >= 0) focusByIdx(nxt); return true;
                }
                if (ke->key() == Qt::Key_Left && edit->cursorPosition() == 0) {
                    int prv = prevUpper(idx, curN_); if (prv >= 0) focusByIdx(prv); return true;
                }
                if (ke->key() == Qt::Key_Down) {
                    if (row + 1 < curN_) focusByIdx((row+1)*curN_+col); return true;
                }
                if (ke->key() == Qt::Key_Up) {
                    if (row > 1 && row - 1 >= col) focusByIdx((row-1)*curN_+col);
                    else if (row > 0) focusByIdx((row-1)*curN_+col);
                    return true;
                }
                if (ke->key() == Qt::Key_Return || ke->key() == Qt::Key_Enter) {
                    int nxt = nextUpper(idx, curN_);
                    if (nxt >= 0) focusByIdx(nxt); else onSolve();
                    return true;
                }
            }
        }
    }
    return QWidget::eventFilter(obj, ev);
}

void SymDiagPage::onSolve() {
    try {
    if (curN_ == 0) return;

    // ---- 1. Parse ----
    Matrix<Fraction> A(static_cast<std::size_t>(curN_), static_cast<std::size_t>(curN_));
    for (int i = 0; i < curN_; ++i)
        for (int j = 0; j < curN_; ++j) {
            try { A(i, j) = parseFraction(cells_[static_cast<std::size_t>(i * curN_ + j)]->text()); }
            catch (...) {
                resultBrowser_->setHtml(QStringLiteral("<p style=\"color:#E74C3C; font-size:14px;\">输入错误：第 %1 行第 %2 列无法解析。</p>").arg(i+1).arg(j+1)); return;
            }
        }

    // ---- 2. Symmetry check ----
    bool sym = true;
    for (std::size_t i = 0; i < A.rows() && sym; ++i)
        for (std::size_t j = i + 1; j < A.cols() && sym; ++j)
            if (!(A(i, j) == A(j, i))) sym = false;

    auto th = RenderTheme::forCurrent();
    auto* doc = resultBrowser_->document(); doc->clear();

    if (!sym) {
        QStringList parts;
        parts << titleHtml(QStringLiteral("实对称矩阵对角化"), th);
        parts << formulaHtml(QStringLiteral("A = ") + matLtx(A), th, doc);
        parts << errorHtml(QStringLiteral("输入矩阵不是实对称矩阵。"));
        resultBrowser_->setHtml(QStringLiteral("<div style=\"padding:4px; line-height:2.2;\">%1</div>").arg(parts.join(QString())));
        return;
    }

    QStringList parts;
    parts << paraHtml(QStringLiteral("求正交矩阵 $T$，使得 $T^{-1}AT$ 为对角矩阵."), th, doc);
    parts << formulaHtml(QStringLiteral("A = ") + matLtx(A), th, doc);
    parts << sectionHtml(QStringLiteral("解"), th);

    // ---- 3. Characteristic polynomial ----
    Polynomial<Fraction> cp = charpoly(A);
    auto rf = factorOverQ(cp);

    QString factoredLtx;
    if (!rf.leadingCoefficient.isOne()) factoredLtx += fracLtx(rf.leadingCoefficient);
    for (const auto& f : rf.factors) {
        if (f.first.degree() == 1) {
            Fraction r = Fraction(-1) * f.first.coeffs()[0] / f.first.coeffs()[1];
            if (r.sign() < 0) factoredLtx += QStringLiteral("(\\lambda + %1)").arg(fracLtx(-r));
            else if (r.isZero()) factoredLtx += QStringLiteral("\\lambda");
            else factoredLtx += QStringLiteral("(\\lambda - %1)").arg(fracLtx(r));
        } else {
            factoredLtx += QStringLiteral("(") + polyLtx(f.first) + QStringLiteral(")");
        }
        if (f.second > 1) factoredLtx += QStringLiteral("^{%1}").arg(f.second);
    }

    parts << paraHtml(QStringLiteral("计算特征多项式："), th);
    parts << formulaHtml(QStringLiteral("|\\lambda I - A| = %1 = %2 = %3.")
        .arg(detLtx(A), polyLtx(cp), factoredLtx), th, doc, 15);

    // ---- 4. Eigenvalues ----
    std::vector<std::pair<Fraction, int>> eigenvals;
    bool allLinear = true;
    for (const auto& f : rf.factors) {
        if (f.first.degree() == 1) {
            Fraction r = Fraction(-1) * f.first.coeffs()[0] / f.first.coeffs()[1];
            eigenvals.push_back({r, f.second});
        } else allLinear = false;
    }

    // ---- 4b. If Q-irreducible factors exist, use numerical eigenvalues ----
    std::vector<std::pair<double, int>> numEvals; // (value, multiplicity)
    if (!allLinear) {
        // Durand-Kerner on characteristic polynomial
        const auto& cs = cp.coeffs();
        int deg = cp.degree();
        double lc = cs[deg].toDouble();
        std::vector<std::complex<double>> compCoeffs(deg + 1);
        for (int i = 0; i <= deg; ++i)
            compCoeffs[i] = std::complex<double>(cs[i].toDouble() / lc, 0.0);

        // Durand-Kerner iteration
        std::vector<std::complex<double>> roots(deg);
        {
            std::complex<double> seed(0.4, 0.9), cur(1.0, 0.0);
            for (int k = 0; k < deg; ++k) { roots[k] = cur; cur *= seed; }
        }
        for (int it = 0; it < 400; ++it) {
            double maxD = 0.0;
            for (int k = 0; k < deg; ++k) {
                if (std::isnan(roots[k].real())) { roots[k] = std::complex<double>(0.4, 0.9); }
                std::complex<double> y = compCoeffs[deg];
                for (int jj = deg - 1; jj >= 0; --jj) y = y * roots[k] + compCoeffs[jj];
                std::complex<double> den(1.0, 0.0);
                for (int jj = 0; jj < deg; ++jj)
                    if (jj != k) den *= (roots[k] - roots[jj]);
                if (std::abs(den) < 1e-300) continue;
                std::complex<double> delta = y / den;
                if (std::abs(delta) > 1e6) delta = delta * (1e6 / std::abs(delta));
                if (std::isnan(delta.real())) continue;
                roots[k] -= delta;
                if (std::abs(delta) > maxD) maxD = std::abs(delta);
            }
            if (maxD < 1e-12) break;
        }

        // Real symmetric matrix: all eigenvalues are real. Treat each root separately.
        for (const auto& r : roots) {
            double th = std::max(1e-7, 1e-10 * std::abs(r.real()));
            if (std::abs(r.imag()) < th) numEvals.push_back({r.real(), 1});
        }
        std::sort(numEvals.begin(), numEvals.end(),
            [](auto& a, auto& b){ return a.first < b.first; });
    }

    // ---- 5. Display eigenvalues ----
    if (allLinear) {
        QString eigStr;
        for (std::size_t i = 0; i < eigenvals.size(); ++i) {
            if (i > 0) eigStr += QStringLiteral(", ");
            eigStr += QStringLiteral("$%1$").arg(fracLtx(eigenvals[i].first));
            if (eigenvals[i].second > 1) eigStr += QStringLiteral("（$%1$ 重）").arg(eigenvals[i].second);
        }
        parts << paraHtml(QStringLiteral("因此 $A$ 的全部特征值是 %1.").arg(eigStr), th, doc);
    } else {
        QString eigStr;
        for (std::size_t i = 0; i < numEvals.size(); ++i) {
            if (i > 0) eigStr += QStringLiteral(", ");
            // Strip trailing zeros: 5.000 → 5, 2.50 → 2.5
            double v = numEvals[i].first;
            QString vs = QString::number(v, 'f', 6);
            if (vs.contains('.')) { while (vs.endsWith('0')) vs.chop(1); if (vs.endsWith('.')) vs.chop(1); }
            eigStr += QStringLiteral("$%1$").arg(vs);
            if (numEvals[i].second > 1) eigStr += QStringLiteral("（$%1$ 重）").arg(numEvals[i].second);
        }
        parts << paraHtml(QStringLiteral("因此 $A$ 的全部特征值是 %1.").arg(eigStr), th, doc);
    }

    // Helper: format double as clean string for Fraction construction
    auto doubleToFrac = [](double x) -> Fraction {
        long long num = static_cast<long long>(std::round(x * 1e8));
        return Fraction(BigInt(num), BigInt(100000000LL));
    };

    // ---- 6. Eigenvectors + Gram-Schmidt + normalize ----
    std::vector<std::pair<Fraction, std::vector<Matrix<Fraction>>>> allEtas;
    int alphaIdx = 1;

    // Merge exact and numerical eigenvalues for processing
    std::vector<std::pair<Fraction, int>> allEvals;
    for (const auto& ev : eigenvals) allEvals.push_back(ev);
    for (const auto& ev : numEvals)
        allEvals.push_back({doubleToFrac(ev.first), ev.second});

    for (const auto& ev : allEvals) {
        Fraction lambda = ev.first;
        QString lamStr;
        if (lambda.denominator() > BigInt(10000)) {
            lamStr = QString::number(lambda.toDouble(), 'f', 6);
            while (lamStr.endsWith(QLatin1Char('0'))) lamStr.chop(1);
            if (lamStr.endsWith(QLatin1Char('.'))) lamStr.chop(1);
        } else {
            lamStr = fracLtx(lambda);
        }

        Matrix<Fraction> M(static_cast<std::size_t>(curN_), static_cast<std::size_t>(curN_));
        for (int i = 0; i < curN_; ++i) {
            for (int j = 0; j < curN_; ++j) M(i, j) = -A(i, j);
            M(i, i) += lambda;
        }
        // Try exact nullspace first; fall back to numerical; then inverse iteration
        std::vector<std::vector<double>> numBasis;
        Matrix<Fraction> N(static_cast<std::size_t>(curN_), 0);
        if (lambda.denominator() <= BigInt(10000))
            N = nullspace(M);
        if (N.cols() == 0)
            numBasis = numNullspace(A, lambda.toDouble());
        if (N.cols() == 0 && numBasis.empty()) {
            // Inverse iteration: (A - λI) is near-singular, solve with regularization
            auto n = static_cast<int>(A.rows());
            std::vector<std::vector<double>> MM(n, std::vector<double>(n));
            for (int i = 0; i < n; ++i) {
                for (int j = 0; j < n; ++j) MM[i][j] = -A(i, j).toDouble();
                MM[i][i] += lambda.toDouble() + 1e-10;
            }
            std::vector<double> x(n, 1.0 / std::sqrt(static_cast<double>(n)));
            for (int it = 0; it < 3; ++it) {
                // Gaussian elimination with pivoting
                auto A2 = MM;
                auto b = x;
                std::vector<int> perm(n);
                for (int i = 0; i < n; ++i) perm[i] = i;
                for (int i = 0; i < n; ++i) {
                    int piv = i;
                    for (int r = i+1; r < n; ++r) if (std::abs(A2[perm[r]][i]) > std::abs(A2[perm[piv]][i])) piv = r;
                    std::swap(perm[i], perm[piv]);
                    double pivVal = A2[perm[i]][i];
                    for (int j = i; j < n; ++j) A2[perm[i]][j] /= pivVal;
                    b[perm[i]] /= pivVal;
                    for (int r = 0; r < n; ++r) {
                        if (r == i) continue;
                        double f = A2[perm[r]][i];
                        for (int j = i; j < n; ++j) A2[perm[r]][j] -= f * A2[perm[i]][j];
                        b[perm[r]] -= f * b[perm[i]];
                    }
                }
                double norm = 0.0;
                for (int i = 0; i < n; ++i) { x[i] = b[perm[i]]; norm += x[i]*x[i]; }
                norm = std::sqrt(norm);
                if (norm > 1e-15) for (auto& v : x) v /= norm;
            }
            numBasis.push_back(x);
        }
        if (N.cols() == 0 && !numBasis.empty()) {
            N = Matrix<Fraction>(static_cast<std::size_t>(curN_), numBasis.size());
            for (std::size_t k = 0; k < numBasis.size(); ++k)
                for (int i = 0; i < curN_; ++i) {
                    long long num = static_cast<long long>(std::round(numBasis[k][i] * 1e8));
                    N(i, k) = Fraction(BigInt(num), BigInt(100000000LL));
                }
        }
        std::size_t geom = N.cols();
        if (geom == 0) continue;

        if (ev.second > 1)
            parts << paraHtml(QStringLiteral("对于特征值 $%1$（%2 重），求出 $(%1 I - A)X = 0$ 的一个基础解系：")
                .arg(lamStr).arg(ev.second), th, doc);
        else
            parts << paraHtml(QStringLiteral("对于特征值 $%1$，求出 $(%1 I - A)X = 0$ 的一个基础解系：")
                .arg(lamStr), th, doc);

        // Show basis vectors
        std::vector<Matrix<Fraction>> alphas;
        {
            QString basisStr;
            for (std::size_t k = 0; k < geom; ++k) {
                if (k > 0) basisStr += QStringLiteral(", \\quad ");
                Matrix<Fraction> v(static_cast<std::size_t>(curN_), 1);
                for (int i = 0; i < curN_; ++i) v(i, 0) = N(i, k);
                basisStr += QStringLiteral("\\alpha_{%1} = %2").arg(alphaIdx++)
                    .arg(allLinear ? matLtx(v) : decColLtx(v));
                alphas.push_back(v);
            }
            parts << formulaHtml(basisStr, th, doc, 15);
        }

        // If geom > 1 for a repeated eigenvalue, need Gram-Schmidt
        if (geom > 1) {
            parts << paraHtml(QStringLiteral("正交化，令"), th);

            std::vector<Matrix<Fraction>> betas;
            betas.push_back(alphas[0]);
            parts << formulaHtml(QStringLiteral("\\beta_{%1} = \\alpha_{%1},")
                .arg(alphaIdx - static_cast<int>(geom)), th, doc);

            for (std::size_t k = 1; k < geom; ++k) {
                Matrix<Fraction> beta = alphas[k];
                QString rhs = QStringLiteral("\\alpha_{%1}").arg(alphaIdx - static_cast<int>(geom) + static_cast<int>(k));
                for (std::size_t j = 0; j < k; ++j) {
                    Fraction num = dotProd(alphas[k], betas[j]);
                    Fraction den = dotProd(betas[j], betas[j]);
                    rhs += QStringLiteral(" - \\frac{%1}{%2}\\beta_{%1}")
                        .arg(fracLtx(num), fracLtx(den)).arg(alphaIdx - static_cast<int>(geom) + static_cast<int>(j));
                    for (std::size_t i = 0; i < beta.rows(); ++i)
                        beta(i, 0) -= num * betas[j](i, 0) / den;
                }
                betas.push_back(beta);

                // Show detailed step
                QString detail = QStringLiteral("\\beta_{%1} = %2")
                    .arg(alphaIdx - static_cast<int>(geom) + static_cast<int>(k) + 1);
                detail += QStringLiteral(" = %1").arg(matLtx(alphas[k]));
                for (std::size_t j = 0; j < k; ++j) {
                    Fraction num = dotProd(alphas[k], betas[j]);
                    Fraction den = dotProd(betas[j], betas[j]);
                    detail += QStringLiteral(" - \\frac{%1}{%2}%3")
                        .arg(fracLtx(num), fracLtx(den), matLtx(betas[j]));
                }
                detail += QStringLiteral(" = %1.").arg(matLtx(beta));
                parts << formulaHtml(detail, th, doc, 14);
            }

            // Normalize
            parts << paraHtml(QStringLiteral("单位化："), th);
            int betaStart = alphaIdx - static_cast<int>(geom);
            QString etaStr;
            std::vector<Matrix<Fraction>> etas;
            for (std::size_t k = 0; k < geom; ++k) {
                Fraction ns = normSq(betas[k]);
                if (k > 0) etaStr += QStringLiteral(", \\quad ");
                etaStr += QStringLiteral("\\eta_{%1} = \\frac{1}{|\\beta_{%2}|}\\beta_{%2}")
                    .arg(betaStart + static_cast<int>(k) + 1).arg(betaStart + static_cast<int>(k) + 1);
                etaStr += QStringLiteral(" = ") + (allLinear ? normVecLtx(betas[k], ns) : decVecLtx(betas[k], ns));
                etas.push_back(betas[k]); // store for later
            }
            parts << formulaHtml(etaStr, th, doc, 14);
            // Store with original eigenvalue for T construction
            // (etas are the beta vectors, not yet normalized — but we store them as symbolic)
            // For simplicity, store the beta vectors
            for (const auto& b : betas)
                allEtas.push_back({lambda, {b}});
        } else {
            // Single eigenvector: just normalize
            Fraction ns = normSq(alphas[0]);
            int idx = alphaIdx - 1;
            QString etaLine = QStringLiteral("\\eta_{%1} = \\frac{1}{|\\alpha_{%2}|}\\alpha_{%2}")
                .arg(idx + 1).arg(idx + 1);
            if (allLinear)
                etaLine += QStringLiteral(" = %1.").arg(normVecLtx(alphas[0], ns));
            else
                etaLine += QStringLiteral(" = %1.").arg(decVecLtx(alphas[0], ns));
            parts << formulaHtml(etaLine, th, doc, 14);
            allEtas.push_back({lambda, {alphas[0]}});
        }
    }

    // ---- 6. Build orthogonal matrix T and diagonal Λ ----
    // Collect all eta vectors in order
    std::vector<Matrix<Fraction>> columns;
    std::vector<Fraction> diagEntries;
    for (const auto& p : allEtas) {
        for (const auto& v : p.second) {
            columns.push_back(v);
            diagEntries.push_back(p.first);
        }
    }

    if (columns.size() == static_cast<std::size_t>(curN_)) {
        // Helper: format double cleanly
        auto fmtD = [](double v){
            if (std::abs(v) < 1e-10) v = 0.0;
            QString s = QString::number(v, 'f', 4);
            while (s.endsWith('0')) s.chop(1);
            if (s.endsWith('.')) s.chop(1);
            return s;
        };

        // Build T
        QString tBody;
        for (std::size_t i = 0; i < static_cast<std::size_t>(curN_); ++i) {
            for (std::size_t j = 0; j < columns.size(); ++j) {
                if (j) tBody += QStringLiteral(" & ");
                if (allLinear) {
                    Fraction ns = normSq(columns[j]);
                    tBody += fracDivSqrtLtx(columns[j](i, 0), ns);
                } else {
                    // Numerical: normalize and show as decimal
                    Fraction ns = normSq(columns[j]);
                    double v = columns[j](i, 0).toDouble() / std::sqrt(ns.toDouble());
                    tBody += fmtD(v);
                }
            }
            if (i + 1 < static_cast<std::size_t>(curN_)) tBody += QStringLiteral(" \\\\\\\\ ");
        }
        QString colSpec(curN_, QLatin1Char('c'));
        QString tLtx = QStringLiteral("\\begin{pmatrix}%1\\end{pmatrix}").arg(tBody);

        // Build Λ
        QString diagBody;
        for (std::size_t i = 0; i < static_cast<std::size_t>(curN_); ++i) {
            for (std::size_t j = 0; j < static_cast<std::size_t>(curN_); ++j) {
                if (j) diagBody += QStringLiteral(" & ");
                if (i == j) {
                    if (allLinear) diagBody += fracLtx(diagEntries[i]);
                    else diagBody += fmtD(diagEntries[i].toDouble());
                } else {
                    diagBody += QStringLiteral("0");
                }
            }
            if (i + 1 < static_cast<std::size_t>(curN_)) diagBody += QStringLiteral(" \\\\\\\\ ");
        }
        QString diagLtx = QStringLiteral("\\begin{pmatrix}%1\\end{pmatrix}").arg(diagBody);

        parts << paraHtml(QStringLiteral("令"), th);
        parts << formulaHtml(QStringLiteral("T = %1,").arg(tLtx), th, doc, 13);
        parts << paraHtml(QStringLiteral("则 $T$ 是正交矩阵，并且有"), th, doc);
        parts << formulaHtml(QStringLiteral("T^{-1}AT = %1.").arg(diagLtx), th, doc);
    }

    resultBrowser_->setHtml(QStringLiteral("<div style=\"padding:4px; line-height:2.2;\">%1</div>").arg(parts.join(QString())));

    } catch (const std::exception& e) {
        resultBrowser_->setHtml(QStringLiteral("<p style=\"color:#E74C3C; font-size:14px;\">计算出错：%1</p>").arg(QString::fromStdString(e.what())));
    } catch (...) {
        resultBrowser_->setHtml(QStringLiteral("<p style=\"color:#E74C3C; font-size:14px;\">计算出错：未知异常。</p>"));
    }
}

void SymDiagPage::onDemo() {
    spinN_->setValue(3); onGenerate();
    // A = [[1,-2,-4],[-2,4,-2],[-4,-2,1]]
    const std::vector<std::vector<int>> demo = {{1,-2,-4},{-2,4,-2},{-4,-2,1}};
    for (int i = 0; i < 3; ++i)
        for (int j = 0; j < 3; ++j)
            cells_[i*3+j]->setText(QString::number(demo[i][j]));
    onSolve();
}

} // namespace AlgeMate::Calculator::Demo
