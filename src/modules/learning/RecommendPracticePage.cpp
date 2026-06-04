#include "RecommendPracticePage.h"
#include "latex/LatexRenderer.h"
#include "latex/LatexTextBrowser.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QScrollArea>
#include <QMessageBox>
#include <QFile>
#include <QSettings>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QDateTime>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QCoreApplication>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>

namespace AlgeMate::Learning {

RecommendPracticePage::RecommendPracticePage(QWidget* parent) : QWidget(parent) {
    auto* renderer = new Latex::LatexRenderer;
    renderer->addMathMacro(QStringLiteral("F"),  QStringLiteral("\\mathbb{F}"));
    renderer->addMathMacro(QStringLiteral("R"),  QStringLiteral("\\mathbb{R}"));
    renderer->addMathMacro(QStringLiteral("C"),  QStringLiteral("\\mathbb{C}"));
    this->setProperty("latex_renderer", QVariant::fromValue(static_cast<void*>(renderer)));

    auto* mainLay = new QVBoxLayout(this);
    mainLay->setContentsMargins(0, 0, 0, 0);

    m_internalStack = new QStackedWidget(this);
    mainLay->addWidget(m_internalStack);

    // ================= 页面 0：批次列表页 =================
    m_batchListPage = new QWidget(this);
    auto* listPageLayout = new QVBoxLayout(m_batchListPage);
    listPageLayout->setContentsMargins(24, 16, 24, 24);

    // 列表页顶栏
    auto* topListLayout = new QHBoxLayout();
    auto* backToLearningBtn = new QPushButton(QStringLiteral("← 返回学习中心"), this);
    backToLearningBtn->setStyleSheet("background: transparent; border: 1px solid #cbd5e0; padding: 6px 12px; border-radius: 6px; color: #4a5568;");
    connect(backToLearningBtn, &QPushButton::clicked, this, &RecommendPracticePage::backRequested);

    auto* titleLabel = new QLabel(QStringLiteral("🎯 推荐练习历史记录"), this);
    titleLabel->setStyleSheet("font-size: 20px; font-weight: bold; color: #2c3e50;");

    auto* regenBtn = new QPushButton(QStringLiteral("➕ 追加生成新题"), this);
    regenBtn->setStyleSheet("background: #ebf8ff; color: #3182ce; border: 1px solid #bee3f8; padding: 6px 12px; border-radius: 6px; font-weight: bold;");
    connect(regenBtn, &QPushButton::clicked, this, [this]() {
        emit requestRegenerate();
    });

    topListLayout->addWidget(backToLearningBtn);
    topListLayout->addSpacing(16);
    topListLayout->addWidget(titleLabel);
    topListLayout->addStretch();
    topListLayout->addWidget(regenBtn);
    listPageLayout->addLayout(topListLayout);

    // 列表滚动区
    auto* scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    scrollArea->setStyleSheet("QScrollArea { border: none; background: #f8fafc; border-radius: 16px; }");
    auto* scrollWidget = new QWidget();
    scrollWidget->setStyleSheet("background: transparent;");
    m_batchListLayout = new QVBoxLayout(scrollWidget);
    m_batchListLayout->setAlignment(Qt::AlignTop);
    scrollArea->setWidget(scrollWidget);
    listPageLayout->addWidget(scrollArea);

    m_internalStack->addWidget(m_batchListPage);


    // ================= 页面 1：具体做题页 =================
    m_practicePage = new QWidget(this);
    auto* practiceLayout = new QVBoxLayout(m_practicePage);
    practiceLayout->setContentsMargins(24, 16, 24, 24);
    practiceLayout->setSpacing(12);

    // 做题页顶栏
    auto* pracTopLayout = new QHBoxLayout();
    auto* backToListBtn = new QPushButton(QStringLiteral("🔙 返回列表"), this);
    backToListBtn->setStyleSheet("background: transparent; border: 1px solid #cbd5e0; padding: 6px 12px; border-radius: 6px; color: #4a5568;");
    connect(backToListBtn, &QPushButton::clicked, this, &RecommendPracticePage::backToList);

    m_progressLabel = new QLabel(this);
    m_progressLabel->setStyleSheet("color: #4a5568; font-size: 13px; background-color: #edf2f7; padding: 8px 12px; border-radius: 6px; font-weight: 500;");

    pracTopLayout->addWidget(backToListBtn);
    pracTopLayout->addSpacing(16);
    pracTopLayout->addWidget(m_progressLabel);
    pracTopLayout->addStretch();
    practiceLayout->addLayout(pracTopLayout);

    // 做题核心滚动区
    auto* pScrollContainer = new QWidget(this);
    pScrollContainer->setStyleSheet("background: #f8fafc; border-radius: 16px;");
    auto* pScrollLay = new QVBoxLayout(pScrollContainer);
    pScrollLay->setContentsMargins(0, 4, 0, 4);
    pScrollLay->setSpacing(16);

    auto* qBrowser = new Latex::LatexTextBrowser(pScrollContainer);
    qBrowser->setFrameShape(QFrame::NoFrame);
    qBrowser->setMinimumHeight(160);
    qBrowser->setStyleSheet("background: white; border: 1px solid #e5e7eb; border-radius: 16px; padding: 20px; font-size:15px; color:#3f3a52;");
    m_questionLabel = reinterpret_cast<QLabel*>(qBrowser);
    pScrollLay->addWidget(qBrowser, 0);

    m_answerWidget = new QWidget(pScrollContainer);
    auto* answerLayout = new QVBoxLayout(m_answerWidget);
    answerLayout->setContentsMargins(0, 0, 0, 0);
    pScrollLay->addWidget(m_answerWidget, 0);

    auto* feedbackBrowser = new Latex::LatexTextBrowser(pScrollContainer);
    feedbackBrowser->setMinimumHeight(160);
    feedbackBrowser->hide();
    m_feedbackLabel = reinterpret_cast<QLabel*>(feedbackBrowser);
    pScrollLay->addWidget(feedbackBrowser, 0);
    pScrollLay->addStretch(1);

    auto* mainScroll = new QScrollArea(this);
    mainScroll->setWidgetResizable(true);
    mainScroll->setFrameShape(QFrame::NoFrame);
    mainScroll->setWidget(pScrollContainer);
    practiceLayout->addWidget(mainScroll, 1);

    // 底部按钮
    auto* bottomLayout = new QHBoxLayout;
    auto* prevBtn = new QPushButton(QStringLiteral("← 上一题"), this);
    auto* submitBtn = new QPushButton(QStringLiteral("确认提交"), this);
    auto* nextBtn = new QPushButton(QStringLiteral("下一题 →"), this);
    QString baseBtnStyle = "QPushButton { padding: 8px 16px; font-size: 13px; border-radius: 6px; font-weight: 500; }";
    prevBtn->setStyleSheet(baseBtnStyle + "background-color: #ffffff; border: 1px solid #cbd5e0; color: #4a5568;");
    nextBtn->setStyleSheet(baseBtnStyle + "background-color: #ffffff; border: 1px solid #cbd5e0; color: #4a5568;");
    submitBtn->setStyleSheet(baseBtnStyle + "background-color: #3182ce; color: white; padding: 10px 24px; font-size: 14px;");

    connect(prevBtn, &QPushButton::clicked, this, &RecommendPracticePage::onPreviousQuestion);
    connect(submitBtn, &QPushButton::clicked, this, &RecommendPracticePage::onSubmitAnswer);
    connect(nextBtn, &QPushButton::clicked, this, &RecommendPracticePage::onNextQuestion);

    bottomLayout->addWidget(prevBtn); bottomLayout->addStretch();
    bottomLayout->addWidget(submitBtn); bottomLayout->addWidget(nextBtn);
    practiceLayout->addLayout(bottomLayout);

    m_internalStack->addWidget(m_practicePage);
    m_internalStack->setCurrentWidget(m_batchListPage);
}

void RecommendPracticePage::showLoadingPlaceholder() {
    m_internalStack->setCurrentWidget(m_batchListPage);

    // 如果原来提示"暂无数据"的文本在，先隐藏掉，防止挤占空间
    for (int i = 0; i < m_batchListLayout->count(); ++i) {
        QWidget* w = m_batchListLayout->itemAt(i)->widget();
        if (w && qobject_cast<QLabel*>(w)) {
            w->hide();
            w->deleteLater();
        }
    }

    auto* loadingBtn = new QPushButton(this);
    loadingBtn->setText(QStringLiteral("⏳ AI 正在为您生成专属练习题集，请稍候..."));
    loadingBtn->setStyleSheet("text-align: left; padding: 18px; font-size: 16px; background: #fffbeb; border: 1px dashed #f6e05e; border-radius: 8px; margin-bottom: 8px; color: #b7791f; font-weight: bold;");
    loadingBtn->setDisabled(true);

    // 强制插入到列表的最顶部
    m_batchListLayout->insertWidget(0, loadingBtn);
}

void RecommendPracticePage::reloadQuestions() {
    QFile file("recommended_questions.json");
    if (!file.open(QIODevice::ReadOnly)) return;

    m_allBatches = QJsonDocument::fromJson(file.readAll()).array();
    file.close();

    // 清空旧的列表 UI
    QLayoutItem* item;
    while ((item = m_batchListLayout->takeAt(0)) != nullptr) {
        if (item->widget()) item->widget()->deleteLater();
        delete item;
    }

    int validBatchCount = 0;

    for (int i = m_allBatches.size() - 1; i >= 0; --i) {
        QJsonObject batchObj = m_allBatches[i].toObject();
        QJsonArray qArr = batchObj["questions"].toArray();
        int total = qArr.size();

        if (total == 0) continue;

        validBatchCount++;

        QString timeStr = batchObj["batchTime"].toString();
        QDateTime qTime = QDateTime::fromString(timeStr, Qt::ISODate);
        QString displayTime = qTime.toString("yyyy-MM-dd HH:mm");

        int completed = 0;
        for (const auto& q : qArr) {
            if (q.toObject()["isCompleted"].toBool()) completed++;
        }

        auto* batchBtn = new QPushButton(this);
        batchBtn->setText(QStringLiteral("🕒 %1 生成的练习题集  |  进度: %2/%3 (未完成: %4道)")
                              .arg(displayTime).arg(completed).arg(total).arg(total - completed));

        if (completed == total) {
            batchBtn->setStyleSheet("text-align: left; padding: 18px; font-size: 16px; background: #f0fff4; border: 1px solid #c6f6d5; border-radius: 8px; margin-bottom: 8px; color: #38a169; font-weight: bold;");
        } else {
            batchBtn->setStyleSheet("text-align: left; padding: 18px; font-size: 16px; background: white; border: 1px solid #e2e8f0; border-radius: 8px; margin-bottom: 8px; color: #2d3748; font-weight: bold;");
        }

        connect(batchBtn, &QPushButton::clicked, this, [this, i]() {
            loadBatch(i);
        });
        m_batchListLayout->addWidget(batchBtn);
    }

    if (validBatchCount == 0) {
        auto* emptyLbl = new QLabel(QStringLiteral("⚠️ 暂无推荐题目，请点击右上角追加生成。"));
        emptyLbl->setStyleSheet("color: #718096; font-size: 15px; padding: 20px;");
        m_batchListLayout->addWidget(emptyLbl);
    }

    m_internalStack->setCurrentWidget(m_batchListPage);
}

void RecommendPracticePage::loadBatch(int batchIndex) {
    m_currentBatchIndex = batchIndex;
    m_questions.clear();

    QJsonObject batchObj = m_allBatches[batchIndex].toObject();
    QJsonArray qArr = batchObj["questions"].toArray();

    for (const auto& val : qArr) {
        QJsonObject obj = val.toObject();
        Question q;
        q.id = obj["id"].toInt();
        q.content = obj["content"].toString();
        q.correctAnswer = obj["correctAnswer"].toString();
        q.score = obj["score"].toInt(10);
        q.type = obj["type"].toString() == "fill" ? QuestionType::Fill : QuestionType::Subjective;
        q.attempts = obj["isCompleted"].toBool() ? 1 : 0;
        m_questions.append(q);
    }

    m_internalStack->setCurrentWidget(m_practicePage);
    loadQuestion(0);
}

void RecommendPracticePage::backToList() {
    reloadQuestions();
}

void RecommendPracticePage::markCurrentQuestionCompleted() {
    if (m_currentBatchIndex < 0 || m_currentBatchIndex >= m_allBatches.size()) return;

    QJsonObject currentBatch = m_allBatches[m_currentBatchIndex].toObject();
    QJsonArray qArr = currentBatch["questions"].toArray();
    QJsonObject currentQ = qArr[m_currentIndex].toObject();

    currentQ["isCompleted"] = true;
    qArr[m_currentIndex] = currentQ;
    currentBatch["questions"] = qArr;
    m_allBatches[m_currentBatchIndex] = currentBatch;

    QFile file("recommended_questions.json");
    if (file.open(QIODevice::WriteOnly)) {
        file.write(QJsonDocument(m_allBatches).toJson());
        file.close();
    }
}

void RecommendPracticePage::loadQuestion(int index) {
    if (index < 0 || index >= m_questions.size()) return;
    m_currentIndex = index;
    const Question& q = m_questions[index];

    m_progressLabel->setText(QStringLiteral(" 📝 当前进度：第 %1 题 / 共 %2 题   |   尝试次数：%3 次")
                                 .arg(index + 1).arg(m_questions.size()).arg(q.attempts));

    auto* qBrowser = reinterpret_cast<Latex::LatexTextBrowser*>(m_questionLabel);
    auto* renderer = static_cast<Latex::LatexRenderer*>(this->property("latex_renderer").value<void*>());
    if (renderer) qBrowser->setHtml(renderer->render(q.content, qBrowser->document()));
    m_feedbackLabel->hide();

    updateUIForQuestion(q);
}

void RecommendPracticePage::updateUIForQuestion(const Question& q) {
    if (auto* oldLayout = m_answerWidget->layout()) {
        QLayoutItem* child;
        while ((child = oldLayout->takeAt(0)) != nullptr) {
            if (child->widget()) child->widget()->deleteLater();
            delete child;
        }
    } else {
        new QVBoxLayout(m_answerWidget);
    }

    auto* answerLayout = qobject_cast<QVBoxLayout*>(m_answerWidget->layout());
    answerLayout->setContentsMargins(4, 10, 4, 10);
    answerLayout->setSpacing(12);

    if (q.type == QuestionType::Subjective) {
        auto* textEdit = new QPlainTextEdit(m_answerWidget);
        textEdit->setObjectName(QStringLiteral("subjectiveAnswer"));
        textEdit->setPlaceholderText(QStringLiteral("请在此写下您的推导步骤，支持输入 LaTeX..."));
        textEdit->setMinimumHeight(110);
        textEdit->setPlainText(q.userAnswer);
        textEdit->setStyleSheet("font-size: 14px; border: 1px solid #cbd5e0; border-radius: 6px; padding: 8px; background: white;");
        answerLayout->addWidget(textEdit);

        auto* aiGradeBtn = new QPushButton(QStringLiteral("🤖 DeepSeek AI 智能判卷"), m_answerWidget);
        aiGradeBtn->setObjectName(QStringLiteral("InnerAiGradeButton"));
        aiGradeBtn->setStyleSheet("background: #6d5bd0; color: white; padding: 10px; border-radius: 6px; font-weight: bold;");
        connect(aiGradeBtn, &QPushButton::clicked, this, &RecommendPracticePage::onAiGradeSubjective);
        answerLayout->addWidget(aiGradeBtn, 0, Qt::AlignLeft);
    }
}

void RecommendPracticePage::onSubmitAnswer() {
    if (m_currentIndex < 0 || m_currentIndex >= m_questions.size()) return;
    Question& q = m_questions[m_currentIndex];
    q.attempts++;
    QMessageBox::information(this, QStringLiteral("提示"), QStringLiteral("这是主观推荐题，请点击【DeepSeek AI 智能判卷】获取解析。"));
    loadQuestion(m_currentIndex);
}

void RecommendPracticePage::onAiGradeSubjective() {
    if (m_currentIndex < 0 || m_currentIndex >= m_questions.size()) return;
    Question& q = m_questions[m_currentIndex];

    auto* textEdit = m_answerWidget->findChild<QPlainTextEdit*>(QStringLiteral("subjectiveAnswer"));
    if (!textEdit || textEdit->toPlainText().trimmed().isEmpty()) {
        QMessageBox::warning(this, QStringLiteral("提示"), QStringLiteral("请先输入作答内容！"));
        return;
    }
    q.userAnswer = textEdit->toPlainText().trimmed();

    QSettings settings(QStringLiteral("AlgeMate"), QStringLiteral("AlgeMateApp"));
    QString apiKey = settings.value(QStringLiteral("AI/DeepSeekApiKey"), QString()).toString().trimmed();

    if (apiKey.isEmpty()) {
        QMessageBox::critical(this, QStringLiteral("提示"), QStringLiteral("未检测到 DeepSeek API Key，请前往【设置中心】配置。"));
        return;
    }

    displayResult(true, QStringLiteral("⏳ AI 正在批阅中..."));

    QString prompt = QString("请对比【标准答案】对【学生作答】进行精确打分和深度批改。首行输出[结果：通过/不通过]。\n题目:%1\n标准答案:%2\n学生作答:%3")
                         .arg(q.content, q.correctAnswer, q.userAnswer);

    QJsonObject rootObj;
    rootObj["model"] = "deepseek-chat";
    QJsonArray messages;
    QJsonObject sysMsg; sysMsg["role"] = "system"; sysMsg["content"] = "你是高代导师。";
    QJsonObject usrMsg; usrMsg["role"] = "user"; usrMsg["content"] = prompt;
    messages.append(sysMsg); messages.append(usrMsg);
    rootObj["messages"] = messages;

    auto* mgr = new QNetworkAccessManager(this);
    QNetworkRequest req{QUrl(QStringLiteral("https://api.deepseek.com/chat/completions"))};
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    req.setRawHeader("Authorization", QString("Bearer %1").arg(apiKey).toUtf8());

    auto* reply = mgr->post(req, QJsonDocument(rootObj).toJson());
    connect(reply, &QNetworkReply::finished, this, [this, reply, mgr, &q]() {
        mgr->deleteLater(); reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            displayResult(false, QStringLiteral("网络错误，批阅失败。")); return;
        }

        QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
        QString aiEval = doc.object()["choices"].toArray()[0].toObject()["message"].toObject()["content"].toString();

        q.isCorrect = !aiEval.contains(QStringLiteral("结果：不通过"));
        q.attempts++;
        displayResult(q.isCorrect, aiEval);

        if (!q.isCorrect) saveToWrongBook(q);

        markCurrentQuestionCompleted();
        loadQuestion(m_currentIndex);
    });
}

void RecommendPracticePage::displayResult(bool isCorrect, const QString& feedback) {
    m_feedbackLabel->show();
    auto* browser = reinterpret_cast<Latex::LatexTextBrowser*>(m_feedbackLabel);
    auto* renderer = static_cast<Latex::LatexRenderer*>(this->property("latex_renderer").value<void*>());
    if (renderer) browser->setHtml(renderer->render(feedback, browser->document()));
    else browser->setText(feedback);

    if (isCorrect) browser->setStyleSheet("background: #f0fdf4; color: #166534; padding: 14px; border-radius: 8px; border: 1px solid #bbf7d0;");
    else browser->setStyleSheet("background: #fef2f2; color: #991b1b; padding: 14px; border-radius: 8px; border: 1px solid #fecaca;");
}

void RecommendPracticePage::saveToWrongBook(const Question& q) {
    QFile file("wrong_questions.json");
    QJsonArray arr;
    if (file.open(QIODevice::ReadOnly)) { arr = QJsonDocument::fromJson(file.readAll()).array(); file.close(); }

    QJsonObject obj;
    obj["id"] = q.id; obj["content"] = q.content; obj["userAnswer"] = q.userAnswer;
    obj["correctAnswer"] = q.correctAnswer; obj["wrongCount"] = 1; obj["type"] = "subjective";
    obj["time"] = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");

    arr.append(obj);
    if (file.open(QIODevice::WriteOnly)) file.write(QJsonDocument(arr).toJson());
}

void RecommendPracticePage::onNextQuestion() {
    if (m_currentIndex < m_questions.size() - 1) {
        loadQuestion(m_currentIndex + 1);
    } else {
        QMessageBox::information(this, QStringLiteral("提示"), QStringLiteral("已经是当前练习题集的最后一题了！"));
    }
}

void RecommendPracticePage::onPreviousQuestion() {
    if (m_currentIndex > 0) {
        loadQuestion(m_currentIndex - 1);
    } else {
        QMessageBox::information(this, QStringLiteral("提示"), QStringLiteral("已经是当前练习题集的第一题了！"));
    }
}

} // namespace AlgeMate::Learning