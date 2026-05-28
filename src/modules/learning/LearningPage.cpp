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
#include <QTreeWidget>
#include <QTreeWidgetItem>

namespace AlgeMate::Learning {


// 工厂函数: 创建带内容的可点击卡片

static ClickableCard* makeStatCard(const QString& label, const QString& value,
                                    const QString& sub, QWidget* parent)
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

    buildDashboard();
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

    // Knowledge → 章节练习（动态捕获当前浏览的微观小节并实现题库精准过滤）changed
    connect(m_knowledgePage, &KnowledgePage::enterChapterPractice, this, [this]() {
        // 1. 获取左侧目录树指针
        if (auto* tree = m_knowledgePage->findChild<QTreeWidget*>(QStringLiteral("KnowledgeTree"))) {
            if (auto* currentItem = tree->currentItem()) {
                // 2. 提取节点中绑定的资源路径（例如：":/knowledge/ch01_1.md"）
                QString fullPath = currentItem->data(0, Qt::UserRole + 1).toString();

                if (!fullPath.isEmpty()) {
                    // 3. 将路径传递给题库页面进行微观过滤
                    m_chapterPracPage->loadQuestionsByMicroChapter(fullPath);
                }
            }
        }
        // 4. 执行原有的切页动作
        showChapterPractice();
    });
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

    auto* stat1 = makeStatCard(QStringLiteral("今日学习"), QStringLiteral("0 分钟"), QStringLiteral("今日暂无记录"), m_dashboard);
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
void LearningPage::showChapterPractice()
{
    // 保底逻辑：如果是直接从快捷入口点进来的，或者是从非知识点页面切过来的，
    // 调用初始化钩子，让其重置为全量题库，避免因为上次残留的筛选导致题目数量过少。
    if (m_chapterPracPage) {
        m_chapterPracPage->onLoadChapterQuestions();
    }
    // 执行切页，切到 StackedWidget 的第 4 页（ChapterPracticePage）
    m_stack->setCurrentIndex(4);
}
void LearningPage::showTopicPractice()       { m_stack->setCurrentIndex(5); }
void LearningPage::showExam()             { m_stack->setCurrentIndex(6); }
void LearningPage::showWrongBook()
{
    m_wrongBookPage->reload(); // 重新读取错题
    m_stack->setCurrentIndex(7);
}
void LearningPage::showLearningCenter()   { m_stack->setCurrentIndex(8); }
void LearningPage::goBack()              { m_stack->setCurrentIndex(0); }

} // namespace AlgeMate::Learning
