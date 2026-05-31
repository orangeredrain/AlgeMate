#ifndef ALGEMATE_LEARNINGPAGE_H
#define ALGEMATE_LEARNINGPAGE_H

#include <QWidget>
#include <QDate>
#include <QDateTime>

class QLabel;
class QStackedWidget;
class QTimer;
class QShowEvent;
class QHideEvent;

namespace AlgeMate::Learning {

class KnowledgePage;
class PracticePage;
class CalculationProblemPage;
class ChapterPracticePage;
class TopicPracticePage;
class ExamPage;
class WrongBookPage;
class LearningCenterPage;

/// 学习中心主界面
class LearningPage : public QWidget {
    Q_OBJECT
public:
    explicit LearningPage(QWidget* parent = nullptr);

    void showKnowledge();
signals:
    // 【新增】请求主窗口导航到首页的目标详情页
    void requestNavigateToHomeGoalDetail();

protected:
    void showEvent(QShowEvent* event) override;
    void hideEvent(QHideEvent* event) override;

private:
    void buildDashboard();
    void loadTodayStudyTime();
    void saveTodayStudyTime() const;
    void saveModuleStudyTime(const QString& moduleKey, int seconds) const;
    void startStudyTimer();
    void stopStudyTimer();
    void handleStudyTimerTick();
    void refreshTodayStudyCard();
    void refreshProgressCard();
    void updateAutoCheckin();
    bool shouldCountStudyTime() const;
    QString currentModuleKey() const;
    QString formatStudyDuration(int seconds) const;

    // 卡片点击 → 子页面导航
    void showPractice();
    void showExam();
    void showWrongBook();
    void showLearningCenter();
    void showCalculationProblem();
    void showChapterPractice();
    void showTopicPractice();

    void goBack();  // 返回 Dashboard

    QStackedWidget* m_stack = nullptr;

    // Dashboard 页
    QWidget* m_dashboard = nullptr;
    QLabel*  m_todayStudyValueLabel = nullptr;
    QLabel*  m_todayStudySubLabel = nullptr;
    QLabel*  m_progressValueLabel = nullptr;
    QLabel*  m_progressSubLabel = nullptr;
    QTimer*  m_studyTimer = nullptr;
    QDate    m_studyDate;
    QDateTime m_lastStudyTick;
    int      m_todayStudySeconds = 0;
    bool     m_todayAutoCheckedIn = false;

    // 子页面
    KnowledgePage*           m_knowledgePage     = nullptr;
    PracticePage*            m_practicePage      = nullptr;
    CalculationProblemPage*  m_calcProbPage      = nullptr;
    ChapterPracticePage*     m_chapterPracPage   = nullptr;
    TopicPracticePage*       m_topicPracPage     = nullptr;
    ExamPage*                m_examPage          = nullptr;
    WrongBookPage*           m_wrongBookPage     = nullptr;
    LearningCenterPage*      m_learningCenterPage = nullptr;
};

} // namespace AlgeMate::Learning

#endif // ALGEMATE_LEARNINGPAGE_H
