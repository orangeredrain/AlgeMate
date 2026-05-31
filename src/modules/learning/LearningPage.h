#ifndef ALGEMATE_LEARNINGPAGE_H
#define ALGEMATE_LEARNINGPAGE_H

#include <QWidget>
#include <QDate>
#include <QDateTime>
#include "RecommendPracticePage.h"

class QLabel;
class QStackedWidget;
class QTimer;
class QShowEvent;
class QHideEvent;
class QNetworkAccessManager;

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
    void showWrongBook();
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
    void refreshWrongCountCard();
    void refreshRecommendCountCard(); // 【新增】刷新推荐练习数量卡片
    void generateRecommendedPractice(int count);
    void updateAutoCheckin();
    void handleRecommendCardClicked();
    bool shouldCountStudyTime() const;
    QString currentModuleKey() const;
    QString formatStudyDuration(int seconds) const;

    // 卡片点击 → 子页面导航
    void showPractice();
    void showExam();
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
    QLabel*  m_wrongCountValueLabel = nullptr;
    QLabel*  m_recommendCountValueLabel = nullptr;
    QLabel*  m_recommendSubLabel = nullptr;
    QNetworkAccessManager* m_networkMgr = nullptr;
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
    RecommendPracticePage* m_recommendPage      = nullptr;
};

} // namespace AlgeMate::Learning

#endif // ALGEMATE_LEARNINGPAGE_H