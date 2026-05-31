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
#include <QSettings> // 读取 DeepSeek 密钥
#include <QMimeData>
#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QDropEvent>
#include <QStandardPaths>
#include <QDir>
#include <QDateTime>
#include <QClipboard>
#include <QApplication>

#include "latex/LatexRenderer.h"
#include "latex/LatexTextBrowser.h"

// 主题管理器: 让 LaTeX 公式颜色跟随亮色 / 暗色切换.
#include "core/ThemeManager.h"

namespace AlgeMate::AiSolver {

AiSolverPage::AiSolverPage(QWidget* parent) : QWidget(parent) {
    networkManager_ = std::make_unique<QNetworkAccessManager>(this);

    setupUI();

    // 初始化对话历史
    QJsonObject systemMsg;
    systemMsg["role"] = "system";
    systemMsg["content"] = QStringLiteral(
        "你是专业数学助手，所有回答必须使用中文、Markdown 格式，数学公式使用 LaTeX（行内 $...$ ，块公式 $$...$$）。\n"
        "\n"
        "【重要：LaTeX 输出规范（渲染器限制，必须严格遵守）】\n"
        "1. 禁止使用 cases / aligned / align / gather / split / eqnarray / equation 环境，它们不能正常渲染。\n"
        "2. 禁止使用单列 array（如 \\begin{array}{l} 或 {c}），会造成公式错位。\n"
        "3. 表示方程组（联立方程、分段定义等）时，必须使用以下双列 array 模板，左边列放表达式、右边列放等号及右端，行间用 \\\\ 分隔：\n"
        "   $$\\left\\{\\begin{array}{ll} a_{11}x_1 + a_{12}x_2 + \\cdots + a_{1n}x_n & = b_1, \\\\ a_{21}x_1 + a_{22}x_2 + \\cdots + a_{2n}x_n & = b_2, \\\\ \\quad\\vdots & \\\\ a_{m1}x_1 + a_{m2}x_2 + \\cdots + a_{mn}x_n & = b_m. \\end{array}\\right.$$\n"
        "4. 禁止使用单列 pmatrix / bmatrix / vmatrix / array（任何 {l}、{c}、{r} 的单列表），都会错位。表示列向量必须使用以下双列 array 模板（列格式 {cr}，每行末尾补一个 & 空列以绕过渲染器单列 bug）：\n"
        "   $$\\left(\\begin{array}{cr} x_1 & \\\\ x_2 & \\\\ \\vdots & \\\\ x_n & \\end{array}\\right)$$\n"
        "   如果是只在行文中提及列向量，可以改用转置记号 $(x_1, x_2, \\ldots, x_n)^{T}$；完整呈现列向量时仍须用上述双列 array 模板。\n"
        "5. 如需多行对齐推导，改用多个独立的 $$...$$ 块公式，或使用上述双列 array 模板。\n"
        "6. 块公式单独成段，前后留空行，避免与文字混排。\n"
        "7. 禁止使用 \\binom{a}{b}（紧凑列向量写法，渲染器不支持）；表示列向量一律使用第 4 条的双列 array 模板。\n"
        "8. 禁止使用 \\boldsymbol{...} / \\bm{...}（渲染器不支持）；需要粗体向量记号时一律改用 \\mathbf{...}，例如 $\\mathbf{x}, \\hat{\\mathbf{\\beta}}$。\n"
        "\n"
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
    // 背景 / 输入框背景走全局 QSS, 避免在这里写死 #FFFFFF 导致暗色下不可读。

    mainLayout_ = new QVBoxLayout(this);
    mainLayout_->setContentsMargins(20, 20, 20, 20);
    mainLayout_->setSpacing(12);

    // ===== Title Section =====
    auto* titleLayout = new QHBoxLayout;
    auto* title = new QLabel(QStringLiteral("AI 智能解题"));
    title->setObjectName(QStringLiteral("AiPageTitle"));
    titleLayout->addWidget(title);

    clearButton_ = new QPushButton(QStringLiteral("清空历史"));
    clearButton_->setObjectName(QStringLiteral("AiClearBtn"));
    clearButton_->setCursor(Qt::PointingHandCursor);
    connect(clearButton_, &QPushButton::clicked, this, &AiSolverPage::onClearHistoryClicked);
    titleLayout->addStretch();
    titleLayout->addWidget(clearButton_);
    mainLayout_->addLayout(titleLayout);

    // 注：移除了原有的 API Key UI 配置，让界面更专注于聊天解题

    // ===== Subtitle & Status =====
    auto* subLayout = new QHBoxLayout;
    auto* subtitle = new QLabel(QStringLiteral("输入题目或上传图片，AI 将为你分步讲解与求解（需在设置中心配置 API）"));
    subtitle->setObjectName(QStringLiteral("AiPageSubtitle"));

    statusLabel_ = new QLabel(QStringLiteral("就绪"));
    statusLabel_->setObjectName(QStringLiteral("AiStatusLabel"));
    statusLabel_->setProperty("state", QStringLiteral("ready"));

    // 在状态标签旁边加一个“一键复制”按钮（类似豆包 / DeepSeek 官网）
    copyAllButton_ = new QPushButton(QStringLiteral("📋 复制全部"));
    copyAllButton_->setObjectName(QStringLiteral("AiCopyAllBtn"));
    copyAllButton_->setToolTip(QStringLiteral("复制本次对话的全部 Markdown / LaTeX 源码到剪贴板"));
    copyAllButton_->setCursor(Qt::PointingHandCursor);
    connect(copyAllButton_, &QPushButton::clicked, this, &AiSolverPage::onCopyAllClicked);

    subLayout->addWidget(subtitle);
    subLayout->addStretch();
    subLayout->addWidget(copyAllButton_);
    subLayout->addWidget(statusLabel_);
    mainLayout_->addLayout(subLayout);

    // ===== Messages Display =====
    renderer_ = new Latex::LatexRenderer;
    renderer_->addMathMacro(QStringLiteral("R"), QStringLiteral("\\mathbb{R}"));
    renderer_->addMathMacro(QStringLiteral("C"), QStringLiteral("\\mathbb{C}"));

    // 暗色下让公式以浅色渲染, 亮色下以深色渲染.
    auto themeTextColor = []() {
        const bool dark = AlgeMate::ThemeManager::instance().currentTheme()
                          == AlgeMate::ThemeManager::Theme::Dark;
        return dark ? QColor("#F3F3FA") : QColor("#1F2033");
    };
    renderer_->setTextColor(themeTextColor());

    resultEdit_ = new Latex::LatexTextBrowser;
    resultEdit_->setObjectName(QStringLiteral("AiResultView"));
    resultEdit_->setReadOnly(true);
    // 伪 URL（latex-tex://、image-src://）仅用于资源查找，不允许跳转
    resultEdit_->setOpenLinks(false);
    resultEdit_->setOpenExternalLinks(false);
    // 矢量化：让公式在 result 重绘时走 LatexInlineHandler
    Latex::attachLatexAutoPostProcess(resultEdit_);
    resultEdit_->setMinimumHeight(300);
    resultEdit_->setPlaceholderText(QStringLiteral("对话内容将显示在这里..."));
    mainLayout_->addWidget(resultEdit_, 1);

    // 主题切换时: 更新公式颜色 + 重渲染已显示的 markdown
    // (参考计算交互 / 演示模块 RenderTheme::forCurrent + setTextColor 的做法).
    connect(&AlgeMate::ThemeManager::instance(),
            &AlgeMate::ThemeManager::themeChanged,
            this, [this, themeTextColor](AlgeMate::ThemeManager::Theme){
                if (!renderer_ || !resultEdit_) return;
                renderer_->setTextColor(themeTextColor());
                if (rawMarkdown_.trimmed().isEmpty()) return;
                // 流式输出中不走重渲染, 以免覆盖进行中的增量 insertPlainText.
                if (isLoading_) return;
                renderer_->clearCache();
                QString html = renderer_->render(rawMarkdown_, resultEdit_->document());
                resultEdit_->setHtml(html);
                Latex::LatexRenderer::postProcessDocument(resultEdit_->document());
                resultEdit_->setSourceMarkdown(rawMarkdown_);
            });

    // ===== Image Selection Status =====
    // 缩略图区域：一个水平容器 = 左侧缩略图信息 QLabel + 右侧 × 删除按钮。
    // 未挂载图片时整个容器 hide；挂载后 show，点×后可不发送即取消。
    imageRow_ = new QWidget;
    auto* imageRowLayout = new QHBoxLayout(imageRow_);
    imageRowLayout->setContentsMargins(0, 0, 0, 0);
    imageRowLayout->setSpacing(8);

    imageLabel_ = new QLabel;
    imageLabel_->setStyleSheet("color: #8b5cf6; font-size: 12px; font-weight: bold;");
    imageRowLayout->addWidget(imageLabel_, 1, Qt::AlignVCenter);

    clearImageButton_ = new QPushButton(QStringLiteral("\u00d7"));
    clearImageButton_->setToolTip(QStringLiteral("取消挂载的图片"));
    clearImageButton_->setCursor(Qt::PointingHandCursor);
    clearImageButton_->setFixedSize(24, 24);
    clearImageButton_->setStyleSheet(QStringLiteral(
        "QPushButton {"
        "  background:#f3f4f6; color:#6b7280;"
        "  border:1px solid #e5e7eb; border-radius:12px;"
        "  font-size:14px; font-weight:bold; padding:0;"
        "}"
        "QPushButton:hover { background:#fee2e2; color:#dc2626; border-color:#fecaca; }"
        "QPushButton:pressed { background:#fecaca; }"
    ));
    connect(clearImageButton_, &QPushButton::clicked, this, &AiSolverPage::onClearImageClicked);
    imageRowLayout->addWidget(clearImageButton_, 0, Qt::AlignVCenter);

    imageRow_->hide();
    mainLayout_->addWidget(imageRow_);

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

    // 深度思考切换按钮（可 checkable）：开启后 DeepSeek 走 reasoner 模型
    deepThinkButton_ = new QPushButton(QStringLiteral("🧠 深度思考"));
    deepThinkButton_->setToolTip(QStringLiteral("开启后 DeepSeek 调用 reasoner 模型进行深度推理，答题更严谨但耗时更长。"));
    deepThinkButton_->setCheckable(true);
    deepThinkButton_->setMinimumSize(96, 45);
    deepThinkButton_->setCursor(Qt::PointingHandCursor);
    deepThinkButton_->setStyleSheet(
        "QPushButton { background-color: #f3f4f6; color: #6b7280; border: 1px solid #e5e7eb; border-radius: 8px; font-size: 13px; padding: 0 10px; }"
        "QPushButton:hover { background-color: #e5e7eb; }"
        "QPushButton:checked { background-color: #ede9fe; color: #6d28d9; border: 1px solid #c4b5fd; font-weight: bold; }"
        "QPushButton:checked:hover { background-color: #ddd6fe; }");
    connect(deepThinkButton_, &QPushButton::toggled, this, [this](bool checked) {
        deepThinkEnabled_ = checked;
        if (checked) {
            statusLabel_->setText(QStringLiteral("🧠 深度思考模式已开启【仅输出最终结果、耗时较长】"));
            statusLabel_->setStyleSheet(QStringLiteral("color: #7c3aed;"));
        } else {
            statusLabel_->setText(QStringLiteral("深度思考已关闭，恢复常规对话模式"));
            statusLabel_->setStyleSheet(QStringLiteral("color: #6b7280;"));
        }
    });

    // 多行输入框
    inputEdit_ = new QTextEdit;
    inputEdit_->setObjectName(QStringLiteral("AiInputEdit"));
    inputEdit_->setPlaceholderText(QStringLiteral("输入表达式 (Enter 执行，Shift+Enter 换行)"));
    inputEdit_->setFixedHeight(50);
    inputEdit_->installEventFilter(this);

    sendButton_ = new QPushButton(QStringLiteral("发送 ↵"));
    sendButton_->setObjectName(QStringLiteral("AiSendBtn"));
    sendButton_->setMinimumSize(80, 45);
    sendButton_->setCursor(Qt::PointingHandCursor);
    connect(sendButton_, &QPushButton::clicked, this, &AiSolverPage::onSendButtonClicked);

    inputLayout->addWidget(uploadImgButton_);
    inputLayout->addWidget(deepThinkButton_);
    inputLayout->addWidget(inputEdit_, 1);
    inputLayout->addWidget(sendButton_);

    mainLayout_->addLayout(inputLayout);

    // 启用拖放：页面本身 + 输入框都接收拖进来的图片
    setAcceptDrops(true);
    inputEdit_->setAcceptDrops(false);   // 让拖放事件冒泡到页面主体
    resultEdit_->setAcceptDrops(false);
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
        QFileInfo fi(filePath);
        mountImagePath(filePath, fi.fileName());
    }
}

// 统一挂载图片路径与刷新提示标签
void AiSolverPage::mountImagePath(const QString &path, const QString &displayName) {
    currentImagePath_ = path;

    // 用富文本在同一个 QLabel 里同时呈现缩略图 + 文件名
    QImage img(path);
    QString thumbHtml;
    if (!img.isNull()) {
        QImage thumb = img.scaled(56, 56, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        QByteArray ba;
        QBuffer buf(&ba);
        buf.open(QIODevice::WriteOnly);
        thumb.save(&buf, "PNG");
        const QString b64 = QString::fromLatin1(ba.toBase64());
        thumbHtml = QStringLiteral(
            "<img src='data:image/png;base64,%1' style='vertical-align:middle;'/>"
        ).arg(b64);
    } else {
        thumbHtml = QStringLiteral("🖼️");
    }

    imageLabel_->setText(QStringLiteral(
        "<table cellpadding='0' cellspacing='0'><tr>"
        "<td>%1</td>"
        "<td style='padding-left:10px;'>"
        "<span style='color:#7e22ce;font-weight:bold;'>已挂载图片</span><br/>"
        "<span style='color:#6b7280;'>%2</span>"
        "<span style='color:#9ca3af;font-weight:normal;'> （点右侧 × 可取消）</span>"
        "</td></tr></table>"
    ).arg(thumbHtml, displayName.toHtmlEscaped()));
    imageRow_->show();
}

// 从 QMimeData 中提取图片：优先本地文件 URL，其次嵌入的二进制图像数据
bool AiSolverPage::extractImageFromMime(const QMimeData *mime) {
    if (!mime) return false;

    static const QStringList kImageSuffixes = {
        QStringLiteral("png"),  QStringLiteral("jpg"),  QStringLiteral("jpeg"),
        QStringLiteral("bmp"),  QStringLiteral("gif"),  QStringLiteral("webp"),
        QStringLiteral("tif"),  QStringLiteral("tiff")
    };

    // 情形 1：拖入本地文件（桌面、资源管理器、微信保存后拖放等）
    if (mime->hasUrls()) {
        const auto urls = mime->urls();
        for (const QUrl &url : urls) {
            if (!url.isLocalFile()) continue;
            const QString localPath = url.toLocalFile();
            QFileInfo fi(localPath);
            if (!fi.isFile()) continue;
            const QString suf = fi.suffix().toLower();
            if (!kImageSuffixes.contains(suf)) continue;
            mountImagePath(localPath, fi.fileName());
            return true;
        }
    }

    // 情形 2：拖入调用者提供的图像数据（微信对话框内的图片拖拽常走这里）
    if (mime->hasImage()) {
        QImage img = qvariant_cast<QImage>(mime->imageData());
        if (img.isNull()) return false;

        // 落盘为临时文件，供后续上传使用
        QString cacheDir = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
        if (cacheDir.isEmpty())
            cacheDir = QDir::tempPath();
        QDir().mkpath(cacheDir);
        const QString fname = QStringLiteral("algemate_drop_%1.png")
                                  .arg(QDateTime::currentMSecsSinceEpoch());
        const QString fullPath = QDir(cacheDir).filePath(fname);
        if (!img.save(fullPath, "PNG")) return false;
        mountImagePath(fullPath, fname);
        return true;
    }

    return false;
}

void AiSolverPage::dragEnterEvent(QDragEnterEvent *event) {
    const QMimeData *mime = event->mimeData();
    if (!mime) {
        event->ignore();
        return;
    }
    // 只要携带 URL 或图像数据，就预接受；dropEvent 中再严格过滤后缀
    if (mime->hasUrls() || mime->hasImage()) {
        event->setDropAction(Qt::CopyAction);
        event->acceptProposedAction();
        return;
    }
    event->ignore();
}

void AiSolverPage::dragMoveEvent(QDragMoveEvent *event) {
    const QMimeData *mime = event->mimeData();
    if (mime && (mime->hasUrls() || mime->hasImage())) {
        event->setDropAction(Qt::CopyAction);
        event->acceptProposedAction();
        return;
    }
    event->ignore();
}

void AiSolverPage::dropEvent(QDropEvent *event) {
    if (extractImageFromMime(event->mimeData())) {
        event->setDropAction(Qt::CopyAction);
        event->acceptProposedAction();
        statusLabel_->setText(QStringLiteral("✅ 已接收拖入的图片"));
        statusLabel_->setStyleSheet(QStringLiteral("color: #10b981;"));
    } else {
        event->ignore();
        statusLabel_->setText(QStringLiteral("⚠️ 未识别到可用图片（仅支持 png/jpg/jpeg/bmp/gif/webp）"));
        statusLabel_->setStyleSheet(QStringLiteral("color: #f59e0b;"));
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
    resultEdit_->moveCursor(QTextCursor::End);
    resultEdit_->insertPlainText(QStringLiteral("\n\n【你】\n"));
    rawMarkdown_ += QStringLiteral("\n\n【你】\n");

    if (!currentImagePath_.isEmpty()) {
        // 在对话区嵌入真实缩略图
        QImage img(currentImagePath_);
        if (!img.isNull()) {
            QImage thumb = img.scaled(220, 220, Qt::KeepAspectRatio, Qt::SmoothTransformation);
            QTextCursor cur = resultEdit_->textCursor();
            cur.movePosition(QTextCursor::End);
            cur.insertImage(thumb);
            cur.insertText(QStringLiteral("\n"));
            resultEdit_->setTextCursor(cur);
        } else {
            resultEdit_->insertPlainText(QStringLiteral("[🖼️ 图片]\n"));
        }
        rawMarkdown_ += QStringLiteral("[🖼️ 图片]\n");
    }
    if (!userInput.isEmpty()) {
        resultEdit_->moveCursor(QTextCursor::End);
        resultEdit_->insertPlainText(userInput + QStringLiteral("\n"));
        rawMarkdown_ += userInput + QStringLiteral("\n");
    }
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
        imageRow_->hide();

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
    statusLabel_->setText(deepThinkEnabled_
                          ? QStringLiteral("⏳ DeepSeek-Reasoner 深度思考中（仅输出最终结果，耗时较长）...")
                          : QStringLiteral("⏳ DeepSeek 正在深度思考解题中..."));
    statusLabel_->setStyleSheet(QStringLiteral("color: #f59e0b;"));

    resultEdit_->moveCursor(QTextCursor::End);
    resultEdit_->insertPlainText(QStringLiteral("\n【AlgeMate AI】\n"));
    rawMarkdown_ += QStringLiteral("\n【AlgeMate AI】\n");

    // 深度思考模式：在原提示词后面追加“先思考、后输出”的要求，
    // 避免在正文里出现“嘿不对、让我重新想一想”等自我修正过程。
    QString promptToSend = finalPrompt;
    if (deepThinkEnabled_) {
        promptToSend += QStringLiteral(
            "\n\n请在内部充分思考、整理好结果后再输出。最终回复要条理清晰、严谨准确，"
            "不要在输出里夾带“嘿不对/等一下/让我重新想一想/刚才说错了”等思考过程或自我修正的表述。");
    }

    // 将组装好的问题加入历史记录
    QJsonObject userMsg;
    userMsg["role"] = "user";
    userMsg["content"] = promptToSend;
    chatHistory_.append(userMsg);

    // 实时读取设置中心的 DeepSeek 密钥
    QSettings settings("AlgeMate", "AlgeMateApp");
    QString dsApiKey = settings.value("AI/DeepSeekApiKey", "").toString();

    QUrl requestUrl(QStringLiteral("https://api.deepseek.com/chat/completions"));
    QNetworkRequest request{requestUrl};
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization", QStringLiteral("Bearer %1").arg(dsApiKey).toUtf8());

    QJsonObject requestBody;
    requestBody["model"] = deepThinkEnabled_
                            ? QStringLiteral("deepseek-reasoner")
                            : QStringLiteral("deepseek-chat");
    requestBody["messages"] = chatHistory_;
    requestBody["stream"] = true;      // 解题过程必须流式输出
    // reasoner 模型不支持 temperature 参数，仅在常规模型下设置
    if (!deepThinkEnabled_) {
        requestBody["temperature"] = 0.7;
    }

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
        // 矢量化：把 doc 里的 latex-vec:// 占位 image 替换为 inline ObjectFormat，
        // 后续重绘走 LatexInlineHandler。
        Latex::LatexRenderer::postProcessDocument(resultEdit_->document());
        // 复制时能还原为原始 markdown / LaTeX 源码
        resultEdit_->setSourceMarkdown(markdown);

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
    imageRow_->hide();
    rawMarkdown_.clear(); // 清空 Markdown 历史

    while (chatHistory_.size() > 1) {
        chatHistory_.removeAt(1);
    }

    statusLabel_->setText(QStringLiteral("✅ 对话历史已清空"));
    statusLabel_->setStyleSheet(QStringLiteral("color: #10b981;"));
}

// 一键复制：直接把 rawMarkdown_（本次会话累积的原始 Markdown / LaTeX 源码）
// 放到系统剪贴板，不依赖选区，体验与豆包复制按钮一致。
void AiSolverPage::onCopyAllClicked() {
    if (rawMarkdown_.trimmed().isEmpty()) {
        statusLabel_->setText(QStringLiteral("⚠️ 暂无内容可复制"));
        statusLabel_->setStyleSheet(QStringLiteral("color: #f59e0b;"));
        return;
    }
    QApplication::clipboard()->setText(rawMarkdown_);
    statusLabel_->setText(QStringLiteral("✅ 已复制全部内容到剪贴板"));
    statusLabel_->setStyleSheet(QStringLiteral("color: #10b981;"));
}

void AiSolverPage::enableInputs(bool enabled) {
    inputEdit_->setEnabled(enabled);
    sendButton_->setEnabled(enabled);
    clearButton_->setEnabled(enabled);
    uploadImgButton_->setEnabled(enabled);
    if (clearImageButton_) clearImageButton_->setEnabled(enabled);
    if (deepThinkButton_)  deepThinkButton_->setEnabled(enabled);
}

// 点击 × 按钮：在不发送的情况下取消已挂载的图片。
void AiSolverPage::onClearImageClicked() {
    if (isLoading_) return; // 请求进行中不允许取消
    if (currentImagePath_.isEmpty()) {
        imageRow_->hide();
        return;
    }
    currentImagePath_.clear();
    imageRow_->hide();
    statusLabel_->setText(QStringLiteral("✅ 已取消挂载的图片"));
    statusLabel_->setStyleSheet(QStringLiteral("color: #10b981;"));
}

}