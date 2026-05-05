#include "CalculatorPage.h"

#include "interactive/InteractivePage.h"
#include "visualize/VisualizePage.h"
#include "demo/DemoPage.h"

#include <QButtonGroup>
#include <QFont>
#include <QGridLayout>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QPainter>
#include <QPixmap>
#include <QPushButton>
#include <QScreen>
#include <QSize>
#include <QStackedWidget>
#include <QToolButton>
#include <QVBoxLayout>
#include <QVector>
#include <QWidget>
#include <QtMath>

#include <jkqtmathtext/jkqtmathtext.h>

namespace AlgeMate::Calculator {

namespace {
struct GreekItem {
    const char* ch;     // UTF-8 小写 Unicode
    const char* label;  // LaTeX 宏
};
// 22 个小写希腊字母 (标准 LaTeX 宏)
static const GreekItem kGreeks[] = {
    {"\xce\xb1", "\\alpha"   },
    {"\xce\xb2", "\\beta"    },
    {"\xce\xb3", "\\gamma"   },
    {"\xce\xb4", "\\delta"   },
    {"\xce\xb5", "\\epsilon" },
    {"\xce\xb6", "\\zeta"    },
    {"\xce\xb7", "\\eta"     },
    {"\xce\xb8", "\\theta"   },
    {"\xce\xb9", "\\iota"    },
    {"\xce\xba", "\\kappa"   },
    {"\xce\xbb", "\\lambda"  },
    {"\xce\xbc", "\\mu"      },
    {"\xce\xbd", "\\nu"      },
    {"\xce\xbe", "\\xi"      },
    {"\xcf\x80", "\\pi"      },
    {"\xcf\x81", "\\rho"     },
    {"\xcf\x83", "\\sigma"   },
    {"\xcf\x84", "\\tau"     },
    {"\xcf\x85", "\\upsilon" },
    {"\xcf\x87", "\\chi"     },
    {"\xcf\x88", "\\psi"     },
    {"\xcf\x89", "\\omega"   },
};

// 用 JKQTMathText 将单个 LaTeX 小器 (如 "\alpha") 渲染为透明背景小图, 供 QPushButton::setIcon
static QPixmap renderGlyphPixmap(const QString& latex, int fontPt, const QColor& color) {
    qreal dpr = 1.0;
    if (QGuiApplication::primaryScreen())
        dpr = QGuiApplication::primaryScreen()->devicePixelRatio();
    if (dpr < 1.0) dpr = 1.0;

    const int ss = 3;  // 3x 超采样
    JKQTMathText mt;
    mt.useXITS();
    mt.setFontSize(fontPt * ss);
    mt.setFontColor(color);
    mt.parse(QStringLiteral("$") + latex + QStringLiteral("$"));

    QPixmap probe(4, 4);
    QPainter pm(&probe);
    QSizeF sz = mt.getSize(pm);
    pm.end();

    const int marginSS = 2 * ss;
    int wSS = qCeil(sz.width())  + marginSS * 2;
    int hSS = qCeil(sz.height()) + marginSS * 2;
    if (wSS < 8) wSS = 8;
    if (hSS < 8) hSS = 8;

    QPixmap big(wSS, hSS);
    big.fill(Qt::transparent);
    {
        QPainter p(&big);
        p.setRenderHint(QPainter::Antialiasing, true);
        p.setRenderHint(QPainter::TextAntialiasing, true);
        p.setRenderHint(QPainter::SmoothPixmapTransform, true);
        mt.draw(p, Qt::AlignCenter, QRectF(0, 0, wSS, hSS), false);
    }

    int wPhysical = int((wSS / ss) * dpr);
    int hPhysical = int((hSS / ss) * dpr);
    if (wPhysical < 2) wPhysical = 2;
    if (hPhysical < 2) hPhysical = 2;
    QPixmap scaled = big.scaled(wPhysical, hPhysical,
                                Qt::IgnoreAspectRatio,
                                Qt::SmoothTransformation);
    scaled.setDevicePixelRatio(dpr);
    return scaled;
}
}

CalculatorPage::CalculatorPage(QWidget* parent) : QWidget(parent) {
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(24, 24, 24, 24);
    root->setSpacing(16);

    // 顶部行
    auto* topRow = new QHBoxLayout;
    topRow->setSpacing(16);

    // 左侧: 标题 + 副标题 + Tab 切换
    auto* leftCol = new QVBoxLayout;
    leftCol->setSpacing(8);

    auto* title = new QLabel(QStringLiteral("计算助手"));
    title->setObjectName(QStringLiteral("PageTitle"));
    auto* subtitle = new QLabel(QStringLiteral(
        "交互式运算  ·  可视化展示  ·  算法过程演示"));
    subtitle->setObjectName(QStringLiteral("PageSubtitle"));

    auto* tabBar = new QWidget;
    tabBar->setObjectName(QStringLiteral("CalcTabBar"));
    auto* tabLayout = new QHBoxLayout(tabBar);
    tabLayout->setContentsMargins(0, 4, 0, 4);
    tabLayout->setSpacing(6);

    auto makeTabBtn = [](const QString& text) {
        auto* btn = new QPushButton(text);
        btn->setObjectName(QStringLiteral("CalcTabBtn"));
        btn->setCheckable(true);
        btn->setCursor(Qt::PointingHandCursor);
        return btn;
    };
    auto* btnInteractive = makeTabBtn(QStringLiteral("交互式"));
    auto* btnVisualize   = makeTabBtn(QStringLiteral("可视化"));
    auto* btnDemo        = makeTabBtn(QStringLiteral("算法演示"));
    btnInteractive->setChecked(true);

    auto* group = new QButtonGroup(this);
    group->setExclusive(true);
    group->addButton(btnInteractive, 0);
    group->addButton(btnVisualize,   1);
    group->addButton(btnDemo,        2);

    tabLayout->addWidget(btnInteractive);
    tabLayout->addWidget(btnVisualize);
    tabLayout->addWidget(btnDemo);
    tabLayout->addStretch(1);

    leftCol->addWidget(title);
    leftCol->addWidget(subtitle);
    leftCol->addWidget(tabBar);

    // 右侧: 希腊字母符号面板 (2 行 × 11 列 = 22 个小写, 按钮显示 LaTeX 渲染的希腊字形)
    auto* symbolPanel = new QWidget;
    symbolPanel->setObjectName(QStringLiteral("CalcSymbolPanel"));
    auto* symGrid = new QGridLayout(symbolPanel);
    symGrid->setContentsMargins(0, 0, 0, 0);
    symGrid->setHorizontalSpacing(2);
    symGrid->setVerticalSpacing(2);

    // 内部页面指针 (先创建, 供 lambda 捕获)
    auto* interactivePage = new Interactive::InteractivePage;

    // 图标颜色: 从当前 palette 取文本色
    const QColor glyphColor = palette().color(QPalette::WindowText);
    const int glyphFontPt = 13;
    const QSize iconSize(22, 22);

    auto symBtns = new QVector<QPushButton*>;
    const int cols = 11;  // 2 行 × 11 列 = 22
    const int total = sizeof(kGreeks) / sizeof(kGreeks[0]);
    for (int i = 0; i < total; ++i) {
        auto* b = new QPushButton;
        b->setObjectName(QStringLiteral("CalcSymBtn"));
        b->setFixedSize(30, 32);  // 高度加大, 防止上下截断
        b->setIconSize(iconSize);
        b->setCursor(Qt::PointingHandCursor);
        b->setFocusPolicy(Qt::NoFocus);

        QPixmap px = renderGlyphPixmap(
            QString::fromUtf8(kGreeks[i].label), glyphFontPt, glyphColor);
        b->setIcon(QIcon(px));
        b->setToolTip(QString::fromUtf8(kGreeks[i].label));
        const QString ch = QString::fromUtf8(kGreeks[i].ch);
        connect(b, &QPushButton::clicked, this, [interactivePage, ch]{
            interactivePage->insertAtCursor(ch);
        });
        symGrid->addWidget(b, i / cols, i % cols);
        symBtns->append(b);
    }

    symbolPanel->setLayout(symGrid);

    // 根号面板 (放在希腊字母面板右侧): 根号 / n 次根号
    auto* opsPanel = new QWidget;
    opsPanel->setObjectName(QStringLiteral("CalcOpsPanel"));
    auto* opsGrid = new QGridLayout(opsPanel);
    opsGrid->setContentsMargins(0, 0, 0, 0);
    opsGrid->setHorizontalSpacing(2);
    opsGrid->setVerticalSpacing(2);

    struct OpItem { const char* latex; const char* tip; int kind; };
    const OpItem kOps[] = {
        { "\\sqrt{\\square}",    "根号 sqrt()",      0 },
        { "\\sqrt[n]{\\square}", "n 次根号 root(n, x)", 1 },
    };
    for (int i = 0; i < int(sizeof(kOps) / sizeof(kOps[0])); ++i) {
        auto* b = new QPushButton;
        b->setObjectName(QStringLiteral("CalcSymBtn"));
        b->setFixedSize(36, 32);
        b->setIconSize(QSize(28, 22));
        b->setCursor(Qt::PointingHandCursor);
        b->setFocusPolicy(Qt::NoFocus);
        QPixmap px = renderGlyphPixmap(
            QString::fromUtf8(kOps[i].latex), glyphFontPt, glyphColor);
        b->setIcon(QIcon(px));
        b->setToolTip(QString::fromUtf8(kOps[i].tip));
        const int kind = kOps[i].kind;
        connect(b, &QPushButton::clicked, this, [interactivePage, kind]{
            if (kind == 0) interactivePage->insertSqrtTemplate();
            else           interactivePage->insertRootTemplate();
        });
        opsGrid->addWidget(b, i, 0);
    }
    opsPanel->setLayout(opsGrid);

    topRow->addLayout(leftCol, 1);
    topRow->addWidget(symbolPanel, 0, Qt::AlignTop | Qt::AlignRight);
    topRow->addWidget(opsPanel,    0, Qt::AlignTop | Qt::AlignRight);

    // 内容区
    auto* stack = new QStackedWidget;
    stack->setObjectName(QStringLiteral("CalcStack"));
    stack->addWidget(interactivePage);
    stack->addWidget(new Visualize::VisualizePage);
    stack->addWidget(new Demo::DemoPage);

    connect(group, &QButtonGroup::idClicked, stack, &QStackedWidget::setCurrentIndex);

    root->addLayout(topRow);
    root->addWidget(stack, 1);
}

}
