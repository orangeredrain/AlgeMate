#ifndef ALGEMATE_HOMEPAGE_H
#define ALGEMATE_HOMEPAGE_H

#include <QWidget>
#include <QHash>
#include <QList>
#include <QString>

class QLabel;
class QEvent;
class QProgressBar;
class QVBoxLayout;
class QStackedWidget;

namespace AlgeMate::Home {

struct SubGoal {
    QString category = QStringLiteral("知识点学习");
    QString subCategory = QStringLiteral("");
    QString name = QStringLiteral("");
    int current = 0;
    int target = 1;
    QString unit = QStringLiteral("");
    QString deadline;
};

// 提前声明弹窗类
class GoalEditDialog;

class HomePage : public QWidget {
    Q_OBJECT
public:
    enum NavTarget {
        Calculator = 0,
        AiSolver   = 1,
        Knowledge  = 2,
        Learning   = 3,
        Settings   = 4,
    };
    Q_ENUM(NavTarget)

    explicit HomePage(QWidget* parent = nullptr);

    void setSubGoalProgress(const QString& goalName, int currentProgress);
    void showGoalDetail();
signals:
    void requestNavigate(int target);

protected:
    bool eventFilter(QObject* obj, QEvent* e) override;

private slots:
    void refreshGreeting();
    void onEditGoalsClicked();

private:
    void updateGoalUI();

    // 数据持久化方法
    void saveGoals();
    void loadGoals();

    QStackedWidget* stackedWidget_ = nullptr;

    QLabel* avatarLabel_   = nullptr;
    QLabel* greetingLabel_ = nullptr;
    QLabel* subtitleLabel_ = nullptr;
    QHash<QObject*, int> cardTargets_;

    QLabel* goalTitleLabel_ = nullptr;
    QLabel* goalPercentLabel_ = nullptr;
    QProgressBar* goalProgressBar_ = nullptr;
    QVBoxLayout* subGoalsLayout_ = nullptr;

    QWidget* detailPageWidget_ = nullptr;
    QVBoxLayout* detailSubGoalsLayout_ = nullptr;
    QLabel* detailPercentLabel_ = nullptr;
    QProgressBar* detailProgressBar_ = nullptr;

    QList<SubGoal> subGoals_;
    GoalEditDialog* editDialog_ = nullptr;
};

}

#endif