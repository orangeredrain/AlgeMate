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
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonDocument>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QMessageBox>
#include <qfile.h>
#include <QRandomGenerator>
#include <QDateTime>
#include <QDialog>

namespace AlgeMate::Learning {

// ==================== 现代风格圆角数字输入弹窗 ====================
class CustomGenerateDialog : public QDialog {
public:
    explicit CustomGenerateDialog(QWidget* parent = nullptr) : QDialog(parent), m_val(3) {
        setWindowTitle(QStringLiteral("智能推荐练习"));
        setFixedSize(340, 220);
        setStyleSheet("QDialog { background-color: #FAFAFC; }");

        auto* lay = new QVBoxLayout(this);
        lay->setContentsMargins(24, 24, 24, 24);
        lay->setSpacing(16);

        auto* titleLbl = new QLabel(QStringLiteral("🤖 准备生成多少道题？"));
        titleLbl->setAlignment(Qt::AlignCenter);
        titleLbl->setStyleSheet("font-size: 18px; font-weight: bold; color: #2D3748;");
        lay->addWidget(titleLbl);

        auto* descLbl = new QLabel(QStringLiteral("AI 将根据您的学习情况精准出题 (1-10)"));
        descLbl->setAlignment(Qt::AlignCenter);
        descLbl->setStyleSheet("font-size: 13px; color: #718096;");
        lay->addWidget(descLbl);

        // --- 手写数字调节器 ---
        auto* spinContainer = new QWidget(this);
        spinContainer->setStyleSheet("QWidget { background: #FFFFFF; border: 1.5px solid #6B7CFF; border-radius: 8px; }");
        auto* spinLay = new QHBoxLayout(spinContainer);
        spinLay->setContentsMargins(4, 4, 4, 4);

        QString btnStyle = "QPushButton { border: none; background: #F8F9FC; color: #6B7CFF; font-size: 20px; font-weight: bold; border-radius: 6px; } "
                           "QPushButton:hover { background: #EBE5FF; } "
                           "QPushButton:pressed { background: #D6CCFF; }";

        auto* minusBtn = new QPushButton(QStringLiteral("-"), spinContainer);
        minusBtn->setFixedSize(40, 36);
        minusBtn->setStyleSheet(btnStyle);
        minusBtn->setCursor(Qt::PointingHandCursor);

        m_numLbl = new QLabel(QString::number(m_val), spinContainer);
        m_numLbl->setAlignment(Qt::AlignCenter);
        m_numLbl->setStyleSheet("border: none; font-size: 22px; font-weight: bold; color: #6B7CFF; background: transparent;");

        auto* plusBtn = new QPushButton(QStringLiteral("+"), spinContainer);
        plusBtn->setFixedSize(40, 36);
        plusBtn->setStyleSheet(btnStyle);
        plusBtn->setCursor(Qt::PointingHandCursor);

        connect(minusBtn, &QPushButton::clicked, this, [this]() {
            if (m_val > 1) { m_val--; m_numLbl->setText(QString::number(m_val)); }
        });
        connect(plusBtn, &QPushButton::clicked, this, [this]() {
            if (m_val < 10) { m_val++; m_numLbl->setText(QString::number(m_val)); }
        });

        spinLay->addWidget(minusBtn);
        spinLay->addWidget(m_numLbl, 1);
        spinLay->addWidget(plusBtn);
        lay->addWidget(spinContainer);

        auto* btnLay = new QHBoxLayout;
        auto* cancelBtn = new QPushButton(QStringLiteral("取消"));
        auto* okBtn = new QPushButton(QStringLiteral("开始生成 ✨"));
        cancelBtn->setCursor(Qt::PointingHandCursor);
        okBtn->setCursor(Qt::PointingHandCursor);

        QString btnBase = "QPushButton { border-radius: 8px; padding: 10px 16px; font-size: 14px; font-weight: bold; }";
        cancelBtn->setStyleSheet(btnBase + "QPushButton { background: #EDF2F7; color: #4A5568; border: none; } QPushButton:hover { background: #E2E8F0; }");
        okBtn->setStyleSheet(btnBase + "QPushButton { background: #6B7CFF; color: white; border: none; } QPushButton:hover { background: #5A6AE0; }");

        connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
        connect(okBtn, &QPushButton::clicked, this, &QDialog::accept);

        btnLay->addWidget(cancelBtn);
        btnLay->addWidget(okBtn);
        lay->addLayout(btnLay);
    }

    int getValue() const { return m_val; }

private:
    int m_val;
    QLabel* m_numLbl;
};

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
    m_examPage         = new ExamPage;
    m_wrongBookPage    = new WrongBookPage;
    m_learningCenterPage = new LearningCenterPage;
    m_recommendPage    = new RecommendPracticePage;

    connect(m_wrongBookPage, &WrongBookPage::wrongCountChanged, this, [this](int count) {
        if (m_wrongCountValueLabel) {
            m_wrongCountValueLabel->setText(QString::number(count));
        }
    });
    refreshWrongCountCard();
    refreshRecommendCountCard();

    m_stack->addWidget(m_dashboard);           // 0
    m_stack->addWidget(m_knowledgePage);       // 1
    m_stack->addWidget(m_practicePage);        // 2
    m_stack->addWidget(m_calcProbPage);        // 3
    m_stack->addWidget(m_chapterPracPage);     // 4
    m_stack->addWidget(new QWidget());         // 5
    m_stack->addWidget(m_examPage);            // 6
    m_stack->addWidget(m_wrongBookPage);       // 7
    m_stack->addWidget(m_learningCenterPage);  // 8
    m_stack->addWidget(m_recommendPage);       // 9

    m_stack->setCurrentIndex(0);
    root->addWidget(m_stack);

    connect(m_knowledgePage, &KnowledgePage::backRequested, this, &LearningPage::goBack);
    connect(m_practicePage, &PracticePage::backRequested, this, &LearningPage::goBack);
    connect(m_calcProbPage, &CalculationProblemPage::backRequested, this, &LearningPage::goBack);
    connect(m_chapterPracPage, &ChapterPracticePage::backRequested, this, &LearningPage::goBack);
    connect(m_examPage, &ExamPage::backRequested, this, &LearningPage::goBack);
    connect(m_wrongBookPage, &WrongBookPage::backRequested, this, &LearningPage::goBack);
    connect(m_learningCenterPage, &LearningCenterPage::backRequested, this, &LearningPage::goBack);
    connect(m_recommendPage, &RecommendPracticePage::backRequested, this, &LearningPage::goBack);

    connect(m_recommendPage, &RecommendPracticePage::requestRegenerate, this, [this]() {
        CustomGenerateDialog dlg(this);
        if (dlg.exec() == QDialog::Accepted) {
            generateRecommendedPractice(dlg.getValue());
        }
    });

    connect(m_practicePage, &PracticePage::calculationProblemRequested,
            this, &LearningPage::showCalculationProblem);
    connect(m_practicePage, &PracticePage::chapterPracticeRequested,
            this, &LearningPage::showChapterPractice);

    connect(m_knowledgePage, &KnowledgePage::enterChapterPractice, this, [this]() {
        if (auto* tree = m_knowledgePage->findChild<QTreeWidget*>(QStringLiteral("KnowledgeTree"))) {
            if (auto* currentItem = tree->currentItem()) {
                QString fullPath = currentItem->data(0, Qt::UserRole + 1).toString();
                if (!fullPath.isEmpty()) {
                    m_chapterPracPage->selectChapterByResourcePath(fullPath);
                }
            }
        }
        m_stack->setCurrentIndex(4);
    });
}

void LearningPage::buildDashboard()
{
    m_dashboard = new QWidget;
    auto* root = new QVBoxLayout(m_dashboard);
    root->setContentsMargins(24, 24, 24, 24);
    root->setSpacing(20);

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

    auto* statsRow = new QHBoxLayout;
    statsRow->setSpacing(12);

    auto* stat1 = makeStatCard(QStringLiteral("今日学习"), QStringLiteral("0 分钟"),
                               QStringLiteral("今日暂无记录"), m_dashboard,
                               &m_todayStudyValueLabel, &m_todayStudySubLabel);
    auto* stat2 = makeStatCard(QStringLiteral("本日进度"), QStringLiteral("0%"),
                               QStringLiteral("首页目标同步中"), m_dashboard,
                               &m_progressValueLabel, &m_progressSubLabel);
    auto* stat3 = makeStatCard(QStringLiteral("错题数"), QStringLiteral("0"), QStringLiteral("共收录"), m_dashboard, &m_wrongCountValueLabel);
    auto* stat4 = makeStatCard(QStringLiteral("推荐练习"), QStringLiteral("0 道未完成"),
                               QStringLiteral("点击智能生成"), m_dashboard,
                               &m_recommendCountValueLabel, &m_recommendSubLabel);

    connect(stat1, &ClickableCard::clicked, this, &LearningPage::showLearningCenter);
    connect(stat2, &ClickableCard::clicked, this, [this]() {
        emit requestNavigateToHomeGoalDetail();
    });
    connect(stat3, &ClickableCard::clicked, this, &LearningPage::showWrongBook);
    connect(stat4, &ClickableCard::clicked, this, &LearningPage::handleRecommendCardClicked);

    statsRow->addWidget(stat1);
    statsRow->addWidget(stat2);
    statsRow->addWidget(stat3);
    statsRow->addWidget(stat4);

    auto* sectionLabel = new QLabel(QStringLiteral("快捷入口"));
    sectionLabel->setStyleSheet("font-size:15px; font-weight:600; color:#8A8FA3; margin-top:4px;");

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

void LearningPage::showKnowledge()        { m_stack->setCurrentIndex(1); }
void LearningPage::showPractice()         { m_stack->setCurrentIndex(2); }
void LearningPage::showCalculationProblem()  { m_stack->setCurrentIndex(3); }
void LearningPage::showChapterPractice()  { m_stack->setCurrentIndex(4); }
void LearningPage::showExam()             { m_stack->setCurrentIndex(6); }
void LearningPage::showWrongBook()
{
    m_wrongBookPage->reload();
    m_stack->setCurrentIndex(7);
}
void LearningPage::showLearningCenter()   { m_stack->setCurrentIndex(8); }
void LearningPage::goBack()              { m_stack->setCurrentIndex(0); }

void LearningPage::showEvent(QShowEvent* event)
{
    QWidget::showEvent(event);
    loadTodayStudyTime();
    refreshTodayStudyCard();
    refreshProgressCard();
    refreshWrongCountCard();
    refreshRecommendCountCard();
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
    if (moduleKey.isEmpty() || seconds <= 0) return;
    QSettings settings;
    settings.beginGroup(QStringLiteral("learning/moduleSecondsByDate"));
    settings.beginGroup(m_studyDate.toString(Qt::ISODate));
    settings.setValue(moduleKey, settings.value(moduleKey, 0).toInt() + seconds);
    settings.endGroup();
    settings.endGroup();
}

void LearningPage::saveHourStudyTime(int hour, int seconds) const
{
    if (hour < 0 || hour > 23 || seconds <= 0) return;
    QSettings settings;
    settings.beginGroup(QStringLiteral("learning/studySecondsByHour"));
    settings.beginGroup(m_studyDate.toString(Qt::ISODate));
    const QString key = QString::number(hour);
    settings.setValue(key, settings.value(key, 0).toInt() + seconds);
    settings.endGroup();
    settings.endGroup();
}

void LearningPage::startStudyTimer()
{
    m_lastStudyTick = QDateTime::currentDateTime();
    m_pendingStudyMs = 0;
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
        // 跨日: 先把昨日最后状态落盘, 再切换. 否则下一次 saveTodayStudyTime 会用 today 写 0 秒覆盖
        if (m_studyDate.isValid() && m_todayStudySeconds > 0) {
            saveTodayStudyTime();
        }
        m_studyDate = today;
        m_todayStudySeconds = 0;
        m_todayAutoCheckedIn = false;
        m_pendingStudyMs = 0;
    }

    const QDateTime now = QDateTime::currentDateTime();
    if (!shouldCountStudyTime()) {
        m_lastStudyTick = now;
        m_pendingStudyMs = 0;        // 非活跃期间不累积残余
        refreshTodayStudyCard();
        return;
    }

    if (m_lastStudyTick.isValid()) {
        // 用毫秒累加, 整秒部分进位。避免 QTimer 1000ms 调度抖动下 secsTo 永返 0.
        const qint64 elapsedMs = m_lastStudyTick.msecsTo(now);
        if (elapsedMs > 0) {
            m_pendingStudyMs += elapsedMs;
            // 防休眠后一次补太多 (原隐患): 超过 5 分钟丢弃
            if (m_pendingStudyMs > 5LL * 60LL * 1000LL) m_pendingStudyMs = 0;
            const int wholeSeconds = static_cast<int>(m_pendingStudyMs / 1000);
            if (wholeSeconds > 0) {
                m_pendingStudyMs -= static_cast<qint64>(wholeSeconds) * 1000;
                m_todayStudySeconds += wholeSeconds;
                saveModuleStudyTime(currentModuleKey(), wholeSeconds);
                saveHourStudyTime(now.time().hour(), wholeSeconds);
                if (m_stack && m_learningCenterPage
                    && m_stack->currentWidget() == m_learningCenterPage) {
                    m_learningCenterPage->refreshData();
                }
            }
        }
    }

    m_lastStudyTick = now;
    updateAutoCheckin();
    refreshTodayStudyCard();

    if (m_todayStudySeconds > 0 && m_todayStudySeconds % 30 == 0) {
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

void LearningPage::refreshProgressCard()
{
    QSettings settings(QStringLiteral("AlgeMate"), QStringLiteral("HomeGoals"));
    QString jsonStr = settings.value(QStringLiteral("SubGoalsData")).toString();

    int avgPercent = 100;
    bool hasGoals = false;

    if (!jsonStr.isEmpty()) {
        QJsonDocument doc = QJsonDocument::fromJson(jsonStr.toUtf8());
        QJsonArray arr = doc.array();
        int totalCount = arr.size();

        if (totalCount > 0) {
            hasGoals = true;
            int completedCount = 0;
            for (int i = 0; i < totalCount; ++i) {
                QJsonObject obj = arr[i].toObject();
                int current = obj["current"].toInt();
                int target = obj["target"].toInt();
                if (target > 0 && current >= target) {
                    completedCount++;
                }
            }
            avgPercent = qRound((double)completedCount / totalCount * 100.0);
        }
    }

    if (m_progressValueLabel) {
        m_progressValueLabel->setText(QStringLiteral("%1%").arg(avgPercent));
        if (avgPercent >= 100) {
            m_progressValueLabel->setStyleSheet("font-size:28px; font-weight:700; color:#4CAF50; background:transparent;");
        } else {
            m_progressValueLabel->setStyleSheet("font-size:28px; font-weight:700; color:#6A5AE0; background:transparent;");
        }
    }

    if (m_progressSubLabel) {
        if (!hasGoals) {
            m_progressSubLabel->setText(QStringLiteral("暂未设置目标"));
        } else if (avgPercent >= 100) {
            m_progressSubLabel->setText(QStringLiteral("今日目标达成 🎉"));
        } else {
            m_progressSubLabel->setText(QStringLiteral("继续加油 😜"));
        }
    }
}

void LearningPage::refreshWrongCountCard()
{
    if (m_wrongCountValueLabel && m_wrongBookPage) {
        int currentCount = m_wrongBookPage->getWrongCount();
        m_wrongCountValueLabel->setText(QString::number(currentCount));
    }
}

void LearningPage::refreshRecommendCountCard()
{
    QFile file("recommended_questions.json");
    int unfinishedCount = 0;
    QDateTime thirtyDaysAgo = QDateTime::currentDateTime().addDays(-30);

    if (file.exists() && file.open(QIODevice::ReadOnly)) {
        QJsonArray batches = QJsonDocument::fromJson(file.readAll()).array();
        for (const auto& batchVal : batches) {
            QJsonObject batchObj = batchVal.toObject();
            QDateTime qTime = QDateTime::fromString(batchObj["batchTime"].toString(), Qt::ISODate);
            if (!qTime.isValid() || qTime >= thirtyDaysAgo) {
                QJsonArray qArr = batchObj["questions"].toArray();
                for (const auto& qVal : qArr) {
                    if (!qVal.toObject()["isCompleted"].toBool()) {
                        unfinishedCount++;
                    }
                }
            }
        }
        file.close();
    }

    if (m_recommendCountValueLabel) {
        if (unfinishedCount > 0) {
            m_recommendCountValueLabel->setText(QStringLiteral("%1 道未完成").arg(unfinishedCount));
            if (m_recommendSubLabel) m_recommendSubLabel->setText(QStringLiteral("点击进入练习"));
        } else {
            m_recommendCountValueLabel->setText(QStringLiteral("0 道未完成"));
            if (m_recommendSubLabel) m_recommendSubLabel->setText(QStringLiteral("点击智能生成"));
        }
    }

    this->setProperty("recommendCount", unfinishedCount);
}

void LearningPage::handleRecommendCardClicked()
{
    int count = this->property("recommendCount").toInt();
    if (count > 0) {
        if (m_recommendPage) {
            m_recommendPage->reloadQuestions();
            m_stack->setCurrentWidget(m_recommendPage);
        }
    } else {
        CustomGenerateDialog dlg(this);
        if (dlg.exec() == QDialog::Accepted) {
            generateRecommendedPractice(dlg.getValue());
        }
    }
}

void LearningPage::updateAutoCheckin()
{
    constexpr int autoCheckinSeconds = 5 * 60;
    if (m_todayAutoCheckedIn || m_todayStudySeconds <= autoCheckinSeconds) return;

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
    if (!m_stack) return QStringLiteral("overview");
    switch (m_stack->currentIndex()) {
    case 1: return QStringLiteral("knowledge");
    case 2:
    case 3:
    case 4:
    case 5: return QStringLiteral("practice");
    case 6: return QStringLiteral("exam");
    case 7: return QStringLiteral("wrongbook");
    case 8: return QStringLiteral("management");
    case 0:
    default: return QStringLiteral("overview");
    }
}

QString LearningPage::formatStudyDuration(int seconds) const
{
    const int minutes = seconds / 60;
    if (minutes < 1) return QStringLiteral("不足 1 分钟");
    const int hours = minutes / 60;
    const int remainMinutes = minutes % 60;
    if (hours <= 0) return QStringLiteral("%1 分钟").arg(minutes);
    if (remainMinutes == 0) return QStringLiteral("%1 小时").arg(hours);
    return QStringLiteral("%1 小时 %2 分钟").arg(hours).arg(remainMinutes);
}

void LearningPage::generateRecommendedPractice(int count)
{
    QString configPath = QCoreApplication::applicationDirPath() + "/algemate_ai.conf";
    QFile configFile(configPath);
    QString apiKey = "";
    if (configFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        apiKey = QString(QByteArray::fromBase64(configFile.readLine().trimmed()));
    }
    if (apiKey.isEmpty()) {
        QMessageBox::critical(this, QStringLiteral("提示"), QStringLiteral("未检测到 API Key，请去 AI 模块配置。"));
        refreshRecommendCountCard();
        return;
    }

    if (m_recommendSubLabel) m_recommendSubLabel->setText(QStringLiteral("⏳ AI 正在生成..."));
    if (m_recommendPage) {
        m_recommendPage->reloadQuestions();
        m_recommendPage->showLoadingPlaceholder();
        m_stack->setCurrentWidget(m_recommendPage);
    }

    QString wrongStr = "无错题记录";
    QFile wrongFile("wrong_questions.json");
    if (wrongFile.open(QIODevice::ReadOnly)) {
        wrongFile.close();
    }

    QString prompt = QString("你是一个大学线性代数/高等代数老师。请根据以下学生的错题记录（如果有），"
                             "或者基于线性代数的核心考点，为他生成 %1 道专属的主观推导计算题。\n\n"
                             "【绝对要求】：\n"
                             "1. 必须生成严格的 JSON 数组格式，绝对不要包含任何 Markdown 格式(如```json)或多余文字。\n"
                             "2. 包含字段：\"content\"（题目，支持 LaTeX），\"correctAnswer\"（解析，支持 LaTeX），\"score\"（分数，如 10），\"type\": \"subjective\"。\n"
                             "3. 数量红线：必须严格生成正好 %1 道题目！绝对不能多，也绝对不能少！\n\n"
                             "错题记录：\n%2").arg(count).arg(wrongStr);

    QJsonObject rootObj;
    rootObj["model"] = "deepseek-chat";
    rootObj["stream"] = false;
    QJsonArray messages;
    QJsonObject sysMsg, usrMsg;
    sysMsg["role"] = "system";
    sysMsg["content"] = "你是专业的考研高代命题专家，严格遵守 JSON 格式输出。";
    usrMsg["role"] = "user";
    usrMsg["content"] = prompt;
    messages.append(sysMsg);
    messages.append(usrMsg);
    rootObj["messages"] = messages;

    m_networkMgr = new QNetworkAccessManager(this);
    QNetworkRequest request{QUrl(QStringLiteral("https://api.deepseek.com/chat/completions"))};
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization", QString("Bearer %1").arg(apiKey).toUtf8());

    QNetworkReply* reply = m_networkMgr->post(request, QJsonDocument(rootObj).toJson());

    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        if (reply->error() == QNetworkReply::NoError) {
            QByteArray rawData = reply->readAll();
            QJsonDocument responseDoc = QJsonDocument::fromJson(rawData);
            QJsonObject responseObj = responseDoc.object();

            QString aiText;
            if (responseObj.contains("choices")) {
                aiText = responseObj["choices"].toArray()[0].toObject()["message"].toObject()["content"].toString();
            } else {
                aiText = QString::fromUtf8(rawData);
            }

            QJsonArray newQuestions;
            int startIndex = aiText.indexOf('[');
            int endIndex = aiText.lastIndexOf(']');
            if (startIndex != -1 && endIndex != -1 && startIndex <= endIndex) {
                QString pureJson = aiText.mid(startIndex, endIndex - startIndex + 1);
                newQuestions = QJsonDocument::fromJson(pureJson.toUtf8()).array();
            }

            if (newQuestions.isEmpty()) {
                if (m_recommendSubLabel) m_recommendSubLabel->setText(QStringLiteral("点击重新生成"));
                QMessageBox::warning(this, QStringLiteral("生成失败"),
                                     QStringLiteral("AI 返回的数据格式不符合要求，解析失败。请再试一次。\n\n(AI 实际返回：%1...)").arg(aiText.left(50)));
                if (m_recommendPage) m_recommendPage->reloadQuestions();
                return;
            }

            QJsonArray historyBatches;
            QFile file("recommended_questions.json");
            QDateTime thirtyDaysAgo = QDateTime::currentDateTime().addDays(-30);

            if (file.exists() && file.open(QIODevice::ReadOnly)) {
                QJsonArray tempArr = QJsonDocument::fromJson(file.readAll()).array();
                for (const auto& val : tempArr) {
                    QJsonObject batchObj = val.toObject();
                    QString timeStr = batchObj["batchTime"].toString();
                    QDateTime qTime = QDateTime::fromString(timeStr, Qt::ISODate);
                    if (!qTime.isValid() || qTime >= thirtyDaysAgo) {
                        historyBatches.append(batchObj);
                    }
                }
                file.close();
            }

            QJsonObject newBatch;
            newBatch["batchTime"] = QDateTime::currentDateTime().toString(Qt::ISODate);
            QJsonArray formattedQuestions;
            for (auto val : newQuestions) {
                QJsonObject qObj = val.toObject();
                qObj["id"] = QRandomGenerator::global()->bounded(1000000, 9999999);
                qObj["isCompleted"] = false;
                formattedQuestions.append(qObj);
            }
            newBatch["questions"] = formattedQuestions;
            historyBatches.append(newBatch);

            if (file.open(QIODevice::WriteOnly)) {
                file.write(QJsonDocument(historyBatches).toJson());
                file.close();
            }

            refreshRecommendCountCard();

            if (m_recommendPage) {
                m_recommendPage->reloadQuestions();
            }

            QMessageBox::information(this, QStringLiteral("生成完毕"),
                                     QStringLiteral("为您成功生成了 %1 道专属推荐题！").arg(newQuestions.size()));
        } else {
            QMessageBox::warning(this, QStringLiteral("网络错误"), QStringLiteral("连接 AI 服务器失败，请检查网络或 API Key 设置。"));
            if (m_recommendPage) m_recommendPage->reloadQuestions();
        }

        reply->deleteLater();
        m_networkMgr->deleteLater();
        m_networkMgr = nullptr;
        refreshRecommendCountCard();
    });
}

} // namespace AlgeMate::Learning