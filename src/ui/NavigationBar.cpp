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
    auto* item = new QListWidgetItem(QStringLiteral("  %1   %2").arg(icon, title), list_);
    item->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
}

void NavigationBar::setCurrentIndex(int index) {
    if (index >= 0 && index < list_->count())
        list_->setCurrentRow(index);
}

int NavigationBar::currentIndex() const {
    return list_->currentRow();
}

}
