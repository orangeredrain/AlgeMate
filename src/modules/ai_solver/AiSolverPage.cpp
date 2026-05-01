#include "AiSolverPage.h"

#include <QVBoxLayout>
#include <QLabel>
#include <QFrame>

namespace AlgeMate::AiSolver {

AiSolverPage::AiSolverPage(QWidget* parent) : QWidget(parent) {
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(24, 24, 24, 24);
    root->setSpacing(16);

    auto* title = new QLabel(QStringLiteral("AI 智能解题"));
    title->setObjectName(QStringLiteral("PageTitle"));
    auto* subtitle = new QLabel(QStringLiteral("输入题目，AI 分步讲解与求解"));
    subtitle->setObjectName(QStringLiteral("PageSubtitle"));

    auto* card = new QFrame;
    card->setObjectName(QStringLiteral("Card"));
    auto* cardLayout = new QVBoxLayout(card);
    cardLayout->setContentsMargins(32, 48, 32, 48);
    auto* placeholder = new QLabel(
        QStringLiteral("AI 智能解题模块\n\n"
                       "规划：题目输入区 / 截图识别 / 分步解答输出 / 解题思路追问"));
    placeholder->setObjectName(QStringLiteral("PlaceholderLabel"));
    placeholder->setAlignment(Qt::AlignCenter);
    placeholder->setWordWrap(true);
    cardLayout->addWidget(placeholder);

    root->addWidget(title);
    root->addWidget(subtitle);
    root->addWidget(card, 1);
}

}
