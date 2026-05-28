#include "WrongBookPage.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QScrollArea>
#include <QFrame>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QPlainTextEdit>
#include <QMessageBox>

    namespace AlgeMate::Learning {

    static QPushButton* makeBackBtn(QWidget* parent = nullptr) {
        auto* btn = new QPushButton(QStringLiteral("← 返回"), parent);
        btn->setObjectName(QStringLiteral("LearnBackBtn"));
        return btn;
    }

    WrongBookPage::WrongBookPage(QWidget* parent)
        : QWidget(parent)
    {
        setStyleSheet(R"(
        QWidget {
            background-color: #f5f7fb;
            font-family: "Microsoft YaHei";
        }
    )");

        auto* mainLayout = new QVBoxLayout(this);
        mainLayout->setContentsMargins(24, 16, 24, 24);
        mainLayout->setSpacing(16);

        // 顶部
        auto* top = new QHBoxLayout;

        auto* back = makeBackBtn(this);
        connect(back,
                &QPushButton::clicked,
                this,
                &WrongBookPage::backRequested);

        auto* title = new QLabel(QStringLiteral("错题本"), this);

        title->setStyleSheet(R"(
        font-size: 30px;
        font-weight: 800;
        color: #111827;
    )");

        top->addWidget(back);
        top->addWidget(title);
        top->addStretch();

        // 滚动区域
        auto* scrollArea = new QScrollArea(this);
        scrollArea->setWidgetResizable(true);
        scrollArea->setFrameShape(QFrame::NoFrame);

        auto* container = new QWidget;

        contentLayout = new QVBoxLayout(container);
        contentLayout->setContentsMargins(0, 0, 0, 0);
        contentLayout->setSpacing(18);

        scrollArea->setWidget(container);

        mainLayout->addLayout(top);
        mainLayout->addWidget(scrollArea, 1);

        loadWrongQuestions();
    }

    void WrongBookPage::loadWrongQuestions()
    {
        QFile file("wrong_questions.json");

        if (!file.exists()) {

            auto* emptyLabel = new QLabel(
                QStringLiteral("🎉 当前还没有错题记录"),
                this);

            emptyLabel->setAlignment(Qt::AlignCenter);

            emptyLabel->setStyleSheet(R"(
            font-size: 22px;
            color: #6b7280;
            padding: 80px;
        )");

            contentLayout->addWidget(emptyLabel);
            return;
        }

        if (!file.open(QIODevice::ReadOnly)) {

            QMessageBox::warning(this,
                                 QStringLiteral("错误"),
                                 QStringLiteral("无法打开错题文件"));
            return;
        }

        QJsonDocument doc =
            QJsonDocument::fromJson(file.readAll());

        file.close();

        QJsonArray arr = doc.array();

        if (arr.isEmpty()) {

            auto* emptyLabel = new QLabel(
                QStringLiteral("🎉 当前还没有错题记录"),
                this);

            emptyLabel->setAlignment(Qt::AlignCenter);

            emptyLabel->setStyleSheet(R"(
            font-size: 22px;
            color: #6b7280;
            padding: 80px;
        )");

            contentLayout->addWidget(emptyLabel);
            return;
        }

        for (int i = arr.size() - 1; i >= 0; --i) {

            QJsonObject obj = arr[i].toObject();

            Question q;

            q.content = obj["content"].toString();
            q.userAnswer = obj["userAnswer"].toString();
            q.correctAnswer = obj["correctAnswer"].toString();
            q.score = obj["score"].toInt();

            QString type = obj["type"].toString();

            if (type == "single") {
                q.type = QuestionType::Single;
            } else if (type == "fill") {
                q.type = QuestionType::Fill;
            } else {
                q.type = QuestionType::Subjective;
            }

            QJsonArray choicesArray = obj["choices"].toArray();

            for (const auto& v : choicesArray) {
                q.choices.append(v.toString());
            }

            addWrongQuestionCard(q,
                                 obj["time"].toString(),
                                 obj["wrongCount"].toInt());
        }

        contentLayout->addStretch();
    }

    void WrongBookPage::addWrongQuestionCard(const Question& q,
                                             const QString& time,
                                             int wrongCount)
    {
        auto* card = new QFrame(this);

        card->setStyleSheet(R"(
        QFrame {
            background: white;
            border-radius: 22px;
            border: 1px solid #e5e7eb;
        }
    )");

        auto* lay = new QVBoxLayout(card);

        lay->setContentsMargins(24, 24, 24, 24);
        lay->setSpacing(14);

        QString typeText;

        if (q.type == QuestionType::Single) {
            typeText = QStringLiteral("单选题");
        } else if (q.type == QuestionType::Fill) {
            typeText = QStringLiteral("填空题");
        } else {
            typeText = QStringLiteral("解答题");
        }

        auto* topRow = new QHBoxLayout;

        auto* typeLabel = new QLabel(
            QStringLiteral("[%1]").arg(typeText),
            this);

        typeLabel->setStyleSheet(R"(
        background: #dbeafe;
        color: #2563eb;
        border-radius: 10px;
        padding: 6px 12px;
        font-weight: bold;
        font-size: 13px;
    )");

        auto* countLabel = new QLabel(
            QStringLiteral("错误次数：%1").arg(wrongCount),
            this);

        countLabel->setStyleSheet(R"(
        color: #ef4444;
        font-size: 14px;
        font-weight: 600;
    )");

        auto* timeLabel = new QLabel(time, this);

        timeLabel->setStyleSheet(R"(
        color: #6b7280;
        font-size: 13px;
    )");

        topRow->addWidget(typeLabel);
        topRow->addSpacing(10);
        topRow->addWidget(countLabel);
        topRow->addStretch();
        topRow->addWidget(timeLabel);

        auto* questionLabel = new QLabel(
            QStringLiteral("题目：%1").arg(q.content),
            this);

        questionLabel->setWordWrap(true);

        questionLabel->setStyleSheet(R"(
        font-size: 17px;
        color: #111827;
        line-height: 1.8;
        font-weight: 600;
    )");

        lay->addLayout(topRow);
        lay->addWidget(questionLabel);

        if (q.type == QuestionType::Single) {

            auto* optionsFrame = new QFrame(this);

            optionsFrame->setStyleSheet(R"(
            QFrame {
                background: #f9fafb;
                border-radius: 14px;
                border: 1px solid #e5e7eb;
            }
        )");

            auto* optionsLayout = new QVBoxLayout(optionsFrame);

            for (int i = 0; i < q.choices.size(); ++i) {

                auto* optionLabel = new QLabel(
                    QStringLiteral("%1. %2")
                        .arg(QChar('A' + i))
                        .arg(q.choices[i]),
                    this);

                optionLabel->setStyleSheet(R"(
                font-size: 14px;
                color: #374151;
                padding: 4px;
            )");

                optionsLayout->addWidget(optionLabel);
            }

            lay->addWidget(optionsFrame);
        }

        auto* userAnswerLabel = new QLabel(
            QStringLiteral("你的答案：%1")
                .arg(q.userAnswer.isEmpty()
                         ? QStringLiteral("未作答")
                         : q.userAnswer),
            this);

        userAnswerLabel->setWordWrap(true);

        userAnswerLabel->setStyleSheet(R"(
        color: #2563eb;
        font-size: 15px;
        font-weight: 600;
    )");

        auto* correctLabel = new QLabel(
            QStringLiteral("正确答案：%1")
                .arg(q.correctAnswer),
            this);

        correctLabel->setWordWrap(true);

        correctLabel->setStyleSheet(R"(
        color: #10b981;
        font-size: 15px;
        font-weight: 600;
    )");

        lay->addWidget(userAnswerLabel);
        lay->addWidget(correctLabel);

        if (q.type == QuestionType::Subjective) {

            auto* analysisBox = new QPlainTextEdit(this);

            analysisBox->setReadOnly(true);

            analysisBox->setPlainText(q.correctAnswer);

            analysisBox->setMinimumHeight(120);

            analysisBox->setStyleSheet(R"(
            QPlainTextEdit {
                background: #f9fafb;
                border-radius: 14px;
                border: 1px solid #e5e7eb;
                padding: 10px;
                font-size: 14px;
            }
        )");

            lay->addWidget(analysisBox);
        }

        auto* retryBtn = new QPushButton(QStringLiteral("重新练习"), this);

        retryBtn->setFixedHeight(42);

        retryBtn->setStyleSheet(R"(
        QPushButton {
            background: #2563eb;
            color: white;
            border-radius: 12px;
            font-size: 15px;
            font-weight: bold;
            padding: 8px 18px;
        }

        QPushButton:hover {
            background: #1d4ed8;
        }
    )");

        connect(retryBtn,
                &QPushButton::clicked,
                this,
                [this]() {

                    QMessageBox::information(this,
                                             QStringLiteral("提示"),
                                             QStringLiteral("后续可扩展为重新进入练习模式"));
                });

        lay->addWidget(retryBtn);

        contentLayout->addWidget(card);
    }

} // namespace AlgeMate::Learning
