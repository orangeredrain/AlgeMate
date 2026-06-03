#include "NavigationBar.h"
#include "core/ThemeManager.h"
#include <QGraphicsDropShadowEffect>

#include <QListWidget>
#include <QVBoxLayout>
#include <QLabel>

namespace AlgeMate {

NavigationBar::NavigationBar(QWidget* parent) : QWidget(parent) {
    setObjectName(QStringLiteral("NavigationBar"));
    setFixedWidth(240);
    buildUi();
}

void NavigationBar::buildUi() {
    setAttribute(Qt::WA_TranslucentBackground);
    setStyleSheet("background: transparent;");

    // 1. 创建内部悬浮的“玻璃态胶囊”容器
    auto* pillContainer = new QFrame(this);
    pillContainer->setObjectName("PillContainer");

    // 柔和清透的弥散阴影
    auto* shadow = new QGraphicsDropShadowEffect(this);
    shadow->setBlurRadius(28);
    shadow->setOffset(0, 10);
    pillContainer->setGraphicsEffect(shadow);

    auto* pillLayout = new QVBoxLayout(pillContainer);
    pillLayout->setContentsMargins(16, 40, 16, 24);
    pillLayout->setSpacing(10);

    // 2. Logo (Brand)
    auto* brand = new QLabel(QStringLiteral("AlgeMate"));
    brand->setObjectName(QStringLiteral("NavBrand"));
    brand->setAlignment(Qt::AlignCenter);
    pillLayout->addWidget(brand);
    pillLayout->addSpacing(20);

    // 3. 顶部主导航列表
    list_ = new QListWidget(pillContainer);
    list_->setObjectName(QStringLiteral("NavList"));
    list_->setFrameShape(QFrame::NoFrame);
    list_->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    list_->setSelectionMode(QAbstractItemView::SingleSelection);
    list_->setCursor(Qt::PointingHandCursor);
    pillLayout->addWidget(list_, 1); // 权重 1，占满上方空间

    // 4. 中间加入弹簧，把底部推到最下边
    pillLayout->addStretch(0);

    // 5. 底部独立列表（视觉上专门用来放“设置中心”）
    auto* bottomList = new QListWidget(pillContainer);
    bottomList->setObjectName(QStringLiteral("BottomList"));
    bottomList->setFrameShape(QFrame::NoFrame);
    bottomList->setSelectionMode(QAbstractItemView::SingleSelection);
    bottomList->setCursor(Qt::PointingHandCursor);
    bottomList->setFixedHeight(56); // 刚刚好容纳一个选项的高度
    pillLayout->addWidget(bottomList);

    // 最外层布局悬浮
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(20, 20, 10, 20);
    root->addWidget(pillContainer);

    // ==============================================================
    // [动态主题切换逻辑]：使用 Lambda 绑定深浅色不同样式
    // ==============================================================
    auto applyTheme = [=]() {
        bool isDark = ThemeManager::instance().currentTheme() == ThemeManager::Theme::Dark;

        if (isDark) {
            // 【深色模式】：暗夜紫，降低亮度和刺眼的白边
            pillContainer->setStyleSheet(R"(
                QFrame#PillContainer {
                    background: qlineargradient(x1: 0, y1: 1, x2: 1, y2: 0,
                                                stop: 0 rgba(42, 32, 85, 0.75),
                                                stop: 1 rgba(85, 70, 145, 0.75));
                    border-top: 1px solid rgba(255, 255, 255, 0.20);
                    border-left: 1px solid rgba(255, 255, 255, 0.12);
                    border-bottom: 1px solid rgba(0, 0, 0, 0.50);
                    border-right: 1px solid rgba(0, 0, 0, 0.40);
                    border-radius: 32px;
                }
            )");
            shadow->setColor(QColor(10, 5, 25, 180)); // 深色阴影
            brand->setStyleSheet("color: rgba(255, 255, 255, 0.9); font-size: 24px; font-weight: 900; background: transparent; border: none;");

            QString darkListStyle = R"(
                QListWidget { background: transparent; border: none; outline: none; }
                QListWidget::item { color: rgba(255, 255, 255, 0.70); border-radius: 16px; padding: 8px 10px; margin-bottom: 8px; }
                QListWidget::item:hover { background-color: rgba(255, 255, 255, 0.08); color: rgba(255, 255, 255, 0.95); }
                QListWidget::item:selected { background-color: rgba(255, 255, 255, 0.15); border: 1px solid rgba(255, 255, 255, 0.20); color: white; font-weight: bold; }
            )";
            list_->setStyleSheet(darkListStyle);
            bottomList->setStyleSheet(darkListStyle);

        } else {
            // 【浅色模式】：高亮冰透紫
            pillContainer->setStyleSheet(R"(
                QFrame#PillContainer {
                    background: qlineargradient(x1: 0, y1: 1, x2: 1, y2: 0,
                                                stop: 0 rgba(106, 90, 224, 0.85),
                                                stop: 1 rgba(210, 195, 255, 0.90));
                    border-top: 1px solid rgba(255, 255, 255, 0.6);
                    border-left: 1px solid rgba(255, 255, 255, 0.4);
                    border-bottom: 1px solid rgba(0, 0, 0, 0.15);
                    border-right: 1px solid rgba(0, 0, 0, 0.15);
                    border-radius: 32px;
                }
            )");
            shadow->setColor(QColor(80, 60, 180, 90)); // 浅色阴影
            brand->setStyleSheet("color: white; font-size: 24px; font-weight: 900; background: transparent; border: none;");

            QString lightListStyle = R"(
                QListWidget { background: transparent; border: none; outline: none; }
                QListWidget::item { color: rgba(255, 255, 255, 0.85); border-radius: 16px; padding: 8px 10px; margin-bottom: 8px; }
                QListWidget::item:hover { background-color: rgba(255, 255, 255, 0.2); color: white; }
                QListWidget::item:selected { background-color: rgba(255, 255, 255, 0.3); border: 1px solid rgba(255, 255, 255, 0.5); color: white; font-weight: bold; }
            )";
            list_->setStyleSheet(lightListStyle);
            bottomList->setStyleSheet(lightListStyle);
        }
    };

    applyTheme();
    connect(&ThemeManager::instance(), &ThemeManager::themeChanged, this,
            [applyTheme](ThemeManager::Theme){ applyTheme(); });


    // ---------- 信号与核心联动逻辑 ---------- //

    connect(list_, &QListWidget::currentRowChanged, this, &NavigationBar::navigated);

    // 同步更新 UI 的高亮状态
    connect(list_, &QListWidget::currentRowChanged, this, [this](int row) {
        auto* bList = this->findChild<QListWidget*>(QStringLiteral("BottomList"));
        if (!bList) return;

        auto* item = list_->item(row);
        if (item && item->isHidden()) {
            bList->blockSignals(true);
            bList->setCurrentRow(0);
            bList->blockSignals(false);
        } else {
            bList->blockSignals(true);
            bList->clearSelection();
            bList->blockSignals(false);
        }
    });

    // 映射回主列表的对应选项
    connect(bottomList, &QListWidget::itemClicked, this, [this]() {
        for (int i = 0; i < list_->count(); ++i) {
            if (list_->item(i)->isHidden()) {
                list_->setCurrentRow(i);
                break;
            }
        }
    });
}

void NavigationBar::addNavItem(const QString& icon, const QString& title) {
    auto* item = new QListWidgetItem(list_);
    item->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    QFont font = item->font();

    if (title.contains(QStringLiteral("设置"))) {
        // 在主列表里藏起来
        item->setText(title);
        item->setHidden(true);

        // 把它克隆显示在底侧列表中
        auto* bList = this->findChild<QListWidget*>(QStringLiteral("BottomList"));
        if (bList) {
            auto* bItem = new QListWidgetItem(bList);
            bItem->setText(QStringLiteral("  ⚙️   %1").arg(title));
            font.setPixelSize(14);
            bItem->setFont(font);
            bItem->setSizeHint(QSize(0, 48));
            bItem->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        }
    } else {
        // 普通项正常显示在顶部
        item->setText(QStringLiteral("  %1   %2").arg(icon, title));
        font.setPixelSize(14);
        item->setFont(font);
        item->setSizeHint(QSize(0, 48));
    }
}

void NavigationBar::setCurrentIndex(int index) {
    if (index >= 0 && index < list_->count())
        list_->setCurrentRow(index);
}

int NavigationBar::currentIndex() const {
    return list_->currentRow();
}

} // namespace AlgeMate