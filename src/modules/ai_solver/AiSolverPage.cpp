#include "AiSolverPage.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QTextEdit>
#include <QPushButton>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QUrl>
#include <QFile>
#include <QTextStream>
#include <QCoreApplication>
#include <QMessageBox>
#include <QTextCursor>
#include <QDebug>

#include "latex/LatexRenderer.h"
#include "latex/LatexTextBrowser.h"

namespace AlgeMate::AiSolver {

AiSolverPage::AiSolverPage(QWidget* parent) : QWidget(parent) {
    networkManager_ = std::make_unique<QNetworkAccessManager>(this);

    setupUI();
    loadApiKey();

    // 初始化对话历史
    QJsonObject systemMsg;
    systemMsg["role"] = "system";
    systemMsg["content"] = QStringLiteral("你是一个专业的线性代数和数学学习助手。请用中文回答用户的问题，"
                                          "提供清晰的解释、步骤和例子。");
    chatHistory_.append(systemMsg);

    qDebug() << "AiSolverPage initialized";
}

AiSolverPage::~AiSolverPage() {
    if (currentReply_ && currentReply_->isRunning()) {
        currentReply_->abort();
    }
    saveApiKey();
}

void AiSolverPage::setupUI() {
    mainLayout_ = new QVBoxLayout(this);
    mainLayout_->setContentsMargins(16, 16, 16, 16);
    mainLayout_->setSpacing(12);

    // ===== Title Section =====
    auto* title = new QLabel(QStringLiteral("AI 智能解题"));
    title->setObjectName(QStringLiteral("PageTitle"));
    title->setStyleSheet(QStringLiteral("font-size: 18px; font-weight: bold;"));
    mainLayout_->addWidget(title);

    // ===== API Key Section =====
    auto* apiLayout = new QHBoxLayout;
    auto* apiLabel = new QLabel(QStringLiteral("API Key: "));
    apiLabel->setStyleSheet(QStringLiteral("font-weight: bold;"));

    apiKeyEdit_ = new QLineEdit;
    apiKeyEdit_->setPlaceholderText(QStringLiteral("输入你的 DeepSeek API Key："));
    apiKeyEdit_->setEchoMode(QLineEdit::Password);
    apiKeyEdit_->setObjectName(QStringLiteral("ApiKeyEdit"));
    apiKeyEdit_->setMaximumHeight(32);

    // API Key 变化时实时保存
    connect(apiKeyEdit_, &QLineEdit::textChanged, this, &AiSolverPage::onApiKeyChanged);

    apiLayout->addWidget(apiLabel);
    apiLayout->addWidget(apiKeyEdit_, 1);
    mainLayout_->addLayout(apiLayout);

    // ===== Subtitle =====
    auto* subtitle = new QLabel(QStringLiteral("输入题目，AI 分步讲解与求解"));
    subtitle->setObjectName(QStringLiteral("PageSubtitle"));
    mainLayout_->addWidget(subtitle);

    // ===== Status Label =====
    statusLabel_ = new QLabel(QStringLiteral("就绪"));
    statusLabel_->setStyleSheet(QStringLiteral("color: green; font-size: 11px;"));
    mainLayout_->addWidget(statusLabel_);

    // ===== Messages Display =====
    resultEdit_ = new QTextEdit;
    resultEdit_->setReadOnly(true);
    resultEdit_->setObjectName(QStringLiteral("ResultEdit"));
    resultEdit_->setMinimumHeight(300);
    resultEdit_->setStyleSheet(
        QStringLiteral("QTextEdit { background-color: #f5f5f5; border: 1px solid #ddd; "
                       "border-radius: 4px; padding: 8px; }")
        );
    resultEdit_->setPlaceholderText(QStringLiteral("对话内容将显示在这里..."));
    mainLayout_->addWidget(resultEdit_, 1);

    // ===== Input Section =====
    auto* inputLayout = new QHBoxLayout;
    inputLayout->setContentsMargins(0, 0, 0, 0);
    inputLayout->setSpacing(8);

    inputEdit_ = new QLineEdit;
    inputEdit_->setPlaceholderText(QStringLiteral("输入题目... (按 Enter 发送)"));
    inputEdit_->setObjectName(QStringLiteral("InputEdit"));
    inputEdit_->setMinimumHeight(40);
    connect(inputEdit_, &QLineEdit::returnPressed,
            this, &AiSolverPage::onSendButtonClicked);

    sendButton_ = new QPushButton(QStringLiteral("发送"));
    sendButton_->setObjectName(QStringLiteral("SendButton"));
    sendButton_->setMinimumHeight(40);
    sendButton_->setMaximumWidth(100);
    sendButton_->setCursor(Qt::PointingHandCursor);
    connect(sendButton_, &QPushButton::clicked,
            this, &AiSolverPage::onSendButtonClicked);

    inputLayout->addWidget(inputEdit_, 1);
    inputLayout->addWidget(sendButton_);
    mainLayout_->addLayout(inputLayout);

    // ===== Clear Button =====
    clearButton_ = new QPushButton(QStringLiteral("清空历史"));
    clearButton_->setObjectName(QStringLiteral("ClearButton"));
    clearButton_->setMaximumWidth(120);
    clearButton_->setCursor(Qt::PointingHandCursor);
    connect(clearButton_, &QPushButton::clicked,
            this, &AiSolverPage::onClearHistoryClicked);
    mainLayout_->addWidget(clearButton_, 0, Qt::AlignRight);
}

void AiSolverPage::onApiKeyChanged() {
    apiKey_ = apiKeyEdit_->text().trimmed();
}

void AiSolverPage::onSendButtonClicked() {
    // 获取最新的 API Key
    apiKey_ = apiKeyEdit_->text().trimmed();
    QString userInput = inputEdit_->text().trimmed();

    qDebug() << "Send button clicked";
    qDebug() << "API Key length:" << apiKey_.length();
    qDebug() << "User input:" << userInput;

    if (apiKey_.isEmpty()) {
        QMessageBox::warning(this, QStringLiteral("错误"),
                             QStringLiteral("请输入 DeepSeek API Key"));
        return;
    }

    if (userInput.isEmpty()) {
        QMessageBox::warning(this, QStringLiteral("提示"),
                             QStringLiteral("请输入问题"));
        return;
    }

    if (isLoading_) {
        QMessageBox::information(this, QStringLiteral("提示"),
                                 QStringLiteral("正在等待 AI 响应，请稍候..."));
        return;
    }

    // Add user message to display
    resultEdit_->moveCursor(QTextCursor::End);
    resultEdit_->insertPlainText(QStringLiteral("\n\n【你】\n") + userInput + QStringLiteral("\n"));

    // Add user message to history
    QJsonObject userMsg;
    userMsg["role"] = "user";
    userMsg["content"] = userInput;
    chatHistory_.append(userMsg);

    // Clear input
    inputEdit_->clear();

    // Prepare request
    QUrl url(QStringLiteral("https://api.deepseek.com/chat/completions"));
    QNetworkRequest request{url};
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization",
                         QStringLiteral("Bearer %1").arg(apiKey_).toUtf8());

    // Build request body
    QJsonObject requestBody;
    requestBody["model"] = "deepseek-v4-pro"; ;
    requestBody["messages"] = chatHistory_;
    requestBody["stream"] = true;
    requestBody["temperature"] = 0.7;

    QJsonDocument doc(requestBody);
    QByteArray postData = doc.toJson();

    qDebug() << "Sending request to DeepSeek API...";

    // Send request
    currentReply_ = networkManager_->post(request, postData);

    if (!currentReply_) {
        statusLabel_->setText(QStringLiteral("❌ 网络请求失败"));
        statusLabel_->setStyleSheet(QStringLiteral("color: red;"));
        return;
    }

    connect(currentReply_, &QNetworkReply::readyRead,
            this, &AiSolverPage::onReplyReadyRead);
    connect(currentReply_, &QNetworkReply::finished,
            this, &AiSolverPage::onReplyFinished);

    enableInputs(false);
    isLoading_ = true;
    statusLabel_->setText(QStringLiteral("⏳ 等待 AI 响应..."));
    statusLabel_->setStyleSheet(QStringLiteral("color: orange;"));

    // Add AI response label
    resultEdit_->moveCursor(QTextCursor::End);
    resultEdit_->insertPlainText(QStringLiteral("\n【AI 助手】\n"));
}

void AiSolverPage::onReplyReadyRead() {
    if (!currentReply_) return;

    QByteArray data = currentReply_->readAll();
    QString strData = QString::fromUtf8(data);

    qDebug() << "Received data:" << strData.left(100);

    // Parse SSE format: "data: {...}\n\n"
    QStringList lines = strData.split("\n", Qt::SkipEmptyParts);
    for (const QString& line : lines) {
        if (line.startsWith("data: ")) {
            QString jsonData = line.mid(6).trimmed();
            if (jsonData == "[DONE]") {
                qDebug() << "Stream finished";
                continue;
            }

            QJsonDocument doc = QJsonDocument::fromJson(jsonData.toUtf8());
            if (doc.isObject()) {
                QJsonObject obj = doc.object();
                QJsonArray choices = obj["choices"].toArray();
                if (!choices.isEmpty()) {
                    QJsonObject choice = choices[0].toObject();
                    QJsonObject delta = choice["delta"].toObject();
                    QString content = delta["content"].toString();

                    if (!content.isEmpty()) {
                        resultEdit_->moveCursor(QTextCursor::End);
                        resultEdit_->insertPlainText(content);
                        qDebug() << "Added content:" << content;
                    }
                }
            }
        }
    }
}

void AiSolverPage::onReplyFinished() {
    qDebug() << "Reply finished";

    if (!currentReply_) {
        return;
    }

    if (currentReply_->error() != QNetworkReply::NoError) {
        QString errorMsg = currentReply_->errorString();
        qDebug() << "Network error:" << errorMsg;

        // Try to parse API error message
        QByteArray errorData = currentReply_->readAll();
        qDebug() << "Error data:" << errorData;

        QJsonDocument doc = QJsonDocument::fromJson(errorData);
        if (doc.isObject()) {
            QJsonObject obj = doc.object();
            if (obj.contains("error")) {
                QString apiError = obj["error"].toObject()["message"].toString();
                if (!apiError.isEmpty()) {
                    errorMsg = apiError;
                }
            }
        }

        resultEdit_->moveCursor(QTextCursor::End);
        resultEdit_->insertPlainText(QStringLiteral("\n\n❌ 错误: ") + errorMsg);

        statusLabel_->setText(QStringLiteral("❌ 错误: ") + errorMsg);
        statusLabel_->setStyleSheet(QStringLiteral("color: red;"));
    } else {
        // Extract full response and add to history
        QString allText = resultEdit_->toPlainText();
        int lastAiPos = allText.lastIndexOf(QStringLiteral("【AI 助手】\n"));
        if (lastAiPos != -1) {
            QString fullResponse = allText.mid(lastAiPos + 10); // Length of "【AI 助手】\n"

            if (!fullResponse.isEmpty()) {
                QJsonObject assistantMsg;
                assistantMsg["role"] = "assistant";
                assistantMsg["content"] = fullResponse.trimmed();
                chatHistory_.append(assistantMsg);

                qDebug() << "Added assistant message to history";
            }
        }

        statusLabel_->setText(QStringLiteral("✅ 完成"));
        statusLabel_->setStyleSheet(QStringLiteral("color: green;"));
    }

    enableInputs(true);
    isLoading_ = false;
    currentReply_->deleteLater();
    currentReply_ = nullptr;
}

void AiSolverPage::onClearHistoryClicked() {
    resultEdit_->clear();

    // Reset chat history to only system message
    while (chatHistory_.size() > 1) {
        chatHistory_.removeAt(1);
    }

    statusLabel_->setText(QStringLiteral("✅ 对话历史已清空"));
    statusLabel_->setStyleSheet(QStringLiteral("color: green;"));

    QMessageBox::information(this, QStringLiteral("成功"),
                             QStringLiteral("对话历史已清空"));
}

void AiSolverPage::enableInputs(bool enabled) {
    inputEdit_->setEnabled(enabled);
    sendButton_->setEnabled(enabled);
    apiKeyEdit_->setEnabled(enabled);
    clearButton_->setEnabled(enabled);
}

void AiSolverPage::saveApiKey() {
    QString configPath = QCoreApplication::applicationDirPath() + "/algemate_ai.conf";
    QFile file(configPath);

    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
        qWarning() << "Failed to save API key:" << file.errorString();
        return;
    }

    // Base64 编码 API Key
    QByteArray encoded = apiKey_.toUtf8().toBase64();
    QTextStream out(&file);
    out << QString(encoded);

    qDebug() << "API key saved to" << configPath;
}

void AiSolverPage::loadApiKey() {
    QString configPath = QCoreApplication::applicationDirPath() + "/algemate_ai.conf";
    QFile file(configPath);

    if (!file.exists()) {
        qDebug() << "No saved API key found";
        return;
    }

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "Failed to load API key:" << file.errorString();
        return;
    }

    QTextStream in(&file);
    QString encoded = in.readAll().trimmed();

    if (!encoded.isEmpty()) {
        QByteArray decoded = QByteArray::fromBase64(encoded.toUtf8());
        apiKey_ = QString(decoded);

        // 设置到输入框
        if (apiKeyEdit_) {
            apiKeyEdit_->setText(apiKey_);
        }

        qDebug() << "API key loaded successfully";
        statusLabel_->setText(QStringLiteral("✅ API Key 已加载"));
        statusLabel_->setStyleSheet(QStringLiteral("color: green;"));
    }
}

}
