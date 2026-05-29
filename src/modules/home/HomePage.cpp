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
#include <QProgressBar>
#include <QLineEdit>
#include <QSpinBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QComboBox>
#include <QScrollArea>
#include <QStyle>

namespace AlgeMate::Home {

// ==================== 自定义目标编辑弹窗 (支持类别与单位联动) ====================
namespace {
class GoalEditDialog : public QDialog {
public:
    GoalEditDialog(const QList<SubGoal>& currentGoals, QWidget* parent = nullptr) : QDialog(parent) {
        setWindowTitle(QStringLiteral("编辑学习目标"));
        setMinimumWidth(720); // 因为选项较多，加宽弹窗
        setMinimumHeight(400);
        setStyleSheet("QDialog { background-color: #FAFAFC; }");

        auto* mainLay = new QVBoxLayout(this);
        mainLay->setSpacing(16);

        auto* titleLbl = new QLabel(QStringLiteral("设定你的阶段性学习目标："));
        titleLbl->setStyleSheet("font-size: 15px; font-weight: bold; color: #333333;");
        mainLay->addWidget(titleLbl);

        // 使用滚动区域，防止目标太多撑爆屏幕
        auto* scrollArea = new QScrollArea;
        scrollArea->setWidgetResizable(true);
        scrollArea->setFrameShape(QFrame::NoFrame);
        scrollArea->setStyleSheet("QScrollArea { background: transparent; } QWidget#ScrollContent { background: transparent; }");

        auto* scrollWidget = new QWidget;
        scrollWidget->setObjectName("ScrollContent");
        rowsLay = new QVBoxLayout(scrollWidget);
        rowsLay->setAlignment(Qt::AlignTop);
        rowsLay->setSpacing(12);
        rowsLay->setContentsMargins(0, 0, 10, 0);
        scrollArea->setWidget(scrollWidget);

        mainLay->addWidget(scrollArea, 1);

        for (const auto& g : currentGoals) {
            addRow(g);
        }

        auto* addBtn = new QPushButton(QStringLiteral("➕ 添加新目标"));
        addBtn->setCursor(Qt::PointingHandCursor);
        addBtn->setStyleSheet("QPushButton { background-color: #F0F4FF; color: #6B7CFF; border: none; border-radius: 8px; padding: 10px; font-weight: bold; font-size: 13px; }"
                              "QPushButton:hover { background-color: #E6E9FF; }");
        connect(addBtn, &QPushButton::clicked, this, [this](){
            SubGoal emptyGoal;
            addRow(emptyGoal);
        });
        mainLay->addWidget(addBtn);

        auto* btnBox = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Cancel);
        btnBox->button(QDialogButtonBox::Save)->setText(QStringLiteral("保存"));
        btnBox->button(QDialogButtonBox::Cancel)->setText(QStringLiteral("取消"));
        connect(btnBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
        connect(btnBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
        mainLay->addWidget(btnBox);
    }

    QList<SubGoal> getGoals() const {
        QList<SubGoal> res;
        for (const auto& r : rows) {
            QString name = r.nameEdit->text().trimmed();
            if (!name.isEmpty()) {
                SubGoal g;
                g.category = r.catCombo->currentText();
                g.subCategory = r.subCombo->isVisible() ? r.subCombo->currentText() : QStringLiteral("");
                g.name = name;
                g.current = r.savedCurrent;
                g.target = r.targetSpin->value();
                g.unit = r.unitCombo->currentText();
                res.append(g);
            }
        }
        return res;
    }

private:
    struct Row { QWidget* w; QComboBox* catCombo; QComboBox* subCombo; QLineEdit* nameEdit; QSpinBox* targetSpin; QComboBox* unitCombo; int savedCurrent; };
    QList<Row> rows;
    QVBoxLayout* rowsLay;

    void addRow(const SubGoal& g) {
        auto* w = new QWidget;
        auto* lay = new QHBoxLayout(w);
        lay->setContentsMargins(0, 0, 0, 0);
        lay->setSpacing(8);

        QString comboStyle = "QComboBox { border: 1px solid #E2E8F0; border-radius: 6px; padding: 4px 8px; background: #FFFFFF; font-size: 12px; min-width: 80px; }";
        QString editStyle = "QLineEdit { border: 1px solid #E2E8F0; border-radius: 6px; padding: 6px; background: #FFFFFF; font-size: 12px; }";
        QString spinStyle = "QSpinBox { border: 1px solid #E2E8F0; border-radius: 6px; padding: 4px 8px; background: #FFFFFF; font-size: 12px; }";

        auto* catCombo = new QComboBox;
        catCombo->addItems({QStringLiteral("知识点学习"), QStringLiteral("练习"), QStringLiteral("测试"), QStringLiteral("自定义")});
        catCombo->setStyleSheet(comboStyle);

        auto* subCombo = new QComboBox;
        subCombo->addItems({QStringLiteral("计算题"), QStringLiteral("章节练习"), QStringLiteral("专题模式")});
        subCombo->setStyleSheet(comboStyle);

        auto* nameEdit = new QLineEdit(g.name);
        nameEdit->setPlaceholderText(QStringLiteral("目标名称..."));
        nameEdit->setStyleSheet(editStyle);

        auto* targetSpin = new QSpinBox;
        targetSpin->setRange(1, 9999);
        targetSpin->setValue(g.target);
        targetSpin->setStyleSheet(spinStyle);

        auto* unitCombo = new QComboBox;
        unitCombo->setStyleSheet(comboStyle);

        auto* delBtn = new QPushButton();
        delBtn->setIcon(delBtn->style()->standardIcon(QStyle::SP_TrashIcon));
        delBtn->setFixedSize(30, 30);
        delBtn->setCursor(Qt::PointingHandCursor);

        // 换成更浅、更柔和的淡红色
        delBtn->setStyleSheet(
            "QPushButton { background-color: #FCA5A5; border: none; border-radius: 6px; }"
            "QPushButton:hover { background-color: #F87171; }"
            "QPushButton:pressed { background-color: #EF4444; }"
            );

        // 核心逻辑：下拉框联动
        auto updateDependencies = [=]() {
            QString cat = catCombo->currentText();
            subCombo->setVisible(cat == QStringLiteral("练习"));

            QString currentUnit = unitCombo->currentText(); // 保存当前单位尝试恢复
            unitCombo->clear();

            if (cat == QStringLiteral("知识点学习")) {
                unitCombo->addItems({QStringLiteral("节"), QStringLiteral("章")});
            } else if (cat == QStringLiteral("练习")) {
                QString sub = subCombo->currentText();
                if (sub == QStringLiteral("计算题")) unitCombo->addItems({QStringLiteral("道")});
                else if (sub == QStringLiteral("章节练习")) unitCombo->addItems({QStringLiteral("章")});
                else if (sub == QStringLiteral("专题模式")) unitCombo->addItems({QStringLiteral("个")}); // "几个专题"用"个"
            } else if (cat == QStringLiteral("测试")) {
                unitCombo->addItems({QStringLiteral("套")});
            } else if (cat == QStringLiteral("自定义")) {
                unitCombo->addItems({QStringLiteral("分钟"), QStringLiteral("次")}); // "打勾"用"次"(比如设为1次完成)
            }

            if (unitCombo->findText(currentUnit) != -1) {
                unitCombo->setCurrentText(currentUnit);
            }
        };

        // 初始设值
        catCombo->setCurrentText(g.category);
        subCombo->setCurrentText(g.subCategory);
        updateDependencies(); // 手动调用一次渲染初始单位
        if (!g.unit.isEmpty() && unitCombo->findText(g.unit) != -1) {
            unitCombo->setCurrentText(g.unit);
        }

        // 绑定事件
        connect(catCombo, &QComboBox::currentTextChanged, w, [=](){ updateDependencies(); });
        connect(subCombo, &QComboBox::currentTextChanged, w, [=](){ updateDependencies(); });

        lay->addWidget(catCombo, 0);
        lay->addWidget(subCombo, 0);
        lay->addWidget(nameEdit, 1);
        lay->addWidget(new QLabel(QStringLiteral("目标总量:")), 0);
        lay->addWidget(targetSpin, 0);
        lay->addWidget(unitCombo, 0);
        lay->addWidget(delBtn, 0, Qt::AlignTop);

        connect(delBtn, &QPushButton::clicked, [this, w]() {
            for (int i = 0; i < rows.size(); ++i) {
                if (rows[i].w == w) {
                    rows.removeAt(i);
                    break;
                }
            }
            w->deleteLater();
        });

        rows.append({w, catCombo, subCombo, nameEdit, targetSpin, unitCombo, g.current});
        rowsLay->addWidget(w);
    }
};
}
// =============================================================

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

    // ==================== 1. 头部信息区 ====================
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

    auto* settingsBtn = new QPushButton();
    settingsBtn->setCursor(Qt::PointingHandCursor);
    settingsBtn->setFixedSize(118, 42);

    auto* btnLay = new QHBoxLayout(settingsBtn);
    btnLay->setContentsMargins(12, 0, 12, 0);
    btnLay->setSpacing(6);

    auto* iconLabel = new QLabel(QStringLiteral("⚙"));
    iconLabel->setObjectName("settingsIcon");
    iconLabel->setAlignment(Qt::AlignCenter);
    iconLabel->setAttribute(Qt::WA_TransparentForMouseEvents);

    auto* textLabel = new QLabel(QStringLiteral("设置中心"));
    textLabel->setObjectName("settingsText");
    textLabel->setAlignment(Qt::AlignVCenter | Qt::AlignLeft);
    textLabel->setAttribute(Qt::WA_TransparentForMouseEvents);

    btnLay->addWidget(iconLabel);
    btnLay->addWidget(textLabel);
    btnLay->addStretch();

    settingsBtn->setStyleSheet(QStringLiteral(
        "QPushButton { background-color: transparent; border: none; }"
        "QPushButton:hover { background-color: #E6E9FF; border-radius: 8px; }"
        "QPushButton QLabel { color: #8A8FA3; background: transparent; }"
        "QPushButton:hover QLabel { color: #333333; }"
        "QLabel#settingsIcon { font-size: 25px; }"
        "QLabel#settingsText { font-size: 18px; font-weight: 600; }"
        ));

    connect(settingsBtn, &QPushButton::clicked, this, [this]() {
        emit requestNavigate(Settings);
    });
    headRow->addWidget(settingsBtn, 0, Qt::AlignTop);

    // ==================== 2. 全新：首页内嵌式多目标看板 ====================
    auto* goalFrame = new QFrame;
    goalFrame->setObjectName("GoalFrame");
    goalFrame->setStyleSheet("QFrame#GoalFrame { background-color: #FAFAFC; border: 1px solid #E2E8F0; border-radius: 16px; }");

    auto* goalLay = new QVBoxLayout(goalFrame);
    goalLay->setContentsMargins(24, 20, 24, 16);
    goalLay->setSpacing(12);

    auto* topGoalRow = new QHBoxLayout;
    topGoalRow->setSpacing(12);

    auto* targetIcon = new QLabel(QStringLiteral("🎯"));
    targetIcon->setFixedSize(40, 40);
    targetIcon->setAlignment(Qt::AlignCenter);
    targetIcon->setStyleSheet("background-color: #FFE8D6; border-radius: 12px; font-size: 20px; border: none;");

    auto* titleCol = new QVBoxLayout;
    goalTitleLabel_ = new QLabel();
    goalTitleLabel_->setStyleSheet("font-size: 16px; font-weight: 700; color: #333333; border: none;");
    goalPercentLabel_ = new QLabel();
    goalPercentLabel_->setStyleSheet("font-size: 14px; font-weight: 700; color: #6B7CFF; border: none;");

    auto* titleAndPercentRow = new QHBoxLayout;
    titleAndPercentRow->addWidget(goalTitleLabel_);
    titleAndPercentRow->addWidget(goalPercentLabel_);
    titleAndPercentRow->addStretch();
    titleCol->addLayout(titleAndPercentRow);

    goalProgressBar_ = new QProgressBar;
    goalProgressBar_->setFixedHeight(8);
    goalProgressBar_->setTextVisible(false);
    titleCol->addWidget(goalProgressBar_);

    topGoalRow->addWidget(targetIcon);
    topGoalRow->addLayout(titleCol, 1);

    goalLay->addLayout(topGoalRow);

    subGoalsLayout_ = new QVBoxLayout;
    subGoalsLayout_->setSpacing(12); // 行间距稍大，更美观
    subGoalsLayout_->setContentsMargins(52, 10, 0, 0);
    goalLay->addLayout(subGoalsLayout_);

    auto* bottomActionRow = new QHBoxLayout;
    bottomActionRow->addStretch();

    auto* editBtn = new QPushButton(QStringLiteral("✏️ 编辑目标"));
    editBtn->setCursor(Qt::PointingHandCursor);
    editBtn->setStyleSheet(QStringLiteral(
        "QPushButton { background: transparent; color: #A0AEC0; font-size: 13px; font-weight: bold; border: none; padding: 4px; }"
        "QPushButton:hover { color: #6B7CFF; }"
        ));
    connect(editBtn, &QPushButton::clicked, this, &HomePage::onEditGoalsClicked);
    bottomActionRow->addWidget(editBtn);

    goalLay->addLayout(bottomActionRow);

    updateGoalUI(); // 默认无目标

    // ==================== 3. 底部功能网格 ====================
    auto* grid = new QGridLayout;
    grid->setSpacing(16);

    struct CardInfo { const char* emoji; const char* title; const char* desc; const char* accent; NavTarget target; int row; int col; };
    const CardInfo infos[] = {
        { "🧮", "打开计算助手",   "矩阵运算、方程组求解、行列式 / 秩 / 特征值", "#EBE5FF", Calculator, 0, 0 },
        { "🤖", "AI 智能解题",     "输入题目，让 AI 帮你分步讲解与求解",         "#FFE8D6", AiSolver,   0, 1 },
        { "📘", "知识点学习",     "章节目录、图文讲解、经典例题随手查",         "#DCF3EA", Knowledge,  1, 0 },
        { "📈", "学习中心",         "进度追踪、错题本、打卡与推荐练习",           "#E6E9FF", Learning,   1, 1 }
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

    // ==================== 4. 组装所有部分 ====================
    root->addLayout(headRow);
    root->addWidget(goalFrame);
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

void HomePage::setSubGoalProgress(const QString& goalName, int currentProgress) {
    for (auto& goal : subGoals_) {
        if (goal.name == goalName) {
            goal.current = qMin(currentProgress, goal.target);
            updateGoalUI();
            return;
        }
    }
}

void HomePage::onEditGoalsClicked() {
    GoalEditDialog dlg(subGoals_, this);
    if (dlg.exec() == QDialog::Accepted) {
        subGoals_ = dlg.getGoals();
        updateGoalUI();
    }
}

// ==================== 目标UI刷新核心逻辑 ====================

void HomePage::updateGoalUI() {
    QLayoutItem* item;
    while ((item = subGoalsLayout_->takeAt(0)) != nullptr) {
        if (QWidget* w = item->widget()) w->deleteLater();
        delete item;
    }

    if (subGoals_.isEmpty()) {
        goalTitleLabel_->setText(QStringLiteral("没有待完成的任务💫"));
        goalPercentLabel_->setText(QStringLiteral(""));
        goalProgressBar_->setRange(0, 100);
        goalProgressBar_->setValue(0);
        goalProgressBar_->setStyleSheet("QProgressBar { background-color: #E2E8F0; border-radius: 4px; border: none; }");
        return;
    }

    goalTitleLabel_->setText(QStringLiteral("学习总进度"));
    double sumOfPercentages = 0.0;

    for (const auto& goal : subGoals_) {
        double percent = (goal.target > 0) ? (double(goal.current) / goal.target * 100.0) : 0.0;
        sumOfPercentages += percent;

        auto* subRow = new QWidget;
        auto* subLay = new QHBoxLayout(subRow);
        subLay->setContentsMargins(0, 0, 0, 0);
        subLay->setSpacing(10);

        // 1. 生成带有颜色的分类徽章 (Tag)
        QString badgeText = goal.category;
        QString badgeColor = "#E2E8F0"; // 默认灰
        QString textColor = "#4A5568";

        if (goal.category == QStringLiteral("知识点学习")) { badgeColor = "#DCF3EA"; textColor = "#22543D"; }
        else if (goal.category == QStringLiteral("练习")) { badgeColor = "#EBE5FF"; textColor = "#44337A"; badgeText += QStringLiteral("·") + goal.subCategory; }
        else if (goal.category == QStringLiteral("测试")) { badgeColor = "#FFE8D6"; textColor = "#7B341E"; }

        auto* badgeLbl = new QLabel(badgeText);
        badgeLbl->setStyleSheet(QStringLiteral("background-color: %1; color: %2; padding: 2px 6px; border-radius: 4px; font-size: 11px; font-weight: bold; border: none;").arg(badgeColor, textColor));
        badgeLbl->setAlignment(Qt::AlignCenter);

        // 2. 目标名称
        auto* nameLbl = new QLabel(goal.name);
        nameLbl->setStyleSheet("color: #4A5568; font-size: 13px; font-weight: 500; border: none;");
        nameLbl->setMinimumWidth(80);

        // 3. 进度条
        auto* subBar = new QProgressBar;
        subBar->setFixedHeight(6);
        subBar->setTextVisible(false);
        subBar->setRange(0, goal.target);
        subBar->setValue(goal.current);

        if (goal.current >= goal.target && goal.target > 0) {
            subBar->setStyleSheet("QProgressBar { background-color: #EDF2F7; border-radius: 3px; border: none; }"
                                  "QProgressBar::chunk { background-color: #4CAF50; border-radius: 3px; }");
        } else {
            subBar->setStyleSheet("QProgressBar { background-color: #EDF2F7; border-radius: 3px; border: none; }"
                                  "QProgressBar::chunk { background-color: #A0AEC0; border-radius: 3px; }");
        }

        // 4. 数值与单位 (例如: "2 / 5 节")
        auto* progLbl = new QLabel(QStringLiteral("%1 / %2 %3").arg(goal.current).arg(goal.target).arg(goal.unit));
        progLbl->setStyleSheet("color: #A0AEC0; font-size: 12px; font-family: monospace; border: none;");
        progLbl->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

        subLay->addWidget(badgeLbl, 0);
        subLay->addWidget(nameLbl, 0);
        subLay->addWidget(subBar, 1);
        subLay->addWidget(progLbl, 0);

        subGoalsLayout_->addWidget(subRow);
    }

    // 计算平均总进度
    int avgPercent = qRound(sumOfPercentages / subGoals_.size());
    goalProgressBar_->setRange(0, 100);
    goalProgressBar_->setValue(avgPercent);
    goalPercentLabel_->setText(QStringLiteral("%1%").arg(avgPercent));

    if (avgPercent >= 100) {
        goalPercentLabel_->setStyleSheet("font-size: 14px; font-weight: 700; color: #4CAF50; border: none;");
        goalProgressBar_->setStyleSheet("QProgressBar { background-color: #E2E8F0; border-radius: 4px; border: none; }"
                                        "QProgressBar::chunk { background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #81C784, stop:1 #4CAF50); border-radius: 4px; }");
    } else {
        goalPercentLabel_->setStyleSheet("font-size: 14px; font-weight: 700; color: #6B7CFF; border: none;");
        goalProgressBar_->setStyleSheet("QProgressBar { background-color: #E2E8F0; border-radius: 4px; border: none; }"
                                        "QProgressBar::chunk { background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #8E9EFF, stop:1 #6B7CFF); border-radius: 4px; }");
    }
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