#include "AiSolverPage.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QTextEdit>
#include <QPushButton>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QUrl>
#include <QCoreApplication>
#include <QMessageBox>
#include <QTextCursor>
#include <QDebug>
#include <QFileDialog>
#include <QBuffer>
#include <QImage>
#include <QFileInfo>
#include <QKeyEvent>
#include <QEvent>
#include <QSettings> // 引入 QSettings

#include "latex/LatexRenderer.h"
#include "latex/LatexTextBrowser.h"

namespace AlgeMate::AiSolver {

AiSolverPage::AiSolverPage(QWidget* parent) : QWidget(parent) {
    networkManager_ = std::make_unique<QNetworkAccessManager>(this);

    setupUI();

    // 初始化对话历史
    QJsonObject systemMsg;
    systemMsg["role"] = "system";
    systemMsg["content"] = QStringLiteral(
        "你是专业数学助手。"
        "所有回答必须使用 Markdown 格式。"
        "数学公式必须使用 LaTeX。"
        "行内公式使用 $...$ 。"
        "块公式使用 $$...$$ 。"
        "请使用中文回答。"
        "如果用户上传了图片，请仔细识别图片中的题目或公式，提供清晰的解释、步骤和例子。"
        );
    chatHistory_.append(systemMsg);

    qDebug() << "AiSolverPage initialized";
}

AiSolverPage::~AiSolverPage() {
    if (currentReply_ && currentReply_->isRunning()) {
        currentReply_->abort();
    }
    if (ocrReply_ && ocrReply_->isRunning()) {
        ocrReply_->abort();
    }
}

void AiSolverPage::setupUI() {
    // 全局美化
    this->setStyleSheet(
        "QWidget { font-family: 'Microsoft YaHei', 'Segoe UI', sans-serif; color: #333333; }"
        "QTextEdit { border: 1px solid #e0dced; border-radius: 8px; background-color: #ffffff; padding: 6px; selection-background-color: #d8b4e2; }"
        "QTextEdit:focus { border: 1px solid #8b5cf6; }"
        );

    mainLayout_ = new QVBoxLayout(this);
    mainLayout_->setContentsMargins(20, 20, 20, 20);
    mainLayout_->setSpacing(12);

    // ===== Title Section =====
    auto* titleLayout = new QHBoxLayout;
    auto* title = new QLabel(QStringLiteral("AI 智能解题"));
    title->setStyleSheet(QStringLiteral("font-size: 20px; font-weight: bold; color: #4c1d95;"));
    titleLayout->addWidget(title);

    clearButton_ = new QPushButton(QStringLiteral("清空历史"));
    clearButton_->setCursor(Qt::PointingHandCursor);
    clearButton_->setStyleSheet("QPushButton { background-color: #ffffff; color: #666666; border: 1px solid #d1d1e0; border-radius: 6px; padding: 6px 12px; }"
                                "QPushButton:hover { background-color: #f3f4f6; color: #333333; }");
    connect(clearButton_, &QPushButton::clicked, this, &AiSolverPage::onClearHistoryClicked);
    titleLayout->addStretch();
    titleLayout->addWidget(clearButton_);
    mainLayout_->addLayout(titleLayout);

    // 注：移除了原有的 API Key UI 配置，让界面更专注于聊天解题

    // ===== Subtitle & Status =====
    auto* subLayout = new QHBoxLayout;
    auto* subtitle = new QLabel(QStringLiteral("输入题目或上传图片，AI 将为你分步讲解与求解（需在设置中心配置 API）"));
    subtitle->setStyleSheet("color: #6b7280; font-size: 13px;");

    statusLabel_ = new QLabel(QStringLiteral("就绪"));
    statusLabel_->setStyleSheet(QStringLiteral("color: #10b981; font-size: 12px; font-weight: bold;"));

    subLayout->addWidget(subtitle);
    subLayout->addStretch();
    subLayout->addWidget(statusLabel_);
    mainLayout_->addLayout(subLayout);

    // ===== Messages Display =====
    renderer_ = new Latex::LatexRenderer;
    renderer_->addMathMacro(QStringLiteral("R"), QStringLiteral("\\mathbb{R}"));
    renderer_->addMathMacro(QStringLiteral("C"), QStringLiteral("\\mathbb{C}"));

    resultEdit_ = new QTextEdit;
    resultEdit_->setReadOnly(true);
    resultEdit_->setMinimumHeight(300);
    resultEdit_->setStyleSheet(
        QStringLiteral("QTextEdit { background-color: #fbfbfe; border: 1px solid #e0dced; border-radius: 12px; padding: 12px; }")
        );
    resultEdit_->setPlaceholderText(QStringLiteral("对话内容将显示在这里..."));
    mainLayout_->addWidget(resultEdit_, 1);

    // ===== Image Selection Status =====
    imageLabel_ = new QLabel;
    imageLabel_->setStyleSheet("color: #8b5cf6; font-size: 12px; font-weight: bold;");
    imageLabel_->hide();
    mainLayout_->addWidget(imageLabel_);

    // ===== Input Section =====
    auto* inputLayout = new QHBoxLayout;
    inputLayout->setContentsMargins(0, 0, 0, 0);
    inputLayout->setSpacing(10);

    uploadImgButton_ = new QPushButton(QStringLiteral("🖼️"));
    uploadImgButton_->setToolTip(QStringLiteral("上传图片进行识别"));
    uploadImgButton_->setMinimumSize(45, 45);
    uploadImgButton_->setCursor(Qt::PointingHandCursor);
    uploadImgButton_->setStyleSheet("QPushButton { background-color: #f3e8ff; color: #7e22ce; border: 1px solid #d8b4e2; border-radius: 8px; font-size: 18px; }"
                                    "QPushButton:hover { background-color: #e9d5ff; }");
    connect(uploadImgButton_, &QPushButton::clicked, this, &AiSolverPage::onUploadImageClicked);

    inputEdit_ = new QTextEdit;
    inputEdit_->setPlaceholderText(QStringLiteral("输入表达式 (Enter 执行，Shift+Enter 换行)"));
    inputEdit_->setFixedHeight(50);
    inputEdit_->installEventFilter(this);

    sendButton_ = new QPushButton(QStringLiteral("发送 ↵"));
    sendButton_->setMinimumSize(80, 45);
    sendButton_->setCursor(Qt::PointingHandCursor);
    sendButton_->setStyleSheet("QPushButton { background-color: #8b5cf6; color: white; border-radius: 8px; font-weight: bold; }"
                               "QPushButton:hover { background-color: #7c3aed; }");
    connect(sendButton_, &QPushButton::clicked, this, &AiSolverPage::onSendButtonClicked);

    inputLayout->addWidget(uploadImgButton_);
    inputLayout->addWidget(inputEdit_, 1);
    inputLayout->addWidget(sendButton_);

    mainLayout_->addLayout(inputLayout);
}

// 拦截文本框的按键事件
bool AiSolverPage::eventFilter(QObject *obj, QEvent *event) {
    if (obj == inputEdit_ && event->type() == QEvent::KeyPress) {
        QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);
        if (keyEvent->key() == Qt::Key_Return || keyEvent->key() == Qt::Key_Enter) {
            if (keyEvent->modifiers() & Qt::ShiftModifier) {
                return false;
            } else {
                onSendButtonClicked();
                return true;
            }
        }
    }
    return QWidget::eventFilter(obj, event);
}

void AiSolverPage::onUploadImageClicked() {
    QString filePath = QFileDialog::getOpenFileName(this, QStringLiteral("选择图片"), "",
                                                    QStringLiteral("Images (*.png *.jpg *.jpeg *.bmp)"));
    if (!filePath.isEmpty()) {
        currentImagePath_ = filePath;
        QFileInfo fi(filePath);
        imageLabel_->setText(QStringLiteral("已挂载图片: ") + fi.fileName() + QStringLiteral(" (发送后自动清除)"));
        imageLabel_->show();
    }
}

void AiSolverPage::onSendButtonClicked() {
    QString userInput = inputEdit_->toPlainText().trimmed();

    if (userInput.isEmpty() && currentImagePath_.isEmpty()) {
        QMessageBox::warning(this, QStringLiteral("提示"), QStringLiteral("请输入问题或上传一张图片"));
        return;
    }
    if (isLoading_) return;

    // 【核心同步机制】从系统统一设置中获取实时的 API Key
    QSettings settings("AlgeMate", "AlgeMateApp");
    QString dsApiKey = settings.value("AI/DeepSeekApiKey", "").toString();
    QString dbApiKey = settings.value("AI/DoubaoApiKey", "").toString();

    // 提前拦截未配置的错误
    if (!currentImagePath_.isEmpty() && dbApiKey.isEmpty()) {
        QMessageBox::warning(this, QStringLiteral("缺少配置"), QStringLiteral("请先在【设置中心】填写【豆包 OCR 密钥】！"));
        return;
    }
    if (currentImagePath_.isEmpty() && dsApiKey.isEmpty()) {
        QMessageBox::warning(this, QStringLiteral("缺少配置"), QStringLiteral("请先在【设置中心】填写【DeepSeek 密钥】！"));
        return;
    }

    // --- 界面显示（用户视角） ---
    QString displayMsg = userInput;
    if (!currentImagePath_.isEmpty()) {
        displayMsg = QStringLiteral("[🖼️ 图片] ") + displayMsg;
    }
    resultEdit_->moveCursor(QTextCursor::End);
    resultEdit_->insertPlainText(QStringLiteral("\n\n【你】\n") + displayMsg + QStringLiteral("\n"));
    rawMarkdown_ += QStringLiteral("\n\n【你】\n") + displayMsg + QStringLiteral("\n");
    inputEdit_->clear();

    enableInputs(false);
    isLoading_ = true;

    // --- 路由决策 ---
    if (!currentImagePath_.isEmpty()) {
        pendingUserInput_ = userInput;
        statusLabel_->setText(QStringLiteral("⏳ 豆包正在提取图片文字 (OCR)..."));
        statusLabel_->setStyleSheet(QStringLiteral("color: #f59e0b;"));

        QUrl url(QStringLiteral("https://ark.cn-beijing.volces.com/api/v3/chat/completions"));
        QNetworkRequest request{url};
        request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
        request.setRawHeader("Authorization", QStringLiteral("Bearer %1").arg(dbApiKey).toUtf8());

        QJsonObject requestBody;
        requestBody["model"] = QStringLiteral("doubao-seed-2-0-pro-260215");
        requestBody["stream"] = false;

        QJsonObject thinkingObj;
        thinkingObj["type"] = "disabled";
        requestBody["thinking"] = thinkingObj;

        QJsonArray messagesArray;
        QJsonObject userMsg;
        userMsg["role"] = "user";

        QJsonArray contentArray;
        QImage image(currentImagePath_);
        QByteArray ba;
        QBuffer buffer(&ba);
        buffer.open(QIODevice::WriteOnly);
        image.scaled(1024, 1024, Qt::KeepAspectRatio, Qt::SmoothTransformation).save(&buffer, "JPEG", 85);

        QJsonObject imageObj;
        imageObj["type"] = "image_url";
        QJsonObject imageUrlObj;
        imageUrlObj["url"] = QStringLiteral("data:image/jpeg;base64,") + QString::fromLatin1(ba.toBase64());
        imageObj["image_url"] = imageUrlObj;
        contentArray.append(imageObj);

        QJsonObject textObj;
        textObj["type"] = "text";
        textObj["text"] = QStringLiteral("请提取图片中的所有文字和数学公式，把题目复写为latex格式，一字不差。你是一个无情的OCR机器，不要尝试解答题目，只输出提取到的文字。");
        contentArray.append(textObj);

        userMsg["content"] = contentArray;
        messagesArray.append(userMsg);
        requestBody["messages"] = messagesArray;

        currentImagePath_.clear();
        imageLabel_->hide();

        ocrReply_ = networkManager_->post(request, QJsonDocument(requestBody).toJson());
        connect(ocrReply_, &QNetworkReply::finished, this, &AiSolverPage::onDoubaoOcrFinished);
    } else {
        // 纯文本，直接发给 DeepSeek
        sendToDeepSeek(userInput);
    }
}

void AiSolverPage::onDoubaoOcrFinished() {
    if (!ocrReply_) return;

    QString ocrResult = QStringLiteral("[图片识别失败，请检查网络或豆包模型ID]");

    if (ocrReply_->error() == QNetworkReply::NoError) {
        QByteArray data = ocrReply_->readAll();
        QJsonDocument doc = QJsonDocument::fromJson(data);
        if (doc.isObject()) {
            QJsonArray choices = doc.object()["choices"].toArray();
            if (!choices.isEmpty()) {
                QJsonObject message = choices[0].toObject()["message"].toObject();
                ocrResult = message["content"].toString().trimmed();
            }
        }
    } else {
        qDebug() << "Doubao OCR Error:" << ocrReply_->errorString();
    }

    ocrReply_->deleteLater();
    ocrReply_ = nullptr;

    QString finalPrompt = pendingUserInput_;
    if (!finalPrompt.isEmpty()) finalPrompt += QStringLiteral("\n\n");
    finalPrompt += QStringLiteral("以下是从图片中提取的题目内容：\n\n") + ocrResult + QStringLiteral("\n\n请帮我详细解答。");

    // 从配置重新检查 DeepSeek 密钥
    QSettings settings("AlgeMate", "AlgeMateApp");
    QString dsApiKey = settings.value("AI/DeepSeekApiKey", "").toString();

    if (dsApiKey.isEmpty()) {
        resultEdit_->moveCursor(QTextCursor::End);
        resultEdit_->insertPlainText(QStringLiteral("\n❌ 错误: 图片提取成功，但设置中没有填写 DeepSeek 密钥，无法进行解答。"));
        enableInputs(true);
        isLoading_ = false;
        statusLabel_->setText(QStringLiteral("就绪"));
        return;
    }

    sendToDeepSeek(finalPrompt);
}

void AiSolverPage::sendToDeepSeek(const QString& finalPrompt) {
    statusLabel_->setText(QStringLiteral("⏳ DeepSeek 正在深度思考解题中..."));
    statusLabel_->setStyleSheet(QStringLiteral("color: #f59e0b;"));

    resultEdit_->moveCursor(QTextCursor::End);
    resultEdit_->insertPlainText(QStringLiteral("\n【AlgeMate AI】\n"));
    rawMarkdown_ += QStringLiteral("\n【AlgeMate AI】\n");

    QJsonObject userMsg;
    userMsg["role"] = "user";
    userMsg["content"] = finalPrompt;
    chatHistory_.append(userMsg);

    // 实时读取设置中心的 DeepSeek 密钥
    QSettings settings("AlgeMate", "AlgeMateApp");
    QString dsApiKey = settings.value("AI/DeepSeekApiKey", "").toString();

    QUrl requestUrl(QStringLiteral("https://api.deepseek.com/chat/completions"));
    QNetworkRequest request{requestUrl};
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization", QStringLiteral("Bearer %1").arg(dsApiKey).toUtf8());

    QJsonObject requestBody;
    requestBody["model"] = QStringLiteral("deepseek-chat");
    requestBody["messages"] = chatHistory_;
    requestBody["stream"] = true;
    requestBody["temperature"] = 0.7;

    currentReply_ = networkManager_->post(request, QJsonDocument(requestBody).toJson());

    connect(currentReply_, &QNetworkReply::readyRead, this, &AiSolverPage::onReplyReadyRead);
    connect(currentReply_, &QNetworkReply::finished, this, &AiSolverPage::onReplyFinished);
}

void AiSolverPage::onReplyReadyRead() {
    if (!currentReply_) return;

    QByteArray data = currentReply_->readAll();
    QString strData = QString::fromUtf8(data);

    QStringList lines = strData.split("\n", Qt::SkipEmptyParts);
    for (const QString& line : lines) {
        if (line.startsWith("data: ")) {
            QString jsonData = line.mid(6).trimmed();
            if (jsonData == "[DONE]") continue;

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
                        rawMarkdown_ += content;
                    }
                }
            }
        }
    }
}

void AiSolverPage::onReplyFinished() {
    if (!currentReply_) return;

    if (currentReply_->error() != QNetworkReply::NoError) {
        QString errorMsg = currentReply_->errorString();
        QByteArray errorData = currentReply_->readAll();
        QJsonDocument doc = QJsonDocument::fromJson(errorData);
        if (doc.isObject() && doc.object().contains("error")) {
            QString apiError = doc.object()["error"].toObject()["message"].toString();
            if (!apiError.isEmpty()) errorMsg = apiError;
        }

        resultEdit_->moveCursor(QTextCursor::End);
        resultEdit_->insertPlainText(QStringLiteral("\n\n❌ 错误: ") + errorMsg);
        statusLabel_->setText(QStringLiteral("❌ 错误: ") + errorMsg);
        statusLabel_->setStyleSheet(QStringLiteral("color: #ef4444;"));
    } else {
        QString allText = rawMarkdown_;
        int lastAiPos = allText.lastIndexOf(QStringLiteral("【AlgeMate AI】\n"));
        if (lastAiPos != -1) {
            QString fullResponse = allText.mid(lastAiPos + 15);
            if (!fullResponse.isEmpty()) {
                QJsonObject assistantMsg;
                assistantMsg["role"] = "assistant";
                assistantMsg["content"] = fullResponse.trimmed();
                chatHistory_.append(assistantMsg);
            }
        }

        QString markdown = rawMarkdown_;
        renderer_->clearCache();
        QString html = renderer_->render(markdown, resultEdit_->document());
        resultEdit_->setHtml(html);

        QTextCursor cursor = resultEdit_->textCursor();
        cursor.movePosition(QTextCursor::End);
        resultEdit_->setTextCursor(cursor);

        statusLabel_->setText(QStringLiteral("✅ 完成"));
        statusLabel_->setStyleSheet(QStringLiteral("color: #10b981;"));
    }

    enableInputs(true);
    isLoading_ = false;
    currentReply_->deleteLater();
    currentReply_ = nullptr;
}

void AiSolverPage::onClearHistoryClicked() {
    resultEdit_->clear();
    currentImagePath_.clear();
    imageLabel_->hide();
    rawMarkdown_.clear();

    while (chatHistory_.size() > 1) {
        chatHistory_.removeAt(1);
    }

    statusLabel_->setText(QStringLiteral("✅ 对话历史已清空"));
    statusLabel_->setStyleSheet(QStringLiteral("color: #10b981;"));
}

void AiSolverPage::enableInputs(bool enabled) {
    inputEdit_->setEnabled(enabled);
    sendButton_->setEnabled(enabled);
    clearButton_->setEnabled(enabled);
    uploadImgButton_->setEnabled(enabled);
}

}