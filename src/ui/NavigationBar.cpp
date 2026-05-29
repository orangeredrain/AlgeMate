#include "NavigationBar.h"

#include <QListWidget>
#include <QVBoxLayout>
#include <QLabel>

namespace AlgeMate {

NavigationBar::NavigationBar(QWidget* parent) : QWidget(parent) {
    setObjectName(QStringLiteral("NavigationBar"));
    setAttribute(Qt::WA_StyledBackground, true);
    setFixedWidth(220);
    buildUi();
}

void NavigationBar::buildUi() {
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    auto* brand = new QLabel(QStringLiteral("AlgeMate"));
    brand->setObjectName(QStringLiteral("NavBrand"));

    list_ = new QListWidget(this);
    list_->setObjectName(QStringLiteral("NavList"));
    list_->setFrameShape(QFrame::NoFrame);
    list_->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    list_->setSelectionMode(QAbstractItemView::SingleSelection);
    list_->setCursor(Qt::PointingHandCursor);

    root->addWidget(brand);
    root->addWidget(list_, 1);

    connect(list_, &QListWidget::currentRowChanged,
            this, &NavigationBar::navigated);
}

void NavigationBar::addNavItem(const QString& icon, const QString& title) {
    auto* item = new QListWidgetItem(list_);
    item->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);

    QFont font = item->font();

    // 判断当前添加的导航项是不是“设置中心”
    if (title.contains(QStringLiteral("设置"))) {
        // 【修改这里】：无视外面传进来的图标，直接强行写死 ⚙️ 加上6个空格，然后再拼接文字(title)
        item->setText(QStringLiteral("  ⚙️   %1").arg(title));

        font.setPixelSize(14);           // 将字号调大 (Emoji图标会跟随字号同比例放大)
        item->setFont(font);
    } else {
        // 其他选项保持原样
        item->setText(QStringLiteral("  %1   %2").arg(icon, title));

        font.setPixelSize(14);           // 其他默认导航选项的字号
        item->setFont(font);
        item->setSizeHint(QSize(0, 46)); // 其他选项的默认高度
    }
}

void NavigationBar::setCurrentIndex(int index) {
    if (index >= 0 && index < list_->count())
        list_->setCurrentRow(index);
}

int NavigationBar::currentIndex() const {
    return list_->currentRow();
}

}
