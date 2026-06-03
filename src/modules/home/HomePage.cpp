#include "HomePage.h"
#include "core/UserProfile.h"
#include "core/ThemeManager.h"
#include <QGraphicsDropShadowEffect>
#include <QPainter>
#include <QPainterPath>

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

// ==================== 柔和插图风格的背景卡片 ====================
class BlobCard : public QFrame {
public:
    explicit BlobCard(const QString& accentColorStr, QWidget* parent = nullptr)
        : QFrame(parent), m_accentColor(accentColorStr) {
        auto* shadow = new QGraphicsDropShadowEffect(this);
        shadow->setOffset(0, 6);
        shadow->setBlurRadius(18);
        QColor shadowColor(m_accentColor);
        shadowColor = shadowColor.darker(140);
        shadowColor.setAlpha(25);
        shadow->setColor(shadowColor);
        this->setGraphicsEffect(shadow);
    }
protected:
    void paintEvent(QPaintEvent *event) override {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);

        bool isDark = AlgeMate::ThemeManager::instance().currentTheme() == AlgeMate::ThemeManager::Theme::Dark;
        QColor baseColor(m_accentColor);

        // 深色模式下，将主题色融入深色背景中
        if (isDark) {
            QColor darkBase("#232236");
            baseColor.setRedF(darkBase.redF() * 0.85 + baseColor.redF() * 0.15);
            baseColor.setGreenF(darkBase.greenF() * 0.85 + baseColor.greenF() * 0.15);
            baseColor.setBlueF(darkBase.blueF() * 0.85 + baseColor.blueF() * 0.15);
        }

        QPainterPath bgPath;
        bgPath.addRoundedRect(rect(), 24, 24);
        painter.fillPath(bgPath, baseColor);

        painter.save();
        painter.setClipPath(bgPath);

        QColor decColor1, decColor2, decColor3;
        if (isDark) {
            QColor accent(m_accentColor);
            accent.setAlphaF(0.08);
            decColor1 = decColor3 = accent;
            accent.setAlphaF(0.04);
            decColor2 = accent;
        } else {
            decColor1 = baseColor.darker(103);
            decColor2 = baseColor.darker(106);
            decColor3 = baseColor.darker(110);
        }

        painter.setPen(Qt::NoPen);
        painter.setBrush(decColor1);
        painter.drawEllipse(width() - 75, -25, 110, 110);

        painter.setBrush(decColor2);
        painter.translate(width() - 25, height() - 10);
        painter.rotate(-25);
        painter.drawRoundedRect(-40, -30, 80, 50, 20, 20);
        painter.resetTransform();

        painter.setBrush(decColor3);
        painter.drawEllipse(width() - 95, height() / 2 + 10, 12, 12);
        painter.drawEllipse(width() - 40, 25, 8, 8);

        painter.restore();
    }
private:
    QColor m_accentColor;
};

// ==================== 智能到期提醒弹窗 ====================
class DeadlineNotifyDialog : public QDialog {
public:
    explicit DeadlineNotifyDialog(const QStringList& tasks, QWidget* parent = nullptr) : QDialog(parent) {
        setWindowTitle(QStringLiteral("今日截止提醒"));
        setMinimumWidth(340);
        bool isDark = AlgeMate::ThemeManager::instance().currentTheme() == AlgeMate::ThemeManager::Theme::Dark;
        setStyleSheet(isDark ? "QDialog { background-color: #1C1B2E; }" : "QDialog { background-color: #FAFAFC; }");

        auto* lay = new QVBoxLayout(this);
        lay->setContentsMargins(24, 32, 24, 24);
        lay->setSpacing(16);

        auto* iconLbl = new QLabel(QStringLiteral("⏰"));
        iconLbl->setAlignment(Qt::AlignCenter);
        iconLbl->setStyleSheet("font-size: 42px; border: none; background: transparent;");
        lay->addWidget(iconLbl);

        auto* titleLbl = new QLabel(QStringLiteral("以下任务请尽快完成哦~"));
        titleLbl->setAlignment(Qt::AlignCenter);
        titleLbl->setStyleSheet(isDark ? "font-size: 18px; font-weight: bold; color: #FC8181; border: none; background: transparent;"
                                       : "font-size: 18px; font-weight: bold; color: #E53E3E; border: none; background: transparent;");
        lay->addWidget(titleLbl);

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
        descLbl->setStyleSheet(isDark ? "font-size: 14px; color: #E6E7F0; line-height: 1.5; border: none; background: transparent;"
                                      : "font-size: 14px; color: #4A5568; line-height: 1.5; border: none; background: transparent;");
        descLbl->setWordWrap(true);
        lay->addWidget(descLbl);

        auto* btn = new QPushButton(QStringLiteral("去完成"));
        btn->setCursor(Qt::PointingHandCursor);
        btn->setFixedHeight(40);
        btn->setStyleSheet(isDark
                               ? "QPushButton { background-color: #312F4A; color: #8FA1FF; border-radius: 8px; font-size: 14px; font-weight: bold; border: none; } QPushButton:hover { background-color: #3B395A; }"
                               : "QPushButton { background-color: #6B7CFF; color: white; border-radius: 8px; font-size: 14px; font-weight: bold; border: none; } QPushButton:hover { background-color: #5A6AE0; }");
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
        bool isDark = AlgeMate::ThemeManager::instance().currentTheme() == AlgeMate::ThemeManager::Theme::Dark;
        setStyleSheet(isDark ? "QDialog { background-color: #1C1B2E; }" : "QDialog { background-color: #FAFAFC; }");

        auto* mainLay = new QVBoxLayout(this);
        mainLay->setSpacing(16);
        mainLay->setContentsMargins(24, 24, 24, 24);

        auto* topLay = new QHBoxLayout;

        auto* prevBtn = new QToolButton;
        prevBtn->setText(QStringLiteral("<"));
        prevBtn->setStyleSheet(isDark ? "color: #8FA1FF; font-weight: bold; border: none; font-size: 16px;" : "color: #6B7CFF; font-weight: bold; border: none; font-size: 16px;");
        prevBtn->setCursor(Qt::PointingHandCursor);

        dateEdit = new QDateEdit(QDate::currentDate());
        dateEdit->setDisplayFormat(QStringLiteral("yyyy 年 MM 月 dd 日"));
        dateEdit->setCalendarPopup(true);
        dateEdit->setMinimumDate(QDate::currentDate().addYears(-1));
        dateEdit->setMaximumDate(QDate::currentDate());
        dateEdit->setFocusPolicy(Qt::StrongFocus);
        dateEdit->setReadOnly(false);
        dateEdit->setCursor(Qt::IBeamCursor);

        dateEdit->setStyleSheet(isDark ? R"(
            QDateEdit { border: 1px solid #3B395A; border-radius: 6px; padding: 4px 8px; background: #28263F; font-size: 14px; color: #E6E7F0; }
            QDateEdit:focus { border: 1.5px solid #8FA1FF; }
            QDateEdit::drop-down { subcontrol-origin: padding; subcontrol-position: top right; width: 26px; border-left: none; background: transparent; }
            QDateEdit::down-arrow { image: none; }
        )" : R"(
            QDateEdit { border: 1px solid #E2E8F0; border-radius: 6px; padding: 4px 8px; background: #FFFFFF; font-size: 14px; color: #333333; }
            QDateEdit:focus { border: 1.5px solid #6B7CFF; }
            QDateEdit::drop-down { subcontrol-origin: padding; subcontrol-position: top right; width: 26px; border-left: none; background: transparent; }
            QDateEdit::down-arrow { image: none; }
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
        cal->setStyleSheet(isDark ?
                               "QCalendarWidget QWidget#qt_calendar_navigationbar { background-color: #28263F; border-bottom: 1px solid #3B395A; min-height: 36px; }"
                               "QCalendarWidget QToolButton { color: #E6E7F0; font-weight: bold; background: transparent; padding: 4px; border-radius: 4px; }"
                               "QCalendarWidget QToolButton:hover { background-color: #3B395A; color: #8FA1FF; }"
                               "QCalendarWidget QToolButton::menu-indicator { image: none; }"
                               "QCalendarWidget QSpinBox { background: transparent; color: #E6E7F0; font-weight: bold; selection-background-color: #8FA1FF; }"
                               "QCalendarWidget QSpinBox::up-button, QCalendarWidget QSpinBox::down-button { subcontrol-origin: border; width: 0px; }"
                               "QCalendarWidget QMenu { background-color: #1C1B2E; color: #E6E7F0; border: 1px solid #3B395A; }"
                               "QCalendarWidget QMenu::item:selected { background-color: #3B395A; color: #8FA1FF; }"
                               "QCalendarWidget QAbstractItemView:enabled { color: #E6E7F0; background-color: #1C1B2E; selection-background-color: #8FA1FF; selection-color: #1C1B2E; outline: none; }"
                               "QCalendarWidget QAbstractItemView:disabled { color: #4B4970; }"
                                  :
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
        nextBtn->setStyleSheet(isDark ? "color: #8FA1FF; font-weight: bold; border: none; font-size: 16px;" : "color: #6B7CFF; font-weight: bold; border: none; font-size: 16px;");
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
        bool isDark = AlgeMate::ThemeManager::instance().currentTheme() == AlgeMate::ThemeManager::Theme::Dark;
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
            row->setStyleSheet(isDark ? "QWidget { background: #28263F; border: 1px solid #3B395A; border-radius: 8px; }" : "QWidget { background: #FFFFFF; border: 1px solid #E2E8F0; border-radius: 8px; }");

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
                badgeLbl->setStyleSheet(isDark ? "background-color: #3B395A; color: #7B7B96; padding: 4px 8px; border-radius: 6px; font-size: 11px; font-weight: bold; border: none;"
                                               : "background-color: #F7FAFC; color: #A0AEC0; padding: 4px 8px; border-radius: 6px; font-size: 11px; font-weight: bold; border: none;");
            } else {
                badgeLbl->setStyleSheet(isDark ? "background-color: #312F4A; color: #8FA1FF; padding: 4px 8px; border-radius: 6px; font-size: 11px; font-weight: bold; border: none;"
                                               : "background-color: #EBE5FF; color: #6B7CFF; padding: 4px 8px; border-radius: 6px; font-size: 11px; font-weight: bold; border: none;");
            }

            auto* nameLbl = new QLabel(name);
            nameLbl->setWordWrap(true);
            if (isDone) {
                nameLbl->setStyleSheet(isDark ? "color: #7B7B96; font-size: 13px; text-decoration: line-through; border: none; background: transparent;"
                                              : "color: #A0AEC0; font-size: 13px; text-decoration: line-through; border: none; background: transparent;");
            } else {
                nameLbl->setStyleSheet(isDark ? "color: #E6E7F0; font-size: 13px; font-weight: 500; border: none; background: transparent;"
                                              : "color: #2D3748; font-size: 13px; font-weight: 500; border: none; background: transparent;");
            }

            hLay->addWidget(checkLbl);
            hLay->addWidget(badgeLbl);
            hLay->addWidget(nameLbl, 1);
            listLay->addWidget(row);
        }

        statusLbl->setText(QStringLiteral("当天共 %1 个任务，完成了 %2 个").arg(totalCount).arg(completedCount));
    }
};

// ==================== 自定义目标编辑弹窗 ====================
class GoalEditDialog : public QDialog {
public:
    GoalEditDialog(const QList<SubGoal>& currentGoals, QWidget* parent = nullptr) : QDialog(parent) {
        setWindowTitle(QStringLiteral("编辑学习目标"));
        setMinimumWidth(660);
        setMinimumHeight(400);
        bool isDark = AlgeMate::ThemeManager::instance().currentTheme() == AlgeMate::ThemeManager::Theme::Dark;
        setStyleSheet(isDark ? "QDialog { background-color: #1C1B2E; }" : "QDialog { background-color: #FAFAFC; }");

        auto* mainLay = new QVBoxLayout(this);
        mainLay->setSpacing(16);

        auto* titleLbl = new QLabel(QStringLiteral("设定你的阶段性学习目标："));
        titleLbl->setStyleSheet(isDark ? "font-size: 15px; font-weight: bold; color: #E6E7F0;" : "font-size: 15px; font-weight: bold; color: #333333;");
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
        addBtn->setStyleSheet(isDark
                                  ? "QPushButton { background-color: #312F4A; color: #8FA1FF; border: none; border-radius: 8px; padding: 10px; font-weight: bold; font-size: 13px; } QPushButton:hover { background-color: #3B395A; }"
                                  : "QPushButton { background-color: #F0F4FF; color: #6B7CFF; border: none; border-radius: 8px; padding: 10px; font-weight: bold; font-size: 13px; } QPushButton:hover { background-color: #E6E9FF; }");
        connect(addBtn, &QPushButton::clicked, this, [this](){
            SubGoal emptyGoal;
            emptyGoal.deadline = QDate::currentDate().toString(Qt::ISODate);
            addRow(emptyGoal);
        });
        mainLay->addWidget(addBtn);

        auto* btnBox = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Cancel);
        btnBox->button(QDialogButtonBox::Save)->setText(QStringLiteral("保存"));
        btnBox->button(QDialogButtonBox::Cancel)->setText(QStringLiteral("取消"));
        btnBox->button(QDialogButtonBox::Save)->setStyleSheet(isDark ? "background-color: #312F4A; color: #8FA1FF; font-weight: bold;" : "background-color: #6B7CFF; color: white; font-weight: bold;");

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
        bool isDark = AlgeMate::ThemeManager::instance().currentTheme() == AlgeMate::ThemeManager::Theme::Dark;
        auto* w = new QWidget;
        auto* lay = new QHBoxLayout(w);
        lay->setContentsMargins(0, 0, 0, 0);
        lay->setSpacing(8);

        QString boxStyle = isDark ? R"(
            QComboBox, QDateEdit { border: 1px solid #3B395A; border-radius: 6px; padding: 4px 24px 4px 8px; background: #28263F; font-size: 12px; min-width: 70px; color: #E6E7F0; }
            QComboBox:focus, QDateEdit:focus { border: 1.5px solid #8FA1FF; }
            QComboBox::drop-down, QDateEdit::drop-down { border: none; background: transparent; width: 24px; }
            QComboBox::down-arrow, QDateEdit::down-arrow { image: none; }
            QComboBox QAbstractItemView { background: #1C1B2E; border: 1px solid #3B395A; selection-background-color: #3B395A; selection-color: #8FA1FF; outline: none; }
        )" : R"(
            QComboBox, QDateEdit { border: 1px solid #E2E8F0; border-radius: 6px; padding: 4px 24px 4px 8px; background: #FFFFFF; font-size: 12px; min-width: 70px; color: #333333; }
            QComboBox:focus, QDateEdit:focus { border: 1.5px solid #6B7CFF; }
            QComboBox::drop-down, QDateEdit::drop-down { border: none; background: transparent; width: 24px; }
            QComboBox::down-arrow, QDateEdit::down-arrow { image: none; }
            QComboBox QAbstractItemView { background: #FFFFFF; border: 1px solid #E2E8F0; selection-background-color: #EBE5FF; selection-color: #6B7CFF; outline: none; }
        )";

        QString editStyle = isDark ? "QLineEdit { border: 1px solid #3B395A; border-radius: 6px; padding: 6px; background: #28263F; font-size: 12px; color: #E6E7F0; }"
                                   : "QLineEdit { border: 1px solid #E2E8F0; border-radius: 6px; padding: 6px; background: #FFFFFF; font-size: 12px; }";

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
        cal->setStyleSheet(isDark ?
                               "QCalendarWidget QWidget#qt_calendar_navigationbar { background-color: #28263F; border-bottom: 1px solid #3B395A; min-height: 36px; }"
                               "QCalendarWidget QToolButton { color: #E6E7F0; font-weight: bold; background: transparent; padding: 4px; border-radius: 4px; }"
                               "QCalendarWidget QToolButton:hover { background-color: #3B395A; color: #8FA1FF; }"
                               "QCalendarWidget QToolButton::menu-indicator { image: none; }"
                               "QCalendarWidget QSpinBox { background: transparent; color: #E6E7F0; font-weight: bold; selection-background-color: #8FA1FF; }"
                               "QCalendarWidget QSpinBox::up-button, QCalendarWidget QSpinBox::down-button { subcontrol-origin: border; width: 0px; }"
                               "QCalendarWidget QMenu { background-color: #1C1B2E; color: #E6E7F0; border: 1px solid #3B395A; }"
                               "QCalendarWidget QMenu::item:selected { background-color: #3B395A; color: #8FA1FF; }"
                               "QCalendarWidget QAbstractItemView:enabled { color: #E6E7F0; background-color: #1C1B2E; selection-background-color: #8FA1FF; selection-color: #1C1B2E; outline: none; }"
                               "QCalendarWidget QAbstractItemView:disabled { color: #4B4970; }"
                                  :
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

// ==================== 插图化排版网格卡片构造 ====================
static QFrame* makeQuickCard(const QString& emoji, const QString& title, const QString& desc, const QString& accent) {
    auto* card = new BlobCard(accent);
    card->setObjectName(QStringLiteral("QuickCard"));
    card->setMinimumHeight(140);
    card->setMaximumHeight(160);
    card->setCursor(Qt::PointingHandCursor);
    card->setStyleSheet("background: transparent; border: none;");

    auto* lay = new QHBoxLayout(card);
    lay->setContentsMargins(24, 24, 20, 24);
    lay->setSpacing(12);

    auto* textCol = new QVBoxLayout;
    textCol->setSpacing(6);

    auto* t = new QLabel(title);
    t->setObjectName(QStringLiteral("QuickCardTitle"));
    t->setStyleSheet("font-size: 18px; font-weight: 800;");

    auto* d = new QLabel(desc);
    d->setObjectName(QStringLiteral("QuickCardDesc"));
    d->setStyleSheet("font-size: 12px; line-height: 1.4;");
    d->setWordWrap(true);
    d->setAlignment(Qt::AlignLeft | Qt::AlignTop);

    textCol->addWidget(t);
    textCol->addWidget(d);
    textCol->addStretch();

    auto* icon = new QLabel(emoji);
    icon->setFixedSize(54, 54);
    icon->setAlignment(Qt::AlignCenter);
    icon->setStyleSheet("font-size: 46px; background: transparent; border: none;");

    lay->addLayout(textCol, 1);
    lay->addWidget(icon, 0, Qt::AlignVCenter | Qt::AlignRight);

    return card;
}

HomePage::HomePage(QWidget* parent) : QWidget(parent) {
    auto* rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(0, 0, 0, 0);

    stackedWidget_ = new QStackedWidget(this);
    rootLayout->addWidget(stackedWidget_);

    auto* mainPageWidget = new QWidget(this);
    auto* root = new QVBoxLayout(mainPageWidget);

    // 【修改点 1】：缩小上边距（32 -> 16），让整个界面向上平移
    root->setContentsMargins(32, 16, 32, 24);
    // 【修改点 2】：缩小各个大板块之间的垂直间距（24 -> 16）
    root->setSpacing(16);

    auto* headRow = new QHBoxLayout;
    headRow->setSpacing(18);

    avatarLabel_ = new QLabel;
    avatarLabel_->setFixedSize(72, 72);

    auto* textCol = new QVBoxLayout;
    textCol->setSpacing(4);
    greetingLabel_ = new QLabel;
    subtitleLabel_ = new QLabel(QStringLiteral("今天也要在线性代数里找到属于自己的节奏 ✨"));
    textCol->addWidget(greetingLabel_);
    textCol->addWidget(subtitleLabel_);

    headRow->addWidget(avatarLabel_);
    headRow->addLayout(textCol, 1);

    auto* settingsBtn = new QPushButton();
    settingsBtn->setCursor(Qt::PointingHandCursor);
    settingsBtn->setFixedSize(150, 42);

    auto* btnLay = new QHBoxLayout(settingsBtn);
    btnLay->setContentsMargins(12, 0, 12, 0);
    btnLay->setSpacing(6);

    auto* iconLabel = new QLabel(QStringLiteral("⚙"));
    iconLabel->setObjectName("settingsIcon");
    iconLabel->setAlignment(Qt::AlignCenter);

    auto* textLabel = new QLabel(QStringLiteral("设置中心"));
    textLabel->setObjectName("settingsText");
    textLabel->setAlignment(Qt::AlignVCenter | Qt::AlignLeft);

    btnLay->addWidget(iconLabel);
    btnLay->addWidget(textLabel);
    btnLay->addStretch();

    connect(settingsBtn, &QPushButton::clicked, this, [this]() {
        emit requestNavigate(Settings);
    });
    headRow->addWidget(settingsBtn, 0, Qt::AlignTop);

    auto* goalFrame = new QFrame;
    goalFrame->setObjectName("GoalFrame");

    // 【修改点 3】：不再写死 340 的高了，改回相对娇小的 240。如果屏幕高，stretch=1 会把它拉长；如果屏幕矮，它也不会和下方的卡片打架了！
    goalFrame->setMinimumHeight(240);
    goalFrame->setCursor(Qt::PointingHandCursor);
    goalFrame->installEventFilter(this);

    auto* frameShadow = new QGraphicsDropShadowEffect(this);
    frameShadow->setBlurRadius(25);
    frameShadow->setOffset(0, 8);
    goalFrame->setGraphicsEffect(frameShadow);

    auto* goalLay = new QVBoxLayout(goalFrame);
    goalLay->setContentsMargins(32, 28, 32, 24);
    goalLay->setSpacing(20);

    auto* topGoalRow = new QHBoxLayout;
    topGoalRow->setSpacing(16);

    auto* titleCol = new QVBoxLayout;
    titleCol->setSpacing(2);
    goalTitleLabel_ = new QLabel();
    goalTitleLabel_->setObjectName(QStringLiteral("GoalTitle"));
    goalTitleLabel_->setStyleSheet("font-size: 14px; font-weight: bold; color: #8A8FA3;");

    goalPercentLabel_ = new QLabel();
    goalPercentLabel_->setObjectName(QStringLiteral("GoalPercent"));
    goalPercentLabel_->setStyleSheet("font-size: 36px; font-weight: 900; letter-spacing: -1px;");

    titleCol->addWidget(goalTitleLabel_);
    titleCol->addWidget(goalPercentLabel_);
    topGoalRow->addLayout(titleCol);

    topGoalRow->addStretch();

    auto* historyBtn = new QPushButton(QStringLiteral("🕒 历史"));
    historyBtn->setObjectName(QStringLiteral("GoalSecondaryBtn"));
    historyBtn->setCursor(Qt::PointingHandCursor);
    historyBtn->setFixedSize(80, 32);
    connect(historyBtn, &QPushButton::clicked, this, [this]() {
        HistoryGoalsDialog dlg(this);
        dlg.exec();
    });

    auto* editBtn = new QPushButton(QStringLiteral("✏️ 编辑"));
    editBtn->setObjectName(QStringLiteral("GoalSecondaryBtn"));
    editBtn->setCursor(Qt::PointingHandCursor);
    editBtn->setFixedSize(80, 32);
    connect(editBtn, &QPushButton::clicked, this, &HomePage::onEditGoalsClicked);

    topGoalRow->addWidget(historyBtn, 0, Qt::AlignTop);
    topGoalRow->addWidget(editBtn, 0, Qt::AlignTop);

    goalLay->addLayout(topGoalRow);

    goalProgressBar_ = new QProgressBar;
    goalProgressBar_->setFixedHeight(12);
    goalProgressBar_->setTextVisible(false);
    goalLay->addWidget(goalProgressBar_);

    auto* scrollArea = new QScrollArea;
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setStyleSheet("QScrollArea { background: transparent; border: none; }");

    auto* scrollContent = new QWidget;
    scrollContent->setStyleSheet("background: transparent;");
    subGoalsLayout_ = new QVBoxLayout(scrollContent);
    subGoalsLayout_->setSpacing(12);
    subGoalsLayout_->setContentsMargins(0, 10, 0, 10);
    subGoalsLayout_->setAlignment(Qt::AlignTop);
    scrollArea->setWidget(scrollContent);
    goalLay->addWidget(scrollArea, 1);

    auto* grid = new QGridLayout;
    grid->setSpacing(16);

    struct CardInfo { const char* emoji; const char* title; const char* desc; const char* accent; NavTarget target; int row; int col; };
    const CardInfo infos[] = {
        { "🧮", "打开计算助手",   "矩阵运算、方程组求解\n行列式、秩、特征值", "#F3EEFF", Calculator, 0, 0 },
        { "🤖", "AI 智能解题",     "输入题目，让 AI 帮你\n分步讲解与深度剖析", "#FFF1E6", AiSolver,   0, 1 },
        { "📘", "知识点学习",     "核心章节、图文结构\n经典例题快速检索",   "#E6F7F1", Knowledge,  1, 0 },
        { "📈", "学习中心",         "任务进度、错题打卡\n及智能薄弱点推荐",   "#ECF0FF", Learning,   1, 1 }
    };
    for (const auto& i : infos) {
        auto* card = makeQuickCard(QString::fromUtf8(i.emoji), QString::fromUtf8(i.title), QString::fromUtf8(i.desc), QString::fromUtf8(i.accent));
        cardTargets_.insert(card, int(i.target));
        card->installEventFilter(this);
        grid->addWidget(card, i.row, i.col);
    }

    root->addLayout(headRow);
    root->addWidget(goalFrame, 1);  // 保留 stretch=1，填满剩余空间
    root->addLayout(grid);

    stackedWidget_->addWidget(mainPageWidget);

    auto* detailPageWidget = new QWidget(this);
    auto* detailLayout = new QVBoxLayout(detailPageWidget);
    detailLayout->setContentsMargins(32, 32, 32, 32);
    detailLayout->setSpacing(20);

    auto* detailHeaderRow = new QHBoxLayout;
    auto* backBtn = new QPushButton(QStringLiteral("⬅  返回首页"));
    backBtn->setCursor(Qt::PointingHandCursor);
    connect(backBtn, &QPushButton::clicked, this, [this]() {
        stackedWidget_->setCurrentIndex(0);
    });

    auto* detailTitleText = new QLabel(QStringLiteral("我的未完成目标明细"));

    auto* detailEditBtn = new QPushButton(QStringLiteral("✏️ 编辑目标"));
    detailEditBtn->setCursor(Qt::PointingHandCursor);
    connect(detailEditBtn, &QPushButton::clicked, this, &HomePage::onEditGoalsClicked);

    detailHeaderRow->addWidget(backBtn);
    detailHeaderRow->addSpacing(20);
    detailHeaderRow->addWidget(detailTitleText);
    detailHeaderRow->addStretch();
    detailHeaderRow->addWidget(detailEditBtn);
    detailLayout->addLayout(detailHeaderRow);

    auto* progressPanel = new QFrame;
    auto* progressPanelLay = new QVBoxLayout(progressPanel);
    progressPanelLay->setContentsMargins(20, 16, 20, 16);

    auto* pTextLay = new QHBoxLayout;
    auto* pStaticLbl = new QLabel(QStringLiteral("当前目标总达成率："));
    detailPercentLabel_ = new QLabel("0%");
    detailPercentLabel_->setStyleSheet("font-size: 18px; font-weight: bold; color: #6B7CFF; border: none;");
    pTextLay->addWidget(pStaticLbl);
    pTextLay->addWidget(detailPercentLabel_);
    pTextLay->addStretch();

    detailProgressBar_ = new QProgressBar;
    detailProgressBar_->setFixedHeight(12);
    detailProgressBar_->setTextVisible(false);

    progressPanelLay->addLayout(pTextLay);
    progressPanelLay->addWidget(detailProgressBar_);
    detailLayout->addWidget(progressPanel);

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

    stackedWidget_->addWidget(detailPageWidget);

    auto applyTheme = [this, settingsBtn, goalFrame, frameShadow, historyBtn, editBtn, detailTitleText, progressPanel, pStaticLbl, backBtn, detailEditBtn]() {
        bool isDark = AlgeMate::ThemeManager::instance().currentTheme() == AlgeMate::ThemeManager::Theme::Dark;

        greetingLabel_->setStyleSheet(isDark ? "font-size:26px; font-weight:700; color:#E6E7F0;" : "font-size:26px; font-weight:700; color:#24253D;");
        subtitleLabel_->setStyleSheet(isDark ? "color:#7B7B96; font-size:13px;" : "color:#8A8FA3; font-size:13px;");

        goalFrame->setStyleSheet(isDark ? "QFrame#GoalFrame { background-color: #232236; border-radius: 24px; border: none; }"
                                        : "QFrame#GoalFrame { background-color: #FFFFFF; border-radius: 24px; border: none; }");
        frameShadow->setColor(isDark ? QColor(0, 0, 0, 80) : QColor(107, 124, 255, 20));
        goalPercentLabel_->setStyleSheet(isDark ? "font-size: 36px; font-weight: 900; letter-spacing: -1px; color: #E6E7F0;"
                                                : "font-size: 36px; font-weight: 900; letter-spacing: -1px; color: #1F2033;");

        QString subBtnStyle = isDark ? "QPushButton { background: #312F4A; color: #8FA1FF; border-radius: 10px; font-size: 13px; font-weight: bold; border: none; } QPushButton:hover { background: #3B395A; }"
                                     : "QPushButton { background: #F0F4FF; color: #6B7CFF; border-radius: 10px; font-size: 13px; font-weight: bold; border: none; } QPushButton:hover { background: #E6E9FF; }";
        historyBtn->setStyleSheet(subBtnStyle);
        editBtn->setStyleSheet(subBtnStyle);

        settingsBtn->setStyleSheet(isDark ?
                                       "QPushButton { background-color: transparent; border: none; }"
                                       "QPushButton:hover { background-color: #312F4A; border-radius: 8px; }"
                                       "QPushButton QLabel { color: #8A8FA3; background: transparent; }"
                                       "QPushButton:hover QLabel { color: #E6E7F0; }"
                                       "QLabel#settingsIcon { font-size: 25px; }"
                                       "QLabel#settingsText { font-size: 18px; font-weight: 600; }"
                                          :
                                       "QPushButton { background-color: transparent; border: none; }"
                                       "QPushButton:hover { background-color: #E6E9FF; border-radius: 8px; }"
                                       "QPushButton QLabel { color: #8A8FA3; background: transparent; }"
                                       "QPushButton:hover QLabel { color: #333333; }"
                                       "QLabel#settingsIcon { font-size: 25px; }"
                                       "QLabel#settingsText { font-size: 18px; font-weight: 600; }"
                                   );

        backBtn->setStyleSheet(isDark ? "QPushButton { background: transparent; border: none; color: #8FA1FF; font-size: 16px; font-weight: bold; } QPushButton:hover { color: #6F77FF; }"
                                      : "QPushButton { background: transparent; border: none; color: #6B7CFF; font-size: 16px; font-weight: bold; } QPushButton:hover { color: #5A6AE0; }");

        detailEditBtn->setStyleSheet(isDark ? "QPushButton { background: #312F4A; color: #8FA1FF; border-radius: 8px; padding: 6px 14px; font-weight: bold; border: none; } QPushButton:hover { background: #3B395A; }"
                                            : "QPushButton { background: #6B7CFF; color: white; border-radius: 8px; padding: 6px 14px; font-weight: bold; border: none; } QPushButton:hover { background: #5A6AE0; }");

        detailTitleText->setStyleSheet(isDark ? "font-size: 20px; font-weight: 700; color: #E6E7F0;" : "font-size: 20px; font-weight: 700; color: #2D3748;");
        progressPanel->setStyleSheet(isDark ? "QFrame { background: #28263F; border-radius: 12px; border: 1px solid #3B395A; }" : "QFrame { background: #F8F9FC; border-radius: 12px; border: 1px solid #E2E8F0; }");
        pStaticLbl->setStyleSheet(isDark ? "font-size: 14px; color: #A0AEC0; border: none;" : "font-size: 14px; color: #4A5568; border: none;");

        for (auto* obj : cardTargets_.keys()) {
            if (auto* card = qobject_cast<QFrame*>(obj)) {
                auto* t = card->findChild<QLabel*>("QuickCardTitle");
                auto* d = card->findChild<QLabel*>("QuickCardDesc");
                if (t) t->setStyleSheet(isDark ? "font-size: 18px; font-weight: 800; color: #E6E7F0; background: transparent;"
                                            : "font-size: 18px; font-weight: 800; color: #1F2033; background: transparent;");
                if (d) d->setStyleSheet(isDark ? "font-size: 12px; color: #8A8FA3; background: transparent;"
                                            : "font-size: 12px; color: #64748B; background: transparent;");
            }
        }

        updateGoalUI();
    };

    applyTheme();
    connect(&AlgeMate::ThemeManager::instance(), &AlgeMate::ThemeManager::themeChanged, this, [applyTheme](AlgeMate::ThemeManager::Theme){ applyTheme(); });

    refreshGreeting();
    connect(&UserProfile::instance(), &UserProfile::profileChanged, this, &HomePage::refreshGreeting);

    loadGoals();
    updateGoalUI();

    auto getDueTasks = [this]() -> QStringList {
        QStringList dueTasks;
        QDate today = QDate::currentDate();
        for (const auto& g : subGoals_) {
            bool isCompleted = (g.current >= g.target && g.target > 0);
            if (!isCompleted) {
                QDate d = QDate::fromString(g.deadline, Qt::ISODate);
                if (d.isValid() && d <= today) {
                    QString prefix = g.category;
                    if (g.category == QStringLiteral("练习") && !g.subCategory.isEmpty()) {
                        prefix += QStringLiteral(" · ") + g.subCategory;
                    }
                    dueTasks.append(prefix + QStringLiteral(" - ") + g.name);
                }
            }
        }
        return dueTasks;
    };

    auto triggerNotifications = [this](const QStringList& dueTasks) {
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
            QTimer::singleShot(500, this, [dueTasks]() {
                QString notifyMsg = QStringLiteral("今天有 %1 个任务需要完成（如：%2），抓紧时间哦！").arg(dueTasks.size()).arg(dueTasks.first());
                sysTray->showMessage(QStringLiteral("AlgeMate 学习提醒 ⏰"), notifyMsg, QSystemTrayIcon::Information, 5000);
            });
        }
        DeadlineNotifyDialog dlg(dueTasks, this);
        dlg.exec();
    };

    QTimer::singleShot(600, this, [this, triggerNotifications, getDueTasks]() {
        QStringList tasks = getDueTasks();
        if (!tasks.isEmpty()) triggerNotifications(tasks);
    });

    QTimer* timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, [this, triggerNotifications, getDueTasks]() {
        QSettings settings(QStringLiteral("AlgeMate"), QStringLiteral("Settings"));
        QString notifyTimeStr = settings.value(QStringLiteral("NotifyTime"), QStringLiteral("08:00")).toString();
        QTime notifyTime = QTime::fromString(notifyTimeStr, "hh:mm");
        if (!notifyTime.isValid()) notifyTime = QTime(8, 0);

        QDate today = QDate::currentDate();
        QString lastNotifyDate = settings.value(QStringLiteral("LastDailyNotifyDate"), QStringLiteral("")).toString();

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
    greetingLabel_->setText(QStringLiteral("%1，%2 👋").arg(UserProfile::greetingByTime(), u.userName()));
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
    bool isDark = AlgeMate::ThemeManager::instance().currentTheme() == AlgeMate::ThemeManager::Theme::Dark;

    QLayoutItem* item;
    while ((item = subGoalsLayout_->takeAt(0)) != nullptr) {
        if (QWidget* w = item->widget()) w->deleteLater();
        delete item;
    }
    while ((item = detailSubGoalsLayout_->takeAt(0)) != nullptr) {
        if (QWidget* w = item->widget()) w->deleteLater();
        delete item;
    }

    if (subGoals_.isEmpty()) {
        goalTitleLabel_->setText(QStringLiteral("今天暂无学习任务，去放松一下吧 🍃"));
        goalPercentLabel_->setText("100%");
        detailPercentLabel_->setText("100%");
        goalProgressBar_->setValue(100);
        detailProgressBar_->setValue(100);
        return;
    }

    goalTitleLabel_->setText(QStringLiteral("整体学习进度"));
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

    std::sort(pendingList.begin(), pendingList.end(), [this](int a, int b) {
        QDate da = QDate::fromString(subGoals_[a].deadline, Qt::ISODate);
        QDate db = QDate::fromString(subGoals_[b].deadline, Qt::ISODate);
        if (!da.isValid()) return false;
        if (!db.isValid()) return true;
        return da < db;
    });

    auto createRowUI = [this, isDark](int i, bool isCompleted) {
        const auto& goal = subGoals_[i];

        for (int mode = 0; mode < 2; ++mode) {
            if (mode == 1 && isCompleted) {
                continue;
            }

            auto* subRow = new QWidget;
            subRow->setObjectName("TaskRow");
            auto* subLay = new QHBoxLayout(subRow);
            subLay->setContentsMargins(16, 12, 16, 12);
            subLay->setSpacing(14);

            QString rowBg = isDark ? "#312F4A" : "#F8F9FC";
            subRow->setStyleSheet(QString("QWidget#TaskRow { background-color: %1; border-radius: 12px; }").arg(rowBg));

            QString themeColor = isDark ? "#A0AEC0" : "#718096";
            QString badgeBg = isDark ? "#28263F" : "#FFFFFF";
            QString badgeText = goal.category;
            QString displayText = goal.name;

            if (goal.category == QStringLiteral("知识点学习")) {
                themeColor = isDark ? "#48BB78" : "#38A169";
            } else if (goal.category == QStringLiteral("练习")) {
                themeColor = isDark ? "#8FA1FF" : "#6B7CFF";
                if (!goal.subCategory.isEmpty()) {
                    badgeText = QStringLiteral("练习 · %1").arg(goal.subCategory);
                }
            } else if (goal.category == QStringLiteral("测试")) {
                themeColor = isDark ? "#ED8936" : "#DD6B20";
            }

            auto* checkBtn = new QPushButton;
            checkBtn->setCursor(Qt::PointingHandCursor);

            if (isCompleted) {
                int btnSize = 24;
                checkBtn->setFixedSize(btnSize, btnSize);
                checkBtn->setText(QStringLiteral("✓"));
                checkBtn->setStyleSheet(QString(
                                            "QPushButton {"
                                            "  background-color: %1; color: white;"
                                            "  border-radius: 12px; border: none; font-size: 12px; font-weight: bold;"
                                            "}"
                                            ).arg(themeColor));
            } else {
                int btnSize = 24;
                checkBtn->setFixedSize(btnSize, btnSize);
                checkBtn->setText(QStringLiteral(""));
                QString outlineBorder = isDark ? "#4B4970" : "#CBD5E0";
                QString hoverBg = isDark ? "#3B395A" : "#E2E8F0";
                checkBtn->setStyleSheet(QString(
                                            "QPushButton {"
                                            "  background-color: %3; border: none;"
                                            "  border-radius: 12px;"
                                            "}"
                                            "QPushButton:hover { background-color: %1; }"
                                            ).arg(themeColor, hoverBg, outlineBorder));
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
                badgeLbl->setStyleSheet(isDark ? "background-color: transparent; color: #7B7B96; font-size: 12px; font-weight: bold; border: none;"
                                               : "background-color: transparent; color: #A0AEC0; font-size: 12px; font-weight: bold; border: none;");
            } else {
                badgeLbl->setStyleSheet(QStringLiteral("background-color: %1; color: %2; padding: 4px 10px; border-radius: 6px; font-size: 11px; font-weight: 800; border: none;").arg(badgeBg, themeColor));
            }

            auto* nameLbl = new QLabel(displayText);
            nameLbl->setWordWrap(true);
            nameLbl->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

            if (isCompleted) {
                nameLbl->setStyleSheet(isDark ? "color: #7B7B96; font-size: 15px; text-decoration: line-through; border: none; background: transparent;"
                                              : "color: #A0AEC0; font-size: 15px; text-decoration: line-through; border: none; background: transparent;");
            } else {
                nameLbl->setStyleSheet(isDark ? "color: #E6E7F0; font-size: 15px; font-weight: bold; border: none; background: transparent;"
                                              : "color: #2D3748; font-size: 15px; font-weight: bold; border: none; background: transparent;");
            }

            subLay->addWidget(checkBtn, 0, Qt::AlignVCenter);
            subLay->addWidget(badgeLbl, 0, Qt::AlignVCenter);
            subLay->addWidget(nameLbl, 1, Qt::AlignVCenter);

            if (!goal.deadline.isEmpty()) {
                QDate targetDate = QDate::fromString(goal.deadline, Qt::ISODate);
                if (targetDate.isValid()) {
                    qint64 daysLeft = QDate::currentDate().daysTo(targetDate);
                    QString timeText;

                    QString capsuleBg = isDark ? "#28263F" : "#FFFFFF";
                    QString capsuleFg = isDark ? "#A0AEC0" : "#718096";

                    if (isCompleted) {
                        timeText = QStringLiteral("已达成");
                        capsuleBg = "transparent";
                        capsuleFg = isDark ? "#7B7B96" : "#CBD5E0";
                    }
                    else if (daysLeft < 0) {
                        timeText = QStringLiteral("已逾期");
                        capsuleBg = isDark ? "#742A2A" : "#FED7D7";
                        capsuleFg = isDark ? "#FC8181" : "#E53E3E";
                    }
                    else if (daysLeft <= 1) {
                        timeText = daysLeft == 0 ? QStringLiteral("今日截止") : QStringLiteral("剩 1 天");
                        capsuleBg = isDark ? "#7B341E" : "#FEEBC8";
                        capsuleFg = isDark ? "#F6AD55" : "#DD6B20";
                    }
                    else {
                        timeText = QStringLiteral("剩 %1 天").arg(daysLeft);
                    }

                    auto* dateLbl = new QLabel(QStringLiteral("⏳ ") + timeText);
                    dateLbl->setStyleSheet(QString("background-color: %1; color: %2; padding: 4px 10px; border-radius: 10px; font-size: 11px; font-weight: 800; border: none;")
                                               .arg(capsuleBg, capsuleFg));
                    subLay->addWidget(dateLbl, 0, Qt::AlignVCenter);
                }
            }

            if (mode == 0) subGoalsLayout_->addWidget(subRow);
            else detailSubGoalsLayout_->addWidget(subRow);
        }
    };

    for (int idx : pendingList) createRowUI(idx, false);
    for (int idx : doneList) createRowUI(idx, true);

    int avgPercent = subGoals_.isEmpty() ? 0 : qRound(sumOfPercentages / subGoals_.size());
    goalProgressBar_->setValue(avgPercent);
    detailProgressBar_->setValue(avgPercent);
    goalPercentLabel_->setText(QString("%1%").arg(avgPercent));
    detailPercentLabel_->setText(QString("%1%").arg(avgPercent));

    QString barStyle = avgPercent >= 100
                           ? (isDark
                                  ? "QProgressBar { background-color: #312F4A; border-radius: 6px; border: none; } QProgressBar::chunk { background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #48BB78, stop:1 #38A169); border-radius: 6px; }"
                                  : "QProgressBar { background-color: #F0F4FF; border-radius: 6px; border: none; } QProgressBar::chunk { background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #81C784, stop:1 #4CAF50); border-radius: 6px; }")
                           : (isDark
                                  ? "QProgressBar { background-color: #312F4A; border-radius: 6px; border: none; } QProgressBar::chunk { background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #8FA1FF, stop:1 #6F77FF); border-radius: 6px; }"
                                  : "QProgressBar { background-color: #F0F4FF; border-radius: 6px; border: none; } QProgressBar::chunk { background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #8E9EFF, stop:1 #6B7CFF); border-radius: 6px; }");

    goalProgressBar_->setStyleSheet(barStyle);
    detailProgressBar_->setStyleSheet(barStyle);
}

bool HomePage::eventFilter(QObject* obj, QEvent* e) {
    if (e->type() == QEvent::Close && obj == this->window()) {
        if (editDialog_ && editDialog_->isVisible()) {
            int ans = QMessageBox::question(this, QStringLiteral("关闭提示"), QStringLiteral("有未保存的目标，是否继续关闭窗口？"), QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
            if (ans == QMessageBox::No) return true;
        }
    }

    if (e->type() == QEvent::MouseButtonRelease) {
        auto* me = static_cast<QMouseEvent*>(e);
        if (me->button() == Qt::LeftButton) {

            if (obj->objectName() == QStringLiteral("GoalFrame")) {
                if (stackedWidget_) {
                    stackedWidget_->setCurrentIndex(1);
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
        stackedWidget_->setCurrentIndex(1);
    }
}

} // namespace AlgeMate::Home