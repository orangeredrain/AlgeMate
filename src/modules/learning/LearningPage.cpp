#include "LearningPage.h"
#include "ClickableCard.h"
#include "KnowledgePage.h"
#include "PracticePage.h"
#include "ExamPage.h"
#include "WrongBookPage.h"
#include "LearningCenterPage.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QStackedWidget>
#include <QLabel>
#include <QPushButton>
#include <QFrame>
#include <QDate>
#include <QSettings>
#include <QTimer>
#include <QShowEvent>
#include <QHideEvent>
#include <QApplication>
#include <QString>

namespace AlgeMate::Learning {


// 工厂函数: 创建带内容的可点击卡片

static ClickableCard* makeStatCard(const QString& label, const QString& value,
                                    const QString& sub, QWidget* parent,
                                    QLabel** valueLabelOut = nullptr,
                                    QLabel** subLabelOut = nullptr)
{
    auto* card = new ClickableCard(parent);
    card->setMinimumHeight(100);
    auto* lay = new QVBoxLayout(card);
    lay->setContentsMargins(20, 16, 20, 14);
    lay->setSpacing(4);

    auto* lb = new QLabel(label);
    lb->setStyleSheet("font-size:13px; color:#8A8FA3; background:transparent;");
    auto* vl = new QLabel(value);
    vl->setStyleSheet("font-size:28px; font-weight:700; color:#6A5AE0; background:transparent;");
    auto* sb = new QLabel(sub);
    sb->setStyleSheet("font-size:12px; color:#B4B8CC; background:transparent;");

    if (valueLabelOut) {
        *valueLabelOut = vl;
    }
    if (subLabelOut) {
        *subLabelOut = sb;
    }

    lay->addWidget(lb);
    lay->addWidget(vl);
    lay->addStretch();
    lay->addWidget(sb);
    return card;
}

static ClickableCard* makeModuleCard(const QString& emoji, const QString& title,
                                      const QString& desc, QWidget* parent)
{
    auto* card = new ClickableCard(parent);
    card->setMinimumHeight(150);
    auto* lay = new QVBoxLayout(card);
    lay->setContentsMargins(24, 22, 24, 20);
    lay->setSpacing(6);

    auto* ic = new QLabel(emoji);
    ic->setStyleSheet("font-size:32px; background:transparent;");
    auto* tl = new QLabel(title);
    tl->setStyleSheet("font-size:17px; font-weight:700; background:transparent;");
    auto* ds = new QLabel(desc);
    ds->setStyleSheet("font-size:13px; background:transparent;");
    ds->setWordWrap(true);

    lay->addWidget(ic);
    lay->addWidget(tl);
    lay->addStretch();
    lay->addWidget(ds);
    return card;
}

// LearningPage

LearningPage::LearningPage(QWidget* parent) : QWidget(parent)
{
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    m_stack = new QStackedWidget;
    m_studyTimer = new QTimer(this);
    m_studyTimer->setInterval(1000);
    connect(m_studyTimer, &QTimer::timeout, this, &LearningPage::handleStudyTimerTick);

    buildDashboard();
    loadTodayStudyTime();
    refreshTodayStudyCard();

    m_knowledgePage    = new KnowledgePage;
    m_practicePage     = new PracticePage;
    m_calcProbPage     = new CalculationProblemPage;
    m_chapterPracPage  = new ChapterPracticePage;
    m_topicPracPage    = new TopicPracticePage;
    m_examPage         = new ExamPage;
    m_wrongBookPage    = new WrongBookPage;
    m_learningCenterPage = new LearningCenterPage;

    m_stack->addWidget(m_dashboard);           // 0
    m_stack->addWidget(m_knowledgePage);       // 1
    m_stack->addWidget(m_practicePage);        // 2
    m_stack->addWidget(m_calcProbPage);        // 3
    m_stack->addWidget(m_chapterPracPage);     // 4
    m_stack->addWidget(m_topicPracPage);       // 5
    m_stack->addWidget(m_examPage);            // 6
    m_stack->addWidget(m_wrongBookPage);       // 7
    m_stack->addWidget(m_learningCenterPage);  // 8

    m_stack->setCurrentIndex(0);
    root->addWidget(m_stack);

    // 返回
    connect(m_knowledgePage, &KnowledgePage::backRequested, this, &LearningPage::goBack);
    connect(m_practicePage, &PracticePage::backRequested, this, &LearningPage::goBack);
    connect(m_calcProbPage, &CalculationProblemPage::backRequested, this, &LearningPage::goBack);
    connect(m_chapterPracPage, &ChapterPracticePage::backRequested, this, &LearningPage::goBack);
    connect(m_topicPracPage, &TopicPracticePage::backRequested, this, &LearningPage::goBack);
    connect(m_examPage, &ExamPage::backRequested, this, &LearningPage::goBack);
    connect(m_wrongBookPage, &WrongBookPage::backRequested, this, &LearningPage::goBack);
    connect(m_learningCenterPage, &LearningCenterPage::backRequested, this, &LearningPage::goBack);

    // Practice → 子模式
    connect(m_practicePage, &PracticePage::calculationProblemRequested,
            this, &LearningPage::showCalculationProblem);
    connect(m_practicePage, &PracticePage::chapterPracticeRequested,
            this, &LearningPage::showChapterPractice);
    connect(m_practicePage, &PracticePage::topicPracticeRequested,
            this, &LearningPage::showTopicPractice);

    // Knowledge → 章节练习
    connect(m_knowledgePage, &KnowledgePage::enterChapterPractice,
            this, &LearningPage::showChapterPractice);
}

void LearningPage::buildDashboard()
{
    m_dashboard = new QWidget;
    auto* root = new QVBoxLayout(m_dashboard);
    root->setContentsMargins(24, 24, 24, 24);
    root->setSpacing(20);

    // 标题行
    auto* titleRow = new QHBoxLayout;
    auto* title = new QLabel(QStringLiteral("学习中心"));
    title->setObjectName(QStringLiteral("PageTitle"));
    auto* toCenter = new QPushButton(QStringLiteral("学习管理中心 →"));
    toCenter->setCursor(Qt::PointingHandCursor);
    toCenter->setFlat(true);
    toCenter->setStyleSheet(QStringLiteral(
        "QPushButton { color:#6A5AE0; font-size:14px; font-weight:500; }"
        "QPushButton:hover { color:#7E70E6; }"));
    connect(toCenter, &QPushButton::clicked, this, &LearningPage::showLearningCenter);
    titleRow->addWidget(title);
    titleRow->addStretch();
    titleRow->addWidget(toCenter);

    // 统计卡片
    auto* statsRow = new QHBoxLayout;
    statsRow->setSpacing(12);

    auto* stat1 = makeStatCard(QStringLiteral("今日学习"), QStringLiteral("0 分钟"),
                                QStringLiteral("今日暂无记录"), m_dashboard,
                                &m_todayStudyValueLabel, &m_todayStudySubLabel);
    auto* stat2 = makeStatCard(QStringLiteral("本周进度"), QStringLiteral("0%"),     QStringLiteral("目标 100%"), m_dashboard);
    auto* stat3 = makeStatCard(QStringLiteral("错题数"),   QStringLiteral("0"),      QStringLiteral("共收录"), m_dashboard);
    auto* stat4 = makeStatCard(QStringLiteral("推荐练习"), QStringLiteral("—"),      QStringLiteral("待系统生成"), m_dashboard);

    connect(stat1, &ClickableCard::clicked, this, &LearningPage::showLearningCenter);
    connect(stat2, &ClickableCard::clicked, this, &LearningPage::showLearningCenter);
    connect(stat3, &ClickableCard::clicked, this, &LearningPage::showLearningCenter);
    connect(stat4, &ClickableCard::clicked, this, &LearningPage::showLearningCenter);

    statsRow->addWidget(stat1);
    statsRow->addWidget(stat2);
    statsRow->addWidget(stat3);
    statsRow->addWidget(stat4);

    // 分区标题
    auto* sectionLabel = new QLabel(QStringLiteral("快捷入口"));
    sectionLabel->setStyleSheet("font-size:15px; font-weight:600; color:#8A8FA3; margin-top:4px;");

    // 四大模块卡片
    auto* grid = new QGridLayout;
    grid->setSpacing(16);

    auto* cardKnow = makeModuleCard(QStringLiteral("📗"), QStringLiteral("知识点学习"),
                                     QStringLiteral("线性代数章节树 · 例题与思考题"), m_dashboard);
    auto* cardPrac = makeModuleCard(QStringLiteral("📙"), QStringLiteral("练习模式"),
                                     QStringLiteral("计算题 · 章节练习 · 专题训练"), m_dashboard);
    auto* cardExam = makeModuleCard(QStringLiteral("📔"), QStringLiteral("考试模式"),
                                     QStringLiteral("模拟真实考试 · 限时答题 · 自动评分"), m_dashboard);
    auto* cardWron = makeModuleCard(QStringLiteral("📓"), QStringLiteral("错题本"),
                                     QStringLiteral("错题归档 · 分类管理 · 重做复习"), m_dashboard);

    connect(cardKnow, &ClickableCard::clicked, this, &LearningPage::showKnowledge);
    connect(cardPrac, &ClickableCard::clicked, this, &LearningPage::showPractice);
    connect(cardExam, &ClickableCard::clicked, this, &LearningPage::showExam);
    connect(cardWron, &ClickableCard::clicked, this, &LearningPage::showWrongBook);

    grid->addWidget(cardKnow, 0, 0);
    grid->addWidget(cardPrac, 0, 1);
    grid->addWidget(cardExam, 1, 0);
    grid->addWidget(cardWron, 1, 1);

    root->addLayout(titleRow);
    root->addLayout(statsRow);
    root->addWidget(sectionLabel);
    root->addLayout(grid);
    root->addStretch();
}

// 导航
void LearningPage::showKnowledge()        { m_stack->setCurrentIndex(1); }
void LearningPage::showPractice()         { m_stack->setCurrentIndex(2); }
void LearningPage::showCalculationProblem()  { m_stack->setCurrentIndex(3); }
void LearningPage::showChapterPractice()     { m_stack->setCurrentIndex(4); }
void LearningPage::showTopicPractice()       { m_stack->setCurrentIndex(5); }
void LearningPage::showExam()             { m_stack->setCurrentIndex(6); }
void LearningPage::showWrongBook()        { m_stack->setCurrentIndex(7); }
void LearningPage::showLearningCenter()   { m_stack->setCurrentIndex(8); }
void LearningPage::goBack()              { m_stack->setCurrentIndex(0); }

void LearningPage::showEvent(QShowEvent* event)
{
    QWidget::showEvent(event);
    loadTodayStudyTime();
    refreshTodayStudyCard();
    startStudyTimer();
}

void LearningPage::hideEvent(QHideEvent* event)
{
    stopStudyTimer();
    QWidget::hideEvent(event);
}

void LearningPage::loadTodayStudyTime()
{
    const QDate today = QDate::currentDate();
    QSettings settings;
    settings.beginGroup(QStringLiteral("learning/todayStudy"));

    const QDate savedDate = QDate::fromString(
        settings.value(QStringLiteral("date")).toString(),
        Qt::ISODate);

    m_studyDate = today;
    if (savedDate == today) {
        m_todayStudySeconds = settings.value(QStringLiteral("seconds"), 0).toInt();
        m_todayAutoCheckedIn = settings.value(QStringLiteral("autoCheckedIn"), false).toBool();
    } else {
        settings.endGroup();
        settings.beginGroup(QStringLiteral("learning/studySecondsByDate"));
        m_todayStudySeconds = settings.value(today.toString(Qt::ISODate), 0).toInt();
        settings.endGroup();
        settings.beginGroup(QStringLiteral("learning/checkinsByDate"));
        m_todayAutoCheckedIn = settings.value(today.toString(Qt::ISODate), false).toBool();
        settings.endGroup();
        updateAutoCheckin();
        return;
    }

    settings.endGroup();
    updateAutoCheckin();
}

void LearningPage::saveTodayStudyTime() const
{
    QSettings settings;
    settings.beginGroup(QStringLiteral("learning/todayStudy"));
    settings.setValue(QStringLiteral("date"), m_studyDate.toString(Qt::ISODate));
    settings.setValue(QStringLiteral("seconds"), m_todayStudySeconds);
    settings.setValue(QStringLiteral("autoCheckedIn"), m_todayAutoCheckedIn);
    settings.endGroup();

    settings.beginGroup(QStringLiteral("learning/studySecondsByDate"));
    settings.setValue(m_studyDate.toString(Qt::ISODate), m_todayStudySeconds);
    settings.endGroup();

    if (m_todayAutoCheckedIn) {
        settings.beginGroup(QStringLiteral("learning/checkinsByDate"));
        settings.setValue(m_studyDate.toString(Qt::ISODate), true);
        settings.endGroup();
    }
}

void LearningPage::saveModuleStudyTime(const QString& moduleKey, int seconds) const
{
    if (moduleKey.isEmpty() || seconds <= 0) {
        return;
    }

    QSettings settings;
    settings.beginGroup(QStringLiteral("learning/moduleSecondsByDate"));
    settings.beginGroup(m_studyDate.toString(Qt::ISODate));
    settings.setValue(moduleKey, settings.value(moduleKey, 0).toInt() + seconds);
    settings.endGroup();
    settings.endGroup();
}

void LearningPage::startStudyTimer()
{
    m_lastStudyTick = QDateTime::currentDateTime();
    if (!m_studyTimer->isActive()) {
        m_studyTimer->start();
    }
}

void LearningPage::stopStudyTimer()
{
    if (m_studyTimer->isActive()) {
        handleStudyTimerTick();
        m_studyTimer->stop();
    }
    saveTodayStudyTime();
}

void LearningPage::handleStudyTimerTick()
{
    const QDate today = QDate::currentDate();
    if (m_studyDate != today) {
        m_studyDate = today;
        m_todayStudySeconds = 0;
        m_todayAutoCheckedIn = false;
    }

    const QDateTime now = QDateTime::currentDateTime();
    if (!shouldCountStudyTime()) {
        m_lastStudyTick = now;
        refreshTodayStudyCard();
        return;
    }

    if (m_lastStudyTick.isValid()) {
        const qint64 elapsed = m_lastStudyTick.secsTo(now);
        if (elapsed > 0) {
            const int elapsedSeconds = static_cast<int>(elapsed);
            m_todayStudySeconds += elapsedSeconds;
            saveModuleStudyTime(currentModuleKey(), elapsedSeconds);
            if (m_stack && m_stack->currentIndex() == 8 && m_learningCenterPage) {
                m_learningCenterPage->refreshData();
            }
        }
    }

    m_lastStudyTick = now;
    updateAutoCheckin();
    refreshTodayStudyCard();

    if (m_todayStudySeconds % 30 == 0) {
        saveTodayStudyTime();
    }
}

void LearningPage::refreshTodayStudyCard()
{
    if (m_todayStudyValueLabel) {
        m_todayStudyValueLabel->setText(formatStudyDuration(m_todayStudySeconds));
    }

    if (m_todayStudySubLabel) {
        if (m_todayStudySeconds <= 0) {
            m_todayStudySubLabel->setText(QStringLiteral("今日暂无记录"));
        } else if (m_todayAutoCheckedIn) {
            m_todayStudySubLabel->setText(QStringLiteral("正在学习中"));
        } else {
            m_todayStudySubLabel->setText(QStringLiteral("超过五分钟自动打卡"));
        }
    }
}

void LearningPage::updateAutoCheckin()
{
    constexpr int autoCheckinSeconds = 5 * 60;
    if (m_todayAutoCheckedIn || m_todayStudySeconds <= autoCheckinSeconds) {
        return;
    }

    m_todayAutoCheckedIn = true;
    saveTodayStudyTime();
    if (m_learningCenterPage) {
        m_learningCenterPage->refreshData();
    }
}

bool LearningPage::shouldCountStudyTime() const
{
    return QApplication::applicationState() == Qt::ApplicationActive
        && window()
        && window()->isActiveWindow();
}

QString LearningPage::currentModuleKey() const
{
    if (!m_stack) {
        return QStringLiteral("overview");
    }

    switch (m_stack->currentIndex()) {
    case 1:
        return QStringLiteral("knowledge");
    case 2:
    case 3:
    case 4:
    case 5:
        return QStringLiteral("practice");
    case 6:
        return QStringLiteral("exam");
    case 7:
        return QStringLiteral("wrongbook");
    case 8:
        return QStringLiteral("management");
    case 0:
    default:
        return QStringLiteral("overview");
    }
}

QString LearningPage::formatStudyDuration(int seconds) const
{
    const int minutes = seconds / 60;
    if (minutes < 1) {
        return QStringLiteral("不足 1 分钟");
    }

    const int hours = minutes / 60;
    const int remainMinutes = minutes % 60;
    if (hours <= 0) {
        return QStringLiteral("%1 分钟").arg(minutes);
    }

    if (remainMinutes == 0) {
        return QStringLiteral("%1 小时").arg(hours);
    }

    return QStringLiteral("%1 小时 %2 分钟").arg(hours).arg(remainMinutes);
}

} // namespace AlgeMate::Learning
