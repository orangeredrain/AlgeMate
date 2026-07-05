#include "CalculatorPage.h"
#include "interactive/InteractivePage.h"
#include "visualize/VisualizePage.h"
#include "demo/DemoPage.h"

#include "core/ThemeManager.h"

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
    const char* ch;     
    const char* label;  
};

static const GreekItem kGreeks[] = {
    {"\xce\xb1", "\\alpha"   }, {"\xce\xb2", "\\beta"    }, {"\xce\xb3", "\\gamma"   },
    {"\xce\xb4", "\\delta"   }, {"\xce\xb5", "\\epsilon" }, {"\xce\xb6", "\\zeta"    },
    {"\xce\xb7", "\\eta"     }, {"\xce\xb8", "\\theta"   }, {"\xce\xb9", "\\iota"    },
    {"\xce\xba", "\\kappa"   }, {"\xce\xbb", "\\lambda"  }, {"\xce\xbc", "\\mu"      },
    {"\xce\xbd", "\\nu"      }, {"\xce\xbe", "\\xi"      }, {"\xcf\x80", "\\pi"      },
    {"\xcf\x81", "\\rho"     }, {"\xcf\x83", "\\sigma"   }, {"\xcf\x84", "\\tau"     },
    {"\xcf\x85", "\\upsilon" }, {"\xcf\x87", "\\chi"     }, {"\xcf\x88", "\\psi"     },
    {"\xcf\x89", "\\omega"   },
    };

struct OpItem { const char* latex; const char* tip; int kind; };
static const OpItem kOps[] = {
    { "\\sqrt{\\square}",    "根号 sqrt()",      0 },
    { "\\sqrt[n]{\\square}", "n 次根号 root(n, x)", 1 },
    };

static QPixmap renderGlyphPixmap(const QString& latex, int fontPt, const QColor& color) {
    qreal dpr = 1.0;
    if (QGuiApplication::primaryScreen()) dpr = QGuiApplication::primaryScreen()->devicePixelRatio();
    if (dpr < 1.0) dpr = 1.0;

    const int ss = 3;
    JKQTMathText mt; mt.useXITS(); mt.setFontSize(fontPt * ss); mt.setFontColor(color);
    mt.parse(QStringLiteral("$") + latex + QStringLiteral("$"));

    QPixmap probe(4, 4); QPainter pm(&probe); QSizeF sz = mt.getSize(pm); pm.end();

    const int marginSS = 2 * ss;
    int wSS = qCeil(sz.width())  + marginSS * 2; int hSS = qCeil(sz.height()) + marginSS * 2;
    if (wSS < 8) wSS = 8; if (hSS < 8) hSS = 8;

    QPixmap big(wSS, hSS); big.fill(Qt::transparent);
    { QPainter p(&big); p.setRenderHint(QPainter::Antialiasing, true); p.setRenderHint(QPainter::TextAntialiasing, true); p.setRenderHint(QPainter::SmoothPixmapTransform, true); mt.draw(p, Qt::AlignCenter, QRectF(0, 0, wSS, hSS), false); }

    int wPhysical = int((wSS / ss) * dpr); int hPhysical = int((hSS / ss) * dpr);
    if (wPhysical < 2) wPhysical = 2; if (hPhysical < 2) hPhysical = 2;
    QPixmap scaled = big.scaled(wPhysical, hPhysical, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    scaled.setDevicePixelRatio(dpr);
    return scaled;
}
}

CalculatorPage::CalculatorPage(QWidget* parent) : QWidget(parent) {
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(24, 24, 24, 24);
    root->setSpacing(16);

    auto* topRow = new QHBoxLayout;
    topRow->setSpacing(16);

    auto* leftCol = new QVBoxLayout;
    leftCol->setSpacing(8);

    auto* title = new QLabel(QStringLiteral("计算助手"));
    title->setObjectName(QStringLiteral("PageTitle"));
    auto* subtitle = new QLabel(QStringLiteral("交互式运算  ·  可视化展示  ·  算法过程演示"));
    subtitle->setObjectName(QStringLiteral("PageSubtitle"));

    auto* tabBar = new QWidget;
    tabBar->setObjectName(QStringLiteral("CalcTabBar"));
    auto* tabLayout = new QHBoxLayout(tabBar);
    tabLayout->setContentsMargins(0, 4, 0, 4);
    tabLayout->setSpacing(6);

    auto makeTabBtn = [](const QString& text) {
        auto* btn = new QPushButton(text);
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
    group->addButton(btnInteractive, 0); group->addButton(btnVisualize, 1); group->addButton(btnDemo, 2);

    tabLayout->addWidget(btnInteractive); tabLayout->addWidget(btnVisualize); tabLayout->addWidget(btnDemo); tabLayout->addStretch(1);

    leftCol->addWidget(title); leftCol->addWidget(subtitle); leftCol->addWidget(tabBar);

    auto* symbolPanel = new QWidget;
    symbolPanel->setObjectName(QStringLiteral("CalcSymbolPanel"));
    auto* symGrid = new QGridLayout(symbolPanel);
    symGrid->setContentsMargins(2, 2, 2, 2);
    symGrid->setHorizontalSpacing(2);
    symGrid->setVerticalSpacing(2);

    auto* interactivePage = new Interactive::InteractivePage;

    auto symBtns = new QVector<QPushButton*>;
    const int cols = 11; const int total = sizeof(kGreeks) / sizeof(kGreeks[0]);
    for (int i = 0; i < total; ++i) {
        auto* b = new QPushButton;
        b->setFixedSize(30, 32); b->setIconSize(QSize(22, 22)); b->setCursor(Qt::PointingHandCursor); b->setFocusPolicy(Qt::NoFocus);
        b->setToolTip(QString::fromUtf8(kGreeks[i].label));
        const QString ch = QString::fromUtf8(kGreeks[i].ch);
        connect(b, &QPushButton::clicked, this, [interactivePage, ch]{ interactivePage->insertAtCursor(ch); });
        symGrid->addWidget(b, i / cols, i % cols);
        symBtns->append(b);
    }
    symbolPanel->setLayout(symGrid);

    auto* opsPanel = new QWidget;
    opsPanel->setObjectName(QStringLiteral("CalcOpsPanel"));
    auto* opsGrid = new QGridLayout(opsPanel);
    opsGrid->setContentsMargins(2, 2, 2, 2);
    opsGrid->setHorizontalSpacing(2);
    opsGrid->setVerticalSpacing(2);

    auto opBtns = new QVector<QPushButton*>;
    for (int i = 0; i < int(sizeof(kOps) / sizeof(kOps[0])); ++i) {
        auto* b = new QPushButton;
        b->setFixedSize(36, 32); b->setIconSize(QSize(28, 22)); b->setCursor(Qt::PointingHandCursor); b->setFocusPolicy(Qt::NoFocus);
        b->setToolTip(QString::fromUtf8(kOps[i].tip));
        const int kind = kOps[i].kind;
        connect(b, &QPushButton::clicked, this, [interactivePage, kind]{ if (kind == 0) interactivePage->insertSqrtTemplate(); else interactivePage->insertRootTemplate(); });
        opsGrid->addWidget(b, i, 0);
        opBtns->append(b);
    }
    opsPanel->setLayout(opsGrid);

    topRow->addLayout(leftCol, 1);
    topRow->addWidget(symbolPanel, 0, Qt::AlignRight);
    topRow->addWidget(opsPanel, 0, Qt::AlignRight);

    auto* stack = new QStackedWidget;
    stack->setObjectName(QStringLiteral("CalcStack"));
    stack->addWidget(interactivePage);
    stack->addWidget(new Visualize::VisualizePage);
    stack->addWidget(new Demo::DemoPage);

    connect(group, &QButtonGroup::idClicked, stack, &QStackedWidget::setCurrentIndex);

    root->addLayout(topRow);
    root->addWidget(stack, 1);

    auto applyTheme = [this, title, subtitle, btnInteractive, btnVisualize, btnDemo, symBtns, opBtns]() {
        bool isDark = AlgeMate::ThemeManager::instance().currentTheme() == AlgeMate::ThemeManager::Theme::Dark;

        title->setStyleSheet(isDark ? "font-size: 28px; font-weight: 900; color: #E6E7F0;" : "font-size: 28px; font-weight: 900; color: #111827;");
        subtitle->setStyleSheet(isDark ? "color: #8A8FA3;" : "color: #6b7280;");

        QString tabStyle = isDark
                               ? "QPushButton { background: transparent; border: none; padding: 6px 14px; border-radius: 8px; font-size: 14px; font-weight: 500; color: #7B7B96; } "
                                 "QPushButton:hover { background: #28263F; color: #E6E7F0; } "
                                 "QPushButton:checked { background: #312F4A; color: #8FA1FF; font-weight: bold; box-shadow: 0 2px 4px rgba(0,0,0,0.2); }"
                               : "QPushButton { background: transparent; border: none; padding: 6px 14px; border-radius: 8px; font-size: 14px; font-weight: 500; color: #64748b; } "
                                 "QPushButton:hover { background: #f1f5f9; color: #1e293b; } "
                                 "QPushButton:checked { background: #ffffff; color: #4f46e5; font-weight: bold; box-shadow: 0 1px 3px rgba(0,0,0,0.1); }";

        btnInteractive->setStyleSheet(tabStyle);
        btnVisualize->setStyleSheet(tabStyle);
        btnDemo->setStyleSheet(tabStyle);

        const QColor glyphColor = isDark ? QColor("#E6E7F0") : QColor("#24253D");
        const int glyphFontPt = 13;

        QString symBtnStyle = isDark
                                  ? "QPushButton { background: #1C1B2E; border: 1px solid #3B395A; border-radius: 4px; } QPushButton:hover { background: #28263F; border-color: #6F77FF; } QPushButton:pressed { background: #312F4A; }"
                                  : "QPushButton { background: #ffffff; border: 1px solid #e2e8f0; border-radius: 4px; } QPushButton:hover { background: #f8fafc; border-color: #cbd5e1; } QPushButton:pressed { background: #f1f5f9; }";

        for (int i = 0; i < symBtns->size(); ++i) {
            symBtns->at(i)->setStyleSheet(symBtnStyle);
            QPixmap px = renderGlyphPixmap(QString::fromUtf8(kGreeks[i].label), glyphFontPt, glyphColor);
            symBtns->at(i)->setIcon(QIcon(px));
        }

        for (int i = 0; i < opBtns->size(); ++i) {
            opBtns->at(i)->setStyleSheet(symBtnStyle);
            QPixmap px = renderGlyphPixmap(QString::fromUtf8(kOps[i].latex), glyphFontPt, glyphColor);
            opBtns->at(i)->setIcon(QIcon(px));
        }
    };

    applyTheme();
    connect(&AlgeMate::ThemeManager::instance(), &AlgeMate::ThemeManager::themeChanged, this, [applyTheme](AlgeMate::ThemeManager::Theme){ applyTheme(); });
}

} 
