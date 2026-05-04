#include "VisualizePage.h"

#include <QVBoxLayout>
#include <QLabel>
#include <QFrame>

namespace AlgeMate::Calculator::Visualize {

VisualizePage::VisualizePage(QWidget* parent) : QWidget(parent) {
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(0, 8, 0, 0);

    auto* card = new QFrame;
    card->setObjectName(QStringLiteral("Card"));
    auto* lay = new QVBoxLayout(card);
    lay->setContentsMargins(32, 64, 32, 64);
    lay->setAlignment(Qt::AlignCenter);

    auto* icon = new QLabel(QStringLiteral("🎨"));
    icon->setAlignment(Qt::AlignCenter);
    icon->setStyleSheet(QStringLiteral("font-size: 56px;"));

    auto* tip = new QLabel(QStringLiteral("可视化模块"));
    tip->setObjectName(QStringLiteral("PlaceholderLabel"));
    tip->setAlignment(Qt::AlignCenter);
    tip->setStyleSheet(QStringLiteral("font-size: 20px; font-weight: 600; color: #E6E7F0;"));

    auto* note = new QLabel(QStringLiteral(
        "规划: 线性变换几何可视化 · 特征向量动画 · 二次型曲面 · 秩与零空间图示"));
    note->setObjectName(QStringLiteral("PlaceholderLabel"));
    note->setAlignment(Qt::AlignCenter);
    note->setWordWrap(true);

    lay->addWidget(icon);
    lay->addSpacing(12);
    lay->addWidget(tip);
    lay->addSpacing(8);
    lay->addWidget(note);

    root->addWidget(card, 1);
}

}
