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
    // 初始化 LaTeX 渲染内核并配置数学宏
    auto* renderer = new Latex::LatexRenderer;
    renderer->addMathMacro(QStringLiteral("F"),  QStringLiteral("\\mathbb{F}"));
    renderer->addMathMacro(QStringLiteral("R"),  QStringLiteral("\\mathbb{R}"));
    renderer->addMathMacro(QStringLiteral("C"),  QStringLiteral("\\mathbb{C}"));
    renderer->addMathMacro(QStringLiteral("Q"),  QStringLiteral("\\mathbb{Q}"));
    renderer->addMathMacro(QStringLiteral("Z"),  QStringLiteral("\\mathbb{Z}"));
    renderer->addMathMacro(QStringLiteral("N"),  QStringLiteral("\\mathbb{N}"));
    this->setProperty("latex_renderer", QVariant::fromValue(static_cast<void*>(renderer)));

    auto* mainLay = new QVBoxLayout(this);
    mainLay->setContentsMargins(24, 16, 24, 24);
    mainLay->setSpacing(12);

    // 1. 固定顶栏：导航
    auto* top = new QHBoxLayout;
    auto* back = makeBackBtn(this);
    connect(back, &QPushButton::clicked, this, &ChapterPracticePage::backRequested);
    auto* t = new QLabel(QStringLiteral("对应章节练习"));
    t->setObjectName(QStringLiteral("PageTitle"));
    t->setStyleSheet("font-size: 20px; font-weight: bold; color: #2c3e50;");
    top->addWidget(back);
    top->addWidget(t);
    top->addStretch();
    mainLay->addLayout(top);

    // 2. 核心改动：引入全局滚动容器，包装所有答题核心组件
    auto* mainScroll = new QScrollArea(this);
    mainScroll->setWidgetResizable(true);
    mainScroll->setFrameShape(QFrame::NoFrame);
    mainScroll->setStyleSheet("QScrollArea { background-color: transparent; }");

    auto* scrollContainer = new QWidget(this);
    scrollContainer->setStyleSheet("background-color: transparent;");
    auto* scrollLay = new QVBoxLayout(scrollContainer);
    scrollLay->setContentsMargins(0, 4, 0, 4);
    scrollLay->setSpacing(16);

    // 进度控制面板
    progressLabel = new QLabel(scrollContainer);
    progressLabel->setStyleSheet(
        "color: #4a5568; font-size: 13px; background-color: #edf2f7; "
        "padding: 8px 12px; border-radius: 6px; font-weight: 500;"
        );
    scrollLay->addWidget(progressLabel);

    // 题目显示（升级为只读的 LatexTextBrowser，直接支持题目内 LaTeX 渲染！）
    auto* qBrowser = new Latex::LatexTextBrowser(scrollContainer);
    qBrowser->setObjectName(QStringLiteral("QuestionTextBrowser"));
    qBrowser->setFrameShape(QFrame::NoFrame);
    qBrowser->setOpenExternalLinks(true);
    qBrowser->setMinimumHeight(100);
    qBrowser->setStyleSheet("background-color: #ffffff; border: 1px solid #e2e8f0; border-radius: 8px; padding: 12px;");
    // 将原指针强转存储以便后面直接刷新使用
    questionLabel = reinterpret_cast<QLabel*>(qBrowser);
    scrollLay->addWidget(qBrowser, 0);

    // 动态交互答案区域卡片
    answerWidget = new QWidget(scrollContainer);
    auto* answerLayout = new QVBoxLayout(answerWidget);
    answerLayout->setContentsMargins(0, 0, 0, 0);
    scrollLay->addWidget(answerWidget, 0);

    // 升级为专业的 LatexTextBrowser
    auto* advancedFeedback = new Latex::LatexTextBrowser(scrollContainer);
    advancedFeedback->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    advancedFeedback->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    advancedFeedback->setObjectName(QStringLiteral("AdvancedFeedbackView"));
    advancedFeedback->setOpenExternalLinks(true);
    advancedFeedback->setMinimumHeight(160);
    advancedFeedback->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    advancedFeedback->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    // 关键修正：让反馈框自适应内容高度，拒绝内部二次憋死产生独立滚动条！
    advancedFeedback->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
    advancedFeedback->hide();
    feedbackLabel = reinterpret_cast<QLabel*>(advancedFeedback);
    scrollLay->addWidget(advancedFeedback, 0);

    scrollLay->addStretch(1); // 撑开内部
    mainScroll->setWidget(scrollContainer);
    mainLay->addWidget(mainScroll, 1);

    // 3. 固定底栏：操作操作区
    auto* bottomLayout = new QHBoxLayout;
    auto* prevBtn = new QPushButton(QStringLiteral("← 上一题"), this);
    auto* submitBtn = new QPushButton(QStringLiteral("确认提交"), this);
    auto* nextBtn = new QPushButton(QStringLiteral("下一题 →"), this);

    QString baseBtnStyle = "QPushButton { padding: 8px 16px; font-size: 13px; border-radius: 6px; font-weight: 500; }";
    prevBtn->setStyleSheet(baseBtnStyle + "QPushButton { background-color: #ffffff; border: 1px solid #cbd5e0; color: #4a5568; } QPushButton:hover { background-color: #f7fafc; }");
    nextBtn->setStyleSheet(baseBtnStyle + "QPushButton { background-color: #ffffff; border: 1px solid #cbd5e0; color: #4a5568; } QPushButton:hover { background-color: #f7fafc; }");
    submitBtn->setStyleSheet(baseBtnStyle + "QPushButton { background-color: #3182ce; border: none; color: white; padding: 10px 24px; font-size: 14px; } QPushButton:hover { background-color: #2b6cb0; }");

    connect(prevBtn, &QPushButton::clicked, this, &ChapterPracticePage::onPreviousQuestion);
    connect(submitBtn, &QPushButton::clicked, this, &ChapterPracticePage::onSubmitAnswer);
    connect(nextBtn, &QPushButton::clicked, this, &ChapterPracticePage::onNextQuestion);

    bottomLayout->addWidget(prevBtn);
    bottomLayout->addStretch();
    bottomLayout->addWidget(submitBtn);
    bottomLayout->addWidget(nextBtn);
    mainLay->addLayout(bottomLayout);

    onLoadChapterQuestions();

    if (!chapterQuestions.isEmpty()) {
        loadQuestion(0);
    }
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
        for (int i = 0; i < q.choices.size(); ++i) {
            auto* radio = new QRadioButton(q.choices[i], answerWidget);
            radio->setObjectName(QStringLiteral("choice_%1").arg(i));
            radio->setStyleSheet("QRadioButton { font-size: 13px; color: #4a5568; padding: 4px; }");
            buttonGroup->addButton(radio, i);
            answerLayout->addWidget(radio);

            if (!q.userAnswer.isEmpty() && q.userAnswer.toInt() == i) {
                radio->setChecked(true);
            }
        }

    } else if (q.type == QuestionType::Fill) {
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

    // 【关键修复点】：不在此处调用重置整个题目的 loadQuestion，而是就地独立更新顶部数据面板，保证反馈框能够完美留存显现
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

    displayResult(true, QStringLiteral("⏳ DeepSeek AI 导师正在仔细审阅您的解题步骤，数学公式解析中，请稍候..."));

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