#include "CalculatorPage.h"

#include <QVBoxLayout>
#include <QLabel>
#include <QFrame>

namespace AlgeMate::Calculator {

CalculatorPage::CalculatorPage(QWidget* parent) : QWidget(parent) {
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(24, 24, 24, 24);
    root->setSpacing(16);

    auto* title = new QLabel(QStringLiteral("计算助手"));
    title->setObjectName(QStringLiteral("PageTitle"));
    auto* subtitle = new QLabel(QStringLiteral("矩阵运算 · 方程组求解 · 行列式 / 秩 / 特征值"));
    subtitle->setObjectName(QStringLiteral("PageSubtitle"));

    auto* card = new QFrame;
    card->setObjectName(QStringLiteral("Card"));
    auto* cardLayout = new QVBoxLayout(card);
    cardLayout->setContentsMargins(32, 48, 32, 48);
    auto* placeholder = new QLabel(
        QStringLiteral("计算助手模块\n\n"
                       "规划：矩阵输入 / 基础与高级运算 / 结果展示等等"));
    placeholder->setObjectName(QStringLiteral("PlaceholderLabel"));
    placeholder->setAlignment(Qt::AlignCenter);
    placeholder->setWordWrap(true);
    cardLayout->addWidget(placeholder);

    root->addWidget(title);
    root->addWidget(subtitle);
    root->addWidget(card, 1);
}

}
