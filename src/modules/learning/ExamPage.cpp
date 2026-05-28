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
#include <QGridLayout>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDateTime>

namespace AlgeMate::Learning {

// ==================== ExamSettingPage ====================

ExamSettingPage::ExamSettingPage(QWidget* parent) : QWidget(parent) {

    auto* lay = new QVBoxLayout(this);
    setStyleSheet(R"(
        QWidget {
            background-color: #f3f6fb;
            font-family: "Microsoft YaHei";
        }
    )");
    lay->setContentsMargins(24, 16, 24, 24);
    lay->setSpacing(20);

    auto* top = new QHBoxLayout;
    auto* back = new QPushButton(QStringLiteral("← 返回"), this);
    back->setObjectName(QStringLiteral("LearnBackBtn"));
    connect(back, &QPushButton::clicked, this, &ExamSettingPage::backRequested);

    auto* title = new QLabel(QStringLiteral("数学模拟考试"));

    title->setStyleSheet(R"(
        font-size: 30px;
        font-weight: 700;
        color: #111827;
    )");
    top->addWidget(back);
    top->addWidget(title);
    top->addStretch();

    // 考试设置区域
    auto* centerLayout = new QHBoxLayout;
    centerLayout->addStretch();
    auto* card = new QFrame(this);
    card->setFixedWidth(520);
    card->setStyleSheet(R"(
        QFrame {
            background: white;
            border-radius: 24px;
            border: 1px solid #e5e7eb;
        }
    )");

    auto* cardLayout = new QVBoxLayout(card);

    cardLayout->setContentsMargins(40,40,40,40);
    cardLayout->setSpacing(24);

    auto* timeLabel = new QLabel(QStringLiteral("考试时间（分钟）："));
    timeLabel->setText("⏱ 考试时长");
    timeLabel->setStyleSheet(R"(
        font-size: 16px;
        font-weight: 600;
        color: #374151;
    )");
    timeSpinBox = new QSpinBox(this);
    timeSpinBox->setStyleSheet(R"(
        QSpinBox {
            border: 2px solid #d1d5db;
            border-radius: 12px;
            padding: 10px;
            font-size: 16px;
            min-width: 120px;
            background: white;
        }

        QSpinBox:focus {
            border: 2px solid #2563eb;
        }
    )");

    timeSpinBox->setMinimum(5);
    timeSpinBox->setMaximum(300);
    timeSpinBox->setValue(60);
    timeSpinBox->setSuffix(QStringLiteral(" 分钟"));

    auto* timeLayout = new QHBoxLayout;
    timeLayout->addWidget(timeLabel);
    timeLayout->addWidget(timeSpinBox);
    timeLayout->addStretch();

    auto* infoLabel = new QLabel(QStringLiteral(
        "📝 选择考试时间后开始考试\n"
        "⏱ 考试过程中自动计时\n"
        "📤 支持提前交卷\n"
        "📊 交卷后查看成绩与解析\n"
        "🤖 支持 AI 判卷与历史记录"
        ));

    infoLabel->setWordWrap(true);

    infoLabel->setStyleSheet(R"(
        QLabel {
            background: #f9fafb;
            border-radius: 16px;
            padding: 20px;
            font-size: 15px;
            line-height: 1.8;
            color: #4b5563;
            border: 1px solid #e5e7eb;
        }
    )");

    auto* startBtn = new QPushButton(QStringLiteral("开始考试"), this);
    startBtn->setMinimumHeight(56);

    startBtn->setStyleSheet(R"(
        QPushButton {
            background: #2563eb;
            color: white;
            border-radius: 16px;
            font-size: 18px;
            font-weight: bold;
        }

        QPushButton:hover {
            background: #1d4ed8;
        }

        QPushButton:pressed {
            background: #1e40af;
        }
    )");
    connect(startBtn, &QPushButton::clicked, this, [this]() {
        emit examStarted(timeSpinBox->value());
    });

    cardLayout->addLayout(timeLayout);
    cardLayout->addWidget(infoLabel);
    cardLayout->addStretch();
    cardLayout->addWidget(startBtn);
    centerLayout->addWidget(card);
    centerLayout->addStretch();

    lay->addLayout(top);
    lay->addLayout(centerLayout, 1);
}

// ==================== ExamProgressPage ====================

ExamProgressPage::ExamProgressPage(const QVector<Question>& questions, int timeMinutes, QWidget* parent)
    : QWidget(parent), questions(questions), currentQuestionIndex(0), remainingSeconds(timeMinutes * 60) {

    auto* lay = new QVBoxLayout(this);
    setStyleSheet(R"(
        QWidget {
            background-color: #f5f7fb;
            font-family: "Microsoft YaHei";
        }
    )");
    lay->setContentsMargins(24, 16, 24, 24);
    lay->setSpacing(14);

    // 顶部：计时器和进度
    auto* top = new QHBoxLayout;
    auto* back = new QPushButton(QStringLiteral("← 返回"), this);
    back->setObjectName(QStringLiteral("LearnBackBtn"));
    connect(back, &QPushButton::clicked, this, &ExamProgressPage::backRequested);

    timerLabel = new QLabel(QStringLiteral("剩余时间: 60:00"), this);
    timerLabel->setStyleSheet(R"(
        background: #fee2e2;
        color: #dc2626;
        border-radius: 10px;
        padding: 8px 14px;
        font-size: 15px;
        font-weight: bold;
    )");

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
    auto* questionCard = new QFrame(this);
    questionCard->setStyleSheet(R"(
        QFrame {
            background: white;
            border-radius: 16px;
            border: 1px solid #e5e7eb;
        }
    )");

    auto* questionCardLayout = new QVBoxLayout(questionCard);
    questionCardLayout->setContentsMargins(24,24,24,24);

    questionLabel = new QLabel(this);
    questionLabel->setWordWrap(true);
    questionLabel->setStyleSheet(R"(
        font-size: 18px;
        line-height: 1.8;
        color: #111827;
    )");

    auto* scrollArea = new QScrollArea(this);
    questionCardLayout->addWidget(questionLabel);
    scrollArea->setWidget(questionCard);
    scrollArea->setWidgetResizable(true);

    // 答案区域（动态创建）
    answerWidget = new QWidget(this);

    //导航栏
    auto* mainContentLayout = new QHBoxLayout;
    auto* navWidget = new QWidget(this);
    navWidget->setFixedWidth(190);

    navWidget->setStyleSheet(R"(
        QWidget {
            background: white;
            border-radius: 16px;
            border: 1px solid #e5e7eb;
        }
    )");

    auto* navLayout = new QVBoxLayout(navWidget);
    auto* grid = new QGridLayout;
    auto* navTitle = new QLabel(QStringLiteral("题目"), this);

    navTitle->setStyleSheet(R"(
        font-size: 18px;
        font-weight: 700;
        color: #111827;
        padding-bottom: 8px;
    )");

    navTitle->setAlignment(Qt::AlignCenter);

    navLayout->addWidget(navTitle);
    navLayout->setContentsMargins(12,12,12,12);
    navLayout->setSpacing(10);

    for (int i = 0; i < questions.size(); ++i) {

        auto* btn = new QPushButton(
            QString::number(i + 1),
            this);

        btn->setFixedSize(42,42);

        btn->setStyleSheet(R"(
        QPushButton {
            background: #f3f4f6;
            border-radius: 22px;
            font-size: 20px;
            font-weight: bold;
        }

        QPushButton:hover {
            background: #dbeafe;
        }
    )");

        connect(btn, &QPushButton::clicked,
                this,
                [this, i]() {
                    loadQuestion(i);
                });

        grid->addWidget(btn, i / 3, i % 3);
        navButtons.append(btn);
    }
    navLayout->addLayout(grid);
    navLayout->addStretch();

    // 底部按钮
    auto* bottomLayout = new QHBoxLayout;
    auto* prevBtn = new QPushButton(QStringLiteral("上一题"), this);
    // auto* submitBtn = new QPushButton(QStringLiteral("提交答案"), this);
    auto* nextBtn = new QPushButton(QStringLiteral("下一题"), this);
    auto* examSubmitBtn = new QPushButton(QStringLiteral("交卷"), this);
    QString btnStyle = R"(
        QPushButton {
            background: #2563eb;
            color: white;
            border-radius: 10px;
            padding: 10px 18px;
            font-size: 14px;
            font-weight: bold;
        }

        QPushButton:hover {
            background: #1d4ed8;
        }
    )";
    prevBtn->setStyleSheet(btnStyle);
    // submitBtn->setStyleSheet(btnStyle);
    nextBtn->setStyleSheet(btnStyle);
    examSubmitBtn->setStyleSheet("background-color: #e74c3c; color: white;");

    connect(prevBtn, &QPushButton::clicked, this, &ExamProgressPage::onPreviousQuestion);
    // connect(submitBtn, &QPushButton::clicked, this, &ExamProgressPage::onSubmitAnswer);
    connect(nextBtn, &QPushButton::clicked, this, &ExamProgressPage::onNextQuestion);
    connect(examSubmitBtn, &QPushButton::clicked, this, &ExamProgressPage::onSubmitExam);

    bottomLayout->addWidget(prevBtn);
    bottomLayout->addStretch();
    // bottomLayout->addWidget(submitBtn);
    bottomLayout->addWidget(nextBtn);
    bottomLayout->addStretch();
    bottomLayout->addWidget(examSubmitBtn);

    auto* rightLayout = new QVBoxLayout;
    rightLayout->setSpacing(14);

    rightLayout->addWidget(scrollArea, 1);
    rightLayout->addWidget(answerWidget);
    rightLayout->addLayout(bottomLayout);

    mainContentLayout->setSpacing(20);
    mainContentLayout->addWidget(navWidget);
    mainContentLayout->addLayout(rightLayout, 1);

    lay->addLayout(top);
    lay->addLayout(infoLayout);
    lay->addLayout(mainContentLayout, 1);
    // lay->addWidget(scrollArea);
    // lay->addWidget(answerWidget);
    // lay->addLayout(bottomLayout);

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
        QStringLiteral("第 %1 / %2 题")
            .arg(index + 1)
            .arg(questions.size())
        );

    // for (int i = 0; i < navButtons.size(); ++i) {
    //     QString style;
    //     if (i == currentQuestionIndex) {
    //         style = R"(
    //         QPushButton {
    //             background: #2563eb;
    //             color: white;
    //             border-radius: 24px;
    //             font-size: 16px;
    //             font-weight: bold;
    //         }
    //     )";

    //     } else if (!questions[i].userAnswer.isEmpty()) {
    //         style = R"(
    //         QPushButton {
    //             background: #10b981;
    //             color: white;
    //             border-radius: 24px;
    //             font-size: 16px;
    //             font-weight: bold;
    //         }
    //     )";
    //     } else {
    //         style = R"(
    //         QPushButton {
    //             background: #f3f4f6;
    //             color: #374151;
    //             border-radius: 24px;
    //             font-size: 16px;
    //             font-weight: bold;
    //         }
    //         QPushButton:hover {
    //             background: #dbeafe;
    //         }
    //     )";
    //     }

    //     navButtons[i]->setStyleSheet(style);
    // }

    updateNavButtons();
    updateUIForQuestion(q);
}

void ExamProgressPage::updateNavButtons() {

    for (int i = 0; i < navButtons.size(); ++i) {

        QString style;

        if (i == currentQuestionIndex) {

            style = R"(
            QPushButton {
                background: #2563eb;
                color: white;
                border-radius: 18px;
                font-size: 16px;
                font-weight: bold;
            }
        )";

        } else if (!questions[i].userAnswer.isEmpty()) {

            style = R"(
            QPushButton {
                background: #10b981;
                color: white;
                border-radius: 18px;
                font-size: 16px;
                font-weight: bold;
            }
        )";

        } else {

            style = R"(
            QPushButton {
                background: #f3f4f6;
                color: #374151;
                border-radius: 18px;
                font-size: 16px;
                font-weight: bold;
            }

            QPushButton:hover {
                background: #dbeafe;
            }
        )";
        }

        navButtons[i]->setStyleSheet(style);
    }
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
            radio->setStyleSheet(R"(
                QRadioButton {
                    background: white;
                    border: 2px solid #d1d5db;
                    border-radius: 12px;
                    padding: 14px;
                    font-size: 15px;
                }

                QRadioButton:hover {
                    border: 2px solid #60a5fa;
                    background: #f0f7ff;
                }

                QRadioButton:checked {
                    border: 2px solid #2563eb;
                    background: #dbeafe;
                    font-weight: bold;
                }
            )");
            radio->setObjectName(QStringLiteral("choice_%1").arg(i));
            buttonGroup->addButton(radio, i);
            answerLayout->addWidget(radio);

            connect(radio,
                    &QRadioButton::clicked,
                    this,
                    [this, i]() {

                        Question& current =
                            questions[currentQuestionIndex];

                        current.userAnswer = QString::number(i);

                        // current.isCorrect =
                        //     (i == current.correctAnswer.toInt());

                        updateNavButtons();
                    });

            // 恢复之前的选择
            if (!q.userAnswer.isEmpty() && q.userAnswer.toInt() == i) {
                radio->setChecked(true);
            }
        }
    } else if (q.type == QuestionType::Fill) {
        // 填空题：输入框
        auto* lineEdit = new QLineEdit(answerWidget);
        lineEdit->setStyleSheet(R"(
            QLineEdit {
                background: white;
                border: 2px solid #d1d5db;
                border-radius: 10px;
                padding: 12px;
                font-size: 15px;
            }

            QLineEdit:focus {
                border: 2px solid #2563eb;
            }
        )");
        lineEdit->setObjectName(QStringLiteral("fillAnswer"));
        lineEdit->setPlaceholderText(QStringLiteral("请输入数字答案"));
        lineEdit->setText(q.userAnswer);
        answerLayout->addWidget(lineEdit);
        connect(lineEdit,
                &QLineEdit::textChanged,
                this,
                [this](const QString& text) {

                    Question& current =
                        questions[currentQuestionIndex];

                    current.userAnswer = text;
                    // bool ok1, ok2;
                    // double userValue =
                    //     text.toDouble(&ok1);
                    // double correctValue =
                    //     current.correctAnswer.toDouble(&ok2);
                    // current.isCorrect =
                    //     ok1 && ok2 &&
                    //     std::abs(userValue - correctValue) < 0.0001;
                    updateNavButtons();
                });
    } else if (q.type == QuestionType::Subjective) {
        // 解答题：文本编辑框
        auto* textEdit = new QPlainTextEdit(answerWidget);
        textEdit->setStyleSheet(R"(
            QPlainTextEdit {
                background: white;
                border: 2px solid #d1d5db;
                border-radius: 12px;
                padding: 12px;
                font-size: 15px;
            }

            QPlainTextEdit:focus {
                border: 2px solid #2563eb;
            }
        )");
        textEdit->setObjectName(QStringLiteral("subjectiveAnswer"));
        textEdit->setPlaceholderText(QStringLiteral("请输入您的解答"));
        textEdit->setMinimumHeight(120);
        textEdit->setPlainText(q.userAnswer);
        answerLayout->addWidget(textEdit);
        connect(textEdit,
                &QPlainTextEdit::textChanged,
                this,
                [this, textEdit]() {

                    Question& current =
                        questions[currentQuestionIndex];
                    current.userAnswer =
                        textEdit->toPlainText();
                    updateNavButtons();
                });
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
    loadQuestion(currentQuestionIndex);
} //是否可以删除呢

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

void ExamProgressPage::saveWrongQuestions()
{
    QFile file("wrong_questions.json");

    QJsonArray allWrongQuestions;

    // 读取旧错题
    if (file.exists()) {

        if (file.open(QIODevice::ReadOnly)) {

            QJsonDocument oldDoc =
                QJsonDocument::fromJson(file.readAll());

            allWrongQuestions = oldDoc.array();

            file.close();
        }
    }

    // 保存新错题
    for (const auto& q : questions) {

        if (q.isCorrect) {
            continue;
        }

        QJsonObject obj;

        obj["content"] = q.content;
        obj["userAnswer"] = q.userAnswer;
        obj["correctAnswer"] = q.correctAnswer;
        obj["score"] = q.score;
        obj["time"] =
            QDateTime::currentDateTime()
                .toString("yyyy-MM-dd hh:mm:ss");

        obj["wrongCount"] = 1;

        if (q.type == QuestionType::Single) {
            obj["type"] = "single";
        } else if (q.type == QuestionType::Fill) {
            obj["type"] = "fill";
        } else {
            obj["type"] = "subjective";
        }

        QJsonArray choicesArray;

        for (const auto& c : q.choices) {
            choicesArray.append(c);
        }

        obj["choices"] = choicesArray;

        allWrongQuestions.append(obj);
    }

    if (!file.open(QIODevice::WriteOnly)) {
        return;
    }

    file.write(
        QJsonDocument(allWrongQuestions)
            .toJson());

    file.close();
}


void ExamProgressPage::onSubmitExam() {
    auto* msgBox = new QMessageBox(this);
    msgBox->setText(QStringLiteral("确定要交卷吗？交卷后将无法修改答案。"));
    msgBox->setStandardButtons(QMessageBox::Yes | QMessageBox::No);

    if (msgBox->exec() == QMessageBox::Yes) {
        // ==================== 统一判卷 ====================

        for (auto& q : questions) {

            if (q.type == QuestionType::Single) {

                q.isCorrect =
                    (q.userAnswer == q.correctAnswer);

            } else if (q.type == QuestionType::Fill) {

                bool ok1, ok2;

                double userValue =
                    q.userAnswer.toDouble(&ok1);

                double correctValue =
                    q.correctAnswer.toDouble(&ok2);

                q.isCorrect =
                    ok1 && ok2 &&
                    std::abs(userValue - correctValue) < 0.0001;

            } else {

                // 主观题先默认错误
                // 后续可接 AI 判卷

                q.isCorrect = false;
            }
        }
        QJsonArray historyArray;

        for (const auto& q : questions) {

            QJsonObject obj;

            obj["content"] = q.content;
            obj["userAnswer"] = q.userAnswer;
            obj["correctAnswer"] = q.correctAnswer;
            obj["isCorrect"] = q.isCorrect;
            obj["score"] = q.score;

            historyArray.append(obj);
        }

        QJsonObject examObj;

        examObj["time"] =
            QDateTime::currentDateTime()
                .toString("yyyy-MM-dd hh:mm:ss");

        examObj["questions"] = historyArray;

        QFile file("exam_history.json");

        QJsonArray allHistory;

        if (file.exists()) {
            if (!file.open(QIODevice::ReadOnly)) {
                return;
            }

            QJsonDocument oldDoc =
                QJsonDocument::fromJson(file.readAll());

            allHistory =
                oldDoc.array();

            file.close();
        }

        allHistory.append(examObj);

        if(!file.open(QIODevice::WriteOnly)){
            return;
        }

        file.write(
            QJsonDocument(allHistory)
                .toJson()
            );

        file.close();
        saveWrongQuestions();
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
    auto* scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    auto* container = new QWidget;
    auto* contentLayout = new QVBoxLayout(container);
    contentLayout->setContentsMargins(24,16,24,24);
    contentLayout->setSpacing(20);
    scrollArea->setWidget(container);
    lay->addWidget(scrollArea);


    setStyleSheet(R"(
        QWidget {
            background-color: #f3f6fb;
            font-family: "Microsoft YaHei";
        }
    )");
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

    contentLayout->addLayout(top);

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
    auto* scoreCard = new QFrame(container);
    scoreCard->setStyleSheet(R"(
        QFrame {
            background: white;
            border-radius: 24px;
            border: 1px solid #e5e7eb;
        }
    )");
    auto* scoreLayout = new QVBoxLayout(scoreCard);
    scoreLayout->setContentsMargins(40,40,40,40);
    scoreLayout->setSpacing(14);

    auto* scoreLabel = new QLabel(
        QStringLiteral("总分：%1/%2").arg(earnedScore).arg(totalScore), this);
    scoreLabel->setStyleSheet(R"(
        font-size: 32px;
        font-weight: 800;
        color: #2563eb;
    )");
    scoreLabel->setAlignment(Qt::AlignCenter);

    double percentage = (totalScore > 0) ? (earnedScore * 100.0 / totalScore) : 0;
    QString level;
    if (percentage >= 90)
        level = "Excellent!";
    else if (percentage >= 75)
        level = "Good!";
    else if (percentage >= 60)
        level = "Pass";
    else
        level = "Needs Improvement";
    auto* percentLabel = new QLabel(
        QStringLiteral("正确率：%1%").arg(static_cast<int>(percentage)), this);
    percentLabel->setStyleSheet(R"(font-size: 22px;font-weight: 600;color: #6b7280;)");
    percentLabel->setAlignment(Qt::AlignCenter);

    scoreLayout->addWidget(scoreLabel);
    scoreLayout->addWidget(percentLabel);
    auto* levelLabel = new QLabel(level, this);

    levelLabel->setAlignment(Qt::AlignCenter);

    levelLabel->setStyleSheet(R"(
        font-size: 28px;
        font-weight: bold;
        color: #10b981;
    )");

    scoreLayout->addWidget(levelLabel);

    contentLayout->addWidget(scoreCard);

    // 显示每道题的结果
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
            q.isCorrect? "color: #10b981; font-weight: bold; font-size: 15px;": "color: #ef4444; font-weight: bold; font-size: 15px;");
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

        auto* itemFrame = new QFrame(container);
        itemFrame->setLayout(itemLayout);
        itemFrame->setStyleSheet(R"(
            QFrame {
                background: white;
                border-radius: 18px;
                border: 1px solid #e5e7eb;
                padding: 8px;
            }
        )");

        resultsLayout->addWidget(itemFrame);
    }
    resultsLayout->addStretch();
    contentLayout->addWidget(resultsWidget);
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