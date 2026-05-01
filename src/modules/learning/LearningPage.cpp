#include "LearningPage.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QFrame>

namespace AlgeMate::Learning {

static QFrame* makeStatCard(const QString& title, const QString& value, const QString& hint) {
    auto* card = new QFrame;
    card->setObjectName(QStringLiteral("Card"));
    card->setMinimumHeight(140);
    auto* lay = new QVBoxLayout(card);
    lay->setContentsMargins(20, 18, 20, 18);
    lay->setSpacing(6);

    auto* t = new QLabel(title);
    t->setStyleSheet("font-size:13px; color:#8A8FA3;");
    auto* v = new QLabel(value);
    v->setStyleSheet("font-size:28px; font-weight:700; color:#6A5AE0;");
    auto* h = new QLabel(hint);
    h->setStyleSheet("font-size:12px; color:#B4B8CC;");

    lay->addWidget(t);
    lay->addWidget(v);
    lay->addStretch();
    lay->addWidget(h);
    return card;
}

LearningPage::LearningPage(QWidget* parent) : QWidget(parent) {
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(24, 24, 24, 24);
    root->setSpacing(16);

    auto* title = new QLabel(QStringLiteral("学习中心"));
    title->setObjectName(QStringLiteral("PageTitle"));
    auto* subtitle = new QLabel(QStringLiteral("进度追踪 · 错题本 · 打卡 · 智能推荐"));
    subtitle->setObjectName(QStringLiteral("PageSubtitle"));

    auto* cardsRow = new QHBoxLayout;
    cardsRow->setSpacing(16);
    cardsRow->addWidget(makeStatCard(QStringLiteral("今日学习"),   QStringLiteral("0 分钟"), QStringLiteral("今日暂无记录")));
    cardsRow->addWidget(makeStatCard(QStringLiteral("本周进度"),   QStringLiteral("0%"),     QStringLiteral("目标 100%")));
    cardsRow->addWidget(makeStatCard(QStringLiteral("错题数"),     QStringLiteral("0"),      QStringLiteral("共收录")));
    cardsRow->addWidget(makeStatCard(QStringLiteral("推荐练习"),   QStringLiteral("—"),      QStringLiteral("待系统生成")));

    auto* card = new QFrame;
    card->setObjectName(QStringLiteral("Card"));
    auto* cardLayout = new QVBoxLayout(card);
    cardLayout->setContentsMargins(32, 48, 32, 48);
    auto* placeholder = new QLabel(
        QStringLiteral("学习中心模块\n\n"
                       "规划：图表（QtCharts 或自绘）/ 错题本 / 打卡日历"));
    placeholder->setObjectName(QStringLiteral("PlaceholderLabel"));
    placeholder->setAlignment(Qt::AlignCenter);
    placeholder->setWordWrap(true);
    cardLayout->addWidget(placeholder);

    root->addWidget(title);
    root->addWidget(subtitle);
    root->addLayout(cardsRow);
    root->addWidget(card, 1);
}

}
