#include "LearningCenterPage.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QPushButton>
#include <QFrame>

namespace AlgeMate::Learning {

static QPushButton* makeBackBtn(QWidget* parent = nullptr) {
    auto* btn = new QPushButton(QStringLiteral("← 返回"), parent);
    btn->setObjectName(QStringLiteral("LearnBackBtn"));
    return btn;
}

LearningCenterPage::LearningCenterPage(QWidget* parent) : QWidget(parent)
{
    auto* lay = new QVBoxLayout(this);
    lay->setContentsMargins(24, 16, 24, 24);
    lay->setSpacing(20);

    auto* top = new QHBoxLayout;
    auto* back = makeBackBtn(this);
    connect(back, &QPushButton::clicked, this, &LearningCenterPage::backRequested);
    auto* t = new QLabel(QStringLiteral("学习管理中心"));
    t->setObjectName(QStringLiteral("PageTitle"));
    top->addWidget(back);
    top->addWidget(t);
    top->addStretch();

    auto* grid = new QGridLayout;
    grid->setSpacing(12);
    auto makeCell = [](const QString& label, const QString& value) {
        auto* f = new QFrame;
        f->setObjectName(QStringLiteral("Card"));
        f->setMinimumHeight(100);
        auto* fl = new QVBoxLayout(f);
        fl->setContentsMargins(20, 16, 20, 14);
        auto* lb = new QLabel(label);
        lb->setStyleSheet("font-size:13px; color:#8A8FA3; background:transparent;");
        auto* vl = new QLabel(value);
        vl->setStyleSheet("font-size:26px; font-weight:700; color:#6A5AE0; background:transparent;");
        fl->addWidget(lb);
        fl->addWidget(vl);
        return f;
    };
    grid->addWidget(makeCell(QStringLiteral("今日学习时长"), QStringLiteral("0 分钟")), 0, 0);
    grid->addWidget(makeCell(QStringLiteral("本周进度"),     QStringLiteral("0%")),      0, 1);
    grid->addWidget(makeCell(QStringLiteral("连续打卡"),     QStringLiteral("0 天")),    0, 2);
    grid->addWidget(makeCell(QStringLiteral("错题趋势"),     QStringLiteral("—")),       0, 3);

    auto* chartArea = new QFrame;
    chartArea->setObjectName(QStringLiteral("Card"));
    chartArea->setMinimumHeight(200);
    auto* chartLay = new QVBoxLayout(chartArea);
    auto* chartPH = new QLabel(QStringLiteral("学习趋势图（开发中...）"));
    chartPH->setAlignment(Qt::AlignCenter);
    chartPH->setObjectName(QStringLiteral("PlaceholderLabel"));
    chartLay->addWidget(chartPH);

    lay->addLayout(top);
    lay->addLayout(grid);
    lay->addWidget(chartArea, 1);
}

} // namespace AlgeMate::Learning
