#include "KnowledgePage.h"

#include <QVBoxLayout>
#include <QLabel>
#include <QFrame>

namespace AlgeMate::Knowledge {

KnowledgePage::KnowledgePage(QWidget* parent) : QWidget(parent) {
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(24, 24, 24, 24);
    root->setSpacing(16);

    auto* title = new QLabel(QStringLiteral("知识点学习"));
    title->setObjectName(QStringLiteral("PageTitle"));
    auto* subtitle = new QLabel(QStringLiteral("章节目录 · 图文讲解 · 经典例题"));
    subtitle->setObjectName(QStringLiteral("PageSubtitle"));

    auto* card = new QFrame;
    card->setObjectName(QStringLiteral("Card"));
    auto* cardLayout = new QVBoxLayout(card);
    cardLayout->setContentsMargins(32, 48, 32, 48);
    auto* placeholder = new QLabel(
        QStringLiteral("知识点学习模块\n\n"
                       "规划：章节 / Markdown 渲染 / 公式图片 / 例题与思考题"));
    placeholder->setObjectName(QStringLiteral("PlaceholderLabel"));
    placeholder->setAlignment(Qt::AlignCenter);
    placeholder->setWordWrap(true);
    cardLayout->addWidget(placeholder);

    root->addWidget(title);
    root->addWidget(subtitle);
    root->addWidget(card, 1);
}

}
