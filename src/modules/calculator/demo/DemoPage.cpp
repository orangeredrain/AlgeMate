#include "DemoPage.h"
#include "EigenPage.h"
#include "GSOPage.h"
#include "JordanFormPage.h"
#include "PolyGCDPage.h"
#include "QuadFormPage.h"
#include "SymReducePage.h"
#include "SymDiagPage.h"
#include "HomoLinearSystemPage.h"
#include "InversePage.h"
#include "MaxIndepPage.h"
#include "NonhomoLinearSystemPage.h"

#include "modules/calculator/interactive/expr/RenderSettings.h"

#include "DemoCommon.h"

#include <QFrame>
#include <QGridLayout>
#include <QLabel>
#include <QPushButton>
#include <QScrollArea>
#include <QStackedWidget>
#include <QVBoxLayout>

using AlgeMate::Calculator::Interactive::RenderTheme;

namespace AlgeMate::Calculator::Demo {

namespace {

bool isLatex(const QString& s) {
    return s.startsWith(QLatin1Char('$')) && s.endsWith(QLatin1Char('$')) && s.size() >= 2;
}

} // anonymous namespace

DemoPage::DemoPage(QWidget* parent) : QWidget(parent) {
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    stack_ = new QStackedWidget;

    // ---- page 0: 算法选择目录 ----
    buildCatalog();

    // ---- page 1: 齐次线性方程组 ----
    auto* homoPage = new HomoLinearSystemPage;
    connect(homoPage, &HomoLinearSystemPage::backRequested,
            this, [this]{ stack_->setCurrentIndex(0); });
    stack_->addWidget(homoPage);

     // ---- page 2: 非齐次线性方程组 ----
    auto* nonhomoPage = new NonhomoLinearSystemPage;
    connect(nonhomoPage, &NonhomoLinearSystemPage::backRequested,
            this, [this]{ stack_->setCurrentIndex(0); });
    stack_->addWidget(nonhomoPage);

    // ---- page 3: 极大线性无关组 ----
    auto* maxIndepPage = new MaxIndepPage;
    connect(maxIndepPage, &MaxIndepPage::backRequested,
            this, [this]{ stack_->setCurrentIndex(0); });
    stack_->addWidget(maxIndepPage);

    // ---- page 4: 矩阵求逆 ----
    auto* inversePage = new InversePage;
    connect(inversePage, &InversePage::backRequested,
            this, [this]{ stack_->setCurrentIndex(0); });
    stack_->addWidget(inversePage);

    // ---- page 5: Schmidt 正交化 ----
    auto* gsoPage = new GSOPage;
    connect(gsoPage, &GSOPage::backRequested,
            this, [this]{ stack_->setCurrentIndex(0); });
    stack_->addWidget(gsoPage);

    // ---- page 6: 特征值与特征向量 ----
    auto* eigenPage = new EigenPage;
    connect(eigenPage, &EigenPage::backRequested,
            this, [this]{ stack_->setCurrentIndex(0); });
    stack_->addWidget(eigenPage);

    // ---- page 7: 实对称矩阵对角化 ----
    auto* symDiagPage = new SymDiagPage;
    connect(symDiagPage, &SymDiagPage::backRequested,
            this, [this]{ stack_->setCurrentIndex(0); });
    stack_->addWidget(symDiagPage);

    // ---- page 8: 实二次型化标准形 ----
    auto* quadFormPage = new QuadFormPage;
    connect(quadFormPage, &QuadFormPage::backRequested,
            this, [this]{ stack_->setCurrentIndex(0); });
    stack_->addWidget(quadFormPage);

    // ---- page 9: 多项式最大公因式 ----
    auto* polyGCDPage = new PolyGCDPage;
    connect(polyGCDPage, &PolyGCDPage::backRequested,
            this, [this]{ stack_->setCurrentIndex(0); });
    stack_->addWidget(polyGCDPage);

    // ---- page 10: 对称多项式化为初等对称多项式 ----
    auto* symReducePage = new SymReducePage;
    connect(symReducePage, &SymReducePage::backRequested,
            this, [this]{ stack_->setCurrentIndex(0); });
    stack_->addWidget(symReducePage);

    // ---- page 11: Jordan 标准形 ----
    auto* jordanFormPage = new JordanFormPage;
    connect(jordanFormPage, &JordanFormPage::backRequested,
            this, [this]{ stack_->setCurrentIndex(0); });
    stack_->addWidget(jordanFormPage);

    root->addWidget(stack_, 1);
}

void DemoPage::buildCatalog() {
    auto* page = new QWidget;
    auto* outer = new QVBoxLayout(page);
    outer->setContentsMargins(24, 24, 24, 24);
    outer->setSpacing(16);

    auto* title = new QLabel(QStringLiteral("算法演示"));
    title->setObjectName(QStringLiteral("PageTitle"));
    auto* subtitle = new QLabel(QStringLiteral(
        "选择一个算法，查看详细的分步求解过程"));
    subtitle->setObjectName(QStringLiteral("PageSubtitle"));
    outer->addWidget(title);
    outer->addWidget(subtitle);
    outer->addSpacing(8);

    auto* grid = new QWidget;
    auto* gridLay = new QGridLayout(grid);
    gridLay->setSpacing(16);
    gridLay->setContentsMargins(0, 0, 0, 0);

    addCard(grid, 0, 0,
            QStringLiteral("$AX=0$"),
            QStringLiteral("解齐次线性方程组"),
            QStringLiteral("高斯消元 → RREF → 基础解系 → 解集"),
            1);

    addCard(grid, 0, 1,
            QStringLiteral("$AX=b$"),
            QStringLiteral("解非齐次线性方程组"),
            QStringLiteral("高斯消元 → RREF → 特解 + 基础解系"),
            2);

    addCard(grid, 1, 0,
            QStringLiteral("$\\operatorname{rank}(A)$"),
            QStringLiteral("求极大线性无关组"),
            QStringLiteral("向量组 → 列矩阵 → 行变换 → 秩 + 极大无关组"),
            3);

    addCard(grid, 1, 1,
            QStringLiteral("$A^{-1}$"),
            QStringLiteral("矩阵求逆"),
            QStringLiteral("(A, I) → 初等行变换 → (I, A⁻¹)"),
            4);

    addCard(grid, 2, 0,
            QStringLiteral("$\\alpha\\perp\\beta$"),
            QStringLiteral("Schmidt正交化"),
            QStringLiteral("极大无关组 → 正交化 → 单位化"),
            5);

    addCard(grid, 2, 1,
            QStringLiteral("$A\\xi = \\lambda \\xi$"),
            QStringLiteral("特征值与特征向量"),
            QStringLiteral("|λI-A| → 特征值 → 特征向量"),
            6);

    addCard(grid, 3, 0,
            QStringLiteral("$T^{-1}AT=\\Lambda$"),
            QStringLiteral("实对称矩阵对角化"),
            QStringLiteral("特征值 → 特征向量 → Schmidt正交化 → 单位化 → T"),
            7);

    addCard(grid, 3, 1,
            QStringLiteral("$f=X^{T}AX$"),
            QStringLiteral("实二次型化标准形"),
            QStringLiteral("成对初等行/列变换→合同对角化→X=CY"),
            8);

    addCard(grid, 4, 0,
            QStringLiteral("$\\gcd(f,g)$"),
            QStringLiteral("多项式最大公因式"),
            QStringLiteral("辗转相除法 → 倍式和表示"),
            9);

    addCard(grid, 4, 1,
            QStringLiteral("$g(\\sigma_1,\\sigma_2,\\cdots,\\sigma_n)$"),
            QStringLiteral("对称多项式用初等对称多项式表出"),
            QStringLiteral("字典序降次法 / 待定系数法 → 表达为σ的多项式"),
            10);

    addCard(grid, 5, 0,
            QStringLiteral("$P^{-1}AP=J$"),
            QStringLiteral("Jordan 标准形"),
            QStringLiteral("λ-矩阵 → 行列式因子 → 不变因子 → 初等因子 → Jordan标准形"),
            11);

    outer->addWidget(grid);
    outer->addStretch(1);

    auto* scrollArea = new QScrollArea;
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setWidget(page);
    stack_->addWidget(scrollArea);
}

void DemoPage::addCard(QWidget* grid, int row, int col,
                       const QString& icon, const QString& title,
                       const QString& desc, int pageIndex) {
    auto* card = new QFrame(grid);
    card->setObjectName(QStringLiteral("Card"));
    card->setCursor(Qt::PointingHandCursor);
    card->setFixedSize(260, 160);

    auto* lay = new QVBoxLayout(card);
    lay->setContentsMargins(20, 20, 20, 20);
    lay->setSpacing(8);

    const auto th = RenderTheme::forCurrent();

    auto* iconLbl = new QLabel;
    if (isLatex(icon)) {
        auto px = renderLatex(icon.mid(1, icon.size() - 2), th, 22);
        iconLbl->setPixmap(px);
    } else {
        iconLbl->setText(icon);
        iconLbl->setStyleSheet(QStringLiteral("font-size: 32px;"));
    }

    auto* titleLbl = new QLabel;
    if (isLatex(title)) {
        auto px = renderLatex(title.mid(1, title.size() - 2), th, 14);
        titleLbl->setPixmap(px);
    } else {
        titleLbl->setText(title);
        titleLbl->setStyleSheet(QStringLiteral(
            "font-size: 15px; font-weight: 600;"));
    }

    auto* descLbl = new QLabel(desc);
    descLbl->setWordWrap(true);
    descLbl->setStyleSheet(QStringLiteral(
        "font-size: 12px; color: #8A8FA3;"));

    lay->addWidget(iconLbl);
    lay->addWidget(titleLbl);
    lay->addWidget(descLbl);
    lay->addStretch(1);

    // 点击卡片切换到对应子页面
    auto* overlay = new QPushButton(card);
    overlay->setFlat(true);
    overlay->setStyleSheet(QStringLiteral(
        "QPushButton { background: transparent; border: none; }"));
    overlay->setCursor(Qt::PointingHandCursor);
    // 让 overlay 覆盖整张卡片
    card->installEventFilter(overlay);
    overlay->setGeometry(0, 0, 260, 160);
    connect(overlay, &QPushButton::clicked, this, [this, pageIndex]{
        stack_->setCurrentIndex(pageIndex);
    });

    auto* gridLay = qobject_cast<QGridLayout*>(grid->layout());
    if (gridLay) gridLay->addWidget(card, row, col);
}

}
