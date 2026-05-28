#include "PracticePage.h"
#include "ClickableCard.h"

// 引入 LaTeX 渲染核心头文件
#include "latex/LatexRenderer.h"
#include "latex/LatexTextBrowser.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QPushButton>
#include <QFrame>
#include <QScrollArea>
#include <QRadioButton>
#include <QButtonGroup>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QMessageBox>
#include <QFile>
#include <QTextStream>
#include <QCoreApplication>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QUrl>
#include <cmath>
#include <QSplitter>
#include <QTreeWidget>
#include <QTreeWidgetItem>

namespace AlgeMate::Learning {

// ==================== 辅助函数 ====================

static QPushButton* makeBackBtn(QWidget* parent = nullptr) {
    auto* btn = new QPushButton(QStringLiteral("← 返回"), parent);
    btn->setObjectName(QStringLiteral("LearnBackBtn"));
    btn->setStyleSheet(
        "QPushButton#LearnBackBtn {"
        "    background-color: transparent;"
        "    border: 1px solid #dcdfe6;"
        "    color: #606266;"
        "    padding: 6px 12px;"
        "    border-radius: 4px;"
        "    font-size: 13px;"
        "}"
        "QPushButton#LearnBackBtn:hover {"
        "    background-color: #f5f7fa;"
        "    color: #409eff;"
        "    border-color: #c6e2ff;"
        "}"
        );
    return btn;
}

static ClickableCard* makeCardItem(const QString& icon, const QString& title,
                                   const QString& desc, QWidget* parent)
{
    auto* card = new ClickableCard(parent);
    card->setMinimumHeight(130);
    auto* lay = new QVBoxLayout(card);
    lay->setContentsMargins(24, 20, 24, 18);
    lay->setSpacing(6);

    auto* ic = new QLabel(icon);
    ic->setStyleSheet("font-size:32px; background:transparent;");
    auto* t  = new QLabel(title);
    t->setStyleSheet("font-size:17px; font-weight:700; background:transparent;");
    auto* d  = new QLabel(desc);
    d->setStyleSheet("font-size:13px; background:transparent; color: #666;");
    d->setWordWrap(true);

    lay->addWidget(ic);
    lay->addWidget(t);
    lay->addStretch();
    lay->addWidget(d);
    return card;
}

// ==================== PracticePage ====================

PracticePage::PracticePage(QWidget* parent) : QWidget(parent)
{
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(24, 16, 24, 24);
    root->setSpacing(16);

    auto* top = new QHBoxLayout;
    auto* back = makeBackBtn(this);
    connect(back, &QPushButton::clicked, this, &PracticePage::backRequested);
    auto* title = new QLabel(QStringLiteral("练习模式"));
    title->setObjectName(QStringLiteral("PageTitle"));
    title->setStyleSheet("font-size: 20px; font-weight: bold; color: #2c3e50;");
    top->addWidget(back);
    top->addWidget(title);
    top->addStretch();

    auto* grid = new QGridLayout;
    grid->setSpacing(16);

    auto* card1 = makeCardItem(QStringLiteral("🔢"), QStringLiteral("计算题"),
                               QStringLiteral("随机生成计算题\n实时输入矩阵并计算"), this);
    auto* card2 = makeCardItem(QStringLiteral("📖"), QStringLiteral("对应章节练习"),
                               QStringLiteral("按当前章节针对性训练\n与知识点章节绑定"), this);
    auto* card3 = makeCardItem(QStringLiteral("🎯"), QStringLiteral("专题模式"),
                               QStringLiteral("跨章节综合训练\n特征值 · 线性空间 · 综合矩阵"), this);

    connect(card1, &ClickableCard::clicked, this, &PracticePage::calculationProblemRequested);
    connect(card2, &ClickableCard::clicked, this, &PracticePage::chapterPracticeRequested);
    connect(card3, &ClickableCard::clicked, this, &PracticePage::topicPracticeRequested);

    grid->addWidget(card1, 0, 0);
    grid->addWidget(card2, 0, 1);
    grid->addWidget(card3, 1, 0);

    root->addLayout(top);
    root->addLayout(grid);
    root->addStretch();
}

// ==================== CalculationProblemPage ====================

CalculationProblemPage::CalculationProblemPage(QWidget* parent) : QWidget(parent)
{
    auto* lay = new QVBoxLayout(this);
    lay->setContentsMargins(24, 16, 24, 24);
    lay->setSpacing(14);

    auto* top = new QHBoxLayout;
    auto* back = makeBackBtn(this);
    connect(back, &QPushButton::clicked, this, &CalculationProblemPage::backRequested);
    auto* t = new QLabel(QStringLiteral("计算题练习"));
    t->setObjectName(QStringLiteral("PageTitle"));
    top->addWidget(back);
    top->addWidget(t);
    top->addStretch();

    auto* ph = new QLabel(QStringLiteral("（计算题功能开发中...）"));
    ph->setAlignment(Qt::AlignCenter);
    ph->setObjectName(QStringLiteral("PlaceholderLabel"));

    lay->addLayout(top);
    lay->addWidget(ph, 1);
}

// ==================== ChapterPracticePage ====================

ChapterPracticePage::ChapterPracticePage(QWidget* parent)
    : QWidget(parent), currentQuestionIndex(0), answerWidget(nullptr)
{
    auto* renderer = new Latex::LatexRenderer;
    renderer->addMathMacro(QStringLiteral("F"),  QStringLiteral("\\mathbb{F}"));
    renderer->addMathMacro(QStringLiteral("R"),  QStringLiteral("\\mathbb{R}"));
    renderer->addMathMacro(QStringLiteral("C"),  QStringLiteral("\\mathbb{C}"));
    this->setProperty("latex_renderer", QVariant::fromValue(static_cast<void*>(renderer)));

    auto* mainLay = new QVBoxLayout(this);
    mainLay->setContentsMargins(24, 16, 24, 24);
    mainLay->setSpacing(12);

    // 1. 固定顶栏：导航
    auto* top = new QHBoxLayout;
    auto* back = makeBackBtn(this);
    connect(back, &QPushButton::clicked, this, &ChapterPracticePage::backRequested);

    auto* t = new QLabel(QStringLiteral("对应章节练习"), this);
    t->setObjectName(QStringLiteral("PageTitle"));
    // t->setStyleSheet("font-size: 20px; font-weight: bold; color: #2c3e50;");

    // 右侧小节实时动态标题指示灯
    m_chapterTitleLabel = new QLabel(QStringLiteral("【当前章节：全量随堂练习】"), this);
    m_chapterTitleLabel->setStyleSheet(
        "font-size: 14px;"
        "color: #6d5bd0;"
        "font-weight: 700;"
        "background: #f1ecff;"
        "padding: 6px 14px;"
        "border-radius: 999px;"
        "margin-left: 12px;"
    );
    top->addWidget(back);
    top->addWidget(t);
    top->addWidget(m_chapterTitleLabel);
    top->addStretch();
    mainLay->addLayout(top);

    // 2. 引入左右分栏机制（完美复刻 KnowledgePage 架构）
    m_splitter = new QSplitter(Qt::Horizontal, this);
    m_splitter->setChildrenCollapsible(false);

    // 左侧：树目录
    m_chapterTree = new QTreeWidget(m_splitter);
    m_chapterTree->setObjectName(QStringLiteral("PracticeChapterTree"));
    m_chapterTree->setHeaderHidden(true);
    m_chapterTree->setMinimumWidth(240);
    m_chapterTree->setIndentation(15);
    buildChapterTree();

    // 右侧：核心答题卡片层
    auto* rightContainer = new QWidget(m_splitter);
    auto* rightLayout = new QVBoxLayout(rightContainer);
    rightLayout->setContentsMargins(0, 0, 0, 0);
    rightLayout->setSpacing(12);

    auto* mainScroll = new QScrollArea(rightContainer);
    mainScroll->setWidgetResizable(true);
    mainScroll->setFrameShape(QFrame::NoFrame);

    auto* scrollContainer = new QWidget(this);
    scrollContainer->setStyleSheet(
        "background: #f8fafc;"
        "border-radius: 16px;"
    );
    auto* scrollLay = new QVBoxLayout(scrollContainer);
    scrollLay->setContentsMargins(0, 4, 0, 4);
    scrollLay->setSpacing(16);

    progressLabel = new QLabel(scrollContainer);
    progressLabel->setStyleSheet("color: #4a5568; font-size: 13px; background-color: #edf2f7; padding: 8px 12px; border-radius: 6px; font-weight: 500;");
    scrollLay->addWidget(progressLabel);

    auto* qBrowser = new Latex::LatexTextBrowser(scrollContainer);
    qBrowser->setObjectName(QStringLiteral("QuestionTextBrowser"));
    qBrowser->setFrameShape(QFrame::NoFrame);
    qBrowser->setMinimumHeight(160);
    qBrowser->setStyleSheet(
        "background: white;"
        "border: 1px solid #e5e7eb;"
        "border-radius: 16px;"
        "padding: 20px;"
    );
    // qBrowser->setStyleSheet("background-color: #ffffff; border: 1px solid #e2e8f0; border-radius: 8px; padding: 12px;");
    questionLabel = reinterpret_cast<QLabel*>(qBrowser);
    scrollLay->addWidget(qBrowser, 0);

    answerWidget = new QWidget(scrollContainer);
    auto* answerLayout = new QVBoxLayout(answerWidget);
    answerLayout->setContentsMargins(0, 0, 0, 0);
    scrollLay->addWidget(answerWidget, 0);

    auto* advancedFeedback = new Latex::LatexTextBrowser(scrollContainer);
    advancedFeedback->setObjectName(QStringLiteral("AdvancedFeedbackView"));
    advancedFeedback->setMinimumHeight(160);
    advancedFeedback->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
    advancedFeedback->hide();
    feedbackLabel = reinterpret_cast<QLabel*>(advancedFeedback);
    scrollLay->addWidget(advancedFeedback, 0);

    scrollLay->addStretch(1);
    mainScroll->setWidget(scrollContainer);
    rightLayout->addWidget(mainScroll, 1);

    // 按钮区
    auto* bottomLayout = new QHBoxLayout;
    auto* prevBtn = new QPushButton(QStringLiteral("← 上一题"), this);
    auto* submitBtn = new QPushButton(QStringLiteral("确认提交"), this);
    auto* nextBtn = new QPushButton(QStringLiteral("下一题 →"), this);
    QString baseBtnStyle = "QPushButton { padding: 8px 16px; font-size: 13px; border-radius: 6px; font-weight: 500; }";
    prevBtn->setStyleSheet(baseBtnStyle + "QPushButton { background-color: #ffffff; border: 1px solid #cbd5e0; color: #4a5568; }");
    nextBtn->setStyleSheet(baseBtnStyle + "QPushButton { background-color: #ffffff; border: 1px solid #cbd5e0; color: #4a5568; }");
    submitBtn->setStyleSheet(baseBtnStyle + "QPushButton { background-color: #3182ce; border: none; color: white; padding: 10px 24px; font-size: 14px; }");

    connect(prevBtn, &QPushButton::clicked, this, &ChapterPracticePage::onPreviousQuestion);
    connect(submitBtn, &QPushButton::clicked, this, &ChapterPracticePage::onSubmitAnswer);
    connect(nextBtn, &QPushButton::clicked, this, &ChapterPracticePage::onNextQuestion);
    bottomLayout->addWidget(prevBtn); bottomLayout->addStretch(); bottomLayout->addWidget(submitBtn); bottomLayout->addWidget(nextBtn);
    rightLayout->addLayout(bottomLayout);

    m_splitter->addWidget(m_chapterTree);
    m_splitter->addWidget(rightContainer);
    m_splitter->setStretchFactor(0, 0);
    m_splitter->setStretchFactor(1, 1);
    mainLay->addWidget(m_splitter, 1);
    this->setStyleSheet(R"(

    /* 整体背景 */
    ChapterPracticePage {
        background-color: #f6f3ff;
    }

    /* 页面标题 */
    QLabel#PageTitle {
        font-size: 24px;
        font-weight: 700;
        color: #443c68;
    }

    /* 当前章节标签 */
    QLabel {
        font-family: "Microsoft YaHei";
    }

    /* 左侧目录树 */
    QTreeWidget#PracticeChapterTree {
        background: #ffffff;
        border: 1px solid #e6e1f5;
        border-radius: 14px;
        padding: 10px;
        font-size: 15px;
        color: #4b445f;
        outline: none;
    }

    /* 树节点 */
    QTreeWidget#PracticeChapterTree::item {
        height: 40px;
        border-radius: 8px;
        padding-left: 10px;
        margin: 2px 0px;
    }

    /* hover */
    QTreeWidget#PracticeChapterTree::item:hover {
        background: #f1ecff;
        color: #6d5bd0;
    }

    /* 选中 */
    QTreeWidget#PracticeChapterTree::item:selected {
        background: #e5dcff;
        color: #5b4db2;
        font-weight: 700;
    }

    /* 一级章节 */
    QTreeWidget#PracticeChapterTree::branch:has-children {
        font-weight: 700;
        color: #443c68;
    }

    /* splitter */
    QSplitter::handle {
        background: transparent;
        width: 8px;
    }

    QSplitter::handle:hover {
        background: #ece6ff;
    }

    /* 进度栏 */
    QLabel {
        color: #5c5470;
    }

    /* 题目区 */
    LatexTextBrowser#QuestionTextBrowser {
        background: #ffffff;
        border: 1px solid #ebe7f7;
        border-radius: 18px;
        padding: 20px;
        font-size: 15px;
        color: #3f3a52;
    }

    /* 反馈区 */
    QTextBrowser#AdvancedFeedbackView {
        border-radius: 18px;
        padding: 18px;
        font-size: 14px;
    }

    /* 输入框 */
    QLineEdit {
        height: 40px;
        border: 1px solid #ddd6f3;
        border-radius: 10px;
        padding-left: 12px;
        background: white;
        font-size: 14px;
        color: #443c68;
    }

    QLineEdit:focus {
        border: 1px solid #9b87f5;
    }

    /* 主观题输入框 */
    QPlainTextEdit {
        border: 1px solid #ddd6f3;
        border-radius: 12px;
        padding: 10px;
        background: white;
        font-size: 14px;
        color: #443c68;
    }

    QPlainTextEdit:focus {
        border: 1px solid #9b87f5;
    }

    /* 普通按钮 */
    QPushButton {
        background: white;
        border: 1px solid #ddd6f3;
        border-radius: 10px;
        padding: 8px 18px;
        font-size: 14px;
        color: #52486b;
    }

    QPushButton:hover {
        background: #f3efff;
    }

    /* AI按钮 */
    QPushButton#InnerAiGradeButton {
        background: #9b87f5;
        color: white;
        border: none;
        font-weight: 700;
    }

    QPushButton#InnerAiGradeButton:hover {
        background: #8a74eb;
    }

    /* 滚动条 */
    QScrollBar:vertical {
        width: 10px;
        background: transparent;
    }

    QScrollBar::handle:vertical {
        background: #d6cdf7;
        border-radius: 5px;
    }

    QScrollBar::handle:vertical:hover {
        background: #bcaef5;
    }

    QScrollBar::add-line:vertical,
    QScrollBar::sub-line:vertical {
        height: 0px;
    }

    )");

    // 左侧目录树节点点击联动逻辑
    connect(m_chapterTree, &QTreeWidget::currentItemChanged, this, [this](QTreeWidgetItem* current, QTreeWidgetItem*) {
        if (!current) return;
        QString path = current->data(0, Qt::UserRole + 1).toString();
        if (!path.isEmpty()) {
            m_chapterTitleLabel->setText(QStringLiteral("【当前章节：%1】").arg(current->text(0)));
            loadQuestionsByMicroChapter(path); // 这里的联动加载在上一轮对话已给写完
        }
    });

    onLoadChapterQuestions();
    if (!chapterQuestions.isEmpty()) {
        loadQuestion(0);
    }
}

// 编写具体的目录树结构生成
void ChapterPracticePage::buildChapterTree() {
    m_chapterTree->clear();

    auto addChapter = [this](const QString& title) {
        auto* node = new QTreeWidgetItem(m_chapterTree);
        node->setText(0, title);
        node->setExpanded(true);
        return node;
    };

    auto addSection = [](QTreeWidgetItem* chapter, const QString& title, const QString& resourcePath) {
        auto* leaf = new QTreeWidgetItem(chapter);
        leaf->setText(0, title);
        // 保持和外部 Lambda 捕获解析一致，统一使用 Qt::UserRole + 1 存储路径
        leaf->setData(0, Qt::UserRole + 1, resourcePath);
        return leaf;
    };

    // ==================== 第 1 章 多项式 ====================
    QTreeWidgetItem* ch1 = addChapter(QStringLiteral("第 1 章 多项式"));
    addSection(ch1, QStringLiteral("1.1 整除与带余除法"), QStringLiteral(":/knowledge/ch01_1.md"));
    addSection(ch1, QStringLiteral("1.2 最大公因式"), QStringLiteral(":/knowledge/ch01_2.md"));
    addSection(ch1, QStringLiteral("1.3 不可约多项式与唯一分解定理"), QStringLiteral(":/knowledge/ch01_3.md"));
    addSection(ch1, QStringLiteral("1.4 重因式"), QStringLiteral(":/knowledge/ch01_4.md"));
    addSection(ch1, QStringLiteral("1.5 n元多项式环与对称多项式"), QStringLiteral(":/knowledge/ch01_5.md"));

    // ==================== 第 2 章 行列式 ====================
    QTreeWidgetItem* ch2 = addChapter(QStringLiteral("第 2 章 行列式"));
    addSection(ch2, QStringLiteral("2.1 行列式的定义"), QStringLiteral(":/knowledge/ch02_1.md"));
    addSection(ch2, QStringLiteral("2.2 克拉默法则与拉普拉斯定理"), QStringLiteral(":/knowledge/ch02_2.md"));

    // ==================== 第 3 章 n维向量与向量空间 ====================
    QTreeWidgetItem* ch3 = addChapter(QStringLiteral("第 3 章 n维向量与向量空间"));
    addSection(ch3, QStringLiteral("3.1 n维向量与向量空间"), QStringLiteral(":/knowledge/ch03_1.md"));
    addSection(ch3, QStringLiteral("3.2 极大线性无关组"), QStringLiteral(":/knowledge/ch03_2.md"));
    addSection(ch3, QStringLiteral("3.3 向量组的秩"), QStringLiteral(":/knowledge/ch03_3.md"));
    addSection(ch3, QStringLiteral("3.4 矩阵的秩"), QStringLiteral(":/knowledge/ch03_4.md"));
    addSection(ch3, QStringLiteral("3.5 线性方程组的解"), QStringLiteral(":/knowledge/ch03_5.md"));

    // ==================== 第 4 章 矩阵的运算 ====================
    QTreeWidgetItem* ch4 = addChapter(QStringLiteral("第 4 章 矩阵的运算"));
    addSection(ch4, QStringLiteral("4.1 矩阵的加法、数乘与乘法"), QStringLiteral(":/knowledge/ch04_1.md"));
    addSection(ch4, QStringLiteral("4.2 可逆矩阵"), QStringLiteral(":/knowledge/ch04_2.md"));
    addSection(ch4, QStringLiteral("4.3 分块矩阵"), QStringLiteral(":/knowledge/ch04_3.md"));

    // ==================== 第 5 章 矩阵的相抵与相似 ====================
    QTreeWidgetItem* ch5 = addChapter(QStringLiteral("第 5 章 矩阵的相抵与相似"));
    addSection(ch5, QStringLiteral("5.1 矩阵的相抵"), QStringLiteral(":/knowledge/ch05_1.md"));
    addSection(ch5, QStringLiteral("5.2 矩阵的相似"), QStringLiteral(":/knowledge/ch05_2.md"));
    addSection(ch5, QStringLiteral("5.3 特征向量与矩阵可对角化"), QStringLiteral(":/knowledge/ch05_3.md"));
    addSection(ch5, QStringLiteral("5.4 实对称矩阵的正交对角化"), QStringLiteral(":/knowledge/ch05_4.md"));

    // ==================== 第 6 章 二次型 ====================
    QTreeWidgetItem* ch6 = addChapter(QStringLiteral("第 6 章 二次型"));
    addSection(ch6, QStringLiteral("6.1 二次型的定义、规范形"), QStringLiteral(":/knowledge/ch06_1.md"));
    addSection(ch6, QStringLiteral("6.2 正定二次型与正定矩阵"), QStringLiteral(":/knowledge/ch06_2.md"));

    // ==================== 第 7 章 线性空间 ====================
    QTreeWidgetItem* ch7 = addChapter(QStringLiteral("第 7 章 线性空间"));
    addSection(ch7, QStringLiteral("7.1 基与维数"), QStringLiteral(":/knowledge/ch07_1.md"));
    addSection(ch7, QStringLiteral("7.2 子空间的交、和与直和"), QStringLiteral(":/knowledge/ch07_2.md"));
    addSection(ch7, QStringLiteral("7.3 线性空间的同构"), QStringLiteral(":/knowledge/ch07_3.md"));
    addSection(ch7, QStringLiteral("7.4 商空间"), QStringLiteral(":/knowledge/ch07_4.md"));

    // ==================== 第 8 章 线性映射 ====================
    QTreeWidgetItem* ch8 = addChapter(QStringLiteral("第 8 章 线性映射"));
    addSection(ch8, QStringLiteral("8.1 线性映射的定义"), QStringLiteral(":/knowledge/ch08_1.md"));
    addSection(ch8, QStringLiteral("8.2 核与像"), QStringLiteral(":/knowledge/ch08_2.md"));
    addSection(ch8, QStringLiteral("8.3 线性映射的矩阵表示"), QStringLiteral(":/knowledge/ch08_3.md"));
    addSection(ch8, QStringLiteral("8.4 不变子空间与 Cayley–Hamilton定理"), QStringLiteral(":/knowledge/ch08_4.md"));
    addSection(ch8, QStringLiteral("8.5 Jordan标准形"), QStringLiteral(":/knowledge/ch08_5.md"));

    // ==================== 第 9 章 lambda-矩阵 ====================
    QTreeWidgetItem* ch9 = addChapter(QStringLiteral("第 9 章 lambda-矩阵"));
    addSection(ch9, QStringLiteral("9.1 lambda-矩阵的定义"), QStringLiteral(":/knowledge/ch09_1.md"));
    addSection(ch9, QStringLiteral("9.2 Smith 标准形"), QStringLiteral(":/knowledge/ch09_2.md"));
    addSection(ch9, QStringLiteral("9.3 不变因子与 Jordan 标准形"), QStringLiteral(":/knowledge/ch09_3.md"));

    // ==================== 第 10 章 具有度量的线性空间 ====================
    QTreeWidgetItem* ch10 = addChapter(QStringLiteral("第 10 章 具有度量的线性空间"));
    addSection(ch10, QStringLiteral("10.1 内积与欧几里得空间"), QStringLiteral(":/knowledge/ch10_1.md"));
    addSection(ch10, QStringLiteral("10.2 正交变换与对称变换"), QStringLiteral(":/knowledge/ch10_2.md"));
    addSection(ch10, QStringLiteral("10.3 酉空间与 Hermite 矩阵"), QStringLiteral(":/knowledge/ch10_3.md"));
    addSection(ch10, QStringLiteral("10.4 最小二乘法"), QStringLiteral(":/knowledge/ch10_4.md"));

    // 默认展开到小节层级
    m_chapterTree->expandToDepth(1);
}

void ChapterPracticePage::onLoadChapterQuestions() {
    // 默认初始化时加载全量题库
    chapterQuestions = QuestionBank::getAllQuestions();
}

// 在下方直接添加这个新函数的完整实现：
void ChapterPracticePage::loadQuestionsByMicroChapter(const QString& microMarkdownName) {
    // 提取出文件名，去除可能存在的 ":/knowledge/" 路径头
    QString cleanName = microMarkdownName;
    if (cleanName.contains('/')) {
        cleanName = cleanName.split('/').last();
    }

    // 直接通过解耦的 QuestionBank 精准获取该小节的 3 道题！
    chapterQuestions = QuestionBank::getQuestionsByChapter(cleanName);

    // 保底策略：如果某个小节暂时没录入题目，加载全量，防止白屏
    if (chapterQuestions.isEmpty()) {
        chapterQuestions = QuestionBank::getAllQuestions();
    }

    // 从第 0 题开始渲染
    loadQuestion(0);
}

void ChapterPracticePage::loadQuestion(int index) {
    if (index < 0 || index >= chapterQuestions.size()) {
        return;
    }

    currentQuestionIndex = index;
    const Question& q = chapterQuestions[index];

    // 显式更新顶部面板
    progressLabel->setText(
        QStringLiteral(" 📝 当前进度：第 %1 题 / 共 %2 题   |   本题得分：%3分   |   尝试次数：%4 次")
            .arg(index + 1)
            .arg(chapterQuestions.size())
            .arg(q.score)
            .arg(q.attempts)
        );

    // newly changed
    auto* qBrowser = reinterpret_cast<Latex::LatexTextBrowser*>(questionLabel);
    auto* renderer = static_cast<Latex::LatexRenderer*>(this->property("latex_renderer").value<void*>());
    if (renderer && qBrowser) {
        qBrowser->setHtml(renderer->render(q.content, qBrowser->document()));
    } else {
        questionLabel->setText(q.content);
    }
    feedbackLabel->hide(); // 切换新题时隐藏上一次的反馈

    updateUIForQuestion(q);
}

void ChapterPracticePage::updateUIForQuestion(const Question& q) {
    // 彻底销毁旧交互组件，阻断混淆残留
    if (auto* oldLayout = answerWidget->layout()) {
        QLayoutItem* child;
        while ((child = oldLayout->takeAt(0)) != nullptr) {
            if (child->widget()) {
                child->widget()->hide();
                child->widget()->deleteLater();
            } else if (child->layout()) {
                QLayoutItem* subChild;
                while ((subChild = child->layout()->takeAt(0)) != nullptr) {
                    if (subChild->widget()) {
                        subChild->widget()->hide();
                        subChild->widget()->deleteLater();
                    }
                    delete subChild;
                }
                child->layout()->deleteLater();
            }
            delete child;
        }
    } else {
        auto* newLayout = new QVBoxLayout(answerWidget);
        answerWidget->setLayout(newLayout);
    }

    auto* answerLayout = qobject_cast<QVBoxLayout*>(answerWidget->layout());
    if (!answerLayout) return;
    answerLayout->setContentsMargins(4, 10, 4, 10);
    answerLayout->setSpacing(12);

    if (q.type == QuestionType::Single) {

        auto* buttonGroup = new QButtonGroup(answerWidget);
        auto* renderer =
            static_cast<Latex::LatexRenderer*>(
                this->property("latex_renderer").value<void*>());

        for (int i = 0; i < q.choices.size(); ++i) {

            // =========================
            // 外层卡片
            // =========================
            auto* optionCard = new QWidget(answerWidget);
            optionCard->setObjectName(QStringLiteral("OptionCard"));

            auto* cardLayout = new QHBoxLayout(optionCard);
            cardLayout->setContentsMargins(16, 14, 16, 14);
            cardLayout->setSpacing(14);

            // =========================
            // 单选按钮
            // =========================
            QString optionLetter = QString(QChar('A' + i));

            auto* radio = new QRadioButton(
                optionLetter,
                optionCard);
            radio->setObjectName(QStringLiteral("choice_%1").arg(i));

            buttonGroup->addButton(radio, i);

            radio->setStyleSheet(R"(

                QRadioButton {
                    font-size: 16px;
                    font-weight: 700;
                    color: #6d5bd0;
                    spacing: 12px;
                }

                QRadioButton::indicator {
                    width: 20px;
                    height: 20px;
                    border-radius: 10px;
                    border: 2px solid #b8abd9;
                    background: white;
                }

                QRadioButton::indicator:hover {
                    border: 2px solid #9b87f5;
                }

                QRadioButton::indicator:checked {
                    background: #9b87f5;
                    border: 2px solid #9b87f5;
                }

            )");

            // =========================
            // Latex 选项内容
            // =========================
            auto* optBrowser =
                new Latex::LatexTextBrowser(optionCard);
            optBrowser->setAttribute(Qt::WA_TransparentForMouseEvents);//让整个区域都能点击

            optBrowser->setFrameShape(QFrame::NoFrame);

            optBrowser->setVerticalScrollBarPolicy(
                Qt::ScrollBarAlwaysOff);

            optBrowser->setHorizontalScrollBarPolicy(
                Qt::ScrollBarAlwaysOff);

            optBrowser->setStyleSheet(
                "background: transparent;"
                "font-size: 15px;"
                "color: #443c68;"
                );

            optBrowser->setSizePolicy(
                QSizePolicy::Expanding,
                QSizePolicy::Preferred);

            optBrowser->setMinimumHeight(40);

            if (renderer) {
                renderer->clearCache();

                optBrowser->setHtml(
                    renderer->render(
                        q.choices[i],
                        optBrowser->document()));
            }
            else {
                optBrowser->setText(q.choices[i]);
            }

            // =========================
            // 卡片布局
            // =========================
            cardLayout->addWidget(radio, 0, Qt::AlignCenter);

            cardLayout->addWidget(optBrowser, 1);

            // =========================
            // 卡片样式
            // =========================
            optionCard->setStyleSheet(R"(

            QWidget#OptionCard {

                background: white;

                border: 1px solid #e6e1f5;

                border-radius: 16px;
            }

            QWidget#OptionCard:hover {

                background: #f8f5ff;

                border: 1px solid #c8baf5;
            }

        )");

            // =========================
            // 点击整个卡片即可选中
            // =========================
            optionCard->installEventFilter(this);

            // 保存 index
            optionCard->setProperty("choiceIndex", i);

            answerLayout->addWidget(optionCard);

            // 恢复历史选择
            if (!q.userAnswer.isEmpty()
                && q.userAnswer.toInt() == i) {

                radio->setChecked(true);

                optionCard->setStyleSheet(R"(

                QWidget#OptionCard {
                    background: #f1ecff;
                    border: 2px solid #9b87f5;
                    border-radius: 16px;
                }

            )");
            }

            // 单选切换高亮
            connect(
                radio,
                &QRadioButton::toggled,
                this,
                [optionCard](bool checked) {

                    if (checked) {

                        optionCard->setStyleSheet(R"(

                        QWidget#OptionCard {

                            background: #f1ecff;

                            border: 2px solid #9b87f5;

                            border-radius: 16px;
                        }

                    )");

                    } else {

                        optionCard->setStyleSheet(R"(

                        QWidget#OptionCard {

                            background: white;

                            border: 1px solid #e6e1f5;

                            border-radius: 16px;
                        }

                        QWidget#OptionCard:hover {

                            background: #f8f5ff;

                            border: 1px solid #c8baf5;
                        }

                    )");
                    }
                });
        }
    }

    else if (q.type == QuestionType::Fill) {
        auto* lineEdit = new QLineEdit(answerWidget);
        lineEdit->setObjectName(QStringLiteral("fillAnswer"));
        lineEdit->setPlaceholderText(QStringLiteral(" 请在此输入最终数值答案..."));
        lineEdit->setText(q.userAnswer);
        lineEdit->setStyleSheet(
            "QLineEdit { height: 36px; font-size: 14px; border: 1px solid #cbd5e0; "
            "border-radius: 6px; padding-left: 8px; background: white; }"
            );
        answerLayout->addWidget(lineEdit);

    } else if (q.type == QuestionType::Subjective) {
        auto* textEdit = new QPlainTextEdit(answerWidget);
        textEdit->setObjectName(QStringLiteral("subjectiveAnswer"));
        textEdit->setPlaceholderText(QStringLiteral("请在此写下您的推导步骤与解答结论..."));
        textEdit->setMinimumHeight(110);
        textEdit->setPlainText(q.userAnswer);
        textEdit->setStyleSheet(
            "QPlainTextEdit { font-size: 13px; border: 1px solid #cbd5e0; "
            "border-radius: 6px; padding: 8px; background: white; line-height: 1.4; }"
            );
        answerLayout->addWidget(textEdit);

        // AI智能判卷按钮专属容器
        auto* btnContainer = new QHBoxLayout();
        auto* aiGradeBtn = new QPushButton(QStringLiteral("🤖 DeepSeek AI 智能判卷"), answerWidget);
        aiGradeBtn->setObjectName(QStringLiteral("InnerAiGradeButton"));
        aiGradeBtn->setCursor(Qt::PointingHandCursor);
        aiGradeBtn->setStyleSheet(
            "QPushButton { background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #4fa8f6, stop:1 #0078d4); "
            "color: white; border: none; font-size: 13px; font-weight: bold; padding: 8px 16px; border-radius: 6px; }"
            "QPushButton:hover { background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #60b4f8, stop:1 #1085e0); }"
            "QPushButton:disabled { background: #cbd5e0; color: #a0aec0; }"
            );

        connect(aiGradeBtn, &QPushButton::clicked, this, &ChapterPracticePage::onAiGradeSubjective);
        btnContainer->addWidget(aiGradeBtn);
        btnContainer->addStretch();
        answerLayout->addLayout(btnContainer);
    }
}

void ChapterPracticePage::onSubmitAnswer() {
    if (currentQuestionIndex < 0 || currentQuestionIndex >= chapterQuestions.size()) {
        return;
    }

    Question& q = chapterQuestions[currentQuestionIndex];
    q.attempts++;

    if (q.type == QuestionType::Single) {
        QList<QRadioButton*> buttons = answerWidget->findChildren<QRadioButton*>();
        int selectedIndex = -1;
        for (int i = 0; i < buttons.size(); ++i) {
            if (buttons[i]->isChecked()) {
                selectedIndex = i;
                break;
            }
        }

        if (selectedIndex == -1) {
            QMessageBox::warning(this, QStringLiteral("提示"), QStringLiteral("请先选择一个备选项。"));
            return;
        }

        q.userAnswer = QString::number(selectedIndex);
        q.isCorrect = (selectedIndex == q.correctAnswer.toInt());

        QString correctChar = QString(QChar('A' + q.correctAnswer.toInt()));
        displayResult(q.isCorrect, q.isCorrect ? QStringLiteral("🎉 恭喜你，回答正确！")
                                               : QStringLiteral("❌ 回答错误。正确答案是【%1】").arg(correctChar));

    } else if (q.type == QuestionType::Fill) {
        auto* lineEdit = answerWidget->findChild<QLineEdit*>(QStringLiteral("fillAnswer"));
        if (!lineEdit || lineEdit->text().trimmed().isEmpty()) {
            QMessageBox::warning(this, QStringLiteral("提示"), QStringLiteral("请输入数值答案"));
            return;
        }

        q.userAnswer = lineEdit->text().trimmed();
        bool ok1, ok2;
        double userValue = q.userAnswer.toDouble(&ok1);
        double correctValue = q.correctAnswer.toDouble(&ok2);

        if (ok1 && ok2) {
            q.isCorrect = (std::abs(userValue - correctValue) < 0.0001);
        } else {
            q.isCorrect = (q.userAnswer == q.correctAnswer);
        }

        displayResult(q.isCorrect, q.isCorrect ? QStringLiteral("🎉 答对了！计算能力非常出色！")
                                               : QStringLiteral("❌ 答案不正确。参考标准答案是: %1").arg(q.correctAnswer));

    } else if (q.type == QuestionType::Subjective) {
        auto* textEdit = answerWidget->findChild<QPlainTextEdit*>(QStringLiteral("subjectiveAnswer"));
        if (!textEdit || textEdit->toPlainText().trimmed().isEmpty()) {
            QMessageBox::warning(this, QStringLiteral("提示"), QStringLiteral("请输入您的解答内容"));
            return;
        }
        q.userAnswer = textEdit->toPlainText();
        QMessageBox::information(this, QStringLiteral("提示"),
                                 QStringLiteral("主观阐述题请直接点击输入框下方的 [🤖 DeepSeek AI 智能判卷] 按钮获取详细的分步点评。"));
    }

    // 【关键】：不在此处调用重置整个题目的 loadQuestion，而是就地独立更新顶部数据面板，保证反馈框能够完美留存显现
    progressLabel->setText(
        QStringLiteral(" 📝 当前进度：第 %1 题 / 共 %2 题   |   本题得分：%3分   |   尝试次数：%4 次")
            .arg(currentQuestionIndex + 1)
            .arg(chapterQuestions.size())
            .arg(q.score)
            .arg(q.attempts)
        );
}

void ChapterPracticePage::onAiGradeSubjective() {
    if (currentQuestionIndex < 0 || currentQuestionIndex >= chapterQuestions.size()) return;

    Question& q = chapterQuestions[currentQuestionIndex];
    auto* textEdit = answerWidget->findChild<QPlainTextEdit*>(QStringLiteral("subjectiveAnswer"));

    if (!textEdit || textEdit->toPlainText().trimmed().isEmpty()) {
        QMessageBox::warning(this, QStringLiteral("提示"), QStringLiteral("请先在文本框输入您的作答内容再进行判卷。"));
        return;
    }

    q.userAnswer = textEdit->toPlainText().trimmed();

    QString configPath = QCoreApplication::applicationDirPath() + "/algemate_ai.conf";
    QFile file(configPath);
    QString apiKey = "";

    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&file);
        QString encoded = in.readAll().trimmed();
        if (!encoded.isEmpty()) {
            apiKey = QString(QByteArray::fromBase64(encoded.toUtf8()));
        }
    }

    if (apiKey.isEmpty()) {
        QMessageBox::critical(this, QStringLiteral("未检测到 API Key"),
                              QStringLiteral("AI 智能判卷需复用您的解题模块凭据。\n请前往【AI智能解题】页面配置并保存您的 DeepSeek API Key 再返回重试。"));
        return;
    }

    displayResult(true, QStringLiteral("⏳ DeepSeek AI 导师正在仔细审阅您的解题步骤，请稍候..."));

    if (auto* btn = answerWidget->findChild<QPushButton*>(QStringLiteral("InnerAiGradeButton"))) {
        btn->setEnabled(false);
    }

    QString prompt = QString(
                         "你是一位严谨的高等代数教授。请根据【标准参考答案】对【学生作答】进行精确的打分和深度批改。\n\n"
                         "【题目内容】:\n%1\n\n"
                         "【标准参考答案】:\n%2\n\n"
                         "【学生作答内容】:\n%3\n\n"
                         "请务必严格按照以下要求进行规范响应：\n"
                         "1. 首行必须返回判卷结论，格式固定为：[结果：优秀/通过/不通过]\n"
                         "2. 随后请分段详细阐述：得分要点、逻辑断层或潜在失误诊断，并指出优化建议。\n"
                         "3. 讲解过程中涉及到的数学核心对象、矩阵表达式或维数公式，必须使用标准 LaTeX (行内 $...$, 块级 $$...$$) 格式进行包裹输出。"
                         ).arg(q.content, q.correctAnswer, q.userAnswer);

    QJsonObject rootObj;
    rootObj["model"] = "deepseek-chat";
    rootObj["stream"] = false;

    QJsonArray messages;
    QJsonObject systemMsg, userMsg;
    systemMsg["role"] = "system";
    systemMsg["content"] = QStringLiteral("你是专业的线性代数试卷批改导师，擅长以 Markdown 与高标准 LaTeX 公式写出极具学术价值的评语。");
    userMsg["role"] = "user";
    userMsg["content"] = prompt;
    messages.append(systemMsg);
    messages.append(userMsg);
    rootObj["messages"] = messages;

    QNetworkAccessManager* mgr = new QNetworkAccessManager(this);
    QNetworkRequest request{QUrl(QStringLiteral("https://api.deepseek.com/chat/completions"))};
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization", QString("Bearer %1").arg(apiKey).toUtf8());

    QNetworkReply* reply = mgr->post(request, QJsonDocument(rootObj).toJson());

    connect(reply, &QNetworkReply::finished, this, [this, reply, mgr, &q]() {
        mgr->deleteLater();
        reply->deleteLater();

        if (auto* btn = answerWidget->findChild<QPushButton*>(QStringLiteral("InnerAiGradeButton"))) {
            btn->setEnabled(true);
        }

        if (reply->error() != QNetworkReply::NoError) {
            displayResult(false, QStringLiteral("❌ 判卷通信失败：%1。请检查网络状态或服务可用性。").arg(reply->errorString()));
            return;
        }

        QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
        if (!doc.isObject()) {
            displayResult(false, QStringLiteral("❌ 无法正确处理 AI 服务器回传的数据结构。"));
            return;
        }

        QJsonObject responseObj = doc.object();
        QJsonArray choices = responseObj["choices"].toArray();
        if (choices.isEmpty()) {
            displayResult(false, QStringLiteral("❌ 接口未返回有效的判卷生成文本。"));
            return;
        }

        QString aiEvaluation = choices[0].toObject()["message"].toObject()["content"].toString();
        if (aiEvaluation.isEmpty()) {
            aiEvaluation = choices[0].toObject()["delta"].toObject()["content"].toString();
        }

        bool scorePass = !aiEvaluation.contains(QStringLiteral("结果：不通过"));
        q.isCorrect = scorePass;
        q.attempts++;

        displayResult(scorePass, QStringLiteral("### 🤖 DeepSeek 智能导师评阅报告\n---\n") + aiEvaluation);

        // 增量刷新顶部面板的统计次数
        progressLabel->setText(
            QStringLiteral(" 📝 当前进度：第 %1 题 / 共 %2 题   |   本题得分：%3分   |   尝试次数：%4 次")
                .arg(currentQuestionIndex + 1)
                .arg(chapterQuestions.size())
                .arg(q.score)
                .arg(q.attempts)
            );
    });
}

void ChapterPracticePage::displayResult(bool isCorrect, const QString& feedback) {
    feedbackLabel->show();

    auto* browser = reinterpret_cast<Latex::LatexTextBrowser*>(feedbackLabel);
    auto* renderer = static_cast<Latex::LatexRenderer*>(this->property("latex_renderer").value<void*>());

    if (renderer && browser) {
        renderer->clearCache();
        QString renderedHtml = renderer->render(feedback, browser->document());
        browser->setHtml(renderedHtml);
        browser->document()->adjustSize();

        int h = static_cast<int>(browser->document()->size().height());

        browser->setMinimumHeight(h + 30);
        browser->setMaximumHeight(h + 40);
    } else {
        browser->setHtml(feedback);
    }

    if (isCorrect) {
        feedbackLabel->setStyleSheet(
            "QTextBrowser#AdvancedFeedbackView { background-color: #f0fdf4; color: #166534; padding: 14px; "
            "border-radius: 8px; border: 1px solid #bbf7d0; font-size: 13px; margin-top: 8px; line-height: 1.5; }"
            );
    } else {
        feedbackLabel->setStyleSheet(
            "QTextBrowser#AdvancedFeedbackView { background-color: #fef2f2; color: #991b1b; padding: 14px; "
            "border-radius: 8px; border: 1px solid #fecaca; font-size: 13px; margin-top: 8px; line-height: 1.5; }"
            );
    }
}

void ChapterPracticePage::onNextQuestion() {
    if (currentQuestionIndex < chapterQuestions.size() - 1) {
        loadQuestion(currentQuestionIndex + 1);
    } else {
        QMessageBox::information(this, QStringLiteral("提示"), QStringLiteral("已到达本章最后一题！"));
    }
}

void ChapterPracticePage::onPreviousQuestion() {
    if (currentQuestionIndex > 0) {
        loadQuestion(currentQuestionIndex - 1);
    } else {
        QMessageBox::information(this, QStringLiteral("提示"), QStringLiteral("已经是第一道题目。"));
    }
}

bool ChapterPracticePage::eventFilter(QObject* watched, QEvent* event)
{
    if (event->type() == QEvent::MouseButtonPress) {

        QWidget* widget = qobject_cast<QWidget*>(watched);

        if (widget &&
            widget->property("choiceIndex").isValid()) {

            int index =
                widget->property("choiceIndex").toInt();

            auto* radio =
                widget->findChild<QRadioButton*>(
                    QStringLiteral("choice_%1").arg(index));

            if (radio) {
                radio->setChecked(true);
            }

            return true;
        }
    }

    return QWidget::eventFilter(watched, event);
}

// ==================== TopicPracticePage ====================

TopicPracticePage::TopicPracticePage(QWidget* parent) : QWidget(parent)
{
    auto* lay = new QVBoxLayout(this);
    lay->setContentsMargins(24, 16, 24, 24);
    lay->setSpacing(14);

    auto* top = new QHBoxLayout;
    auto* back = makeBackBtn(this);
    connect(back, &QPushButton::clicked, this, &TopicPracticePage::backRequested);
    auto* t = new QLabel(QStringLiteral("专题练习"));
    t->setObjectName(QStringLiteral("PageTitle"));
    top->addWidget(back);
    top->addWidget(t);
    top->addStretch();

    auto* ph = new QLabel(QStringLiteral("（专题练习功能开发中...）"));
    ph->setAlignment(Qt::AlignCenter);
    ph->setObjectName(QStringLiteral("PlaceholderLabel"));

    lay->addLayout(top);
    lay->addWidget(ph, 1);
}

} // namespace AlgeMate::Learning