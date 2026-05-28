#include "HomePage.h"
#include "core/UserProfile.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QFrame>
#include <QPushButton>
#include <QMouseEvent>
#include <QEvent>

namespace AlgeMate::Home {

static QFrame* makeQuickCard(const QString& emoji, const QString& title, const QString& desc, const QString& accent) {
    auto* card = new QFrame;
    card->setObjectName(QStringLiteral("Card"));
    card->setMinimumHeight(150);
    card->setCursor(Qt::PointingHandCursor);

    auto* lay = new QVBoxLayout(card);
    lay->setContentsMargins(20, 18, 20, 18);
    lay->setSpacing(8);

    auto* icon = new QLabel(emoji);
    icon->setFixedSize(44, 44);
    icon->setAlignment(Qt::AlignCenter);
    icon->setStyleSheet(QStringLiteral(
        "background-color:%1; border-radius:12px; font-size:22px;").arg(accent));

    auto* t = new QLabel(title);
    t->setStyleSheet("font-size:16px; font-weight:700;");
    auto* d = new QLabel(desc);
    d->setStyleSheet("color:#8A8FA3; font-size:12px;");
    d->setWordWrap(true);

    lay->addWidget(icon);
    lay->addWidget(t);
    lay->addWidget(d);
    lay->addStretch();
    return card;
}

HomePage::HomePage(QWidget* parent) : QWidget(parent) {
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(32, 32, 32, 32);
    root->setSpacing(24);

    auto* headRow = new QHBoxLayout;
    headRow->setSpacing(18);

    avatarLabel_ = new QLabel;
    avatarLabel_->setFixedSize(72, 72);

    auto* textCol = new QVBoxLayout;
    textCol->setSpacing(4);
    greetingLabel_ = new QLabel;
    greetingLabel_->setStyleSheet("font-size:26px; font-weight:700;");
    subtitleLabel_ = new QLabel(QStringLiteral("今天也要在线性代数里找到属于自己的节奏 ✨"));
    subtitleLabel_->setStyleSheet("color:#8A8FA3; font-size:13px;");
    textCol->addWidget(greetingLabel_);
    textCol->addWidget(subtitleLabel_);

    headRow->addWidget(avatarLabel_);
    headRow->addLayout(textCol, 1);

    auto* grid = new QGridLayout;
    grid->setSpacing(16);

    struct CardInfo { const char* emoji; const char* title; const char* desc; const char* accent; NavTarget target; int row; int col; };
    const CardInfo infos[] = {
        { "🧮", "打开计算助手",   "矩阵运算、方程组求解、行列式 / 秩 / 特征值", "#EBE5FF", Calculator, 0, 0 },
        { "🤖", "AI 智能解题",     "输入题目，让 AI 帮你分步讲解与求解",         "#FFE8D6", AiSolver,   0, 1 },
        { "📘", "知识点学习",     "章节目录、图文讲解、经典例题随手查",         "#DCF3EA", Knowledge,  1, 0 },
        { "📈", "学习中心",         "进度追踪、错题本、打卡与推荐练习",           "#E6E9FF", Learning,   1, 1 },
        { "⚙",  "设置中心",         "个性化外观、账号、API 与快捷键",             "#F0F0F0", Settings,   2, 0 },
    };
    for (const auto& i : infos) {
        auto* card = makeQuickCard(QString::fromUtf8(i.emoji),
                                   QString::fromUtf8(i.title),
                                   QString::fromUtf8(i.desc),
                                   QString::fromUtf8(i.accent));
        cardTargets_.insert(card, int(i.target));
        card->installEventFilter(this);
        grid->addWidget(card, i.row, i.col);
    }

    root->addLayout(headRow);
    root->addLayout(grid);
    root->addStretch();

    refreshGreeting();
    connect(&UserProfile::instance(), &UserProfile::profileChanged,
            this, &HomePage::refreshGreeting);
}

void HomePage::refreshGreeting() {
    auto& u = UserProfile::instance();
    avatarLabel_->setPixmap(u.avatarPixmap(72));
    greetingLabel_->setText(QStringLiteral("%1，%2 👋")
        .arg(UserProfile::greetingByTime(), u.userName()));
}

bool HomePage::eventFilter(QObject* obj, QEvent* e) {
    if (e->type() == QEvent::MouseButtonRelease) {
        auto it = cardTargets_.constFind(obj);
        if (it != cardTargets_.constEnd()) {
            auto* me = static_cast<QMouseEvent*>(e);
            if (me->button() == Qt::LeftButton) {
                emit requestNavigate(it.value());
                return true;
            }
        }
    }
    return QWidget::eventFilter(obj, e);
}

}
