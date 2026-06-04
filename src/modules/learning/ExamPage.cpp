#include "ExamPage.h"
#include "modules/ai_solver/OcrAttachWidget.h"

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

#include "PastExams.h"
#include <QComboBox>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QCoreApplication>
#include <QUrl>
#include <QRegularExpression>
#include <QDialog>
#include <QTextBrowser>
#include "latex/LatexRenderer.h"
#include "latex/LatexTextBrowser.h"
#include "core/ThemeManager.h"

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

    auto* title = new QLabel(QStringLiteral("数学模拟考试"));

    top->addWidget(back);
    top->addWidget(title);
    top->addStretch();

    // 考试设置区域
    auto* centerLayout = new QHBoxLayout;
    centerLayout->addStretch();
    auto* card = new QFrame(this);
    card->setFixedWidth(520);

    auto* cardLayout = new QVBoxLayout(card);
    cardLayout->setContentsMargins(40,40,40,40);
    cardLayout->setSpacing(24);

    auto* timeLabel = new QLabel(QStringLiteral("考试时间（分钟）："));
    timeLabel->setText("⏱ 考试时长");
    timeSpinBox = new QSpinBox(this);
    timeSpinBox->setMinimum(5);
    timeSpinBox->setMaximum(300);
    timeSpinBox->setValue(120);
    timeSpinBox->setSuffix(QStringLiteral(" 分钟"));

    // 试卷选择
    auto* examLabel = new QLabel(QStringLiteral("📜 选择试卷："));
    examComboBox = new QComboBox(this);
    examComboBox->addItems(PastExams::getExamList()); // 从 PastExams 加载列表

    auto* examLayout = new QHBoxLayout;
    examLayout->addWidget(examLabel);
    examLayout->addWidget(examComboBox);
    examLayout->addStretch();

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
    infoLabel->setObjectName(QStringLiteral("ExamInfoLabel"));
    infoLabel->setWordWrap(true);

    auto* startBtn = new QPushButton(QStringLiteral("开始考试"), this);
    startBtn->setMinimumHeight(56);

    connect(startBtn, &QPushButton::clicked, this, [this]() {
        emit examStarted(timeSpinBox->value(), examComboBox->currentIndex(), examComboBox->currentText());
    });

    cardLayout->addLayout(examLayout);
    cardLayout->addLayout(timeLayout);
    cardLayout->addWidget(infoLabel);
    cardLayout->addStretch();
    cardLayout->addWidget(startBtn);
    centerLayout->addWidget(card);
    centerLayout->addStretch();

    lay->addLayout(top);
    lay->addLayout(centerLayout, 1);

    // 主题感知: 所有原本硬编码颜色集中在 lambda, 跟随亮/暗主题切换.
    auto applyTheme = [=, this]() {
        const bool dark = AlgeMate::ThemeManager::instance().currentTheme()
                          == AlgeMate::ThemeManager::Theme::Dark;
        this->setStyleSheet(dark
            ? "QWidget { background-color: #1C1B2E; font-family: \"Microsoft YaHei\"; color: #E6E7F0; }"
            : "QWidget { background-color: #f3f6fb; font-family: \"Microsoft YaHei\"; }");
        title->setStyleSheet(dark
            ? "font-size: 30px; font-weight: 700; color: #F3F3FA; background: transparent;"
            : "font-size: 30px; font-weight: 700; color: #111827; background: transparent;");
        card->setStyleSheet(dark
            ? "QFrame { background: #28263F; border-radius: 24px; border: 1px solid #3A3754; }"
            : "QFrame { background: white; border-radius: 24px; border: 1px solid #e5e7eb; }");
        timeLabel->setStyleSheet(dark
            ? "font-size: 16px; font-weight: 600; color: #E6E7F0; background: transparent;"
            : "font-size: 16px; font-weight: 600; color: #374151; background: transparent;");
        examLabel->setStyleSheet(dark
            ? "font-size: 16px; font-weight: 600; color: #E6E7F0; background: transparent;"
            : "font-size: 16px; font-weight: 600; color: #374151; background: transparent;");
        timeSpinBox->setStyleSheet(dark
            ? "QSpinBox { border: 2px solid #3A3754; border-radius: 12px; padding: 10px; font-size: 16px; min-width: 120px; background: #2C2A45; color: #F3F3FA; } QSpinBox:focus { border: 2px solid #8B7BFF; }"
            : "QSpinBox { border: 2px solid #d1d5db; border-radius: 12px; padding: 10px; font-size: 16px; min-width: 120px; background: white; } QSpinBox:focus { border: 2px solid #2563eb; }");
        examComboBox->setStyleSheet(dark
            ? "QComboBox { border: 2px solid #3A3754; border-radius: 12px; padding: 10px; font-size: 15px; min-width: 280px; background: #2C2A45; color: #F3F3FA; } QComboBox:focus { border: 2px solid #8B7BFF; } QComboBox QAbstractItemView { background: #28263F; color: #E6E7F0; selection-background-color: #3A3460; }"
            : "QComboBox { border: 2px solid #d1d5db; border-radius: 12px; padding: 10px; font-size: 15px; min-width: 280px; background: white; } QComboBox:focus { border: 2px solid #2563eb; }");
        infoLabel->setStyleSheet(dark
            ? "QLabel#ExamInfoLabel { background: #2C2A45; border-radius: 16px; padding: 20px; font-size: 15px; line-height: 1.8; color: #C9CCE6; border: 1px solid #3A3754; }"
            : "QLabel#ExamInfoLabel { background: #f9fafb; border-radius: 16px; padding: 20px; font-size: 15px; line-height: 1.8; color: #4b5563; border: 1px solid #e5e7eb; }");
        startBtn->setStyleSheet(dark
            ? "QPushButton { background: #6B7CFF; color: white; border-radius: 16px; font-size: 18px; font-weight: bold; } QPushButton:hover { background: #8B7BFF; } QPushButton:pressed { background: #5A6BEE; }"
            : "QPushButton { background: #2563eb; color: white; border-radius: 16px; font-size: 18px; font-weight: bold; } QPushButton:hover { background: #1d4ed8; } QPushButton:pressed { background: #1e40af; }");
    };
    applyTheme();
    connect(&AlgeMate::ThemeManager::instance(),
            &AlgeMate::ThemeManager::themeChanged,
            this, [applyTheme](AlgeMate::ThemeManager::Theme){ applyTheme(); });
}

// ==================== ExamProgressPage ====================

ExamProgressPage::ExamProgressPage(const QVector<Question>& questions, int timeMinutes, const QString& examName, QWidget* parent)
    : QWidget(parent), questions(questions), currentQuestionIndex(0), remainingSeconds(timeMinutes * 60), m_examName(examName) {
    auto* lay = new QVBoxLayout(this);
    lay->setContentsMargins(24, 16, 24, 24);
    lay->setSpacing(14);

    // 顶部：计时器和进度
    auto* top = new QHBoxLayout;
    auto* back = new QPushButton(QStringLiteral("← 返回"), this);
    back->setObjectName(QStringLiteral("LearnBackBtn"));
    connect(back, &QPushButton::clicked, this, &ExamProgressPage::backRequested);

    timerLabel = new QLabel(QStringLiteral("剩余时间: 60:00"), this);

    // 考试名称 Label
    auto* nameLabel = new QLabel(m_examName, this);

    top->addWidget(back);
    top->addStretch();
    top->addWidget(nameLabel); // 居中显示试卷名
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

    auto* questionCardLayout = new QVBoxLayout(questionCard);
    questionCardLayout->setContentsMargins(24,24,24,24);

    questionBrowser = new Latex::LatexTextBrowser(this);

    auto* scrollArea = new QScrollArea(this);
    questionCardLayout->addWidget(questionBrowser);
    scrollArea->setWidget(questionCard);
    scrollArea->setWidgetResizable(true);

    // 答案区域（动态创建）
    answerWidget = new QWidget(this);

    //导航栏
    auto* mainContentLayout = new QHBoxLayout;
    auto* navWidget = new QWidget(this);
    navWidget->setFixedWidth(190);

    auto* navLayout = new QVBoxLayout(navWidget);
    auto* grid = new QGridLayout;
    auto* navTitle = new QLabel(QStringLiteral("题目"), this);
    navTitle->setAlignment(Qt::AlignCenter);

    navLayout->addWidget(navTitle);
    navLayout->setContentsMargins(12,12,12,12);
    navLayout->setSpacing(10);

    for (int i = 0; i < questions.size(); ++i) {

        auto* btn = new QPushButton(
            QString::number(i + 1),
            this);

        btn->setFixedSize(42,42);

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
    auto* nextBtn = new QPushButton(QStringLiteral("下一题"), this);
    auto* examSubmitBtn = new QPushButton(QStringLiteral("交卷"), this);

    connect(prevBtn, &QPushButton::clicked, this, &ExamProgressPage::onPreviousQuestion);
    connect(nextBtn, &QPushButton::clicked, this, &ExamProgressPage::onNextQuestion);
    connect(examSubmitBtn, &QPushButton::clicked, this, &ExamProgressPage::onSubmitExam);

    bottomLayout->addWidget(prevBtn);
    bottomLayout->addStretch();
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

    // 主题感知: 原本硬编码颜色集中在 lambda, 跟随主题切换.
    auto applyTheme = [=, this]() {
        const bool dark = AlgeMate::ThemeManager::instance().currentTheme()
                          == AlgeMate::ThemeManager::Theme::Dark;
        this->setStyleSheet(dark
            ? "QWidget { background-color: #1C1B2E; font-family: \"Microsoft YaHei\"; color: #E6E7F0; }"
            : "QWidget { background-color: #f5f7fb; font-family: \"Microsoft YaHei\"; }");
        timerLabel->setStyleSheet(dark
            ? "background: #4A1F1F; color: #FF8888; border-radius: 10px; padding: 8px 14px; font-size: 15px; font-weight: bold;"
            : "background: #fee2e2; color: #dc2626; border-radius: 10px; padding: 8px 14px; font-size: 15px; font-weight: bold;");
        nameLabel->setStyleSheet(dark
            ? "font-size: 18px; font-weight: bold; color: #F3F3FA; background: transparent;"
            : "font-size: 18px; font-weight: bold; color: #1f2937; background: transparent;");
        questionCard->setStyleSheet(dark
            ? "QFrame { background: #28263F; border-radius: 16px; border: 1px solid #3A3754; }"
            : "QFrame { background: white; border-radius: 16px; border: 1px solid #e5e7eb; }");
        questionBrowser->setStyleSheet(dark
            ? "LatexTextBrowser { background: transparent; border: none; font-size: 18px; color: #F3F3FA; font-family: \"Microsoft YaHei\"; }"
            : "LatexTextBrowser { background: transparent; border: none; font-size: 18px; color: #111827; font-family: \"Microsoft YaHei\"; }");
        navWidget->setStyleSheet(dark
            ? "QWidget { background: #28263F; border-radius: 16px; border: 1px solid #3A3754; }"
            : "QWidget { background: white; border-radius: 16px; border: 1px solid #e5e7eb; }");
        navTitle->setStyleSheet(dark
            ? "font-size: 18px; font-weight: 700; color: #F3F3FA; padding-bottom: 8px; background: transparent; border: none;"
            : "font-size: 18px; font-weight: 700; color: #111827; padding-bottom: 8px; background: transparent; border: none;");
        const QString navBtnStyle = dark
            ? "QPushButton { background: #2C2A45; color: #E6E7F0; border-radius: 22px; font-size: 20px; font-weight: bold; border: 1px solid #3A3754; } QPushButton:hover { background: #312F4A; color: #B8ACFF; }"
            : "QPushButton { background: #f3f4f6; border-radius: 22px; font-size: 20px; font-weight: bold; } QPushButton:hover { background: #dbeafe; }";
        for (auto* nb : navButtons) {
            if (nb) nb->setStyleSheet(navBtnStyle);
        }
        const QString btnStyle = dark
            ? "QPushButton { background: #6B7CFF; color: white; border-radius: 10px; padding: 10px 18px; font-size: 14px; font-weight: bold; } QPushButton:hover { background: #8B7BFF; }"
            : "QPushButton { background: #2563eb; color: white; border-radius: 10px; padding: 10px 18px; font-size: 14px; font-weight: bold; } QPushButton:hover { background: #1d4ed8; }";
        prevBtn->setStyleSheet(btnStyle);
        nextBtn->setStyleSheet(btnStyle);
        examSubmitBtn->setStyleSheet(dark
            ? "QPushButton { background-color: #c0392b; color: white; border-radius: 10px; padding: 10px 18px; font-size: 14px; font-weight: bold; } QPushButton:hover { background: #d44837; }"
            : "QPushButton { background-color: #e74c3c; color: white; border-radius: 10px; padding: 10px 18px; font-size: 14px; font-weight: bold; } QPushButton:hover { background: #c0392b; }");
    };
    applyTheme();
    connect(&AlgeMate::ThemeManager::instance(),
            &AlgeMate::ThemeManager::themeChanged,
            this, [applyTheme](AlgeMate::ThemeManager::Theme){ applyTheme(); });

    // 设置计时器
    m_examTimer = new QTimer(this);
    connect(m_examTimer, &QTimer::timeout, this, &ExamProgressPage::onTimeUpdate);
    m_examTimer->start(1000);

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
    // questionLabel->setText(q.content);
    // 使用 Latex 渲染设置题目内容
    const bool dark = AlgeMate::ThemeManager::instance().currentTheme()
                      == AlgeMate::ThemeManager::Theme::Dark;
    Latex::LatexRenderer renderer;
    renderer.setTextColor(dark ? QColor("#F3F3FA") : QColor("#1F2033"));
    renderer.addMathMacro(QStringLiteral("F"),  QStringLiteral("\\mathbb{F}"));
    renderer.addMathMacro(QStringLiteral("R"),  QStringLiteral("\\mathbb{R}"));
    renderer.addMathMacro(QStringLiteral("C"),  QStringLiteral("\\mathbb{C}"));
    questionBrowser->setHtml(renderer.render(q.content, questionBrowser->document()));

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
            radio->setStyleSheet(dark ? R"(
                QRadioButton {
                    background: #28263F;
                    color: #E6E7F0;
                    border: 2px solid #3B395A;
                    border-radius: 12px;
                    padding: 14px;
                    font-size: 15px;
                }
                QRadioButton:hover {
                    border: 2px solid #6F77FF;
                    background: #312F4A;
                }
                QRadioButton:checked {
                    border: 2px solid #6F77FF;
                    background: #3B395A;
                    font-weight: bold;
                }
            )" : R"(
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
        lineEdit->setStyleSheet(dark ? R"(
            QLineEdit {
                background: #28263F;
                color: #E6E7F0;
                border: 2px solid #3B395A;
                border-radius: 10px;
                padding: 12px;
                font-size: 15px;
            }
            QLineEdit:focus {
                border: 2px solid #6F77FF;
            }
        )" : R"(
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
        // 解答题：文本编辑框 + 图像上传 OCR（可拖拽）
        auto* textEdit = new QPlainTextEdit(answerWidget);
        textEdit->setStyleSheet(dark ? R"(
            QPlainTextEdit {
                background: #28263F;
                color: #E6E7F0;
                border: 2px solid #3B395A;
                border-radius: 12px;
                padding: 12px;
                font-size: 15px;
            }
            QPlainTextEdit:focus {
                border: 2px solid #6F77FF;
            }
        )" : R"(
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
        textEdit->setPlaceholderText(QStringLiteral("请输入您的解答，或下方上传手写拍照 / 图片进行 OCR 识别"));
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

        // OCR 上传组件（点击选图 / 拖拽折入都可）
        auto* ocr = new AiSolver::OcrAttachWidget(
            QStringLiteral("可以点击“📷 上传图片识别”或直接拖拽手写拍照到这里识别后追加到补答区"),
            answerWidget);
        connect(ocr, &AiSolver::OcrAttachWidget::ocrTextReady,
                this, [textEdit](const QString& text) {
                    if (text.isEmpty()) return;
                    QString cur = textEdit->toPlainText();
                    if (!cur.isEmpty() && !cur.endsWith(QLatin1Char('\n')))
                        cur += QLatin1Char('\n');
                    cur += text;
                    textEdit->setPlainText(cur);
                });
        answerLayout->addWidget(ocr);
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

/*
void ExamProgressPage::saveWrongQuestions()
{
    QFile file("wrong_questions.json");
    QJsonArray allWrongQuestions;

    // 读取旧错题
    if (file.exists()) {
        if (file.open(QIODevice::ReadOnly)) {
            QJsonDocument oldDoc = QJsonDocument::fromJson(file.readAll());
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

        // ⚡ 核心修复：必须保存题目的真实 ID，否则默认为0会触发全删 Bug！
        obj["id"] = q.id;

        obj["content"] = q.content;
        obj["userAnswer"] = q.userAnswer;
        obj["correctAnswer"] = q.correctAnswer;
        obj["score"] = q.score;
        obj["time"] = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");
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

    if (!file.open(QIODevice::WriteOnly)) return;
    file.write(QJsonDocument(allWrongQuestions).toJson());
    file.close();
}*/

void ExamProgressPage::saveWrongQuestions() {
    QFile file("wrong_questions.json");
    QJsonArray allWrongQuestions;

    if (file.exists() && file.open(QIODevice::ReadOnly)) {
        allWrongQuestions = QJsonDocument::fromJson(file.readAll()).array();
        file.close();
    }

    QJsonArray updatedArray;
    for (int i = 0; i < allWrongQuestions.size(); ++i) {
        updatedArray.append(allWrongQuestions[i]);
    }

    for (const auto& q : questions) {
        if (q.isCorrect) continue;

        bool alreadyExists = false;
        for (int i = 0; i < updatedArray.size(); ++i) {
            QJsonObject obj = updatedArray[i].toObject();
            if (obj["id"].toInt() == q.id) {
                alreadyExists = true;
                obj["wrongCount"] = obj["wrongCount"].toInt() + 1;
                obj["time"] = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");
                obj["userAnswer"] = q.userAnswer;
                updatedArray[i] = obj; // 更新旧记录
                break;
            }
        }

        if (!alreadyExists) {
            QJsonObject obj;
            obj["id"] = q.id;
            obj["content"] = q.content;
            obj["userAnswer"] = q.userAnswer;
            obj["correctAnswer"] = q.correctAnswer;
            obj["score"] = q.score;
            obj["earnedScore"] = q.earnedScore;
            obj["aiReport"] = q.aiReport;
            obj["time"] = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");
            obj["wrongCount"] = 1;
            obj["type"] = (q.type == QuestionType::Single) ? "single" : (q.type == QuestionType::Fill ? "fill" : "subjective");

            QJsonArray choicesArray;
            for (const auto& c : q.choices) { choicesArray.append(c); }
            obj["choices"] = choicesArray;

            updatedArray.append(obj); // 插入新错题
        }
    }

    if (file.open(QIODevice::WriteOnly)) {
        file.write(QJsonDocument(updatedArray).toJson(QJsonDocument::Indented));
        file.close();
    }
}

// void ExamProgressPage::onSubmitExam() {
//     auto* msgBox = new QMessageBox(this);
//     msgBox->setText(QStringLiteral("确定要交卷吗？提交后不可更改答案\n交卷后客观题将立即出分，解答题将交由 DeepSeek AI 智能批阅。"));
//     msgBox->setStandardButtons(QMessageBox::Yes | QMessageBox::No);

//     if (msgBox->exec() == QMessageBox::Yes) {
//         if (m_examTimer) m_examTimer->stop();
//         this->setEnabled(false); // 避免判卷期间二次点击

//         m_pendingAITasks = 0;

//         // 1. 自动批改客观题，并初始化得分；筛选出主观题交由 AI 处理
//         for (int i = 0; i < questions.size(); ++i) {
//             auto& q = questions[i];
//             if (q.type == QuestionType::Single) {
//                 q.isCorrect = (q.userAnswer == q.correctAnswer);
//                 q.earnedScore = q.isCorrect ? q.score : 0; // 客观题得分赋值
//             } else if (q.type == QuestionType::Fill) {
//                 bool ok1, ok2;
//                 double userValue = q.userAnswer.toDouble(&ok1);
//                 double correctValue = q.correctAnswer.toDouble(&ok2);
//                 q.isCorrect = ok1 && ok2 && std::abs(userValue - correctValue) < 0.0001;
//                 q.earnedScore = q.isCorrect ? q.score : 0; // 客观题得分赋值
//             } else {
//                 // 主观题累计进入 AI 异步流
//                 m_pendingAITasks++;
//                 gradeSubjectiveWithAI(i);
//             }
//         }

//         // 如果整张试卷没有主观题，直接进入收尾保存
//         if (m_pendingAITasks == 0) {
//             finishExamAndSave();
//         }
//     }
// }

void ExamProgressPage::onSubmitExam() {
    auto* msgBox = new QMessageBox(this);
    msgBox->setText(QStringLiteral("确定要交卷吗？提交后不可更改答案\n交卷后客观题将立即出分，解答题将交由 DeepSeek AI 智能批阅。"));
    msgBox->setStandardButtons(QMessageBox::Yes | QMessageBox::No);

    if (msgBox->exec() == QMessageBox::Yes) {
        if (m_examTimer) m_examTimer->stop();
        this->setEnabled(false); // 避免判卷期间二次点击

        m_pendingAITasks = 0;

        // 1. 先遍历一遍，完成客观题批改，并【预先统计】主观题总数
        for (int i = 0; i < questions.size(); ++i) {
            auto& q = questions[i];
            if (q.type == QuestionType::Single) {
                q.isCorrect = (q.userAnswer == q.correctAnswer);
                q.earnedScore = q.isCorrect ? q.score : 0;
            } else if (q.type == QuestionType::Fill) {
                bool ok1, ok2;
                double userValue = q.userAnswer.toDouble(&ok1);
                double correctValue = q.correctAnswer.toDouble(&ok2);
                q.isCorrect = ok1 && ok2 && std::abs(userValue - correctValue) < 0.0001;
                q.earnedScore = q.isCorrect ? q.score : 0;
            } else {
                // 只统计数量，先不执行 AI
                m_pendingAITasks++;
            }
        }

        // 2. 如果整张试卷没有主观题，直接进入收尾保存
        if (m_pendingAITasks == 0) {
            finishExamAndSave();
            return; // 结束函数，避免往下走
        }

        // 3. 开始统一派发 AI 判卷任务
        // 此时 m_pendingAITasks 已经是总数（比如 8），哪怕遇到空答案立刻 --，也不会马上变成 0 触发提前交卷
        for (int i = 0; i < questions.size(); ++i) {
            if (questions[i].type == QuestionType::Subjective) {
                gradeSubjectiveWithAI(i);
            }
        }
    }
}

// 调用 DeepSeek AI 批改单道题
void ExamProgressPage::gradeSubjectiveWithAI(int index) {
    Question& q = questions[index];
    QString configPath = QCoreApplication::applicationDirPath() + "/algemate_ai.conf";
    QFile file(configPath);
    QString apiKey = "";
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&file);
        QString encoded = in.readLine().trimmed(); // 只读第一行
        if (!encoded.isEmpty()) apiKey = QString(QByteArray::fromBase64(encoded.toUtf8()));
    }

    // 未配置 Key 或未作答的保底直接判 0 分
    if (apiKey.isEmpty() || q.userAnswer.trimmed().isEmpty()) {
        q.isCorrect = false;
        q.earnedScore = 0;
        q.aiReport = q.userAnswer.trimmed().isEmpty() ? QStringLiteral("学生未作答，自动判 0 分。") : QStringLiteral("未检测到 API 凭证，无法出分。");
        m_pendingAITasks--;
        if (m_pendingAITasks == 0) finishExamAndSave();
        return;
    }

    // 强制要求 AI 给出明确的分数结构
    QString prompt = QString(
                         "你是一位严谨的线性代数教授。请对比【参考答案】对【学生作答】进行精确的百分制比例打分和深度批改。\n\n"
                         "【题目内容】:\n%1\n\n"
                         "【本题满分】: %2 分\n\n"
                         "【参考标准答案】:\n%3\n\n"
                         "【学生作答内容】:\n%4\n\n"
                         "请务必严格按照以下规范进行响应：\n"
                         "1. 回复的第一行必须固定返回得分结论，格式为：[得分：X] （X为一个介于 0 到 %2 之间的整数分数）。\n"
                         "2. 随后请分段详细阐述：题目难度、得分点、逻辑断层或失误诊断，并指出优化建议。\n"
                         "3. 讲解中涉及的核心数学表达式请使用标准 LaTeX 格式包裹。"
                         ).arg(q.content).arg(q.score).arg(q.correctAnswer).arg(q.userAnswer);

    QJsonObject rootObj;
    rootObj["model"] = "deepseek-chat";
    rootObj["stream"] = false;
    QJsonArray messages;
    QJsonObject sysMsg, usrMsg;
    sysMsg["role"] = "system"; sysMsg["content"] = "你是专业的线性代数阅卷官，擅长给出准确的分数和极具学术价值的评语。";
    usrMsg["role"] = "user"; usrMsg["content"] = prompt;
    messages.append(sysMsg); messages.append(usrMsg);
    rootObj["messages"] = messages;

    QNetworkAccessManager* mgr = new QNetworkAccessManager(this);
    QNetworkRequest request{QUrl(QStringLiteral("https://api.deepseek.com/chat/completions"))};
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization", QString("Bearer %1").arg(apiKey).toUtf8());

    QNetworkReply* reply = mgr->post(request, QJsonDocument(rootObj).toJson());

    connect(reply, &QNetworkReply::finished, this, [this, index, reply, mgr]() {
        mgr->deleteLater(); reply->deleteLater();
        Question& qRef = questions[index];

        if (reply->error() == QNetworkReply::NoError) {
            QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
            QString aiEval = doc.object()["choices"].toArray()[0].toObject()["message"].toObject()["content"].toString();
            qRef.aiReport = aiEval; // 将完整的评阅报告留存入结构体

            // 利用正则表达式提取 AI 分数
            double extractedScore = 0;
            QRegularExpression re(QStringLiteral("\\[得分：(\\d+(?:\\.\\d+)?)\\]"));
            QRegularExpressionMatch match = re.match(aiEval);
            if (match.hasMatch()) {
                extractedScore = match.captured(1).toDouble();
            } else {
                // 备用兜底解析
                QRegularExpression reBackup(QStringLiteral("得分：(\\d+)"));
                QRegularExpressionMatch matchBackup = reBackup.match(aiEval);
                if (matchBackup.hasMatch()) extractedScore = matchBackup.captured(1).toInt();
            }

            qRef.earnedScore = static_cast<int>(extractedScore);

            // 【修改】判定核心：如果分数大于等于 60% 视为通过
            if (qRef.earnedScore >= (qRef.score * 0.6)) {
                qRef.isCorrect = true;
            } else {
                qRef.isCorrect = false;
            }
        } else {
            qRef.aiReport = QStringLiteral("网络超时或判卷通信失败。");
            qRef.earnedScore = 0;
            qRef.isCorrect = false;
        }

        m_pendingAITasks--;
        if (m_pendingAITasks == 0) finishExamAndSave();
    });
}

// 所有判卷任务结束后的统一存档
void ExamProgressPage::finishExamAndSave() {
    this->setEnabled(true);

    QJsonArray historyArray;
    for (const auto& q : questions) {
        QJsonObject obj;
        obj["id"] = q.id;
        obj["content"] = q.content;
        obj["userAnswer"] = q.userAnswer;
        obj["correctAnswer"] = q.correctAnswer;
        obj["isCorrect"] = q.isCorrect;
        obj["score"] = q.score;
        obj["earnedScore"] = q.earnedScore;
        obj["aiReport"] = q.aiReport;
        historyArray.append(obj);
    }

    QJsonObject examObj;
    examObj["examName"] = m_examName; // 【修复】把考试名称存入历史，以便后续查询区分
    examObj["time"] = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");
    examObj["questions"] = historyArray;

    QFile file("exam_history.json");
    QJsonArray allHistory;
    if (file.exists() && file.open(QIODevice::ReadOnly)) {
        allHistory = QJsonDocument::fromJson(file.readAll()).array();
        file.close();
    }

    allHistory.append(examObj);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(QJsonDocument(allHistory).toJson(QJsonDocument::Indented));
        file.close();
    }

    saveWrongQuestions();
    emit examFinished(questions); // 切页前往成绩单
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
        if (m_examTimer) m_examTimer->stop();
        QMessageBox::information(this, QStringLiteral("提示"), QStringLiteral("考试时间已到，自动交卷"));
        emit examFinished(questions);
    }
}

// ==================== ExamResultPage ====================

ExamResultPage::ExamResultPage(const QVector<Question>& results, QWidget* parent)
    : QWidget(parent) {

    auto* lay = new QVBoxLayout(this);
    lay->setContentsMargins(0, 0, 0, 0);
    lay->setSpacing(0);

    const bool dark = AlgeMate::ThemeManager::instance().currentTheme()
                      == AlgeMate::ThemeManager::Theme::Dark;

    // 全局背景色
    setStyleSheet(dark
        ? "QWidget { background-color: #1C1B2E; color: #E6E7F0; font-family: \"Microsoft YaHei\", \"PingFang SC\"; }"
        : "QWidget { background-color: #f3f6fb; font-family: \"Microsoft YaHei\", \"PingFang SC\"; }");

    auto* scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);

    auto* container = new QWidget;
    auto* contentLayout = new QVBoxLayout(container);
    contentLayout->setContentsMargins(32, 24, 32, 40); // 增加四周留白呼吸感
    contentLayout->setSpacing(24);
    scrollArea->setWidget(container);
    lay->addWidget(scrollArea);

    // ==========================================
    // 1. 顶部导航栏 (清爽风格)
    // ==========================================
    auto* top = new QHBoxLayout;

    auto* back = new QPushButton(QStringLiteral("← 返回主页"), this);
    back->setStyleSheet(dark
        ? "QPushButton { background: transparent; color: #C9C9DC; border: 1px solid #3B395A; border-radius: 8px; padding: 8px 16px; font-size: 14px; font-weight: bold; } QPushButton:hover { background: #28263F; color: #6F77FF; border-color: #6F77FF; }"
        : "QPushButton { background: transparent; color: #64748b; border: 1px solid #cbd5e1; border-radius: 8px; padding: 8px 16px; font-size: 14px; font-weight: bold; } QPushButton:hover { background: #f1f5f9; color: #3b82f6; border-color: #93c5fd; }");
    connect(back, &QPushButton::clicked, this, &ExamResultPage::backRequested);

    auto* title = new QLabel(QStringLiteral("考试成绩单"));
    title->setStyleSheet(dark ? "font-size: 22px; font-weight: 800; color: #E6E7F0;"
                              : "font-size: 22px; font-weight: 800; color: #1e293b;");

    auto* restartBtn = new QPushButton(QStringLiteral("↻ 重新考试"), this);
    restartBtn->setStyleSheet(dark
        ? "QPushButton { background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #6F77FF, stop:1 #5563E8); color: white; border-radius: 8px; padding: 8px 20px; font-size: 14px; font-weight: bold; } QPushButton:hover { background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #8FA1FF, stop:1 #6F77FF); }"
        : "QPushButton { background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #3b82f6, stop:1 #2563eb); color: white; border-radius: 8px; padding: 8px 20px; font-size: 14px; font-weight: bold; } QPushButton:hover { background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #60a5fa, stop:1 #3b82f6); }");
    connect(restartBtn, &QPushButton::clicked, this, [this]() {
        if (auto* examPage = qobject_cast<ExamPage*>(this->parentWidget())) {
            examPage->resetToSettings();
        }
    });

    top->addWidget(back);
    top->addStretch();
    top->addWidget(title);
    top->addStretch();
    top->addWidget(restartBtn);
    contentLayout->addLayout(top);

    // ==========================================
    // 2. 成绩汇总卡片 (高级渐变 + 核心数据展示)
    // ==========================================
    int totalScore = 0;
    int earnedScore = 0;
    for (const auto& q : results) {
        totalScore += q.score;
        earnedScore += q.earnedScore; // 严格取实际得分
    }

    auto* scoreCard = new QFrame(container);
    scoreCard->setStyleSheet(dark
        ? "QFrame { background: qlineargradient(x1:0, y1:0, x2:1, y2:1, stop:0 #28263F, stop:1 #1F1E33); border-radius: 24px; border: 1px solid #3B395A; }"
        : "QFrame { background: qlineargradient(x1:0, y1:0, x2:1, y2:1, stop:0 #ffffff, stop:1 #f8fafc); border-radius: 24px; border: 1px solid #e2e8f0; }");

    auto* scoreLayout = new QVBoxLayout(scoreCard);
    scoreLayout->setContentsMargins(40, 40, 40, 40);
    scoreLayout->setSpacing(12);

    auto* scoreLabel = new QLabel(QStringLiteral("%1<span style='font-size:24px; color:#94a3b8;'> / %2分</span>")
                                      .arg(earnedScore).arg(totalScore), this);
    scoreLabel->setTextFormat(Qt::RichText);
    scoreLabel->setStyleSheet(dark ? "font-size: 64px; font-weight: 900; color: #8FA1FF;"
                                   : "font-size: 64px; font-weight: 900; color: #2563eb;");
    scoreLabel->setAlignment(Qt::AlignCenter);

    double percentage = (totalScore > 0) ? (earnedScore * 100.0 / totalScore) : 0;
    QString level = (percentage >= 90) ? "🏆 极具天赋，继续保持！" :
                        (percentage >= 75) ? "🌟 掌握良好，大有可为！" :
                        (percentage >= 60) ? "✅ 顺利及格，仍需巩固。" : "💪 暂未通过，请查漏补缺。";

    auto* levelLabel = new QLabel(level, this);
    levelLabel->setAlignment(Qt::AlignCenter);
    levelLabel->setStyleSheet(QString("font-size: 18px; font-weight: bold; color: %1;")
                                  .arg(percentage >= 60 ? (dark ? "#34d399" : "#10b981")
                                                        : (dark ? "#fbbf24" : "#f59e0b")));

    scoreLayout->addWidget(scoreLabel);
    scoreLayout->addWidget(levelLabel);
    contentLayout->addWidget(scoreCard);

    // ==========================================
    // 3. 逐题解析列表 (精致化卡片流)
    // ==========================================
    auto* resultsWidget = new QWidget;
    auto* resultsLayout = new QVBoxLayout(resultsWidget);
    resultsLayout->setSpacing(20); // 卡片间距

    for (int i = 0; i < results.size(); ++i) {
        const auto& q = results[i];

        auto* itemFrame = new QFrame(container);
        itemFrame->setStyleSheet(dark
            ? "QFrame { background: #28263F; border-radius: 20px; border: 1px solid #3B395A; } QFrame:hover { border: 1px solid #6F77FF; background: #312F4A; }"
            : "QFrame { background: white; border-radius: 20px; border: 1px solid #e2e8f0; } QFrame:hover { border: 1px solid #bfdbfe; background: #fdfeff; }");

        auto* itemLayout = new QVBoxLayout(itemFrame);
        itemLayout->setContentsMargins(24, 20, 24, 20);
        itemLayout->setSpacing(12);

        // --- 头部：题型、序号、对错徽章、具体得分 ---
        QString typeStr = (q.type == QuestionType::Single) ? QStringLiteral("单选题") :
                              (q.type == QuestionType::Fill) ? QStringLiteral("填空题") : QStringLiteral("解答题");

        QString statusBadge = q.isCorrect
                                  ? QStringLiteral("<span style='background-color:#d1fae5; color:#059669; padding:2px 8px; border-radius:4px;'>✓ 正确</span>")
                                  : QStringLiteral("<span style='background-color:#fee2e2; color:#dc2626; padding:2px 8px; border-radius:4px;'>✗ 错误</span>");

        // 【核心修复】显示具体得分
        auto* titleLabel = new QLabel(
            QStringLiteral("<span style='color:#64748b;'>[%1]</span> <b>第 %2 题</b> &nbsp;&nbsp; %3 &nbsp;&nbsp; <span style='color:#3b82f6;'>得分: %4/%5 分</span>")
                .arg(typeStr).arg(i + 1).arg(statusBadge).arg(q.earnedScore).arg(q.score), this);
        titleLabel->setTextFormat(Qt::RichText);
        titleLabel->setStyleSheet(dark ? "font-size: 16px; color: #E6E7F0;"
                                       : "font-size: 16px; color: #1e293b;");
        itemLayout->addWidget(titleLabel);

        // --- 题干内容 (支持 LaTeX 渲染) ---
        auto* contentBrowser = new Latex::LatexTextBrowser(this);
        contentBrowser->setStyleSheet(dark
            ? "LatexTextBrowser { background: transparent; border: none; color: #C9C9DC; font-size: 15px; margin-top: 4px; margin-bottom: 8px; }"
            : "LatexTextBrowser { background: transparent; border: none; color: #475569; font-size: 15px; margin-top: 4px; margin-bottom: 8px; }");
        Latex::LatexRenderer renderer;
        renderer.setTextColor(dark ? QColor("#F3F3FA") : QColor("#1F2033"));
        renderer.addMathMacro(QStringLiteral("F"),  QStringLiteral("\\mathbb{F}"));
        renderer.addMathMacro(QStringLiteral("R"),  QStringLiteral("\\mathbb{R}"));
        renderer.addMathMacro(QStringLiteral("C"),  QStringLiteral("\\mathbb{C}"));
        contentBrowser->setHtml(renderer.render(q.content, contentBrowser->document()));

        itemLayout->addWidget(contentBrowser);

        // --- 作答与标准答案比对区 (灰色轻背景包裹) ---
        auto* answerBox = new QFrame;
        answerBox->setStyleSheet(dark ? "background: #1F1E33; border-radius: 12px; padding: 12px; border: 1px solid #3B395A;"
                                      : "background: #f8fafc; border-radius: 12px; padding: 12px; border: 1px solid #f1f5f9;");
        auto* ansLayout = new QVBoxLayout(answerBox);
        ansLayout->setContentsMargins(12, 12, 12, 12);
        ansLayout->setSpacing(8);

        // 格式化选项 (A/B/C/D)
        QString displayUserAns = q.userAnswer;
        QString displayCorrectAns = q.correctAnswer;
        if (q.type == QuestionType::Single) {
            if (!displayUserAns.isEmpty()) displayUserAns = QString(QChar('A' + displayUserAns.toInt()));
            if (!displayCorrectAns.isEmpty()) displayCorrectAns = QString(QChar('A' + displayCorrectAns.toInt()));
        }
        if (displayUserAns.trimmed().isEmpty()) displayUserAns = QStringLiteral("<span style='color:#94a3b8; font-style:italic;'>未作答</span>");

        // auto* answerLabel = new QLabel(QStringLiteral("<b>您的作答：</b>%1").arg(displayUserAns), this);
        // answerLabel->setWordWrap(true);
        // answerLabel->setTextFormat(Qt::RichText);
        // answerLabel->setStyleSheet(QString("font-size: 14px; %1").arg(q.isCorrect ? "color: #059669;" : "color: #dc2626;"));

        // auto* correctLabel = new QLabel(QStringLiteral("<b>标准答案：</b><span style='color:#059669;'>%1</span>").arg(displayCorrectAns), this);
        // correctLabel->setWordWrap(true);
        // correctLabel->setTextFormat(Qt::RichText);
        // correctLabel->setStyleSheet("font-size: 14px; color: #334155;");

        // ansLayout->addWidget(answerLabel);
        // ansLayout->addWidget(correctLabel);

        //为作答和答案应用 LaTeX 渲染
        // Latex::LatexRenderer renderer;
        renderer.addMathMacro(QStringLiteral("F"),  QStringLiteral("\\mathbb{F}"));
        renderer.addMathMacro(QStringLiteral("R"),  QStringLiteral("\\mathbb{R}"));
        renderer.addMathMacro(QStringLiteral("C"),  QStringLiteral("\\mathbb{C}"));

        // 渲染
        auto* answerBrowser = new Latex::LatexTextBrowser(this);
        QString ansColor = q.isCorrect ? (dark ? "#34d399" : "#059669")
                                       : (dark ? "#f87171" : "#dc2626");
        answerBrowser->setStyleSheet(QString(R"(
            LatexTextBrowser {
                background: transparent;
                border: none;
                font-size: 14px;
                color: %1;
            }
        )").arg(ansColor));

        // 处理可能遗留的字面量换行符
        displayUserAns.replace("\\n", "<br>").replace("\n", "<br>");
        QString userHtml = QStringLiteral("<b>您的作答：</b><br>") + displayUserAns;
        answerBrowser->setHtml(renderer.render(userHtml, answerBrowser->document()));

        // 渲染标准答案
        auto* correctBrowser = new Latex::LatexTextBrowser(this);
        correctBrowser->setStyleSheet(dark
            ? "LatexTextBrowser { background: transparent; border: none; font-size: 14px; color: #34d399; }"
            : "LatexTextBrowser { background: transparent; border: none; font-size: 14px; color: #059669; }");

        displayCorrectAns.replace("\\n", "<br>").replace("\n", "<br>");
        QString correctHtml = QStringLiteral("<b style='color:#334155;'>标准答案：</b><br>") + displayCorrectAns;
        correctBrowser->setHtml(renderer.render(correctHtml, correctBrowser->document()));

        ansLayout->addWidget(answerBrowser);
        ansLayout->addWidget(correctBrowser);
        itemLayout->addWidget(answerBox);

        // --- AI 详情按钮 (解答题强制显示，兜底空报告) ---
        if (q.type == QuestionType::Subjective) {
            auto* detailLayout = new QHBoxLayout();
            detailLayout->addStretch();

            auto* detailBtn = new QPushButton(QStringLiteral("🤖 查看 AI 导师评阅详情"), itemFrame);
            detailBtn->setCursor(Qt::PointingHandCursor);
            detailBtn->setStyleSheet(dark
                ? "QPushButton { background: #312F4A; color: #8FA1FF; border: 1px solid #3B395A; border-radius: 8px; padding: 8px 16px; font-size: 13px; font-weight: bold; } QPushButton:hover { background: #3B395A; color: #B0BBFF; }"
                : "QPushButton { background: #eff6ff; color: #2563eb; border: 1px solid #bfdbfe; border-radius: 8px; padding: 8px 16px; font-size: 13px; font-weight: bold; } QPushButton:hover { background: #dbeafe; color: #1d4ed8; }");

            // 兜底：如果没作答或由于网络断开导致 aiReport 为空，给一个默认提示
            QString currentReport = q.aiReport.trimmed().isEmpty()
                                        ? QStringLiteral("### 🤖 智能评阅提醒\n\n系统未检测到您的作答，或网络连接超时。\n建议您下次作答完毕后再提交，以便 AI 导师为您提供详尽的解题思路和失分分析。")
                                        : q.aiReport;

            connect(detailBtn, &QPushButton::clicked, this, [this, currentReport]() {
                const bool dlgDark = AlgeMate::ThemeManager::instance().currentTheme()
                                     == AlgeMate::ThemeManager::Theme::Dark;
                QDialog* dlg = new QDialog(this);
                dlg->setWindowTitle(QStringLiteral("🤖 DeepSeek 智能导师评阅报告"));
                dlg->setMinimumSize(650, 500);
                dlg->setStyleSheet(dlgDark ? "QDialog { background-color: #1C1B2E; }"
                                           : "QDialog { background-color: #f8fafc; }");

                auto* dLayout = new QVBoxLayout(dlg);
                auto* textBrowser = new Latex::LatexTextBrowser(dlg);
                textBrowser->setStyleSheet(dlgDark
                    ? "LatexTextBrowser { background: #28263F; border: 1px solid #3B395A; border-radius: 12px; padding: 16px; font-family: 'Microsoft YaHei'; font-size: 14px; color: #E6E7F0; line-height: 1.6; }"
                    : "LatexTextBrowser { background: white; border: 1px solid #e2e8f0; border-radius: 12px; padding: 16px; font-family: 'Microsoft YaHei'; font-size: 14px; color: #334155; line-height: 1.6; }");

                Latex::LatexRenderer renderer;
                renderer.setTextColor(dlgDark ? QColor("#F3F3FA") : QColor("#1F2033"));
                renderer.addMathMacro(QStringLiteral("F"),  QStringLiteral("\\mathbb{F}"));
                renderer.addMathMacro(QStringLiteral("R"),  QStringLiteral("\\mathbb{R}"));
                renderer.addMathMacro(QStringLiteral("C"),  QStringLiteral("\\mathbb{C}"));

                // textBrowser->setMarkdown(currentReport);
                textBrowser->setHtml(renderer.render(currentReport, textBrowser->document()));
                dLayout->addWidget(textBrowser);
                dlg->exec();
                dlg->deleteLater();
            });

            detailLayout->addWidget(detailBtn);
            itemLayout->addLayout(detailLayout);
        }

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


}


void ExamPage::onStartExam(int timeMinutes, int examIndex, const QString& examName) {
    auto* oldPage = currentPage;
    auto* layout = this->layout();
    layout->removeWidget(oldPage);
    oldPage->deleteLater();

    // 动态从真题库获取试卷
    QVector<Question> examQuestions = PastExams::getExamPaper(examIndex);

    auto* progressPage = new ExamProgressPage(examQuestions, timeMinutes, examName, this);
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

void ExamPage::resetToSettings() {
    // 1. 彻底移除并销毁当前的页面（比如考试结果页）
    if (currentPage) {
        auto* layout = this->layout();
        layout->removeWidget(currentPage);
        currentPage->deleteLater();
        currentPage = nullptr;
    }

    // 2. 重新创建全新的考试设置页
    auto* settingPage = new ExamSettingPage(this);
    connect(settingPage, &ExamSettingPage::backRequested, this, &ExamPage::backRequested);
    connect(settingPage, &ExamSettingPage::examStarted, this, &ExamPage::onStartExam);

    currentPage = settingPage;
    this->layout()->addWidget(currentPage);
}

} // namespace AlgeMate::Learning