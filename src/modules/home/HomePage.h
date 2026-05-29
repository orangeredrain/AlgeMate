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

namespace AlgeMate::Home {

// 全新升级的子目标数据结构
struct SubGoal {
    QString category = QStringLiteral("知识点学习"); // 类别
    QString subCategory = QStringLiteral("");       // 子类别 (仅练习适用)
    QString name = QStringLiteral("");              // 目标名称
    int current = 0;                                // 当前进度
    int target = 1;                                 // 总量
    QString unit = QStringLiteral("节");            // 单位
};

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

    // 预留接口：外部模块通过调用此方法来自动更新目标进度
    // 例如：homePage->setSubGoalProgress("第一周任务", 3);
    void setSubGoalProgress(const QString& goalName, int currentProgress);

signals:
    void requestNavigate(int target);

protected:
    bool eventFilter(QObject* obj, QEvent* e) override;

private slots:
    void refreshGreeting();
    void onEditGoalsClicked();

private:
    void updateGoalUI();

    QLabel* avatarLabel_   = nullptr;
    QLabel* greetingLabel_ = nullptr;
    QLabel* subtitleLabel_ = nullptr;
    QHash<QObject*, int> cardTargets_;

    QLabel* goalTitleLabel_ = nullptr;
    QLabel* goalPercentLabel_ = nullptr;
    QProgressBar* goalProgressBar_ = nullptr;
    QVBoxLayout* subGoalsLayout_ = nullptr;

    QList<SubGoal> subGoals_;
};

}

#endif