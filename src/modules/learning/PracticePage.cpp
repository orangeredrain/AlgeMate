#include "PracticePage.h"
#include "ClickableCard.h"

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
#include <QInputDialog>
#include <cmath>

namespace AlgeMate::Learning {

// ==================== 辅助函数 ====================

static QPushButton* makeBackBtn(QWidget* parent = nullptr) {
    auto* btn = new QPushButton(QStringLiteral("← 返回"), parent);
    btn->setObjectName(QStringLiteral("LearnBackBtn"));
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
    d->setStyleSheet("font-size:13px; background:transparent;");
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
    auto* lay = new QVBoxLayout(this);
    lay->setContentsMargins(24, 16, 24, 24);
    lay->setSpacing(14);

    // 顶部导航
    auto* top = new QHBoxLayout;
    auto* back = makeBackBtn(this);
    connect(back, &QPushButton::clicked, this, &ChapterPracticePage::backRequested);
    auto* t = new QLabel(QStringLiteral("对应章节练习"));
    t->setObjectName(QStringLiteral("PageTitle"));
    top->addWidget(back);
    top->addWidget(t);
    top->addStretch();

    // 进度标签
    progressLabel = new QLabel(this);
    progressLabel->setStyleSheet("color: #3498db; font-size: 12px;");

    // 题目显示区域
    questionLabel = new QLabel(this);
    questionLabel->setWordWrap(true);
    questionLabel->setStyleSheet("font-size: 13px; line-height: 1.6; padding: 15px;");

    auto* scrollArea = new QScrollArea(this);
    scrollArea->setWidget(questionLabel);
    scrollArea->setWidgetResizable(true);

    // 答案区域（动态创建）
    answerWidget = new QWidget(this);
    auto* answerLayout = new QVBoxLayout(answerWidget);
    answerLayout->setContentsMargins(0, 0, 0, 0);

    // 反馈区域
    feedbackLabel = new QLabel(this);
    feedbackLabel->setWordWrap(true);
    feedbackLabel->setStyleSheet("padding: 10px; border-radius: 4px; display: none;");

    // 底部按钮
    auto* bottomLayout = new QHBoxLayout;
    auto* prevBtn = new QPushButton(QStringLiteral("上一题"), this);
    auto* submitBtn = new QPushButton(QStringLiteral("提交答案"), this);
    auto* nextBtn = new QPushButton(QStringLiteral("下一题"), this);

    connect(prevBtn, &QPushButton::clicked, this, &ChapterPracticePage::onPreviousQuestion);
    connect(submitBtn, &QPushButton::clicked, this, &ChapterPracticePage::onSubmitAnswer);
    connect(nextBtn, &QPushButton::clicked, this, &ChapterPracticePage::onNextQuestion);

    bottomLayout->addWidget(prevBtn);
    bottomLayout->addStretch();
    bottomLayout->addWidget(submitBtn);
    bottomLayout->addWidget(nextBtn);

    lay->addLayout(top);
    lay->addWidget(progressLabel);
    lay->addWidget(scrollArea, 1);
    lay->addWidget(answerWidget);
    lay->addWidget(feedbackLabel);
    lay->addLayout(bottomLayout);

    // 加载章节题目
    onLoadChapterQuestions();

    // 加载第一题
    if (!chapterQuestions.isEmpty()) {
        loadQuestion(0);
    }
}

void ChapterPracticePage::onLoadChapterQuestions() {
    // 示例：从MD文件或配置读取章节题目
    // 实际应用中可以从外部文件读取

    Question q1;
    q1.id = 1;
    q1.contentPath = "chapter1/q1.md";
    q1.content = "计算以下矩阵的行列式：\n"
                 "A = [[2, 1], [3, 4]]";
    q1.type = QuestionType::Fill;
    q1.score = 5;
    q1.correctAnswer = "5";  // 2*4 - 1*3 = 8 - 3 = 5
    q1.attempts = 0;
    q1.isCorrect = false;
    chapterQuestions.append(q1);

    Question q2;
    q2.id = 2;
    q2.contentPath = "chapter1/q2.md";
    q2.content = "矩阵转置的性质是什么？";
    q2.type = QuestionType::Single;
    q2.score = 5;
    q2.correctAnswer = "2";
    q2.choices = {
        QStringLiteral("(A^T)^T = -A"),
        QStringLiteral("(AB)^T = B^T·A^T"),
        QStringLiteral("(A+B)^T = B^T + A^T"),
        QStringLiteral("A^T = A (对所有矩阵)")
    };
    q2.attempts = 0;
    q2.isCorrect = false;
    chapterQuestions.append(q2);

    Question q3;
    q3.id = 3;
    q3.contentPath = "chapter1/q3.md";
    q3.content = "简述矩阵的秩的定义和几何意义。";
    q3.type = QuestionType::Subjective;
    q3.score = 10;
    q3.correctAnswer = "标准答案：矩阵的秩是其行向量或列向量中线性无关的最大个数。"
                       "几何意义是该矩阵所代表的线性变换的像空间的维数。";
    q3.attempts = 0;
    q3.isCorrect = false;
    chapterQuestions.append(q3);
}

void ChapterPracticePage::loadQuestion(int index) {
    if (index < 0 || index >= chapterQuestions.size()) {
        return;
    }

    currentQuestionIndex = index;
    const Question& q = chapterQuestions[index];

    // 更新进度标签
    progressLabel->setText(
        QStringLiteral("第 %1 题 / 共 %2 题  |  做题次数: %3")
            .arg(index + 1)
            .arg(chapterQuestions.size())
            .arg(q.attempts)
        );

    // 更新题目内容
    questionLabel->setText(q.content);

    // 清空反馈
    feedbackLabel->hide();

    updateUIForQuestion(q);
}

void ChapterPracticePage::updateUIForQuestion(const Question& q) {
    // 清空旧的答案区域
    if (auto* oldLayout = answerWidget->layout()) {
        while (QLayoutItem* item = oldLayout->takeAt(0)) {
            delete item->widget();
        }
    } else {
        auto* newLayout = new QVBoxLayout(answerWidget);
        answerWidget->setLayout(newLayout);
    }

    QVBoxLayout* answerLayout = qobject_cast<QVBoxLayout*>(answerWidget->layout());
    if (!answerLayout) return;
    answerLayout->setContentsMargins(0, 10, 0, 10);
    answerLayout->setSpacing(10);

    // 根据题目类型创建不同的答案控件
    if (q.type == QuestionType::Single) {
        // 单选题：显示单选按钮
        auto* buttonGroup = new QButtonGroup(answerWidget);
        for (int i = 0; i < q.choices.size(); ++i) {
            auto* radio = new QRadioButton(q.choices[i], answerWidget);
            radio->setObjectName(QStringLiteral("choice_%1").arg(i));
            buttonGroup->addButton(radio, i);
            answerLayout->addWidget(radio);

            // 恢复之前的选择
            if (!q.userAnswer.isEmpty() && q.userAnswer.toInt() == i) {
                radio->setChecked(true);
            }
        }

    } else if (q.type == QuestionType::Fill) {
        // 填空题：输入框
        auto* lineEdit = new QLineEdit(answerWidget);
        lineEdit->setObjectName(QStringLiteral("fillAnswer"));
        lineEdit->setPlaceholderText(QStringLiteral("请输入数字答案"));
        lineEdit->setText(q.userAnswer);
        answerLayout->addWidget(lineEdit);

    } else if (q.type == QuestionType::Subjective) {
        // 解答题：文本编辑框
        auto* textEdit = new QPlainTextEdit(answerWidget);
        textEdit->setObjectName(QStringLiteral("subjectiveAnswer"));
        textEdit->setPlaceholderText(QStringLiteral("请输入您的解答"));
        textEdit->setMinimumHeight(120);
        textEdit->setPlainText(q.userAnswer);
        answerLayout->addWidget(textEdit);

        // 添加AI判卷按钮
        auto* aiGradeBtn = new QPushButton(QStringLiteral("AI判卷"), answerWidget);
        aiGradeBtn->setMaximumWidth(120);
        connect(aiGradeBtn, &QPushButton::clicked, this, &ChapterPracticePage::onAiGradeSubjective);
        answerLayout->addWidget(aiGradeBtn);
    }

    answerLayout->addStretch();
}

void ChapterPracticePage::onSubmitAnswer() {
    if (currentQuestionIndex < 0 || currentQuestionIndex >= chapterQuestions.size()) {
        return;
    }

    Question& q = chapterQuestions[currentQuestionIndex];

    if (q.type == QuestionType::Single) {
        // 单选题判卷
        QList<QRadioButton*> buttons = answerWidget->findChildren<QRadioButton*>();
        int selectedIndex = -1;
        for (int i = 0; i < buttons.size(); ++i) {
            if (buttons[i]->isChecked()) {
                selectedIndex = i;
                break;
            }
        }

        if (selectedIndex == -1) {
            QMessageBox::warning(this, QStringLiteral("提示"), QStringLiteral("请选择一个选项"));
            return;
        }

        q.userAnswer = QString::number(selectedIndex);
        q.isCorrect = (selectedIndex == q.correctAnswer.toInt());
        displayResult(q.isCorrect, q.isCorrect ? QStringLiteral("✓ 正确！")
                                               : QStringLiteral("✗ 错误。正确答案是选项 %1").arg(q.correctAnswer));

    } else if (q.type == QuestionType::Fill) {
        // 填空题判卷
        auto* lineEdit = answerWidget->findChild<QLineEdit*>(QStringLiteral("fillAnswer"));
        if (!lineEdit || lineEdit->text().isEmpty()) {
            QMessageBox::warning(this, QStringLiteral("提示"), QStringLiteral("请输入答案"));
            return;
        }

        q.userAnswer = lineEdit->text();
        bool ok;
        double userValue = lineEdit->text().toDouble(&ok);
        double correctValue = q.correctAnswer.toDouble(&ok);

        // 允许小数误差 (0.0001)
        q.isCorrect = (std::abs(userValue - correctValue) < 0.0001);
        displayResult(q.isCorrect, q.isCorrect ? QStringLiteral("✓ 正确！")
                                               : QStringLiteral("✗ 错误。正确答案是 %1").arg(q.correctAnswer));

    } else if (q.type == QuestionType::Subjective) {
        // 解答题需要AI判卷
        auto* textEdit = answerWidget->findChild<QPlainTextEdit*>(QStringLiteral("subjectiveAnswer"));
        if (!textEdit || textEdit->toPlainText().isEmpty()) {
            QMessageBox::warning(this, QStringLiteral("提示"), QStringLiteral("请输入解答"));
            return;
        }

        q.userAnswer = textEdit->toPlainText();
        QMessageBox::information(this, QStringLiteral("提示"),
                                 QStringLiteral("请点击 AI判卷 按钮让AI评阅您的答案"));
    }

    q.attempts++;
}

void ChapterPracticePage::onAiGradeSubjective() {
    // 模仿 AiSolverPage 实现 AI 判卷
    if (currentQuestionIndex < 0 || currentQuestionIndex >= chapterQuestions.size()) {
        return;
    }

    Question& q = chapterQuestions[currentQuestionIndex];
    auto* textEdit = answerWidget->findChild<QPlainTextEdit*>(QStringLiteral("subjectiveAnswer"));

    if (!textEdit || textEdit->toPlainText().isEmpty()) {
        QMessageBox::warning(this, QStringLiteral("提示"), QStringLiteral("请先输入解答"));
        return;
    }

    // 弹窗输入 API Key 和选择模型
    bool ok;
    QString apiKey = QInputDialog::getText(this, QStringLiteral("AI判卷"),
                                           QStringLiteral("请输入 OpenAI API Key:"),
                                           QLineEdit::Password, "", &ok);
    if (!ok || apiKey.isEmpty()) {
        return;
    }

    // 这里应该调用实际的 API 进行判卷
    // 示例代码：使用 QNetworkAccessManager 发送请求到 OpenAI API
    // 注意：实际应用中应该异步处理，避免阻塞UI

    // 伪代码示例：
    /*
    QNetworkRequest request(QUrl("https://api.openai.com/v1/chat/completions"));
    request.setHeader(QNetworkRequestHeader::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization", QString("Bearer %1").arg(apiKey).toUtf8());

    QString prompt = QString(
        "请根据以下标准答案评阅学生的解答。\n"
        "题目：%1\n"
        "标准答案：%2\n"
        "学生答案：%3\n"
        "请给出：1) 是否正确(是/否) 2) 评分(0-100) 3) 反馈意见"
    ).arg(q.content, q.correctAnswer, textEdit->toPlainText());

    QJsonObject json;
    json["model"] = "gpt-3.5-turbo";
    json["messages"] = QJsonArray({
        QJsonObject{{"role", "user"}, {"content", prompt}}
    });

    QByteArray data = QJsonDocument(json).toJson(QJsonDocument::Compact);

    // 发送请求...
    */

    // 这里使用模拟的 AI 反馈
    bool isCorrect = textEdit->toPlainText().contains(QStringLiteral("秩")) ||
                     textEdit->toPlainText().contains(QStringLiteral("线性无关"));

    q.isCorrect = isCorrect;
    QString feedback = isCorrect ? QStringLiteral("✓ AI 评阅: 答案基本正确")
                                 : QStringLiteral("✗ AI 评阅: 答案不完整或有误。\n"
                                                  "提示：应该提到秩的定义、线性无关性和几何意义。");

    displayResult(isCorrect, feedback);
}

void ChapterPracticePage::displayResult(bool isCorrect, const QString& feedback) {
    // 显示反馈信息
    feedbackLabel->show();
    if (isCorrect) {
        feedbackLabel->setStyleSheet(
            "background-color: #d4edda; color: #155724; padding: 10px; border-radius: 4px; "
            "border: 1px solid #c3e6cb;"
            );
    } else {
        feedbackLabel->setStyleSheet(
            "background-color: #f8d7da; color: #721c24; padding: 10px; border-radius: 4px; "
            "border: 1px solid #f5c6cb;"
            );
    }
    feedbackLabel->setText(feedback);
}

void ChapterPracticePage::onNextQuestion() {
    if (currentQuestionIndex < chapterQuestions.size() - 1) {
        loadQuestion(currentQuestionIndex + 1);
    } else {
        QMessageBox::information(this, QStringLiteral("提示"), QStringLiteral("已是最后一题"));
    }
}

void ChapterPracticePage::onPreviousQuestion() {
    if (currentQuestionIndex > 0) {
        loadQuestion(currentQuestionIndex - 1);
    } else {
        QMessageBox::information(this, QStringLiteral("提示"), QStringLiteral("已是第一题"));
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