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
// 新增包含的头文件
#include <QFileDialog>
#include <QBuffer>
#include <QImage>
#include <QFileInfo>
#include <QKeyEvent>
#include <QEvent>

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
    if (ocrReply_ && ocrReply_->isRunning()) { // newly added
        ocrReply_->abort();
    }
    saveApiKey();
}

void AiSolverPage::setupUI() {
    // 全局美化：浅紫/白色主色调
    this->setStyleSheet(
        "QWidget { font-family: 'Microsoft YaHei', 'Segoe UI', sans-serif; color: #333333; }"
        "QLineEdit, QTextEdit { border: 1px solid #e0dced; border-radius: 8px; background-color: #ffffff; padding: 6px; selection-background-color: #d8b4e2; }"
        "QLineEdit:focus, QTextEdit:focus { border: 1px solid #8b5cf6; }"
        );

    mainLayout_ = new QVBoxLayout(this);
    mainLayout_->setContentsMargins(20, 20, 20, 20);
    mainLayout_->setSpacing(12);

    // ===== Title Section =====
    auto* titleLayout = new QHBoxLayout;
    auto* title = new QLabel(QStringLiteral("AI 智能解题"));
    title->setStyleSheet(QStringLiteral("font-size: 20px; font-weight: bold; color: #4c1d95;")); // 深紫色
    titleLayout->addWidget(title);

    // ===== Clear Button (移到标题旁边更美观) =====
    clearButton_ = new QPushButton(QStringLiteral("清空历史"));
    clearButton_->setCursor(Qt::PointingHandCursor);
    clearButton_->setStyleSheet("QPushButton { background-color: #ffffff; color: #666666; border: 1px solid #d1d1e0; border-radius: 6px; padding: 6px 12px; }"
                                "QPushButton:hover { background-color: #f3f4f6; color: #333333; }");
    connect(clearButton_, &QPushButton::clicked, this, &AiSolverPage::onClearHistoryClicked);
    titleLayout->addStretch();
    titleLayout->addWidget(clearButton_);
    mainLayout_->addLayout(titleLayout);

    // // ===== API Key Section =====
    // auto* apiLayout = new QHBoxLayout;
    // auto* apiLabel = new QLabel(QStringLiteral("API Key: "));
    // apiLabel->setStyleSheet(QStringLiteral("font-weight: bold; color: #6b7280;"));

    // apiKeyEdit_ = new QLineEdit;
    // apiKeyEdit_->setPlaceholderText(QStringLiteral("输入你的 DeepSeek API Key..."));
    // apiKeyEdit_->setEchoMode(QLineEdit::Password);
    // apiKeyEdit_->setMaximumHeight(32);
    // connect(apiKeyEdit_, &QLineEdit::textChanged, this, &AiSolverPage::onApiKeyChanged);

    // apiLayout->addWidget(apiLabel);
    // apiLayout->addWidget(apiKeyEdit_, 1);
    // mainLayout_->addLayout(apiLayout);

    // ===== 双 API Key 区域 =====
    auto* apiContainer = new QVBoxLayout;
    apiContainer->setSpacing(6);

    // 1. DeepSeek 行
    auto* dsLayout = new QHBoxLayout;
    auto* dsLabel = new QLabel(QStringLiteral("DeepSeek API Key:"));
    dsLabel->setFixedWidth(130);
    dsLabel->setStyleSheet(QStringLiteral("font-weight: bold; color: #6b7280;"));
    apiKeyEdit_ = new QLineEdit;
    apiKeyEdit_->setPlaceholderText(QStringLiteral("输入 DeepSeek API Key (用于纯文本)..."));
    apiKeyEdit_->setEchoMode(QLineEdit::Password);
    apiKeyEdit_->setMaximumHeight(30);
    connect(apiKeyEdit_, &QLineEdit::textChanged, this, &AiSolverPage::onApiKeyChanged);
    dsLayout->addWidget(dsLabel);
    dsLayout->addWidget(apiKeyEdit_, 1);
    apiContainer->addLayout(dsLayout);

    // 2. 豆包 行
    auto* dbLayout = new QHBoxLayout;
    auto* dbLabel = new QLabel(QStringLiteral("豆包 API Key:"));
    dbLabel->setFixedWidth(130);
    dbLabel->setStyleSheet(QStringLiteral("font-weight: bold; color: #6b7280;"));
    doubaoApiKeyEdit_ = new QLineEdit;
    doubaoApiKeyEdit_->setPlaceholderText(QStringLiteral("输入豆包 API Key (用于图片 OCR 解析)..."));
    doubaoApiKeyEdit_->setEchoMode(QLineEdit::Password);
    doubaoApiKeyEdit_->setMaximumHeight(30);
    connect(doubaoApiKeyEdit_, &QLineEdit::textChanged, this, &AiSolverPage::onDoubaoApiKeyChanged);
    dbLayout->addWidget(dbLabel);
    dbLayout->addWidget(doubaoApiKeyEdit_, 1);
    apiContainer->addLayout(dbLayout);

    mainLayout_->addLayout(apiContainer);

    // ===== Subtitle & Status =====
    auto* subLayout = new QHBoxLayout;
    auto* subtitle = new QLabel(QStringLiteral("输入题目或上传图片，AI 将为你分步讲解与求解"));
    subtitle->setStyleSheet("color: #6b7280; font-size: 13px;");

    statusLabel_ = new QLabel(QStringLiteral("就绪"));
    statusLabel_->setStyleSheet(QStringLiteral("color: #10b981; font-size: 12px; font-weight: bold;")); // 绿色

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
    // 美化输出框：浅紫底色，平滑圆角
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

    // OCR 图片上传按钮
    uploadImgButton_ = new QPushButton(QStringLiteral("🖼️"));
    uploadImgButton_->setToolTip(QStringLiteral("上传图片进行识别"));
    uploadImgButton_->setMinimumSize(45, 45);
    uploadImgButton_->setCursor(Qt::PointingHandCursor);
    uploadImgButton_->setStyleSheet("QPushButton { background-color: #f3e8ff; color: #7e22ce; border: 1px solid #d8b4e2; border-radius: 8px; font-size: 18px; }"
                                    "QPushButton:hover { background-color: #e9d5ff; }");
    connect(uploadImgButton_, &QPushButton::clicked, this, &AiSolverPage::onUploadImageClicked);

    // 多行输入框
    inputEdit_ = new QTextEdit;
    inputEdit_->setPlaceholderText(QStringLiteral("输入表达式 (Enter 执行，Shift+Enter 换行)"));
    inputEdit_->setFixedHeight(50); // 大概支持两行输入
    inputEdit_->installEventFilter(this); // 拦截回车键

    sendButton_ = new QPushButton(QStringLiteral("发送 ↵"));
    sendButton_->setMinimumSize(80, 45);
    sendButton_->setCursor(Qt::PointingHandCursor);
    // 美化发送按钮：主色调紫罗兰
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
                return false; // Shift+Enter 正常换行
            } else {
                onSendButtonClicked(); // 单独 Enter 直接发送
                return true;
            }
        }
    }
    return QWidget::eventFilter(obj, event);
}

void AiSolverPage::onApiKeyChanged() {
    apiKey_ = apiKeyEdit_->text().trimmed();
}

void AiSolverPage::onDoubaoApiKeyChanged() { // 豆包
    doubaoApiKey_ = doubaoApiKeyEdit_->text().trimmed();
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

// void AiSolverPage::onSendButtonClicked() {
//     apiKey_ = apiKeyEdit_->text().trimmed();
//     QString userInput = inputEdit_->toPlainText().trimmed(); // QTextEdit 获取文本方式

//     if (apiKey_.isEmpty()) {
//         QMessageBox::warning(this, QStringLiteral("错误"), QStringLiteral("请输入 DeepSeek API Key"));
//         return;
//     }

//     if (userInput.isEmpty() && currentImagePath_.isEmpty()) {
//         QMessageBox::warning(this, QStringLiteral("提示"), QStringLiteral("请输入问题或上传一张图片"));
//         return;
//     }

//     if (isLoading_) {
//         QMessageBox::information(this, QStringLiteral("提示"), QStringLiteral("正在等待 AI 响应，请稍候..."));
//         return;
//     }

//     // ===== 构建用户消息（遵循 DeepSeek V4 官方专有格式） =====
//     QJsonObject userMsg;
//     userMsg["role"] = "user";

//     QString displayMsg = userInput;

//     if (currentImagePath_.isEmpty()) {
//         // 纯文本请求
//         userMsg["content"] = userInput;
//     } else {
//         // 多模态图片请求
//         // 1. content 必须是纯字符串
//         userMsg["content"] = userInput.isEmpty() ? QStringLiteral("请仔细识别图片中的题目或公式，并提供清晰的解答：") : userInput;

//         // 2. 处理图片
//         QImage image(currentImagePath_);
//         if (!image.isNull()) {
//             QByteArray ba;
//             QBuffer buffer(&ba);
//             buffer.open(QIODevice::WriteOnly);

//             // 约束：尺寸控制在 4096px 以内，强制转为 JPEG
//             image.scaled(2048, 2048, Qt::KeepAspectRatio, Qt::SmoothTransformation)
//                 .save(&buffer, "JPEG", 85);

//             // 约束：只保留纯 Base64 字符串，去除 data 前缀
//             QString base64 = QString::fromLatin1(ba.toBase64());

//             // 核心修改：image_data 必须作为独立字段，与 role 和 content 平级！
//             userMsg["image_data"] = base64;

//             displayMsg = QStringLiteral("[🖼️ 图片] ") + displayMsg;
//         }

//         // 清理当前图片状态
//         currentImagePath_.clear();
//         imageLabel_->hide();
//     }

//     // 界面显示
//     resultEdit_->moveCursor(QTextCursor::End);
//     resultEdit_->insertPlainText(QStringLiteral("\n\n【你】\n") + displayMsg + QStringLiteral("\n"));
//     // 同步记录到 Markdown 历史中
//     rawMarkdown_ += QStringLiteral("\n\n【你】\n") + displayMsg + QStringLiteral("\n");

//     // 计入历史
//     chatHistory_.append(userMsg);
//     inputEdit_->clear();

//     // Prepare request
//     QUrl url(QStringLiteral("https://api.deepseek.com/chat/completions"));
//     QNetworkRequest request{url};
//     request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
//     request.setRawHeader("Authorization", QStringLiteral("Bearer %1").arg(apiKey_).toUtf8());

//     // Build request body
//     QJsonObject requestBody;
//     requestBody["model"] = "deepseek-v4-pro"; // 确保调用支持视觉的模型
//     requestBody["messages"] = chatHistory_;
//     requestBody["stream"] = true;
//     requestBody["temperature"] = 0.7;

//     QJsonDocument doc(requestBody);
//     QByteArray postData = doc.toJson();

//     currentReply_ = networkManager_->post(request, postData);

//     if (!currentReply_) {
//         statusLabel_->setText(QStringLiteral("❌ 网络请求失败"));
//         statusLabel_->setStyleSheet(QStringLiteral("color: #ef4444;")); // 红色
//         return;
//     }

//     connect(currentReply_, &QNetworkReply::readyRead, this, &AiSolverPage::onReplyReadyRead);
//     connect(currentReply_, &QNetworkReply::finished, this, &AiSolverPage::onReplyFinished);

//     enableInputs(false);
//     isLoading_ = true;
//     statusLabel_->setText(QStringLiteral("⏳ AI 思考中..."));
//     statusLabel_->setStyleSheet(QStringLiteral("color: #f59e0b;")); // 橘黄色

//     resultEdit_->moveCursor(QTextCursor::End);
//     resultEdit_->insertPlainText(QStringLiteral("\n【AlgeMate AI】\n"));
//     // 同步记录 AI 前缀
//     rawMarkdown_ += QStringLiteral("\n【AlgeMate AI】\n");
// }

void AiSolverPage::onSendButtonClicked() {
    apiKey_ = apiKeyEdit_->text().trimmed();
    doubaoApiKey_ = doubaoApiKeyEdit_->text().trimmed();
    QString userInput = inputEdit_->toPlainText().trimmed();

    if (userInput.isEmpty() && currentImagePath_.isEmpty()) {
        QMessageBox::warning(this, QStringLiteral("提示"), QStringLiteral("请输入问题或上传一张图片"));
        return;
    }
    if (isLoading_) return;

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
        // 【有图片】：交由豆包进行 OCR 识别
        if (doubaoApiKey_.isEmpty()) {
            QMessageBox::warning(this, QStringLiteral("错误"), QStringLiteral("请先填写豆包 API Key 用于图像识别"));
            enableInputs(true); isLoading_ = false; return;
        }

        pendingUserInput_ = userInput; // 暂存用户的提问
        statusLabel_->setText(QStringLiteral("⏳ 豆包正在提取图片文字 (OCR)..."));
        statusLabel_->setStyleSheet(QStringLiteral("color: #f59e0b;"));

        QUrl url(QStringLiteral("https://ark.cn-beijing.volces.com/api/v3/chat/completions"));
        QNetworkRequest request{url};
        request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
        request.setRawHeader("Authorization", QStringLiteral("Bearer %1").arg(doubaoApiKey_).toUtf8());

        QJsonObject requestBody;
        requestBody["model"] = QStringLiteral("doubao-seed-2-0-pro-260215"); // 填入豆包模型ID
        requestBody["stream"] = false; // 注意：OCR 不要流式，直接拿最终的完整文字

        QJsonObject thinkingObj;
        thinkingObj["type"] = "disabled";
        requestBody["thinking"] = thinkingObj;

        QJsonArray messagesArray;
        QJsonObject userMsg;
        userMsg["role"] = "user";

        QJsonArray contentArray;
        // 压入图片
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

        // 压入强制指令（只提取，不解答）
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
        // 【纯文本】：没有图片，直接跳过豆包，走 DeepSeek 解题
        if (apiKey_.isEmpty()) {
            QMessageBox::warning(this, QStringLiteral("错误"), QStringLiteral("请先填写 DeepSeek API Key"));
            enableInputs(true); isLoading_ = false; return;
        }
        sendToDeepSeek(userInput);
    }
}

void AiSolverPage::onDoubaoOcrFinished() {
    if (!ocrReply_) return;

    QString ocrResult = QStringLiteral("[图片识别失败，请检查网络或豆包模型ID]");

    // 解析豆包非流式 JSON 返回值
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

    // --- 拼装最终给 DeepSeek 的提示词 ---
    QString finalPrompt = pendingUserInput_;
    if (!finalPrompt.isEmpty()) {
        finalPrompt += QStringLiteral("\n\n");
    }
    finalPrompt += QStringLiteral("以下是从图片中提取的题目内容：\n\n") + ocrResult + QStringLiteral("\n\n请帮我详细解答。");

    // 检查是否有 DeepSeek Key
    if (apiKey_.isEmpty()) {
        resultEdit_->moveCursor(QTextCursor::End);
        resultEdit_->insertPlainText(QStringLiteral("\n❌ 错误: 图片提取成功，但没有填写 DeepSeek API Key，无法进行解答。"));
        enableInputs(true);
        isLoading_ = false;
        statusLabel_->setText(QStringLiteral("就绪"));
        return;
    }

    // 给 DeepSeek
    sendToDeepSeek(finalPrompt);
}

void AiSolverPage::sendToDeepSeek(const QString& finalPrompt) {
    statusLabel_->setText(QStringLiteral("⏳ DeepSeek 正在深度思考解题中..."));
    statusLabel_->setStyleSheet(QStringLiteral("color: #f59e0b;"));

    resultEdit_->moveCursor(QTextCursor::End);
    resultEdit_->insertPlainText(QStringLiteral("\n【AlgeMate AI】\n"));
    rawMarkdown_ += QStringLiteral("\n【AlgeMate AI】\n");

    // 将组装好的问题加入历史记录
    QJsonObject userMsg;
    userMsg["role"] = "user";
    userMsg["content"] = finalPrompt;
    chatHistory_.append(userMsg);

    QUrl requestUrl(QStringLiteral("https://api.deepseek.com/chat/completions"));
    QNetworkRequest request{requestUrl};
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization", QStringLiteral("Bearer %1").arg(apiKey_).toUtf8());

    QJsonObject requestBody;
    requestBody["model"] = QStringLiteral("deepseek-chat");
    requestBody["messages"] = chatHistory_;
    requestBody["stream"] = true;      // 解题过程必须流式输出
    requestBody["temperature"] = 0.7;

    currentReply_ = networkManager_->post(request, QJsonDocument(requestBody).toJson());

    // 绑定 DeepSeek 的流式输出和完成事件
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
        // QString allText = resultEdit_->toPlainText();
        QString allText = rawMarkdown_; // 使用保存的完整原版 Markdown
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

        // QString markdown = resultEdit_->toPlainText();
        QString markdown = rawMarkdown_;
        renderer_->clearCache();
        QString html = renderer_->render(markdown, resultEdit_->document());
        resultEdit_->setHtml(html);

        //界面滚回底部
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
    rawMarkdown_.clear(); // 清空 Markdown 历史

    while (chatHistory_.size() > 1) {
        chatHistory_.removeAt(1);
    }

    statusLabel_->setText(QStringLiteral("✅ 对话历史已清空"));
    statusLabel_->setStyleSheet(QStringLiteral("color: #10b981;"));
}

void AiSolverPage::enableInputs(bool enabled) {
    inputEdit_->setEnabled(enabled);
    sendButton_->setEnabled(enabled);
    apiKeyEdit_->setEnabled(enabled);
    doubaoApiKeyEdit_->setEnabled(enabled); // 豆包
    clearButton_->setEnabled(enabled);
    uploadImgButton_->setEnabled(enabled);
}

// void AiSolverPage::saveApiKey() {
//     QString configPath = QCoreApplication::applicationDirPath() + "/algemate_ai.conf";
//     QFile file(configPath);

//     if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) return;

//     QByteArray encoded = apiKey_.toUtf8().toBase64();
//     QTextStream out(&file);
//     out << QString(encoded);
// }

// void AiSolverPage::loadApiKey() {
//     QString configPath = QCoreApplication::applicationDirPath() + "/algemate_ai.conf";
//     QFile file(configPath);

//     if (!file.exists() || !file.open(QIODevice::ReadOnly | QIODevice::Text)) return;

//     QTextStream in(&file);
//     QString encoded = in.readAll().trimmed();

//     if (!encoded.isEmpty()) {
//         QByteArray decoded = QByteArray::fromBase64(encoded.toUtf8());
//         apiKey_ = QString(decoded);

//         if (apiKeyEdit_) apiKeyEdit_->setText(apiKey_);
//         statusLabel_->setText(QStringLiteral("✅ API Key 已加载"));
//         statusLabel_->setStyleSheet(QStringLiteral("color: #10b981;"));
//     }
// }

void AiSolverPage::saveApiKey() {
    QString configPath = QCoreApplication::applicationDirPath() + "/algemate_ai.conf";
    QFile file(configPath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) return;

    QTextStream out(&file);
    out << QString(apiKey_.toUtf8().toBase64()) << "\n";
    out << QString(doubaoApiKey_.toUtf8().toBase64()) << "\n";
}

void AiSolverPage::loadApiKey() {
    QString configPath = QCoreApplication::applicationDirPath() + "/algemate_ai.conf";
    QFile file(configPath);
    if (!file.exists() || !file.open(QIODevice::ReadOnly | QIODevice::Text)) return;

    QTextStream in(&file);
    QString dsEncoded = in.readLine().trimmed();
    QString dbEncoded = in.readLine().trimmed();

    if (!dsEncoded.isEmpty()) {
        apiKey_ = QString(QByteArray::fromBase64(dsEncoded.toUtf8()));
        if (apiKeyEdit_) apiKeyEdit_->setText(apiKey_);
    }
    if (!dbEncoded.isEmpty()) {
        doubaoApiKey_ = QString(QByteArray::fromBase64(dbEncoded.toUtf8()));
        if (doubaoApiKeyEdit_) doubaoApiKeyEdit_->setText(doubaoApiKey_);
    }

    if (!apiKey_.isEmpty() || !doubaoApiKey_.isEmpty()) {
        statusLabel_->setText(QStringLiteral("✅ API Keys 已加载"));
        statusLabel_->setStyleSheet(QStringLiteral("color: #10b981;"));
    }
}

}