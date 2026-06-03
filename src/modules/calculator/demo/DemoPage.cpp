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
#include "core/ThemeManager.h"

#include <QFrame>
#include <QGridLayout>
#include <QLabel>
#include <QScrollArea>
#include <QStackedWidget>
#include <QVBoxLayout>
#include <QGraphicsDropShadowEffect>
#include <QHash>
#include <QPainter>
#include <QPainterPath>
#include <QMouseEvent>
#include <cmath>
#include <functional> // 引入 std::function

using AlgeMate::Calculator::Interactive::RenderTheme;

namespace AlgeMate::Calculator::Demo {

namespace {

bool isLatex(const QString& s) {
    return s.startsWith(QLatin1Char('$')) && s.endsWith(QLatin1Char('$')) && s.size() >= 2;
}

// 核心黑科技：继承自 QWidget，彻底摆脱全局 QPushButton 样式的裁剪！
class BubbleWidget : public QWidget {
public:
    BubbleWidget(int size, const QString& icon, const QString& title, const QString& desc, std::function<void()> onClick, QWidget* parent = nullptr)
        : QWidget(parent), m_size(size), m_icon(icon), m_title(title), m_desc(desc), m_onClick(onClick) {

        setObjectName(QStringLiteral("DemoBubbleWidget"));
        setFixedSize(size, size);
        setCursor(Qt::PointingHandCursor);
        setAttribute(Qt::WA_Hover, true); // 确保悬浮事件生效
        setAttribute(Qt::WA_TranslucentBackground);

        // 投影渲染（悬浮弥散感）
        shadow = new QGraphicsDropShadowEffect(this);
        shadow->setBlurRadius(32);
        shadow->setOffset(0, 12);
        setGraphicsEffect(shadow);

        updateTheme();
    }

    void updateTheme() {
        const auto th = RenderTheme::forCurrent();
        if (isLatex(m_icon)) m_iconPix = renderLatex(m_icon.mid(1, m_icon.size() - 2), th, 20);
        if (isLatex(m_title)) m_titlePix = renderLatex(m_title.mid(1, m_title.size() - 2), th, 12);

        bool isDark = AlgeMate::ThemeManager::instance().currentTheme() == AlgeMate::ThemeManager::Theme::Dark;
        shadow->setColor(isDark ? QColor(0, 0, 0, 200) : QColor(90, 110, 200, 50));
        update();
    }

protected:
    // 监听鼠标点击，完美替代 QPushButton 的 clicked 信号
    void mouseReleaseEvent(QMouseEvent* event) override {
        if (event->button() == Qt::LeftButton && rect().contains(event->pos())) {
            if (m_onClick) m_onClick();
        }
        QWidget::mouseReleaseEvent(event);
    }

    // 监听悬浮状态触发重绘
    bool event(QEvent* e) override {
        if (e->type() == QEvent::HoverEnter || e->type() == QEvent::HoverLeave) {
            update();
        }
        return QWidget::event(e);
    }

    void paintEvent(QPaintEvent* event) override {
        Q_UNUSED(event);
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);
        p.setRenderHint(QPainter::SmoothPixmapTransform);
        p.setRenderHint(QPainter::TextAntialiasing);

        bool isDark = AlgeMate::ThemeManager::instance().currentTheme() == AlgeMate::ThemeManager::Theme::Dark;
        bool hover = underMouse();

        // 1. 绘制逼真的多层渐变肥皂泡底色
        QRectF rect(0, 0, m_size, m_size);
        QRadialGradient grad(rect.width() * 0.3, rect.height() * 0.2, rect.width() * 0.85);
        if (isDark) {
            if (hover) {
                grad.setColorAt(0, QColor(140, 130, 255, 90));
                grad.setColorAt(0.7, QColor(45, 42, 75, 200));
                grad.setColorAt(1, QColor(100, 120, 255, 160));
            } else {
                grad.setColorAt(0, QColor(120, 110, 240, 50));
                grad.setColorAt(0.7, QColor(30, 28, 50, 180));
                grad.setColorAt(1, QColor(80, 100, 230, 130));
            }
        } else {
            if (hover) {
                grad.setColorAt(0, QColor(255, 255, 255, 255));
                grad.setColorAt(0.7, QColor(240, 248, 255, 200));
                grad.setColorAt(1, QColor(170, 190, 255, 240));
            } else {
                grad.setColorAt(0, QColor(255, 255, 255, 255));
                grad.setColorAt(0.7, QColor(230, 240, 255, 140));
                grad.setColorAt(1, QColor(150, 180, 255, 190));
            }
        }
        p.setBrush(grad);

        // 玻璃高光边框
        QPen pen;
        pen.setWidthF(hover ? 4.0 : 2.5);
        if (isDark) pen.setColor(hover ? QColor(143, 161, 255, 220) : QColor(130, 145, 255, 90));
        else pen.setColor(hover ? QColor(79, 70, 229, 160) : QColor(255, 255, 255, 250));
        p.setPen(pen);
        p.drawEllipse(rect.adjusted(2, 2, -2, -2));

        // 2. 依次渲染内部元素
        int cx = m_size / 2;
        int currentY = m_size * 0.18; // 从气泡顶部 18% 处开始往下排

        // 渲染公式/图标
        if (!m_iconPix.isNull()) {
            int logicW = m_iconPix.width() / m_iconPix.devicePixelRatio();
            int logicH = m_iconPix.height() / m_iconPix.devicePixelRatio();
            p.drawPixmap(cx - logicW / 2, currentY, m_iconPix);
            currentY += logicH + 8;
        } else {
            p.setPen(isDark ? QColor("#E6E7F0") : QColor("#111827"));
            QFont f = p.font(); f.setPixelSize(24); f.setBold(true); p.setFont(f);
            QFontMetrics fm(f);
            p.drawText(cx - fm.horizontalAdvance(m_icon)/2, currentY + fm.ascent(), m_icon);
            currentY += fm.height() + 8;
        }

        // 渲染标题
        if (!m_titlePix.isNull()) {
            int logicW = m_titlePix.width() / m_titlePix.devicePixelRatio();
            int logicH = m_titlePix.height() / m_titlePix.devicePixelRatio();
            p.drawPixmap(cx - logicW / 2, currentY, m_titlePix);
            currentY += logicH + 10;
        } else {
            p.setPen(isDark ? QColor("#E6E7F0") : QColor("#111827"));
            QFont f = p.font(); f.setPixelSize(14); f.setBold(true); p.setFont(f);
            QFontMetrics fm(f);
            p.drawText(cx - fm.horizontalAdvance(m_title)/2, currentY + fm.ascent(), m_title);
            currentY += fm.height() + 10;
        }

        // 3. 【核心技术】沿着圆形内壁严格包裹绘制说明文字
        p.setPen(isDark ? QColor(150, 155, 175, 240) : QColor(90, 105, 125, 240));
        QFont fDesc = p.font(); fDesc.setPixelSize(11); fDesc.setBold(false); p.setFont(fDesc);
        drawCircularWrappedText(p, m_desc, rect, currentY);
    }

private:
    int m_size;
    QString m_icon, m_title, m_desc;
    QPixmap m_iconPix, m_titlePix;
    QGraphicsDropShadowEffect* shadow;
    std::function<void()> m_onClick;

    // 圆形内壁包裹折行算法
    void drawCircularWrappedText(QPainter& p, const QString& text, const QRectF& bounds, int startY) {
        int R = bounds.width() / 2;
        int cx = bounds.center().x();
        int cy = bounds.center().y();
        int R_safe = R - 14;

        QFontMetrics fm = p.fontMetrics();
        int lh = fm.lineSpacing();
        int y = startY;

        QStringList tokens;
        QString currentToken;
        for (QChar c : text) {
            if (c.unicode() >= 0x4E00 && c.unicode() <= 0x9FA5) {
                if (!currentToken.isEmpty()) { tokens << currentToken; currentToken.clear(); }
                tokens << QString(c);
            } else if (c.isSpace()) {
                if (!currentToken.isEmpty()) { tokens << currentToken; currentToken.clear(); }
                tokens << " ";
            } else {
                currentToken += c;
            }
        }
        if (!currentToken.isEmpty()) tokens << currentToken;

        QString line;
        for (const QString& token : tokens) {
            int dy = (y + lh / 2) - cy;
            int availW = 0;
            if (qAbs(dy) < R_safe) {
                availW = 2 * std::sqrt(R_safe * R_safe - dy * dy);
            }

            if (fm.horizontalAdvance(line + token) <= availW) {
                line += token;
            } else {
                QString drawStr = line.trimmed();
                if (!drawStr.isEmpty()) {
                    p.drawText(cx - fm.horizontalAdvance(drawStr) / 2, y + fm.ascent(), drawStr);
                    y += lh + 2;
                }
                line = token;
                if (line == " ") line = "";
            }
        }
        QString drawStr = line.trimmed();
        if (!drawStr.isEmpty()) {
            p.drawText(cx - fm.horizontalAdvance(drawStr) / 2, y + fm.ascent(), drawStr);
        }
    }
};

} // anonymous namespace


DemoPage::DemoPage(QWidget* parent) : QWidget(parent) {
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    stack_ = new QStackedWidget;
    buildCatalog();

    auto* homoPage = new HomoLinearSystemPage;
    connect(homoPage, &HomoLinearSystemPage::backRequested, this, [this]{ stack_->setCurrentIndex(0); });
    stack_->addWidget(homoPage);

    auto* nonhomoPage = new NonhomoLinearSystemPage;
    connect(nonhomoPage, &NonhomoLinearSystemPage::backRequested, this, [this]{ stack_->setCurrentIndex(0); });
    stack_->addWidget(nonhomoPage);

    auto* maxIndepPage = new MaxIndepPage;
    connect(maxIndepPage, &MaxIndepPage::backRequested, this, [this]{ stack_->setCurrentIndex(0); });
    stack_->addWidget(maxIndepPage);

    auto* inversePage = new InversePage;
    connect(inversePage, &InversePage::backRequested, this, [this]{ stack_->setCurrentIndex(0); });
    stack_->addWidget(inversePage);

    auto* gsoPage = new GSOPage;
    connect(gsoPage, &GSOPage::backRequested, this, [this]{ stack_->setCurrentIndex(0); });
    stack_->addWidget(gsoPage);

    auto* eigenPage = new EigenPage;
    connect(eigenPage, &EigenPage::backRequested, this, [this]{ stack_->setCurrentIndex(0); });
    stack_->addWidget(eigenPage);

    auto* symDiagPage = new SymDiagPage;
    connect(symDiagPage, &SymDiagPage::backRequested, this, [this]{ stack_->setCurrentIndex(0); });
    stack_->addWidget(symDiagPage);

    auto* quadFormPage = new QuadFormPage;
    connect(quadFormPage, &QuadFormPage::backRequested, this, [this]{ stack_->setCurrentIndex(0); });
    stack_->addWidget(quadFormPage);

    auto* polyGCDPage = new PolyGCDPage;
    connect(polyGCDPage, &PolyGCDPage::backRequested, this, [this]{ stack_->setCurrentIndex(0); });
    stack_->addWidget(polyGCDPage);

    auto* symReducePage = new SymReducePage;
    connect(symReducePage, &SymReducePage::backRequested, this, [this]{ stack_->setCurrentIndex(0); });
    stack_->addWidget(symReducePage);

    auto* jordanFormPage = new JordanFormPage;
    connect(jordanFormPage, &JordanFormPage::backRequested, this, [this]{ stack_->setCurrentIndex(0); });
    stack_->addWidget(jordanFormPage);

    root->addWidget(stack_, 1);
}

void DemoPage::buildCatalog() {
    auto* page = new QWidget;
    auto* outer = new QVBoxLayout(page);
    outer->setContentsMargins(40, 40, 40, 100);
    outer->setSpacing(10);

    auto* titleLbl = new QLabel(QStringLiteral("算法演示"));
    titleLbl->setObjectName(QStringLiteral("PageTitle"));
    auto* subtitleLbl = new QLabel(QStringLiteral("选择一个算法，查看详细的分步求解过程"));
    subtitleLbl->setObjectName(QStringLiteral("PageSubtitle"));
    outer->addWidget(titleLbl, 0, Qt::AlignHCenter);
    outer->addWidget(subtitleLbl, 0, Qt::AlignHCenter);
    outer->addSpacing(30);

    // 【核心】不使用 Layout，直接创建一个容器用于绝对定位
    auto* clusterContainer = new QWidget(page);

    struct AlgItem { QString icon, title, desc; int pageIdx; };
    AlgItem items[] = {
        {QStringLiteral("$AX=0$"), QStringLiteral("解齐次线性方程组"), QStringLiteral("高斯消元 → RREF → 基础解系"), 1},
        {QStringLiteral("$AX=b$"), QStringLiteral("解非齐次线性方程组"), QStringLiteral("高斯消元 → RREF → 特解"), 2},
        {QStringLiteral("$\\operatorname{rank}(A)$"), QStringLiteral("求极大无关组"), QStringLiteral("向量组 → 列矩阵 → 行变换"), 3},
        {QStringLiteral("$A^{-1}$"), QStringLiteral("矩阵求逆"), QStringLiteral("(A, I) → 初等行变换 → (I, A⁻¹)"), 4},
        {QStringLiteral("$\\alpha\\perp\\beta$"), QStringLiteral("Schmidt正交化"), QStringLiteral("极大无关组 → 正交化 → 单位化"), 5},
        {QStringLiteral("$A\\xi = \\lambda \\xi$"), QStringLiteral("特征值与特征向量"), QStringLiteral("|λI-A| → 特征值 → 特征向量"), 6},
        {QStringLiteral("$T^{-1}AT=\\Lambda$"), QStringLiteral("实对称矩阵对角化"), QStringLiteral("特征值 → 特征向量 → 正交化"), 7},
        {QStringLiteral("$f=X^{T}AX$"), QStringLiteral("实二次型化标准形"), QStringLiteral("成对初等变换 → 合同对角化"), 8},
        {QStringLiteral("$\\gcd(f,g)$"), QStringLiteral("多项式最大公因式"), QStringLiteral("辗转相除法 → 倍式和表示"), 9},
        {QStringLiteral("$g(\\sigma_i)$"), QStringLiteral("对称多项式化简"), QStringLiteral("字典序降次 / 待定系数法"), 10},
        {QStringLiteral("$P^{-1}AP=J$"), QStringLiteral("Jordan 标准形"), QStringLiteral("行列式因子 → 不变因子"), 11}
    };

    struct PlacedBubble { BubbleWidget* w; int cx; int cy; int r; };
    QList<PlacedBubble> placed;

    int minX = 0, minY = 0, maxX = 0, maxY = 0;

    for (int i = 0; i < 11; ++i) {
        uint h = qHash(items[i].title) ^ qHash(items[i].desc);
        // 大小随机化，但进一步缩小下限让整体更聚拢
        int size = 145 + (h % 35);
        int r = size / 2;

        auto onClick = [this, pageIdx = items[i].pageIdx]() { stack_->setCurrentIndex(pageIdx); };
        auto* bubble = new BubbleWidget(size, items[i].icon, items[i].title, items[i].desc, onClick, clusterContainer);

        // 【黑科技：阿基米德螺旋线碰撞算法】
        // 从中心开始，随机选一个起始角度，沿着螺旋线向外探测，找到第一个不重叠的空隙就塞进去。
        double angle = (h % 360) * (M_PI / 180.0);
        double dist = 0.0;
        int cx = 0, cy = 0;
        bool collides = true;

        while (collides) {
            cx = dist * std::cos(angle);
            cy = dist * std::sin(angle);
            collides = false;

            for (const auto& p : placed) {
                double d = std::hypot(cx - p.cx, cy - p.cy);
                // "8" 是气泡间的极限缝隙（像素），数字越小贴得越紧
                if (d < (r + p.r + 8)) {
                    collides = true;
                    break;
                }
            }
            if (collides) {
                dist += 2.0;    // 每次向外扩2像素
                angle += 0.45;  // 螺旋旋转
            }
        }

        placed.append({bubble, cx, cy, r});

        // 记录外包围盒的边界
        minX = std::min(minX, cx - r);
        minY = std::min(minY, cy - r);
        maxX = std::max(maxX, cx + r);
        maxY = std::max(maxY, cy + r);
    }

    // 给阴影流出足够的安全边距（防止被容器切割）
    int padding = 45;
    clusterContainer->setFixedSize(maxX - minX + padding * 2, maxY - minY + padding * 2);

    // 计算出容器大小后，将所有气泡移动到正数坐标轴的位置
    for (const auto& p : placed) {
        int finalX = p.cx - p.r - minX + padding;
        int finalY = p.cy - p.r - minY + padding;
        p.w->move(finalX, finalY);
    }

    outer->addWidget(clusterContainer, 0, Qt::AlignHCenter);
    outer->addStretch(1);

    auto* scrollArea = new QScrollArea;
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setWidget(page);
    stack_->addWidget(scrollArea);

    auto applyTheme = [this, titleLbl, subtitleLbl]() {
        bool isDark = AlgeMate::ThemeManager::instance().currentTheme() == AlgeMate::ThemeManager::Theme::Dark;
        titleLbl->setStyleSheet(isDark ? "font-size: 32px; font-weight: 900; color: #E6E7F0;" : "font-size: 32px; font-weight: 900; color: #111827;");
        subtitleLbl->setStyleSheet(isDark ? "font-size: 15px; color: #8A8FA3;" : "font-size: 15px; color: #64748B;");

        QList<QWidget*> btns = this->findChildren<QWidget*>(QStringLiteral("DemoBubbleWidget"));
        for (auto* btn : btns) {
            static_cast<BubbleWidget*>(btn)->updateTheme();
        }
    };

    applyTheme();
    connect(&AlgeMate::ThemeManager::instance(), &AlgeMate::ThemeManager::themeChanged, this, [applyTheme](AlgeMate::ThemeManager::Theme){ applyTheme(); });
}

void DemoPage::addCard(QWidget* grid, int row, int col,
                       const QString& icon, const QString& title,
                       const QString& desc, int pageIndex) {

    uint h = qHash(title) ^ qHash(desc);

    // 1. 稍微缩小气泡的基础尺寸上限（原为 165+70=235，现改为 150+40=190）
    // 这样不仅排列更紧凑，也显得更精致
    int size = 150 + (h % 40);

    auto onClick = [this, pageIndex]() { stack_->setCurrentIndex(pageIndex); };
    auto* bubble = new BubbleWidget(size, icon, title, desc, onClick, grid);

    QWidget* wrapper = new QWidget(grid);
    auto* wlay = new QVBoxLayout(wrapper);

    // 2. 减小中间列的错位下沉量（原为 120，现改为 50，让行与行贴合更紧密）
    int staggerY = (col == 1) ? 50 : 0;

    // 3. 减小随机散布的偏移范围（原为 -40 到 40，现改为 -15 到 15）
    int dx = (h % 30) - 15;
    int dy = ((h >> 3) % 30) - 15;

    // 4. 精确控制边距（原统设为50）：
    // 左右上方留 20px 即可；但底部必须留 35px，否则悬浮时的下落阴影(Offset 12 + Blur 32)会被 wrapper 强行裁剪。
    int marginLeft   = 20 + dx;
    int marginTop    = 20 + staggerY + dy;
    int marginRight  = 20 - dx;
    int marginBottom = 35 - dy;

    wlay->setContentsMargins(marginLeft, marginTop, marginRight, marginBottom);
    wlay->addWidget(bubble, 0, Qt::AlignCenter);

    auto* gridLay = qobject_cast<QGridLayout*>(grid->layout());
    if (gridLay) {
        gridLay->addWidget(wrapper, row, col);
    }
}

}