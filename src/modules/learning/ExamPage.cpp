#include "ExamPage.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QSpinBox>
#include <QTimer>
#include <QRadioButton>
#include <QButtonGroup>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QFile>
#include <QScrollArea>
#include <QProgressBar>
#include <QMessageBox>
#include <cmath>

namespace AlgeMate::Learning {

// ==================== ExamSettingPage ====================

ExamSettingPage::ExamSettingPage(QWidget* parent) : QWidget(parent) {
    auto* lay = new QVBoxLayout(this);
    lay->setContentsMargins(24, 16, 24, 24);
    lay->setSpacing(20);

    auto* top = new QHBoxLayout;
    auto* back = new QPushButton(QStringLiteral("← 返回"), this);
    back->setObjectName(QStringLiteral("LearnBackBtn"));
    connect(back, &QPushButton::clicked, this, &ExamSettingPage::backRequested);

    auto* title = new QLabel(QStringLiteral("考试模式"));
    title->setObjectName(QStringLiteral("PageTitle"));
    top->addWidget(back);
    top->addWidget(title);
    top->addStretch();

    // 考试设置区域
    auto* settingLayout = new QVBoxLayout;
    settingLayout->setSpacing(15);
    settingLayout->setContentsMargins(20, 20, 20, 20);

    auto* timeLabel = new QLabel(QStringLiteral("考试时间（分钟）："));
    timeSpinBox = new QSpinBox(this);
    timeSpinBox->setMinimum(5);
    timeSpinBox->setMaximum(300);
    timeSpinBox->setValue(60);
    timeSpinBox->setSuffix(QStringLiteral(" 分钟"));

    auto* timeLayout = new QHBoxLayout;
    timeLayout->addWidget(timeLabel);
    timeLayout->addWidget(timeSpinBox);
    timeLayout->addStretch();

    auto* infoLabel = new QLabel(QStringLiteral(
        "• 选择考试时间后点击开始考试\n"
        "• 考试过程中可以随时提前交卷\n"
        "• 交卷后才能查看分数和答案\n"
        "• 支持单选题、填空题、解答题"
        ));
    infoLabel->setStyleSheet("color: #666; font-size: 12px;");

    auto* startBtn = new QPushButton(QStringLiteral("开始考试"), this);
    startBtn->setMinimumHeight(40);
    connect(startBtn, &QPushButton::clicked, this, [this]() {
        emit examStarted(timeSpinBox->value());
    });

    settingLayout->addLayout(timeLayout);
    settingLayout->addStretch();
    settingLayout->addWidget(infoLabel);
    settingLayout->addWidget(startBtn);

    lay->addLayout(top);
    lay->addLayout(settingLayout, 1);
}

// ==================== ExamProgressPage ====================

ExamProgressPage::ExamProgressPage(const QVector<Question>& questions, int timeMinutes, QWidget* parent)
    : QWidget(parent), questions(questions), currentQuestionIndex(0), remainingSeconds(timeMinutes * 60) {

    auto* lay = new QVBoxLayout(this);
    lay->setContentsMargins(24, 16, 24, 24);
    lay->setSpacing(14);

    // 顶部：计时器和进度
    auto* top = new QHBoxLayout;
    auto* back = new QPushButton(QStringLiteral("← 返回"), this);
    back->setObjectName(QStringLiteral("LearnBackBtn"));
    connect(back, &QPushButton::clicked, this, &ExamProgressPage::backRequested);

    timerLabel = new QLabel(QStringLiteral("剩余时间: 60:00"), this);
    timerLabel->setStyleSheet("font-weight: bold; color: #e74c3c; font-size: 14px;");

    top->addWidget(back);
    top->addStretch();
    top->addWidget(timerLabel);

    // 题目信息
    auto* infoLayout = new QHBoxLayout;
    scoreLabel = new QLabel(this);
    auto* totalLabel = new QLabel(
        QStringLiteral("共 %1 题").arg(questions.size()), this);
    infoLayout->addWidget(scoreLabel);
    infoLayout->addStretch();
    infoLayout->addWidget(totalLabel);

    // 题目内容
    questionLabel = new QLabel(this);
    questionLabel->setWordWrap(true);
    questionLabel->setStyleSheet("font-size: 13px; line-height: 1.6;");

    auto* scrollArea = new QScrollArea(this);
    scrollArea->setWidget(questionLabel);
    scrollArea->setWidgetResizable(true);

    // 答案区域（动态创建）
    answerWidget = new QWidget(this);

    // 底部按钮
    auto* bottomLayout = new QHBoxLayout;
    auto* prevBtn = new QPushButton(QStringLiteral("上一题"), this);
    auto* submitBtn = new QPushButton(QStringLiteral("提交答案"), this);
    auto* nextBtn = new QPushButton(QStringLiteral("下一题"), this);
    auto* examSubmitBtn = new QPushButton(QStringLiteral("交卷"), this);
    examSubmitBtn->setStyleSheet("background-color: #e74c3c; color: white;");

    connect(prevBtn, &QPushButton::clicked, this, &ExamProgressPage::onPreviousQuestion);
    connect(submitBtn, &QPushButton::clicked, this, &ExamProgressPage::onSubmitAnswer);
    connect(nextBtn, &QPushButton::clicked, this, &ExamProgressPage::onNextQuestion);
    connect(examSubmitBtn, &QPushButton::clicked, this, &ExamProgressPage::onSubmitExam);

    bottomLayout->addWidget(prevBtn);
    bottomLayout->addStretch();
    bottomLayout->addWidget(submitBtn);
    bottomLayout->addWidget(nextBtn);
    bottomLayout->addStretch();
    bottomLayout->addWidget(examSubmitBtn);

    lay->addLayout(top);
    lay->addLayout(infoLayout);
    lay->addWidget(scrollArea);
    lay->addWidget(answerWidget);
    lay->addLayout(bottomLayout);

    // 设置计时器
    auto* timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &ExamProgressPage::onTimeUpdate);
    timer->start(1000);

    loadQuestion(0);
}

void ExamProgressPage::loadQuestion(int index) {
    if (index < 0 || index >= questions.size()) {
        return;
    }

    currentQuestionIndex = index;
    const Question& q = questions[index];

    // 更新分数标签
    int totalScore = 0;
    int earnedScore = 0;
    for (const auto& question : questions) {
        totalScore += question.score;
        if (question.isCorrect) {
            earnedScore += question.score;
        }
    }
    scoreLabel->setText(
        QStringLiteral("第 %1 题 | 当前得分: %2/%3")
            .arg(index + 1)
            .arg(earnedScore)
            .arg(totalScore)
        );

    updateUIForQuestion(q);
}

void ExamProgressPage::updateUIForQuestion(const Question& q) {
    // 设置题目内容
    questionLabel->setText(q.content);

    // 清空旧的答案区域
    if (answerWidget->layout()) {
        QLayout* oldLayout = answerWidget->layout();
        while (QLayoutItem* item = oldLayout->takeAt(0)) {
            delete item->widget();
        }
        delete oldLayout;
    }

    auto* answerLayout = new QVBoxLayout(answerWidget);
    answerLayout->setContentsMargins(0, 10, 0, 10);
    answerLayout->setSpacing(10);

    // 根据题目类型创建不同的答案控件
    if (q.type == QuestionType::Single) {
        // 单选题：显示选项
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
    }

    answerLayout->addStretch();
}

void ExamProgressPage::onSubmitAnswer() {
    if (currentQuestionIndex < 0 || currentQuestionIndex >= questions.size()) {
        return;
    }

    Question& q = questions[currentQuestionIndex];

    if (q.type == QuestionType::Single) {
        // 检查是否有选择
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

    } else if (q.type == QuestionType::Fill) {
        auto* lineEdit = answerWidget->findChild<QLineEdit*>(QStringLiteral("fillAnswer"));
        if (!lineEdit || lineEdit->text().isEmpty()) {
            QMessageBox::warning(this, QStringLiteral("提示"), QStringLiteral("请输入答案"));
            return;
        }

        q.userAnswer = lineEdit->text();
        bool ok;
        double userValue = lineEdit->text().toDouble(&ok);
        double correctValue = q.correctAnswer.toDouble(&ok);

        // 允许小数误差
        q.isCorrect = (std::abs(userValue - correctValue) < 0.0001);

    } else if (q.type == QuestionType::Subjective) {
        auto* textEdit = answerWidget->findChild<QPlainTextEdit*>(QStringLiteral("subjectiveAnswer"));
        if (!textEdit || textEdit->toPlainText().isEmpty()) {
            QMessageBox::warning(this, QStringLiteral("提示"), QStringLiteral("请输入解答"));
            return;
        }

        q.userAnswer = textEdit->toPlainText();
        // 解答题需要通过AI判卷（这里先标记为待判卷）
        q.isCorrect = false;  // 需要AI判卷
    }

    q.attempts++;
    QMessageBox::information(this, QStringLiteral("提示"), QStringLiteral("答案已提交"));
}

void ExamProgressPage::onNextQuestion() {
    if (currentQuestionIndex < questions.size() - 1) {
        loadQuestion(currentQuestionIndex + 1);
    } else {
        QMessageBox::information(this, QStringLiteral("提示"), QStringLiteral("已是最后一题"));
    }
}

void ExamProgressPage::onPreviousQuestion() {
    if (currentQuestionIndex > 0) {
        loadQuestion(currentQuestionIndex - 1);
    } else {
        QMessageBox::information(this, QStringLiteral("提示"), QStringLiteral("已是第一题"));
    }
}

void ExamProgressPage::onSubmitExam() {
    auto* msgBox = new QMessageBox(this);
    msgBox->setText(QStringLiteral("确定要交卷吗？交卷后将无法修改答案。"));
    msgBox->setStandardButtons(QMessageBox::Yes | QMessageBox::No);

    if (msgBox->exec() == QMessageBox::Yes) {
        emit examFinished(questions);
    }
}

void ExamProgressPage::onTimeUpdate() {
    remainingSeconds--;

    int minutes = remainingSeconds / 60;
    int seconds = remainingSeconds % 60;
    timerLabel->setText(
        QStringLiteral("剩余时间: %1:%2")
            .arg(minutes, 2, 10, QChar('0'))
            .arg(seconds, 2, 10, QChar('0'))
        );

    if (remainingSeconds <= 0) {
        QMessageBox::information(this, QStringLiteral("提示"), QStringLiteral("考试时间已到，自动交卷"));
        emit examFinished(questions);
    }
}

// ==================== ExamResultPage ====================

ExamResultPage::ExamResultPage(const QVector<Question>& results, QWidget* parent)
    : QWidget(parent) {

    auto* lay = new QVBoxLayout(this);
    lay->setContentsMargins(24, 16, 24, 24);
    lay->setSpacing(14);

    auto* top = new QHBoxLayout;
    auto* back = new QPushButton(QStringLiteral("← 返回"), this);
    back->setObjectName(QStringLiteral("LearnBackBtn"));
    connect(back, &QPushButton::clicked, this, &ExamResultPage::backRequested);

    auto* title = new QLabel(QStringLiteral("考试结果"));
    title->setObjectName(QStringLiteral("PageTitle"));
    top->addWidget(back);
    top->addWidget(title);
    top->addStretch();

    lay->addLayout(top);

    // 计算总分和得分
    int totalScore = 0;
    int earnedScore = 0;
    for (const auto& q : results) {
        totalScore += q.score;
        if (q.isCorrect) {
            earnedScore += q.score;
        }
    }

    // 显示总体成绩
    auto* scoreLayout = new QVBoxLayout;
    auto* scoreLabel = new QLabel(
        QStringLiteral("总分：%1/%2").arg(earnedScore).arg(totalScore), this);
    scoreLabel->setStyleSheet("font-size: 24px; font-weight: bold; color: #2ecc71;");
    scoreLabel->setAlignment(Qt::AlignCenter);

    double percentage = (totalScore > 0) ? (earnedScore * 100.0 / totalScore) : 0;
    auto* percentLabel = new QLabel(
        QStringLiteral("正确率：%1%").arg(static_cast<int>(percentage)), this);
    percentLabel->setStyleSheet("font-size: 16px; color: #3498db;");
    percentLabel->setAlignment(Qt::AlignCenter);

    scoreLayout->addWidget(scoreLabel);
    scoreLayout->addWidget(percentLabel);

    lay->addLayout(scoreLayout);

    // 显示每道题的结果
    auto* resultsArea = new QScrollArea(this);
    auto* resultsWidget = new QWidget;
    auto* resultsLayout = new QVBoxLayout(resultsWidget);

    for (int i = 0; i < results.size(); ++i) {
        const auto& q = results[i];

        auto* itemLayout = new QVBoxLayout;
        itemLayout->setContentsMargins(15, 10, 15, 10);
        itemLayout->setSpacing(5);

        QString typeStr;
        if (q.type == QuestionType::Single) {
            typeStr = QStringLiteral("单选题");
        } else if (q.type == QuestionType::Fill) {
            typeStr = QStringLiteral("填空题");
        } else {
            typeStr = QStringLiteral("解答题");
        }

        auto* titleLabel = new QLabel(
            QStringLiteral("[%1] 第 %2 题 (%3分) - %4")
                .arg(typeStr)
                .arg(i + 1)
                .arg(q.score)
                .arg(q.isCorrect ? QStringLiteral("✓ 正确") : QStringLiteral("✗ 错误")), this);
        titleLabel->setStyleSheet(
            q.isCorrect ? "color: #2ecc71; font-weight: bold;" : "color: #e74c3c; font-weight: bold;");
        titleLabel->setWordWrap(true);

        auto* contentLabel = new QLabel(QStringLiteral("题目：%1").arg(q.content), this);
        contentLabel->setWordWrap(true);
        contentLabel->setStyleSheet("color: #555; font-size: 12px;");

        auto* answerLabel = new QLabel(
            QStringLiteral("您的答案：%1").arg(q.userAnswer.isEmpty() ? QStringLiteral("未作答") : q.userAnswer), this);
        answerLabel->setWordWrap(true);
        answerLabel->setStyleSheet("color: #3498db; font-size: 12px;");

        auto* correctLabel = new QLabel(
            QStringLiteral("正确答案：%1").arg(q.correctAnswer), this);
        correctLabel->setWordWrap(true);
        correctLabel->setStyleSheet("color: #27ae60; font-size: 12px;");

        itemLayout->addWidget(titleLabel);
        itemLayout->addWidget(contentLabel);
        itemLayout->addWidget(answerLabel);
        itemLayout->addWidget(correctLabel);

        auto* itemFrame = new QFrame(this);
        itemFrame->setLayout(itemLayout);
        itemFrame->setStyleSheet("border: 1px solid #ddd; border-radius: 4px;");

        resultsLayout->addWidget(itemFrame);
    }

    resultsLayout->addStretch();
    resultsArea->setWidget(resultsWidget);
    resultsArea->setWidgetResizable(true);

    lay->addWidget(resultsArea, 1);
}

// ==================== ExamPage ====================

ExamPage::ExamPage(QWidget* parent) : QWidget(parent), currentPage(nullptr) {
    auto* lay = new QVBoxLayout(this);
    lay->setContentsMargins(0, 0, 0, 0);
    lay->setSpacing(0);

    // 初始化考试设置页面
    auto* settingPage = new ExamSettingPage(this);
    connect(settingPage, &ExamSettingPage::backRequested, this, &ExamPage::backRequested);
    connect(settingPage, &ExamSettingPage::examStarted, this, &ExamPage::onStartExam);

    currentPage = settingPage;
    lay->addWidget(currentPage);

    loadQuestions();
}

void ExamPage::loadQuestions() {
    // 示例：从MD文件加载题目
    // 这里可以扩展为从配置文件或数据库读取题目

    Question q1;
    q1.id = 1;
    q1.contentPath = "questions/q1.md";
    q1.content = "计算矩阵A的行列式，其中A = [[1, 2], [3, 4]]";
    q1.type = QuestionType::Fill;
    q1.score = 10;
    q1.correctAnswer = "-2";
    q1.attempts = 0;
    q1.isCorrect = false;
    examQuestions.append(q1);

    Question q2;
    q2.id = 2;
    q2.contentPath = "questions/q2.md";
    q2.content = "以下哪个是矩阵的性质？";
    q2.type = QuestionType::Single;
    q2.score = 5;
    q2.correctAnswer = "1";
    q2.choices = {
        QStringLiteral("(A+B)² = A² + 2AB + B²"),
        QStringLiteral("矩阵乘法满足交换律"),
        QStringLiteral("可逆矩阵的逆是唯一的"),
        QStringLiteral("所有矩阵都可以相加")
    };
    q2.attempts = 0;
    q2.isCorrect = false;
    examQuestions.append(q2);

    Question q3;
    q3.id = 3;
    q3.contentPath = "questions/q3.md";
    q3.content = "证明：如果矩阵A可逆，则A的逆矩阵是唯一的。";
    q3.type = QuestionType::Subjective;
    q3.score = 20;
    q3.correctAnswer = "标准答案：假设A有两个逆矩阵B和C，则AB=BA=I，AC=CA=I。"
                       "则B=BI=B(AC)=(BA)C=IC=C，所以B=C，逆矩阵唯一。";
    q3.attempts = 0;
    q3.isCorrect = false;
    examQuestions.append(q3);
}

void ExamPage::onStartExam(int timeMinutes) {
    // 移除当前页面
    auto* oldPage = currentPage;
    auto* layout = this->layout();
    layout->removeWidget(oldPage);
    oldPage->deleteLater();

    // 创建考试进行页面
    auto* progressPage = new ExamProgressPage(examQuestions, timeMinutes, this);
    connect(progressPage, &ExamProgressPage::backRequested, this, &ExamPage::backRequested);
    connect(progressPage, &ExamProgressPage::examFinished, this, &ExamPage::onExamFinished);

    currentPage = progressPage;
    layout->addWidget(currentPage);
}

void ExamPage::onExamFinished(const QVector<Question>& results) {
    // 移除当前页面
    auto* oldPage = currentPage;
    auto* layout = this->layout();
    layout->removeWidget(oldPage);
    oldPage->deleteLater();

    // 创建结果页面
    auto* resultPage = new ExamResultPage(results, this);
    connect(resultPage, &ExamResultPage::backRequested, this, &ExamPage::backRequested);

    currentPage = resultPage;
    layout->addWidget(currentPage);
}

} // namespace AlgeMate::Learning