#include "PracticePage.h"
#include "ClickableCard.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QPushButton>
#include <QFrame>

namespace AlgeMate::Learning {

// helpers

static QPushButton* makeBackBtn(QWidget* parent = nullptr) {
    auto* btn = new QPushButton(QStringLiteral("← 返回"), parent);
    btn->setObjectName(QStringLiteral("LearnBackBtn"));
    return btn;
}

static ClickableCard* makeCardItem(const QString& icon, const QString& title,
                                    const QString& desc, QWidget* parent)
{
    auto* card = new ClickableCard(parent);
    card->setMinimumHeight(130);
    auto* lay = new QVBoxLayout(card);
    lay->setContentsMargins(24, 20, 24, 18);
    lay->setSpacing(6);

    auto* ic = new QLabel(icon);
    ic->setStyleSheet("font-size:32px; background:transparent;");
    auto* t  = new QLabel(title);
    t->setStyleSheet("font-size:17px; font-weight:700; background:transparent;");
    auto* d  = new QLabel(desc);
    d->setStyleSheet("font-size:13px; background:transparent;");
    d->setWordWrap(true);

    lay->addWidget(ic);
    lay->addWidget(t);
    lay->addStretch();
    lay->addWidget(d);
    return card;
}

// PracticePage

PracticePage::PracticePage(QWidget* parent) : QWidget(parent)
{
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(24, 16, 24, 24);
    root->setSpacing(16);

    auto* top = new QHBoxLayout;
    auto* back = makeBackBtn(this);
    connect(back, &QPushButton::clicked, this, &PracticePage::backRequested);
    auto* title = new QLabel(QStringLiteral("练习模式"));
    title->setObjectName(QStringLiteral("PageTitle"));
    top->addWidget(back);
    top->addWidget(title);
    top->addStretch();

    auto* grid = new QGridLayout;
    grid->setSpacing(16);

    auto* card1 = makeCardItem(QStringLiteral("🔢"), QStringLiteral("计算题"),
                                QStringLiteral("随机生成计算题\n实时输入矩阵并计算"), this);
    auto* card2 = makeCardItem(QStringLiteral("📖"), QStringLiteral("对应章节练习"),
                                QStringLiteral("按当前章节针对性训练\n与知识点章节绑定"), this);
    auto* card3 = makeCardItem(QStringLiteral("🎯"), QStringLiteral("专题模式"),
                                QStringLiteral("跨章节综合训练\n特征值 · 线性空间 · 综合矩阵"), this);

    connect(card1, &ClickableCard::clicked, this, &PracticePage::calculationProblemRequested);
    connect(card2, &ClickableCard::clicked, this, &PracticePage::chapterPracticeRequested);
    connect(card3, &ClickableCard::clicked, this, &PracticePage::topicPracticeRequested);

    grid->addWidget(card1, 0, 0);
    grid->addWidget(card2, 0, 1);
    grid->addWidget(card3, 1, 0);

    root->addLayout(top);
    root->addLayout(grid);
    root->addStretch();
}

// CalculationProblemPage

CalculationProblemPage::CalculationProblemPage(QWidget* parent) : QWidget(parent)
{
    auto* lay = new QVBoxLayout(this);
    lay->setContentsMargins(24, 16, 24, 24);
    lay->setSpacing(14);

    auto* top = new QHBoxLayout;
    auto* back = makeBackBtn(this);
    connect(back, &QPushButton::clicked, this, &CalculationProblemPage::backRequested);
    auto* t = new QLabel(QStringLiteral("计算题练习"));
    t->setObjectName(QStringLiteral("PageTitle"));
    top->addWidget(back);
    top->addWidget(t);
    top->addStretch();

    auto* ph = new QLabel(QStringLiteral("（计算题功能开发中...）"));
    ph->setAlignment(Qt::AlignCenter);
    ph->setObjectName(QStringLiteral("PlaceholderLabel"));

    lay->addLayout(top);
    lay->addWidget(ph, 1);
}

// ChapterPracticePage

ChapterPracticePage::ChapterPracticePage(QWidget* parent) : QWidget(parent)
{
    auto* lay = new QVBoxLayout(this);
    lay->setContentsMargins(24, 16, 24, 24);
    lay->setSpacing(14);

    auto* top = new QHBoxLayout;
    auto* back = makeBackBtn(this);
    connect(back, &QPushButton::clicked, this, &ChapterPracticePage::backRequested);
    auto* t = new QLabel(QStringLiteral("对应章节练习"));
    t->setObjectName(QStringLiteral("PageTitle"));
    top->addWidget(back);
    top->addWidget(t);
    top->addStretch();

    auto* ph = new QLabel(QStringLiteral("（章节练习功能开发中...）"));
    ph->setAlignment(Qt::AlignCenter);
    ph->setObjectName(QStringLiteral("PlaceholderLabel"));

    lay->addLayout(top);
    lay->addWidget(ph, 1);
}

// TopicPracticePage

TopicPracticePage::TopicPracticePage(QWidget* parent) : QWidget(parent)
{
    auto* lay = new QVBoxLayout(this);
    lay->setContentsMargins(24, 16, 24, 24);
    lay->setSpacing(14);

    auto* top = new QHBoxLayout;
    auto* back = makeBackBtn(this);
    connect(back, &QPushButton::clicked, this, &TopicPracticePage::backRequested);
    auto* t = new QLabel(QStringLiteral("专题练习"));
    t->setObjectName(QStringLiteral("PageTitle"));
    top->addWidget(back);
    top->addWidget(t);
    top->addStretch();

    auto* ph = new QLabel(QStringLiteral("（专题练习功能开发中...）"));
    ph->setAlignment(Qt::AlignCenter);
    ph->setObjectName(QStringLiteral("PlaceholderLabel"));

    lay->addLayout(top);
    lay->addWidget(ph, 1);
}

} // namespace AlgeMate::Learning
