#include "ExamPage.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>

namespace AlgeMate::Learning {

static QPushButton* makeBackBtn(QWidget* parent = nullptr) {
    auto* btn = new QPushButton(QStringLiteral("← 返回"), parent);
    btn->setObjectName(QStringLiteral("LearnBackBtn"));
    return btn;
}

ExamPage::ExamPage(QWidget* parent) : QWidget(parent)
{
    auto* lay = new QVBoxLayout(this);
    lay->setContentsMargins(24, 16, 24, 24);
    lay->setSpacing(14);

    auto* top = new QHBoxLayout;
    auto* back = makeBackBtn(this);
    connect(back, &QPushButton::clicked, this, &ExamPage::backRequested);
    auto* t = new QLabel(QStringLiteral("考试模式"));
    t->setObjectName(QStringLiteral("PageTitle"));
    top->addWidget(back);
    top->addWidget(t);
    top->addStretch();

    auto* ph = new QLabel(QStringLiteral("（考试模式功能开发中...）"));
    ph->setAlignment(Qt::AlignCenter);
    ph->setObjectName(QStringLiteral("PlaceholderLabel"));

    lay->addLayout(top);
    lay->addWidget(ph, 1);
}

} // namespace AlgeMate::Learning
