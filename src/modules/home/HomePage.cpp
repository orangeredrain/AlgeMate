#include "HomePage.h"
#include "core/UserProfile.h"

#include <QStackedWidget>
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
#include <QDialog>
#include <QDialogButtonBox>
#include <QComboBox>
#include <QScrollArea>
#include <QStyle>
#include <QSettings>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonDocument>
#include <QTimer>
#include <QMessageBox>
#include <QDateEdit>
#include <QToolButton>
#include <QDate>
#include <QTime>
#include <QCalendarWidget>
#include <QSystemTrayIcon>
#include <QApplication>
#include <algorithm>

namespace AlgeMate::Home {

// ==================== 智能到期提醒弹窗 (应用内) ====================
class DeadlineNotifyDialog : public QDialog {
public:
    explicit DeadlineNotifyDialog(const QStringList& tasks, QWidget* parent = nullptr) : QDialog(parent) {
        setWindowTitle(QStringLiteral("今日截止提醒"));
        setMinimumWidth(340);
        setStyleSheet("QDialog { background-color: #FAFAFC; }");

        auto* lay = new QVBoxLayout(this);
        lay->setContentsMargins(24, 32, 24, 24);
        lay->setSpacing(16);

        auto* iconLbl = new QLabel(QStringLiteral("⏰"));
        iconLbl->setAlignment(Qt::AlignCenter);
        iconLbl->setStyleSheet("font-size: 42px; border: none; background: transparent;");
        lay->addWidget(iconLbl);

        auto* titleLbl = new QLabel(QStringLiteral("以下任务请尽快完成哦~"));
        titleLbl->setAlignment(Qt::AlignCenter);
        titleLbl->setStyleSheet("font-size: 18px; font-weight: bold; color: #E53E3E; border: none; background: transparent;");
        lay->addWidget(titleLbl);

        // 截取前3个任务展示，多了就用省略号
        QStringList displayList;
        for (int i = 0; i < qMin(3, (int)tasks.size()); ++i) {
            displayList.append(QStringLiteral("• ") + tasks[i]);
        }
        if (tasks.size() > 3) {
            displayList.append(QStringLiteral("...等共 %1 个任务").arg(tasks.size()));
        }

        auto* descLbl = new QLabel(QStringLiteral("今天有 %1 个任务需要完成：\n\n%2\n\n抓紧时间哦！💪")
                                       .arg(tasks.size()).arg(displayList.join(QStringLiteral("\n"))));
        descLbl->setAlignment(Qt::AlignCenter);
        descLbl->setStyleSheet("font-size: 14px; color: #4A5568; line-height: 1.5; border: none; background: transparent;");
        descLbl->setWordWrap(true);
        lay->addWidget(descLbl);

        auto* btn = new QPushButton(QStringLiteral("去完成"));
        btn->setCursor(Qt::PointingHandCursor);
        btn->setFixedHeight(40);
        btn->setStyleSheet(
            "QPushButton { background-color: #6B7CFF; color: white; border-radius: 8px; font-size: 14px; font-weight: bold; border: none; }"
            "QPushButton:hover { background-color: #5A6AE0; }"
            );
        connect(btn, &QPushButton::clicked, this, &QDialog::accept);
        lay->addWidget(btn);
    }
};

// ==================== 历史学习目标弹窗 ====================
class HistoryGoalsDialog : public QDialog {
public:
    explicit HistoryGoalsDialog(QWidget* parent = nullptr) : QDialog(parent) {
        setWindowTitle(QStringLiteral("过往学习目标查询"));
        setFixedSize(500, 500);
        setStyleSheet("QDialog { background-color: #FAFAFC; }");

        auto* mainLay = new QVBoxLayout(this);
        mainLay->setSpacing(16);
        mainLay->setContentsMargins(24, 24, 24, 24);

        auto* topLay = new QHBoxLayout;

        auto* prevBtn = new QToolButton;
        prevBtn->setText(QStringLiteral("<"));
        prevBtn->setStyleSheet("color: #6B7CFF; font-weight: bold; border: none; font-size: 16px;");
        prevBtn->setCursor(Qt::PointingHandCursor);

        dateEdit = new QDateEdit(QDate::currentDate());
        dateEdit->setDisplayFormat(QStringLiteral("yyyy 年 MM 月 dd 日"));
        dateEdit->setCalendarPopup(true);
        dateEdit->setMinimumDate(QDate::currentDate().addYears(-1));
        dateEdit->setMaximumDate(QDate::currentDate());
        dateEdit->setFocusPolicy(Qt::StrongFocus);
        dateEdit->setReadOnly(false);
        dateEdit->setCursor(Qt::IBeamCursor);

        dateEdit->setStyleSheet(R"(
            QDateEdit {
                border: 1px solid #E2E8F0;
                border-radius: 6px;
                padding: 4px 8px;
                background: #FFFFFF;
                font-size: 14px;
                color: #333333;
            }
            QDateEdit:focus {
                border: 1.5px solid #6B7CFF;
            }
            QDateEdit::drop-down {
                subcontrol-origin: padding;
                subcontrol-position: top right;
                width: 26px;
                border-left: none;
                background: transparent;
            }
            QDateEdit::down-arrow {
                image: none;
            }
        )");
        dateEdit->setCursor(Qt::PointingHandCursor);

        auto* innerLay = new QHBoxLayout(dateEdit);
        innerLay->setContentsMargins(0, 0, 8, 0);
        innerLay->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

        auto* arrowLbl = new QLabel("▼");
        arrowLbl->setStyleSheet("color: #8A8FA3; font-size: 10px; border: none; background: transparent;");
        arrowLbl->setAttribute(Qt::WA_TransparentForMouseEvents);

        innerLay->addWidget(arrowLbl);

        auto* cal = new QCalendarWidget;
        cal->setStyleSheet(
            "QCalendarWidget QWidget#qt_calendar_navigationbar { background-color: #F8F9FC; border-bottom: 1px solid #E2E8F0; min-height: 36px; }"
            "QCalendarWidget QToolButton { color: #24253D; font-weight: bold; background: transparent; padding: 4px; border-radius: 4px; }"
            "QCalendarWidget QToolButton:hover { background-color: #EBE5FF; color: #6A5AE0; }"
            "QCalendarWidget QToolButton::menu-indicator { image: none; }"
            "QCalendarWidget QSpinBox { background: transparent; color: #24253D; font-weight: bold; selection-background-color: #6A5AE0; }"
            "QCalendarWidget QSpinBox::up-button, QCalendarWidget QSpinBox::down-button { subcontrol-origin: border; width: 0px; }"
            "QCalendarWidget QMenu { background-color: #FFFFFF; color: #24253D; border: 1px solid #E2E8F0; }"
            "QCalendarWidget QMenu::item:selected { background-color: #EBE5FF; color: #6A5AE0; }"
            "QCalendarWidget QAbstractItemView:enabled { color: #24253D; background-color: #FFFFFF; selection-background-color: #6A5AE0; selection-color: #FFFFFF; outline: none; }"
            "QCalendarWidget QAbstractItemView:disabled { color: #B4B8CC; }"
            );
        cal->setGridVisible(false);
        cal->setMinimumSize(320, 260);
        cal->setVerticalHeaderFormat(QCalendarWidget::NoVerticalHeader);
        dateEdit->setCalendarWidget(cal);

        auto* nextBtn = new QToolButton;
        nextBtn->setText(QStringLiteral(">"));
        nextBtn->setStyleSheet("color: #6B7CFF; font-weight: bold; border: none; font-size: 16px;");
        nextBtn->setCursor(Qt::PointingHandCursor);

        topLay->addStretch();
        topLay->addWidget(prevBtn);
        topLay->addWidget(dateEdit);
        topLay->addWidget(nextBtn);
        topLay->addStretch();
        mainLay->addLayout(topLay);

        statusLbl = new QLabel;
        statusLbl->setAlignment(Qt::AlignCenter);
        statusLbl->setStyleSheet("color: #8A8FA3; font-size: 13px;");
        mainLay->addWidget(statusLbl);

        auto* scrollArea = new QScrollArea;
        scrollArea->setWidgetResizable(true);
        scrollArea->setFrameShape(QFrame::NoFrame);
        scrollArea->setStyleSheet("QScrollArea { background: transparent; } QWidget#ListContent { background: transparent; }");

        auto* listContent = new QWidget;
        listContent->setObjectName("ListContent");
        listLay = new QVBoxLayout(listContent);
        listLay->setAlignment(Qt::AlignTop);
        listLay->setSpacing(12);
        scrollArea->setWidget(listContent);
        mainLay->addWidget(scrollArea, 1);

        connect(prevBtn, &QToolButton::clicked, this, [this](){ dateEdit->setDate(dateEdit->date().addDays(-1)); });
        connect(nextBtn, &QToolButton::clicked, this, [this](){
            if (dateEdit->date() < QDate::currentDate()) dateEdit->setDate(dateEdit->date().addDays(1));
        });
        connect(dateEdit, &QDateEdit::dateChanged, this, &HistoryGoalsDialog::loadDataForDate);

        loadDataForDate(QDate::currentDate());
    }

private:
    QDateEdit* dateEdit;
    QVBoxLayout* listLay;
    QLabel* statusLbl;

    void loadDataForDate(const QDate& date) {
        QLayoutItem* item;
        while ((item = listLay->takeAt(0)) != nullptr) {
            if (QWidget* w = item->widget()) w->deleteLater();
            delete item;
        }

        QSettings settings(QStringLiteral("AlgeMate"), QStringLiteral("HomeGoals"));
        QString key = QStringLiteral("History_") + date.toString(Qt::ISODate);
        QString jsonStr = settings.value(key).toString();

        if (jsonStr.isEmpty() || QJsonDocument::fromJson(jsonStr.toUtf8()).array().isEmpty()) {
            statusLbl->setText(QStringLiteral("这一天暂时没有记录学习目标哦 🍃"));
            return;
        }

        QJsonArray arr = QJsonDocument::fromJson(jsonStr.toUtf8()).array();
        int completedCount = 0;
        int totalCount = arr.size();

        for (int i = 0; i < arr.size(); ++i) {
            QJsonObject obj = arr[i].toObject();
            QString cat = obj["category"].toString();
            QString subCat = obj["subCategory"].toString();
            QString name = obj["name"].toString();
            int current = obj["current"].toInt();
            int target = obj["target"].toInt();

            bool isDone = (target > 0 && current >= target);
            if (isDone) completedCount++;

            auto* row = new QWidget;
            auto* hLay = new QHBoxLayout(row);
            hLay->setContentsMargins(12, 12, 12, 12);
            row->setStyleSheet("QWidget { background: #FFFFFF; border: 1px solid #E2E8F0; border-radius: 8px; }");

            QString badgeText = cat;
            if (cat == QStringLiteral("练习") && !subCat.isEmpty()) {
                badgeText += QStringLiteral(" · ") + subCat;
            }

            auto* checkLbl = new QLabel(isDone ? QStringLiteral("✅") : QStringLiteral("⭕"));
            checkLbl->setFixedSize(24, 24);
            checkLbl->setAlignment(Qt::AlignCenter);
            checkLbl->setStyleSheet("border: none; background: transparent; font-size: 14px;");

            auto* badgeLbl = new QLabel(badgeText);
            badgeLbl->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
            if (isDone) {
                badgeLbl->setStyleSheet("background-color: #F7FAFC; color: #A0AEC0; padding: 4px 8px; border-radius: 6px; font-size: 11px; font-weight: bold; border: none;");
            } else {
                badgeLbl->setStyleSheet("background-color: #EBE5FF; color: #6B7CFF; padding: 4px 8px; border-radius: 6px; font-size: 11px; font-weight: bold; border: none;");
            }

            auto* nameLbl = new QLabel(name);
            nameLbl->setWordWrap(true);
            if (isDone) {
                nameLbl->setStyleSheet("color: #A0AEC0; font-size: 13px; text-decoration: line-through; border: none; background: transparent;");
            } else {
                nameLbl->setStyleSheet("color: #2D3748; font-size: 13px; font-weight: 500; border: none; background: transparent;");
            }

            hLay->addWidget(checkLbl);
            hLay->addWidget(badgeLbl);
            hLay->addWidget(nameLbl, 1);
            listLay->addWidget(row);
        }

        statusLbl->setText(QStringLiteral("当天共 %1 个任务，完成了 %2 个").arg(totalCount).arg(completedCount));
    }
};

// ==================== 自定义目标编辑弹窗 (支持日期设定) ====================
class GoalEditDialog : public QDialog {
public:
    GoalEditDialog(const QList<SubGoal>& currentGoals, QWidget* parent = nullptr) : QDialog(parent) {
        setWindowTitle(QStringLiteral("编辑学习目标"));
        setMinimumWidth(660);
        setMinimumHeight(400);
        setStyleSheet("QDialog { background-color: #FAFAFC; }");

        auto* mainLay = new QVBoxLayout(this);
        mainLay->setSpacing(16);

        auto* titleLbl = new QLabel(QStringLiteral("设定你的阶段性学习目标："));
        titleLbl->setStyleSheet("font-size: 15px; font-weight: bold; color: #333333;");
        mainLay->addWidget(titleLbl);

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
            emptyGoal.deadline = QDate::currentDate().toString(Qt::ISODate);
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

                if (g.category == QStringLiteral("练习")) {
                    g.subCategory = r.subCombo->currentText();
                } else {
                    g.subCategory = QStringLiteral("");
                }

                g.name = name;
                g.current = r.savedCurrent;
                g.target = 1;
                g.unit = QStringLiteral("");
                g.deadline = r.dateEdit->date().toString(Qt::ISODate);
                res.append(g);
            }
        }
        return res;
    }

private:
    struct Row { QWidget* w; QComboBox* catCombo; QComboBox* subCombo; QLineEdit* nameEdit; QDateEdit* dateEdit; int savedCurrent; };
    QList<Row> rows;
    QVBoxLayout* rowsLay;

    void addRow(const SubGoal& g) {
        auto* w = new QWidget;
        auto* lay = new QHBoxLayout(w);
        lay->setContentsMargins(0, 0, 0, 0);
        lay->setSpacing(8);

        QString boxStyle = R"(
            QComboBox, QDateEdit {
                border: 1px solid #E2E8F0;
                border-radius: 6px;
                padding: 4px 24px 4px 8px;
                background: #FFFFFF;
                font-size: 12px;
                min-width: 70px;
            }
            QComboBox:focus, QDateEdit:focus {
                border: 1.5px solid #6B7CFF;
            }
            QComboBox::drop-down, QDateEdit::drop-down {
                border: none;
                background: transparent;
                width: 24px;
            }
            QComboBox::down-arrow, QDateEdit::down-arrow {
                image: none;
            }
            QComboBox QAbstractItemView {
                background: #FFFFFF;
                border: 1px solid #E2E8F0;
                selection-background-color: #EBE5FF;
                selection-color: #6B7CFF;
                outline: none;
            }
        )";

        QString editStyle = "QLineEdit { border: 1px solid #E2E8F0; border-radius: 6px; padding: 6px; background: #FFFFFF; font-size: 12px; }";

        auto* catCombo = new QComboBox;
        catCombo->addItems({QStringLiteral("知识点学习"), QStringLiteral("练习"), QStringLiteral("测试"), QStringLiteral("自定义")});
        catCombo->setStyleSheet(boxStyle);
        catCombo->setCursor(Qt::PointingHandCursor);

        auto* subCombo = new QComboBox;
        subCombo->addItems({QStringLiteral("计算题"), QStringLiteral("章节练习"), QStringLiteral("专题模式")});
        subCombo->setStyleSheet(boxStyle);
        subCombo->setCursor(Qt::PointingHandCursor);

        auto* dateEdit = new QDateEdit;
        dateEdit->setDisplayFormat("yyyy/MM/dd");
        dateEdit->setCalendarPopup(true);
        dateEdit->setStyleSheet(boxStyle);
        dateEdit->setFocusPolicy(Qt::StrongFocus);
        dateEdit->setCursor(Qt::IBeamCursor);
        dateEdit->setMinimumWidth(105);

        auto* cal = new QCalendarWidget;
        cal->setStyleSheet(
            "QCalendarWidget QWidget#qt_calendar_navigationbar { background-color: #F8F9FC; border-bottom: 1px solid #E2E8F0; min-height: 36px; }"
            "QCalendarWidget QToolButton { color: #24253D; font-weight: bold; background: transparent; padding: 4px; border-radius: 4px; }"
            "QCalendarWidget QToolButton:hover { background-color: #EBE5FF; color: #6A5AE0; }"
            "QCalendarWidget QToolButton::menu-indicator { image: none; }"
            "QCalendarWidget QSpinBox { background: transparent; color: #24253D; font-weight: bold; selection-background-color: #6A5AE0; }"
            "QCalendarWidget QSpinBox::up-button, QCalendarWidget QSpinBox::down-button { subcontrol-origin: border; width: 0px; }"
            "QCalendarWidget QMenu { background-color: #FFFFFF; color: #24253D; border: 1px solid #E2E8F0; }"
            "QCalendarWidget QMenu::item:selected { background-color: #EBE5FF; color: #6A5AE0; }"
            "QCalendarWidget QAbstractItemView:enabled { color: #24253D; background-color: #FFFFFF; selection-background-color: #6A5AE0; selection-color: #FFFFFF; outline: none; }"
            "QCalendarWidget QAbstractItemView:disabled { color: #B4B8CC; }"
            );
        cal->setGridVisible(false);
        cal->setVerticalHeaderFormat(QCalendarWidget::NoVerticalHeader);
        cal->setMinimumSize(340, 260);
        dateEdit->setCalendarWidget(cal);

        QDate targetDate = QDate::fromString(g.deadline, Qt::ISODate);
        if (targetDate.isValid()) {
            dateEdit->setDate(targetDate);
        } else {
            dateEdit->setDate(QDate::currentDate());
        }

        auto addArrowToWidget = [](QWidget* widget) {
            auto* innerLay = new QHBoxLayout(widget);
            innerLay->setContentsMargins(0, 0, 8, 0);
            innerLay->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

            auto* arrowLbl = new QLabel("▼");
            arrowLbl->setStyleSheet("color: #8A8FA3; font-size: 9px; border: none; background: transparent;");
            arrowLbl->setAttribute(Qt::WA_TransparentForMouseEvents);

            innerLay->addWidget(arrowLbl);
        };

        addArrowToWidget(catCombo);
        addArrowToWidget(subCombo);
        addArrowToWidget(dateEdit);

        auto* nameEdit = new QLineEdit(g.name);
        nameEdit->setPlaceholderText(QStringLiteral("你要做点什么..."));
        nameEdit->setStyleSheet(editStyle);

        auto* delBtn = new QPushButton();
        delBtn->setIcon(delBtn->style()->standardIcon(QStyle::SP_TrashIcon));
        delBtn->setFixedSize(30, 30);
        delBtn->setCursor(Qt::PointingHandCursor);
        delBtn->setStyleSheet(
            "QPushButton { background-color: #FCA5A5; border: none; border-radius: 6px; }"
            "QPushButton:hover { background-color: #F87171; }"
            "QPushButton:pressed { background-color: #EF4444; }"
            );

        auto updateDependencies = [=]() {
            QString cat = catCombo->currentText();
            subCombo->setVisible(cat == QStringLiteral("练习"));
        };

        catCombo->setCurrentText(g.category);
        subCombo->setCurrentText(g.subCategory);
        updateDependencies();

        connect(catCombo, &QComboBox::currentTextChanged, w, [=](){ updateDependencies(); });

        lay->addWidget(catCombo, 0);
        lay->addWidget(subCombo, 0);
        lay->addWidget(nameEdit, 1);
        lay->addWidget(dateEdit, 0);
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

        rows.append({w, catCombo, subCombo, nameEdit, dateEdit, g.current});
        rowsLay->addWidget(w);
    }
};

static QFrame* makeQuickCard(const QString& emoji, const QString& title, const QString& desc, const QString& accent) {
    auto* card = new QFrame;
    card->setObjectName(QStringLiteral("Card"));

    card->setMinimumHeight(130);
    card->setMaximumHeight(150);

    card->setStyleSheet(
        "QFrame#Card { background-color: #FFFFFF; border: 1px solid #E2E8F0; border-radius: 16px; }"
        "QFrame#Card:hover { border-color: #6B7CFF; background-color: #F8F9FC; }"
        );

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
    // 整个 HomePage 的最外层布局，只用来放置 stackedWidget
    auto* rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(0, 0, 0, 0);

    stackedWidget_ = new QStackedWidget(this);
    rootLayout->addWidget(stackedWidget_);

    // =========================================================================
    // ==================== 页面 0：主页视图 (Main Page) ========================
    // =========================================================================
    auto* mainPageWidget = new QWidget(this);
    auto* root = new QVBoxLayout(mainPageWidget);
    root->setContentsMargins(32, 32, 32, 32);
    root->setSpacing(24);

    // ---------------- [头像与欢迎语区域] ----------------
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

    // ---------------- [目标区域大框 (GoalFrame)] ----------------
    auto* goalFrame = new QFrame;
    goalFrame->setObjectName("GoalFrame");
    goalFrame->setStyleSheet("QFrame#GoalFrame { background-color: #FAFAFC; border: 1px solid #E2E8F0; border-radius: 16px; }"
                             "QFrame#GoalFrame:hover { border-color: #6B7CFF; background-color: #F8F9FC; }");
    goalFrame->setMinimumHeight(270);

    // ====== 【核心修改】让大框具备可点击的视觉反馈并安装过滤器 ======
    goalFrame->setCursor(Qt::PointingHandCursor);
    goalFrame->installEventFilter(this);

    auto* goalLay = new QVBoxLayout(goalFrame);
    goalLay->setContentsMargins(24, 20, 24, 16);
    goalLay->setSpacing(16);

    auto* topGoalRow = new QHBoxLayout;
    topGoalRow->setSpacing(12);

    auto* targetIcon = new QLabel(QStringLiteral("🎯"));
    targetIcon->setFixedSize(40, 40);
    targetIcon->setAlignment(Qt::AlignCenter);
    targetIcon->setStyleSheet("background-color: #FFE8D6; border-radius: 12px; font-size: 20px; border: none;");

    auto* titleCol = new QVBoxLayout;
    titleCol->setSpacing(6);
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

    auto* historyBtn = new QPushButton(QStringLiteral("🕒 历史记录"));
    historyBtn->setCursor(Qt::PointingHandCursor);
    historyBtn->setStyleSheet(QStringLiteral(
        "QPushButton { background: #FFFFFF; border: 1px solid #E2E8F0; color: #4A5568; font-size: 12px; font-weight: bold; border-radius: 14px; padding: 6px 12px; }"
        "QPushButton:hover { background: #F7FAFC; color: #6B7CFF; border-color: #6B7CFF; }"
        ));
    connect(historyBtn, &QPushButton::clicked, this, [this]() {
        HistoryGoalsDialog dlg(this);
        dlg.exec();
    });

    auto* editBtn = new QPushButton(QStringLiteral("✏️ 编辑目标"));
    editBtn->setCursor(Qt::PointingHandCursor);
    editBtn->setStyleSheet(QStringLiteral(
        "QPushButton { background: #FFFFFF; border: 1px solid #E2E8F0; color: #4A5568; font-size: 12px; font-weight: bold; border-radius: 14px; padding: 6px 12px; }"
        "QPushButton:hover { background: #F7FAFC; color: #6B7CFF; border-color: #6B7CFF; }"
        ));
    connect(editBtn, &QPushButton::clicked, this, &HomePage::onEditGoalsClicked);

    topGoalRow->addWidget(historyBtn, 0, Qt::AlignTop);
    topGoalRow->addWidget(editBtn, 0, Qt::AlignTop);
    goalLay->addLayout(topGoalRow);

    // 主页内置的简易滚动目标列表
    auto* scrollArea = new QScrollArea;
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setStyleSheet("QScrollArea { background: transparent; border: none; }");

    auto* scrollContent = new QWidget;
    scrollContent->setStyleSheet("background: transparent;");
    subGoalsLayout_ = new QVBoxLayout(scrollContent);
    subGoalsLayout_->setSpacing(12);
    subGoalsLayout_->setContentsMargins(52, 10, 10, 10);
    subGoalsLayout_->setAlignment(Qt::AlignTop);
    scrollArea->setWidget(scrollContent);
    goalLay->addWidget(scrollArea, 1);

    // ---------------- [下方网格卡片功能区] ----------------
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
        auto* card = makeQuickCard(QString::fromUtf8(i.emoji), QString::fromUtf8(i.title), QString::fromUtf8(i.desc), QString::fromUtf8(i.accent));
        cardTargets_.insert(card, int(i.target));
        card->installEventFilter(this);
        grid->addWidget(card, i.row, i.col);
    }

    root->addLayout(headRow);
    root->addWidget(goalFrame);
    root->addLayout(grid);
    root->addStretch();

    stackedWidget_->addWidget(mainPageWidget); // 将主页面加入 Stacking 管理

    // =========================================================================
    // ==================== 页面 1：目标详情展开页 (Detail Page) ===============
    // =========================================================================
    detailPageWidget_ = new QWidget(this);
    auto* detailLayout = new QVBoxLayout(detailPageWidget_);
    detailLayout->setContentsMargins(32, 32, 32, 32);
    detailLayout->setSpacing(20);

    // 顶部导航返回行
    auto* detailHeaderRow = new QHBoxLayout;
    auto* backBtn = new QPushButton(QStringLiteral("⬅  返回首页"));
    backBtn->setCursor(Qt::PointingHandCursor);
    backBtn->setStyleSheet("QPushButton { background: transparent; border: none; color: #6B7CFF; font-size: 16px; font-weight: bold; }"
                           "QPushButton:hover { color: #5A6AE0; }");
    connect(backBtn, &QPushButton::clicked, this, [this]() {
        stackedWidget_->setCurrentIndex(0); // 切回主页
    });

    auto* detailTitleText = new QLabel(QStringLiteral("我的未完成目标明细"));
    detailTitleText->setStyleSheet("font-size: 20px; font-weight: 700; color: #2D3748;");

    auto* detailEditBtn = new QPushButton(QStringLiteral("✏️ 编辑目标"));
    detailEditBtn->setCursor(Qt::PointingHandCursor);
    detailEditBtn->setStyleSheet("QPushButton { background: #6B7CFF; color: white; border-radius: 8px; padding: 6px 14px; font-weight: bold; border: none; }"
                                 "QPushButton:hover { background: #5A6AE0; }");
    connect(detailEditBtn, &QPushButton::clicked, this, &HomePage::onEditGoalsClicked);

    detailHeaderRow->addWidget(backBtn);
    detailHeaderRow->addSpacing(20);
    detailHeaderRow->addWidget(detailTitleText);
    detailHeaderRow->addStretch();
    detailHeaderRow->addWidget(detailEditBtn);
    detailLayout->addLayout(detailHeaderRow);

    // 总进度总览面板
    auto* progressPanel = new QFrame;
    progressPanel->setStyleSheet("QFrame { background: #F8F9FC; border-radius: 12px; border: 1px solid #E2E8F0; }");
    auto* progressPanelLay = new QVBoxLayout(progressPanel);
    progressPanelLay->setContentsMargins(20, 16, 20, 16);

    auto* pTextLay = new QHBoxLayout;
    auto* pStaticLbl = new QLabel(QStringLiteral("当前目标总达成率："));
    pStaticLbl->setStyleSheet("font-size: 14px; color: #4A5568; border: none;");
    detailPercentLabel_ = new QLabel("0%");
    detailPercentLabel_->setStyleSheet("font-size: 18px; font-weight: bold; color: #6B7CFF; border: none;");
    pTextLay->addWidget(pStaticLbl);
    pTextLay->addWidget(detailPercentLabel_);
    pTextLay->addStretch();

    detailProgressBar_ = new QProgressBar;
    detailProgressBar_->setFixedHeight(10);
    detailProgressBar_->setTextVisible(false);

    progressPanelLay->addLayout(pTextLay);
    progressPanelLay->addWidget(detailProgressBar_);
    detailLayout->addWidget(progressPanel);

    // 展开后的独立全量滚动区域
    auto* detailScrollArea = new QScrollArea;
    detailScrollArea->setWidgetResizable(true);
    detailScrollArea->setStyleSheet("QScrollArea { background: transparent; border: none; }"
                                    "QScrollBar:vertical { width: 6px; background: transparent; }"
                                    "QScrollBar::handle:vertical { background: #CBD5E0; border-radius: 3px; }");
    auto* detailScrollContent = new QWidget;
    detailScrollContent->setStyleSheet("background: transparent;");
    detailSubGoalsLayout_ = new QVBoxLayout(detailScrollContent);
    detailSubGoalsLayout_->setSpacing(14);
    detailSubGoalsLayout_->setContentsMargins(10, 10, 10, 10);
    detailSubGoalsLayout_->setAlignment(Qt::AlignTop);

    detailScrollArea->setWidget(detailScrollContent);
    detailLayout->addWidget(detailScrollArea, 1);

    stackedWidget_->addWidget(detailPageWidget_); // 将详情展开面加入 Stacking 管理

    // =========================================================================
    // ==================== 后置通用状态加载初始化 ============================
    // =========================================================================
    refreshGreeting();
    connect(&UserProfile::instance(), &UserProfile::profileChanged, this, &HomePage::refreshGreeting);

    loadGoals();
    updateGoalUI();

    // ==================== 智能通知核心逻辑 ====================

    // 1. 获取所有今天截止 / 已经逾期的未完成任务
    auto getDueTasks = [this]() -> QStringList {
        QStringList dueTasks;
        QDate today = QDate::currentDate();
        for (const auto& g : subGoals_) {
            bool isCompleted = (g.current >= g.target && g.target > 0);
            if (!isCompleted) {
                QDate d = QDate::fromString(g.deadline, Qt::ISODate);
                if (d.isValid() && d <= today) {

                    // 👇 构建提醒前缀
                    QString prefix = g.category;
                    // 如果是“练习”且子类别不为空，就拼接上子类别
                    if (g.category == QStringLiteral("练习") && !g.subCategory.isEmpty()) {
                        prefix += QStringLiteral(" · ") + g.subCategory;
                    }

                    // 最终格式：类别[ · 子类别] · 具体名称
                    dueTasks.append(prefix + QStringLiteral(" - ") + g.name);
                }
            }
        }
        return dueTasks;
    };
    // 2. 触发弹窗和系统通知的通用函数
    auto triggerNotifications = [this](const QStringList& dueTasks) {
        // [修复系统通知不显示]：系统托盘创建后需要一点时间注入系统底层，直接调用会导致被系统抛弃
        if (QSystemTrayIcon::isSystemTrayAvailable()) {
            static QSystemTrayIcon* sysTray = nullptr;
            if (!sysTray) {
                sysTray = new QSystemTrayIcon(this);
                QIcon appIcon = QApplication::windowIcon();
                if (appIcon.isNull()) {
                    QPixmap pix(32, 32);
                    pix.fill(Qt::transparent);
                    appIcon = QIcon(pix);
                }
                sysTray->setIcon(appIcon);
                sysTray->show();
            }

            // 延迟 500 毫秒，等托盘彻底就绪后再发通知，提高成功率
            QTimer::singleShot(500, this, [dueTasks]() {
                QString notifyMsg = QStringLiteral("今天有 %1 个任务需要完成（如：%2），抓紧时间哦！")
                                        .arg(dueTasks.size())
                                        .arg(dueTasks.first());
                sysTray->showMessage(QStringLiteral("AlgeMate 学习提醒 ⏰"), notifyMsg, QSystemTrayIcon::Information, 5000);
            });
        }

        // 应用内精美弹窗
        DeadlineNotifyDialog dlg(dueTasks, this);
        dlg.exec();
    };

    // --- 场景 A：每次打开软件时，只要有紧迫任务就必须弹窗 ---
    QTimer::singleShot(600, this, [this, triggerNotifications, getDueTasks]() {
        QStringList tasks = getDueTasks();
        if (!tasks.isEmpty()) {
            triggerNotifications(tasks);
        }
    });

    // --- 场景 B：挂着软件时，到了指定时间触发（每天一次） ---
    QTimer* timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, [this, triggerNotifications, getDueTasks]() {
        QSettings settings(QStringLiteral("AlgeMate"), QStringLiteral("Settings"));
        QString notifyTimeStr = settings.value(QStringLiteral("NotifyTime"), QStringLiteral("08:00")).toString();
        QTime notifyTime = QTime::fromString(notifyTimeStr, "hh:mm");
        if (!notifyTime.isValid()) notifyTime = QTime(8, 0);

        QDate today = QDate::currentDate();
        QString lastNotifyDate = settings.value(QStringLiteral("LastDailyNotifyDate"), QStringLiteral("")).toString();

        // 判定条件：时间已经过了设定的闹钟，且今天还没执行过“挂机提醒”
        if (QTime::currentTime() >= notifyTime && lastNotifyDate != today.toString(Qt::ISODate)) {
            QStringList tasks = getDueTasks();
            if (!tasks.isEmpty()) {
                settings.setValue(QStringLiteral("LastDailyNotifyDate"), today.toString(Qt::ISODate));
                triggerNotifications(tasks);
            }
        }
    });
    timer->start(60000);
}

void HomePage::saveGoals() {
    QSettings settings(QStringLiteral("AlgeMate"), QStringLiteral("HomeGoals"));
    QJsonArray arr;
    for (const auto& g : subGoals_) {
        QJsonObject obj;
        obj["category"] = g.category;
        obj["subCategory"] = g.subCategory;
        obj["name"] = g.name;
        obj["current"] = g.current;
        obj["target"] = g.target;
        obj["deadline"] = g.deadline;
        arr.append(obj);
    }
    QString jsonStr = QJsonDocument(arr).toJson(QJsonDocument::Compact);
    settings.setValue(QStringLiteral("SubGoalsData"), jsonStr);

    QString todayKey = QStringLiteral("History_") + QDate::currentDate().toString(Qt::ISODate);
    settings.setValue(todayKey, jsonStr);
}

void HomePage::loadGoals() {
    QSettings settings(QStringLiteral("AlgeMate"), QStringLiteral("HomeGoals"));
    QString jsonStr = settings.value(QStringLiteral("SubGoalsData")).toString();

    QString todayKey = QStringLiteral("History_") + QDate::currentDate().toString(Qt::ISODate);
    if (!settings.contains(todayKey) && !jsonStr.isEmpty()) {
        settings.setValue(todayKey, jsonStr);
    }
    if (jsonStr.isEmpty()) return;

    QJsonDocument doc = QJsonDocument::fromJson(jsonStr.toUtf8());
    QJsonArray arr = doc.array();
    subGoals_.clear();

    for (int i = 0; i < arr.size(); ++i) {
        QJsonObject obj = arr[i].toObject();
        SubGoal g;
        g.category = obj["category"].toString();
        g.subCategory = obj["subCategory"].toString();
        g.name = obj["name"].toString();
        g.current = obj["current"].toInt();
        g.target = obj["target"].toInt();
        g.deadline = obj["deadline"].toString();
        subGoals_.append(g);
    }
}

void HomePage::refreshGreeting() {
    auto& u = UserProfile::instance();
    avatarLabel_->setPixmap(u.avatarPixmap(72));
    greetingLabel_->setText(QStringLiteral("%1，%2 👋")
                                .arg(UserProfile::greetingByTime(), u.userName()));
}

void HomePage::setSubGoalProgress(const QString& goalName, int currentProgress) {
    for (int i = 0; i < subGoals_.size(); ++i) {
        if (subGoals_[i].name == goalName) {
            subGoals_[i].current = qMin(currentProgress, subGoals_[i].target);
            updateGoalUI();
            saveGoals();
            return;
        }
    }
}

void HomePage::onEditGoalsClicked() {
    if (editDialog_) {
        editDialog_->raise();
        editDialog_->activateWindow();
        return;
    }

    editDialog_ = new GoalEditDialog(subGoals_, this);
    editDialog_->setAttribute(Qt::WA_DeleteOnClose);
    editDialog_->setWindowModality(Qt::NonModal);

    connect(editDialog_, &QDialog::accepted, this, [this]() {
        if (editDialog_) {
            subGoals_ = editDialog_->getGoals();
            updateGoalUI();
            saveGoals();
        }
    });

    connect(editDialog_, &QObject::destroyed, this, [this]() {
        editDialog_ = nullptr;
    });

    editDialog_->show();
}

void HomePage::updateGoalUI() {
    // 1. 清空主页面的小列表
    QLayoutItem* item;
    while ((item = subGoalsLayout_->takeAt(0)) != nullptr) {
        if (QWidget* w = item->widget()) w->deleteLater();
        delete item;
    }
    // 2. 清空展开页的大列表
    while ((item = detailSubGoalsLayout_->takeAt(0)) != nullptr) {
        if (QWidget* w = item->widget()) w->deleteLater();
        delete item;
    }

    if (subGoals_.isEmpty()) {
        QString emptyText = QStringLiteral("没有待完成的任务💫");
        goalTitleLabel_->setText(emptyText);
        goalPercentLabel_->setText("100%");
        detailPercentLabel_->setText("100%");
        goalProgressBar_->setValue(100);
        detailProgressBar_->setValue(100);
        return;
    }

    goalTitleLabel_->setText(QStringLiteral("学习总进度"));
    double sumOfPercentages = 0.0;
    QList<int> pendingList;
    QList<int> doneList;

    for (int i = 0; i < subGoals_.size(); ++i) {
        const auto& goal = subGoals_[i];
        if (goal.current >= goal.target && goal.target > 0) {
            sumOfPercentages += 100.0;
            doneList.append(i);
        } else {
            pendingList.append(i);
        }
    }

    // 按剩余时间排序
    std::sort(pendingList.begin(), pendingList.end(), [this](int a, int b) {
        QDate da = QDate::fromString(subGoals_[a].deadline, Qt::ISODate);
        QDate db = QDate::fromString(subGoals_[b].deadline, Qt::ISODate);
        if (!da.isValid()) return false;
        if (!db.isValid()) return true;
        return da < db;
    });

    // 辅助闭包：由于两边展现形式完全一致，我们让它同时构造并加进两个布局
    auto createRowUI = [this](int i, bool isCompleted) {
        const auto& goal = subGoals_[i];

        // 同时为主页列表(mode 0)和详情页列表(mode 1)渲染UI
        for (int mode = 0; mode < 2; ++mode) {

            // 👇👇👇 核心修改：如果是展开详情页 (mode == 1) 且目标已完成，则直接跳过不生成界面 👇👇👇
            if (mode == 1 && isCompleted) {
                continue;
            }

            auto* subRow = new QWidget;
            auto* subLay = new QHBoxLayout(subRow);
            subLay->setContentsMargins(0, 4, 0, 4);
            subLay->setSpacing(12);

            if (mode == 1) {
                subRow->setStyleSheet("QWidget { background: #FFFFFF; border: 1px solid #E2E8F0; border-radius: 10px; }");
                subLay->setContentsMargins(12, 10, 12, 10);
            }

            QString themeColor = "#718096";
            QString badgeBg = "#E2E8F0";
            QString badgeText = goal.category;
            QString displayText = goal.name;

            if (goal.category == QStringLiteral("知识点学习")) {
                themeColor = "#38A169";
                badgeBg = "#DCF3EA";
            } else if (goal.category == QStringLiteral("练习")) {
                themeColor = "#6B7CFF";
                badgeBg = "#EBE5FF";
                if (!goal.subCategory.isEmpty()) {
                    badgeText = QStringLiteral("练习 · %1").arg(goal.subCategory);
                }
            } else if (goal.category == QStringLiteral("测试")) {
                themeColor = "#DD6B20";
                badgeBg = "#FFE8D6";
            }

            auto* checkBtn = new QPushButton;
            checkBtn->setCursor(Qt::PointingHandCursor);

            if (isCompleted) {
                int btnSize = 22;
                checkBtn->setFixedSize(btnSize, btnSize);
                checkBtn->setText(QStringLiteral("✓"));
                checkBtn->setStyleSheet(QString(
                                            "QPushButton {"
                                            "  min-width: 22px; max-width: 22px; min-height: 22px; max-height: 22px;"
                                            "  background-color: %1; color: white;"
                                            "  border-radius: 11px; border: none; font-size: 12px; font-weight: bold;"
                                            "  padding: 0px; margin: 0px;"
                                            "}"
                                            ).arg(themeColor));
            } else {
                int btnSize = 30;
                checkBtn->setFixedSize(btnSize, btnSize);
                checkBtn->setText(QStringLiteral(""));
                checkBtn->setStyleSheet(QString(
                                            "QPushButton {"
                                            "  min-width: 30px; max-width: 30px; min-height: 30px; max-height: 30px;"
                                            "  background-color: transparent; border: 2px solid #CBD5E0;"
                                            "  border-radius: 15px; padding: 0px; margin: 0px;"
                                            "}"
                                            "QPushButton:hover { border-color: %1; background-color: %2; }"
                                            ).arg(themeColor, badgeBg));
            }

            connect(checkBtn, &QPushButton::clicked, this, [this, i]() {
                bool currentlyDone = (subGoals_[i].current >= subGoals_[i].target && subGoals_[i].target > 0);
                if (currentlyDone) {
                    subGoals_[i].current = 0;
                } else {
                    subGoals_[i].current = subGoals_[i].target;
                }
                updateGoalUI();
                saveGoals();
            });

            auto* badgeLbl = new QLabel(badgeText);
            badgeLbl->setAlignment(Qt::AlignCenter);
            badgeLbl->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);

            if (isCompleted) {
                badgeLbl->setStyleSheet(QStringLiteral("background-color: #F7FAFC; color: #A0AEC0; padding: 4px 10px; border-radius: 6px; font-size: 11px; font-weight: bold; border: none;"));
            } else {
                badgeLbl->setStyleSheet(QStringLiteral("background-color: %1; color: %2; padding: 4px 10px; border-radius: 6px; font-size: 11px; font-weight: bold; border: none;").arg(badgeBg, themeColor));
            }

            auto* nameLbl = new QLabel(displayText);
            nameLbl->setWordWrap(true);
            nameLbl->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

            if (isCompleted) {
                nameLbl->setStyleSheet("color: #A0AEC0; font-size: 14px; text-decoration: line-through; border: none; background: transparent;");
            } else {
                nameLbl->setStyleSheet("color: #2D3748; font-size: 14px; font-weight: 500; border: none; background: transparent;");
            }

            subLay->addWidget(checkBtn, 0, Qt::AlignVCenter);
            subLay->addWidget(badgeLbl, 0, Qt::AlignVCenter);
            subLay->addWidget(nameLbl, 1, Qt::AlignVCenter);

            if (!goal.deadline.isEmpty()) {
                QDate targetDate = QDate::fromString(goal.deadline, Qt::ISODate);
                if (targetDate.isValid()) {
                    qint64 daysLeft = QDate::currentDate().daysTo(targetDate);
                    QString timeText;
                    QString timeStyle;

                    // 👇👇👇 核心修改：明确显式具体的“截止日期”并在括号内加上倒计时 👇👇👇
                    QString dateStr = targetDate.toString("MM/dd");

                    if (isCompleted) {
                        timeText = QStringLiteral("已达成");
                        timeStyle = "color: #CBD5E0; font-size: 12px; border: none; font-weight: bold;";
                    }
                    else if (daysLeft < 0) {
                        timeText = QStringLiteral("%1 (已逾期)").arg(dateStr);
                        timeStyle = "color: #E53E3E; font-size: 12px; border: none; font-weight: bold;";
                    }
                    else if (daysLeft <= 1) {
                        timeText = QStringLiteral("%1 %2").arg(dateStr, daysLeft == 0 ? QStringLiteral("(今日截止)") : QStringLiteral("(剩余 1 天)"));
                        timeStyle = "color: #E53E3E; font-size: 12px; border: none; font-weight: bold;";
                    }
                    else {
                        timeText = QStringLiteral("%1 (剩余 %2 天)").arg(dateStr).arg(daysLeft);
                        timeStyle = "color: #A0AEC0; font-size: 12px; border: none; font-weight: bold;";
                    }

                    auto* dateLbl = new QLabel(QStringLiteral("⏳ ") + timeText);
                    dateLbl->setStyleSheet(timeStyle + " background: transparent;");
                    subLay->addWidget(dateLbl, 0, Qt::AlignVCenter);
                }
            }

            if (mode == 0) subGoalsLayout_->addWidget(subRow);
            else detailSubGoalsLayout_->addWidget(subRow);
        }
    };

    for (int idx : pendingList) createRowUI(idx, false);
    for (int idx : doneList) createRowUI(idx, true);

    // 👇 附加优化：如果详情页的未完成任务全部清空了，展示一个庆祝文本
    if (pendingList.isEmpty()) {
        auto* emptyDetailLbl = new QLabel(QStringLiteral("🎉 太棒啦！当前阶段任务已全部清理完毕！"));
        emptyDetailLbl->setStyleSheet("color: #8A8FA3; font-size: 14px; margin-top: 30px; border: none;");
        emptyDetailLbl->setAlignment(Qt::AlignCenter);
        detailSubGoalsLayout_->addWidget(emptyDetailLbl);
    }

    // 更新两个页面的百分比与进度条
    int avgPercent = subGoals_.isEmpty() ? 0 : qRound(sumOfPercentages / subGoals_.size());
    goalProgressBar_->setValue(avgPercent);
    detailProgressBar_->setValue(avgPercent);
    goalPercentLabel_->setText(QString("%1%").arg(avgPercent));
    detailPercentLabel_->setText(QString("%1%").arg(avgPercent));

    // 进度条颜色微调
    QString barStyle = avgPercent >= 100 ? "QProgressBar { background-color: #E2E8F0; border-radius: 4px; border: none; } QProgressBar::chunk { background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #81C784, stop:1 #4CAF50); border-radius: 4px; }"
                                         : "QProgressBar { background-color: #E2E8F0; border-radius: 4px; border: none; } QProgressBar::chunk { background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #8E9EFF, stop:1 #6B7CFF); border-radius: 4px; }";
    goalProgressBar_->setStyleSheet(barStyle);
    detailProgressBar_->setStyleSheet(barStyle);
}

bool HomePage::eventFilter(QObject* obj, QEvent* e) {
    // 保持你原本拦截主窗体关闭以确认未保存编辑的逻辑不变
    if (e->type() == QEvent::Close && obj == this->window()) {
        if (editDialog_ && editDialog_->isVisible()) {
            int ans = QMessageBox::question(this, QStringLiteral("关闭提示"), QStringLiteral("有未保存的目标，是否继续关闭窗口？"), QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
            if (ans == QMessageBox::No) return true;
        }
    }

    if (e->type() == QEvent::MouseButtonRelease) {
        auto* me = static_cast<QMouseEvent*>(e);
        if (me->button() == Qt::LeftButton) {

            // ====== 【核心修改】点击 GoalFrame 区域时，无缝平滑切到展开详情页 ======
            if (obj->objectName() == QStringLiteral("GoalFrame")) {
                if (stackedWidget_) {
                    stackedWidget_->setCurrentIndex(1); // 切换到详情页面 (Detail Page)
                }
                return true;
            }

            auto it = cardTargets_.constFind(obj);
            if (it != cardTargets_.constEnd()) {
                emit requestNavigate(it.value());
                return true;
            }
        }
    }
    return QWidget::eventFilter(obj, e);
}
void HomePage::showGoalDetail() {
    if (stackedWidget_ && stackedWidget_->currentIndex() != 1) {
        stackedWidget_->setCurrentIndex(1); // 强制切到详情页
    }
}
}