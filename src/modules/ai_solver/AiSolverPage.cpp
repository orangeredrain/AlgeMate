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
#include <QSettings>
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
#include "core/ThemeManager.h"

namespace {
// 提取 API 地址为常量，方便统一管理
constexpr char DOUBAO_API_URL[] = "https://ark.cn-beijing.volces.com/api/v3/chat/completions";
constexpr char DEEPSEEK_API_URL[] = "https://api.deepseek.com/chat/completions";

// 提取系统提示词，使用原始字符串 (Raw String Literal) 保持格式清晰
const QString SYSTEM_PROMPT = QStringLiteral(R"(你是专业数学助手，所有回答必须使用中文、Markdown 格式，数学公式使用 LaTeX（行内 $...$ ，块公式 $$...$$）。
【重要：LaTeX 输出规范（渲染器限制，必须严格遵守）】
1. 禁止使用 cases / aligned / align / gather / split / eqnarray / equation 环境，它们不能正常渲染。
2. 禁止使用单列 array（如 \begin{array}{l} 或 {c}），会造成公式错位。
3. 表示方程组（联立方程、分段定义等）时，必须使用以下双列 array 模板，左边列放表达式、右边列放等号及右端，行间用 \\ 分隔：
   $$\left\{\begin{array}{ll} a_{11}x_1 + a_{12}x_2 + \cdots + a_{1n}x_n & = b_1, \\ a_{21}x_1 + a_{22}x_2 + \cdots + a_{2n}x_n & = b_2, \\ \quad\vdots & \\ a_{m1}x_1 + a_{m2}x_2 + \cdots + a_{mn}x_n & = b_m. \end{array}\right.$$
4. 禁止使用单列 pmatrix / bmatrix / vmatrix / array（任何 {l}、{c}、{r} 的单列表），都会错位。表示列向量必须使用以下双列 array 模板（列格式 {cr}，每行末尾补一个 & 空列以绕过渲染器单列 bug）：
   $$\left(\begin{array}{cr} x_1 & \\ x_2 & \\ \vdots & \\ x_n & \end{array}\right)$$
5. 如需多行对齐推导，改用多个独立的 $$...$$ 块公式，或使用上述双列 array 模板。
6. 块公式单独成段，前后留空行，避免与文字混排。
7. 禁止使用 \binom{a}{b}（紧凑列向量写法，渲染器不支持）；表示列向量一律使用第 4 条的双列 array 模板。
8. 禁止使用 \boldsymbol{...} / \bm{...}（渲染器不支持）；需要粗体向量记号时一律改用 \mathbf{...}，例如 $\mathbf{x}, \hat{\mathbf{\beta}}$。
如果用户上传了图片，请仔细识别图片中的题目或公式，提供清晰的解释、步骤和例子。)");

// === 辅助函数：统一处理状态标签更新，避免修改 .h 文件 ===
void updateStatusLabel(QLabel* label, const QString& text, const QString& lightColor, const QString& darkColor) {
    if (!label) return;
    label->setText(text);
    bool isDark = AlgeMate::ThemeManager::instance().currentTheme() == AlgeMate::ThemeManager::Theme::Dark;
    label->setStyleSheet(QStringLiteral("color: %1;").arg(isDark ? darkColor : lightColor));
}
}

namespace AlgeMate::AiSolver {

AiSolverPage::AiSolverPage(QWidget* parent) : QWidget(parent) {
    networkManager_ = std::make_unique<QNetworkAccessManager>(this);
    setupUI();

    QJsonObject systemMsg;
    systemMsg["role"] = "system";
    systemMsg["content"] = SYSTEM_PROMPT;
    chatHistory_.append(systemMsg);
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
    mainLayout_ = new QVBoxLayout(this);
    mainLayout_->setContentsMargins(20, 20, 20, 20);
    mainLayout_->setSpacing(12);

    // 1. 标题栏
    auto* titleLayout = new QHBoxLayout;
    auto* title = new QLabel(QStringLiteral("AI 智能解题"));
    title->setObjectName(QStringLiteral("AiPageTitle"));
    titleLayout->addWidget(title);

    clearButton_ = new QPushButton(QStringLiteral("清空历史"));
    clearButton_->setCursor(Qt::PointingHandCursor);
    connect(clearButton_, &QPushButton::clicked, this, &AiSolverPage::onClearHistoryClicked);

    titleLayout->addStretch();
    titleLayout->addWidget(clearButton_);
    mainLayout_->addLayout(titleLayout);

    // 2. 副标题与工具栏
    auto* subLayout = new QHBoxLayout;
    auto* subtitle = new QLabel(QStringLiteral("输入题目或上传图片，AI 将为你分步讲解与求解（需在设置中心配置 API）"));
    subtitle->setObjectName(QStringLiteral("AiPageSubtitle"));

    statusLabel_ = new QLabel(QStringLiteral("就绪"));
    statusLabel_->setProperty("state", QStringLiteral("ready"));

    copyAllButton_ = new QPushButton(QStringLiteral("📋 复制全部"));
    copyAllButton_->setToolTip(QStringLiteral("复制本次对话的全部 Markdown / LaTeX 源码到剪贴板"));
    copyAllButton_->setCursor(Qt::PointingHandCursor);
    connect(copyAllButton_, &QPushButton::clicked, this, &AiSolverPage::onCopyAllClicked);

    subLayout->addWidget(subtitle);
    subLayout->addStretch();
    subLayout->addWidget(copyAllButton_);
    subLayout->addWidget(statusLabel_);
    mainLayout_->addLayout(subLayout);

    // 3. LaTeX 渲染与展示区
    renderer_ = new Latex::LatexRenderer;
    renderer_->addMathMacro(QStringLiteral("R"), QStringLiteral("\\mathbb{R}"));
    renderer_->addMathMacro(QStringLiteral("C"), QStringLiteral("\\mathbb{C}"));

    resultEdit_ = new Latex::LatexTextBrowser;
    resultEdit_->setReadOnly(true);
    resultEdit_->setOpenLinks(false);
    resultEdit_->setOpenExternalLinks(false);
    Latex::attachLatexAutoPostProcess(resultEdit_);
    resultEdit_->setMinimumHeight(300);
    resultEdit_->setPlaceholderText(QStringLiteral("对话内容将显示在这里..."));
    mainLayout_->addWidget(resultEdit_, 1);

    // 4. 图片挂载展示区
    imageRow_ = new QWidget;
    auto* imageRowLayout = new QHBoxLayout(imageRow_);
    imageRowLayout->setContentsMargins(0, 0, 0, 0);
    imageRowLayout->setSpacing(8);

    imageLabel_ = new QLabel;
    imageRowLayout->addWidget(imageLabel_, 1, Qt::AlignVCenter);

    clearImageButton_ = new QPushButton(QStringLiteral("\u00d7"));
    clearImageButton_->setCursor(Qt::PointingHandCursor);
    clearImageButton_->setFixedSize(24, 24);
    connect(clearImageButton_, &QPushButton::clicked, this, &AiSolverPage::onClearImageClicked);
    imageRowLayout->addWidget(clearImageButton_, 0, Qt::AlignVCenter);

    imageRow_->hide();
    mainLayout_->addWidget(imageRow_);

    // 5. 底部输入区
    auto* inputLayout = new QHBoxLayout;
    inputLayout->setContentsMargins(0, 0, 0, 0);
    inputLayout->setSpacing(10);

    uploadImgButton_ = new QPushButton(QStringLiteral("🖼️"));
    uploadImgButton_->setMinimumSize(45, 45);
    uploadImgButton_->setCursor(Qt::PointingHandCursor);
    connect(uploadImgButton_, &QPushButton::clicked, this, &AiSolverPage::onUploadImageClicked);

    deepThinkButton_ = new QPushButton(QStringLiteral("🧠 深度思考"));
    deepThinkButton_->setCheckable(true);
    deepThinkButton_->setMinimumSize(96, 45);
    deepThinkButton_->setCursor(Qt::PointingHandCursor);
    connect(deepThinkButton_, &QPushButton::toggled, this, [this](bool checked) {
        deepThinkEnabled_ = checked;
        if (checked) {
            updateStatusLabel(statusLabel_, QStringLiteral("🧠 深度思考模式已开启【仅输出最终结果、耗时较长】"), "#7c3aed", "#B0BBFF");
        } else {
            updateStatusLabel(statusLabel_, QStringLiteral("深度思考已关闭，恢复常规对话模式"), "#6b7280", "#C9C9DC");
        }
    });

    inputEdit_ = new QTextEdit;
    inputEdit_->setPlaceholderText(QStringLiteral("输入表达式 (Enter 执行，Shift+Enter 换行)"));
    inputEdit_->setFixedHeight(50);
    inputEdit_->installEventFilter(this);

    sendButton_ = new QPushButton(QStringLiteral("发送 ↵"));
    sendButton_->setMinimumSize(80, 45);
    sendButton_->setCursor(Qt::PointingHandCursor);
    connect(sendButton_, &QPushButton::clicked, this, &AiSolverPage::onSendButtonClicked);

    inputLayout->addWidget(uploadImgButton_);
    inputLayout->addWidget(deepThinkButton_);
    inputLayout->addWidget(inputEdit_, 1);
    inputLayout->addWidget(sendButton_);
    mainLayout_->addLayout(inputLayout);

    setAcceptDrops(true);
    inputEdit_->setAcceptDrops(false);
    resultEdit_->setAcceptDrops(false);

    // ==================== 主题样式应用 ====================
    auto applyTheme = [this, title, subtitle]() {
        bool isDark = AlgeMate::ThemeManager::instance().currentTheme() == AlgeMate::ThemeManager::Theme::Dark;

        // 使用 Raw String 优化 QSS 阅读体验
        title->setStyleSheet(isDark ? R"(font-size: 24px; font-weight: bold; color: #E6E7F0;)"
                                    : R"(font-size: 24px; font-weight: bold; color: #111827;)");
        subtitle->setStyleSheet(isDark ? "color: #7B7B96;" : "color: #6b7280;");

        clearButton_->setStyleSheet(isDark
                                        ? R"(QPushButton { background: transparent; border: 1px solid #3B395A; border-radius: 6px; padding: 6px 12px; color: #C9C9DC; } QPushButton:hover { background: #28263F; })"
                                        : R"(QPushButton { background: transparent; border: 1px solid #e5e7eb; border-radius: 6px; padding: 6px 12px; color: #4b5563; } QPushButton:hover { background: #f3f4f6; })");

        copyAllButton_->setStyleSheet(isDark
                                          ? R"(QPushButton { background: transparent; border: none; color: #8FA1FF; font-weight: bold; } QPushButton:hover { color: #6F77FF; text-decoration: underline; })"
                                          : R"(QPushButton { background: transparent; border: none; color: #6d28d9; font-weight: bold; } QPushButton:hover { color: #5b21b6; text-decoration: underline; })");

        resultEdit_->setStyleSheet(isDark
                                       ? R"(QTextBrowser { background: #1C1B2E; color: #E6E7F0; border: 1px solid #3B395A; border-radius: 8px; padding: 10px; })"
                                       : R"(QTextBrowser { background: #ffffff; color: #1f2937; border: 1px solid #e5e7eb; border-radius: 8px; padding: 10px; })");

        inputEdit_->setStyleSheet(isDark
                                      ? R"(QTextEdit { background: #28263F; color: #E6E7F0; border: 1px solid #3B395A; border-radius: 8px; padding: 8px; font-size: 14px; })"
                                      : R"(QTextEdit { background: #f9fafb; color: #1f2937; border: 1px solid #e5e7eb; border-radius: 8px; padding: 8px; font-size: 14px; })");

        sendButton_->setStyleSheet(isDark
                                       ? R"(QPushButton { background: #312F4A; color: #8FA1FF; border: none; border-radius: 8px; font-weight: bold; } QPushButton:hover { background: #3B395A; })"
                                       : R"(QPushButton { background: #7c3aed; color: #ffffff; border: none; border-radius: 8px; font-weight: bold; } QPushButton:hover { background: #6d28d9; })");

        uploadImgButton_->setStyleSheet(isDark
                                            ? R"(QPushButton { background: #28263F; color: #8FA1FF; border: 1px solid #3B395A; border-radius: 8px; font-size: 18px; } QPushButton:hover { background: #312F4A; })"
                                            : R"(QPushButton { background: #f3e8ff; color: #7e22ce; border: 1px solid #d8b4e2; border-radius: 8px; font-size: 18px; } QPushButton:hover { background: #e9d5ff; })");

        deepThinkButton_->setStyleSheet(isDark
                                            ? R"(QPushButton { background-color: #28263F; color: #C9C9DC; border: 1px solid #3B395A; border-radius: 8px; font-size: 13px; padding: 0 10px; }
                 QPushButton:hover { background-color: #3B395A; }
                 QPushButton:checked { background-color: #312F4A; color: #8FA1FF; border: 1px solid #6F77FF; font-weight: bold; }
                 QPushButton:checked:hover { background-color: #3B395A; })"
                                            : R"(QPushButton { background-color: #f3f4f6; color: #6b7280; border: 1px solid #e5e7eb; border-radius: 8px; font-size: 13px; padding: 0 10px; }
                 QPushButton:hover { background-color: #e5e7eb; }
                 QPushButton:checked { background-color: #ede9fe; color: #6d28d9; border: 1px solid #c4b5fd; font-weight: bold; }
                 QPushButton:checked:hover { background-color: #ddd6fe; })");

        clearImageButton_->setStyleSheet(isDark
                                             ? R"(QPushButton { background: #3B395A; color: #C9C9DC; border: 1px solid #4B4970; border-radius: 12px; font-size: 14px; font-weight: bold; padding: 0; }
                 QPushButton:hover { background: #6B2A3A; color: #FC8181; border-color: #FC8181; })"
                                             : R"(QPushButton { background: #f3f4f6; color: #6b7280; border: 1px solid #e5e7eb; border-radius: 12px; font-size: 14px; font-weight: bold; padding: 0; }
                 QPushButton:hover { background: #fee2e2; color: #dc2626; border-color: #fecaca; })");

        statusLabel_->setStyleSheet(isDark ? "color: #C9C9DC;" : "color: #6b7280;");

        // 重新挂载图片以刷新颜色
        if (!currentImagePath_.isEmpty()) {
            mountImagePath(currentImagePath_, property("currentDisplayName").toString());
        }

        // 重新渲染 Markdown 内容
        if (renderer_ && resultEdit_) {
            renderer_->setTextColor(isDark ? QColor("#E6E7F0") : QColor("#1F2033"));
            if (!rawMarkdown_.trimmed().isEmpty() && !isLoading_) {
                renderer_->clearCache();
                QString html = renderer_->render(rawMarkdown_, resultEdit_->document());
                resultEdit_->setHtml(html);
                Latex::LatexRenderer::postProcessDocument(resultEdit_->document());
                resultEdit_->setSourceMarkdown(rawMarkdown_);
            }
        }
    };

    applyTheme();
    connect(&AlgeMate::ThemeManager::instance(), &AlgeMate::ThemeManager::themeChanged, this, [applyTheme](AlgeMate::ThemeManager::Theme){ applyTheme(); });
}

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
    QString filePath = QFileDialog::getOpenFileName(this, QStringLiteral("选择图片"), "", QStringLiteral("Images (*.png *.jpg *.jpeg *.bmp)"));
    if (!filePath.isEmpty()) {
        QFileInfo fi(filePath);
        mountImagePath(filePath, fi.fileName());
    }
}

void AiSolverPage::mountImagePath(const QString &path, const QString &displayName) {
    currentImagePath_ = path;
    setProperty("currentDisplayName", displayName);

    bool isDark = AlgeMate::ThemeManager::instance().currentTheme() == AlgeMate::ThemeManager::Theme::Dark;
    QString mainColor = isDark ? "#8FA1FF" : "#7e22ce";
    QString subColor = isDark ? "#C9C9DC" : "#6b7280";

    QImage img(path);
    QString thumbHtml = QStringLiteral("🖼️");

    if (!img.isNull()) {
        QImage thumb = img.scaled(56, 56, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        QByteArray ba;
        QBuffer buf(&ba);
        buf.open(QIODevice::WriteOnly);
        thumb.save(&buf, "PNG");

        const QString b64 = QString::fromLatin1(ba.toBase64());
        thumbHtml = QStringLiteral("<img src='data:image/png;base64,%1' style='vertical-align:middle;'/>").arg(b64);
    }

    QString htmlTemplate = R"(
        <table cellpadding='0' cellspacing='0'><tr>
        <td>%1</td>
        <td style='padding-left:10px;'>
        <span style='color:%3;font-weight:bold;'>已挂载图片</span><br/>
        <span style='color:%4;'>%2</span>
        <span style='color:#9ca3af;font-weight:normal;'> （点右侧 × 可取消）</span>
        </td></tr></table>
    )";

    imageLabel_->setText(QString(htmlTemplate).arg(thumbHtml, displayName.toHtmlEscaped(), mainColor, subColor));
    imageRow_->show();
}

bool AiSolverPage::extractImageFromMime(const QMimeData *mime) {
    if (!mime) return false;

    static const QStringList kImageSuffixes = { "png", "jpg", "jpeg", "bmp", "gif", "webp", "tif", "tiff" };

    if (mime->hasUrls()) {
        for (const QUrl &url : mime->urls()) {
            if (!url.isLocalFile()) continue;

            QString localPath = url.toLocalFile();
            QFileInfo fi(localPath);

            if (fi.isFile() && kImageSuffixes.contains(fi.suffix().toLower())) {
                mountImagePath(localPath, fi.fileName());
                return true;
            }
        }
    }

    if (mime->hasImage()) {
        QImage img = qvariant_cast<QImage>(mime->imageData());
        if (img.isNull()) return false;

        QString cacheDir = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
        if (cacheDir.isEmpty()) cacheDir = QDir::tempPath();
        QDir().mkpath(cacheDir);

        QString fname = QStringLiteral("algemate_drop_%1.png").arg(QDateTime::currentMSecsSinceEpoch());
        QString fullPath = QDir(cacheDir).filePath(fname);

        if (img.save(fullPath, "PNG")) {
            mountImagePath(fullPath, fname);
            return true;
        }
    }
    return false;
}

void AiSolverPage::dragEnterEvent(QDragEnterEvent *event) {
    if (event->mimeData() && (event->mimeData()->hasUrls() || event->mimeData()->hasImage())) {
        event->setDropAction(Qt::CopyAction);
        event->acceptProposedAction();
    } else {
        event->ignore();
    }
}

void AiSolverPage::dragMoveEvent(QDragMoveEvent *event) {
    if (event->mimeData() && (event->mimeData()->hasUrls() || event->mimeData()->hasImage())) {
        event->setDropAction(Qt::CopyAction);
        event->acceptProposedAction();
    } else {
        event->ignore();
    }
}

void AiSolverPage::dropEvent(QDropEvent *event) {
    if (extractImageFromMime(event->mimeData())) {
        event->setDropAction(Qt::CopyAction);
        event->acceptProposedAction();
        updateStatusLabel(statusLabel_, QStringLiteral("✅ 已接收拖入的图片"), "#10b981", "#48BB78");
    } else {
        event->ignore();
        updateStatusLabel(statusLabel_, QStringLiteral("⚠️ 未识别到可用图片"), "#f59e0b", "#ED8936");
    }
}

void AiSolverPage::onSendButtonClicked() {
    QString userInput = inputEdit_->toPlainText().trimmed();

    if (userInput.isEmpty() && currentImagePath_.isEmpty()) {
        QMessageBox::warning(this, QStringLiteral("提示"), QStringLiteral("请输入问题或上传一张图片"));
        return;
    }

    if (isLoading_) return;

    QSettings settings("AlgeMate", "AlgeMateApp");
    QString dsApiKey = settings.value("AI/DeepSeekApiKey", "").toString();
    QString dbApiKey = settings.value("AI/DoubaoApiKey", "").toString();

    if (!currentImagePath_.isEmpty() && dbApiKey.isEmpty()) {
        QMessageBox::warning(this, QStringLiteral("缺少配置"), QStringLiteral("请先在【设置中心】填写【豆包 OCR 密钥】！"));
        return;
    }
    if (currentImagePath_.isEmpty() && dsApiKey.isEmpty()) {
        QMessageBox::warning(this, QStringLiteral("缺少配置"), QStringLiteral("请先在【设置中心】填写【DeepSeek 密钥】！"));
        return;
    }
    if (dbApiKey.isEmpty()) {
        QMessageBox::warning(this, QStringLiteral("缺少配置"), QStringLiteral("请先在【设置中心】填写【豆包 API Key】！"));
        return;
    }

    // UI 显示用户输入
    resultEdit_->moveCursor(QTextCursor::End);
    resultEdit_->insertPlainText(QStringLiteral("\n\n【你】\n"));
    rawMarkdown_ += QStringLiteral("\n\n【你】\n");

    if (!currentImagePath_.isEmpty()) {
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

    // 分发请求逻辑
    if (!currentImagePath_.isEmpty()) {
        pendingUserInput_ = userInput;
        updateStatusLabel(statusLabel_, QStringLiteral("⏳ 豆包正在提取图片文字 (OCR)..."), "#f59e0b", "#ED8936");

        // 构造豆包 OCR 请求
        QNetworkRequest request{QUrl(DOUBAO_API_URL)};
        request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
        request.setRawHeader("Authorization", QStringLiteral("Bearer %1").arg(dbApiKey).toUtf8());

        QJsonObject requestBody;
        requestBody["model"] = QStringLiteral("doubao-seed-2-0-pro-260215");
        requestBody["stream"] = false;

        QJsonObject thinkingObj;
        thinkingObj["type"] = "disabled";
        requestBody["thinking"] = thinkingObj;

        QJsonArray messagesArray, contentArray;
        QJsonObject userMsg;
        userMsg["role"] = "user";

        // 图片转 Base64
        QImage image(currentImagePath_);
        QByteArray ba;
        QBuffer buffer(&ba);
        buffer.open(QIODevice::WriteOnly);
        image.scaled(1024, 1024, Qt::KeepAspectRatio, Qt::SmoothTransformation).save(&buffer, "JPEG", 85);

        QJsonObject imageObj, imageUrlObj, textObj;
        imageObj["type"] = "image_url";
        imageUrlObj["url"] = QStringLiteral("data:image/jpeg;base64,") + QString::fromLatin1(ba.toBase64());
        imageObj["image_url"] = imageUrlObj;

        textObj["type"] = "text";
        textObj["text"] = QStringLiteral("请提取图片中的所有文字和数学公式，把题目复写为latex格式，一字不差。不要尝试解答题目。");

        contentArray.append(imageObj);
        contentArray.append(textObj);
        userMsg["content"] = contentArray;
        messagesArray.append(userMsg);
        requestBody["messages"] = messagesArray;

        currentImagePath_.clear();
        imageRow_->hide();

        ocrReply_ = networkManager_->post(request, QJsonDocument(requestBody).toJson());
        connect(ocrReply_, &QNetworkReply::finished, this, &AiSolverPage::onDoubaoOcrFinished);
    } else {
        sendToDeepSeek(userInput);
    }
}

void AiSolverPage::onDoubaoOcrFinished() {
    if (!ocrReply_) return;

    QString ocrResult = QStringLiteral("[图片识别失败，请检查网络或豆包模型ID]");
    if (ocrReply_->error() == QNetworkReply::NoError) {
        QJsonDocument doc = QJsonDocument::fromJson(ocrReply_->readAll());
        if (doc.isObject()) {
            QJsonArray choices = doc.object()["choices"].toArray();
            if (!choices.isEmpty()) {
                ocrResult = choices[0].toObject()["message"].toObject()["content"].toString().trimmed();
            }
        }
    }
    ocrReply_->deleteLater();
    ocrReply_ = nullptr;

    QString finalPrompt = pendingUserInput_;
    if (!finalPrompt.isEmpty()) finalPrompt += QStringLiteral("\n\n");
    finalPrompt += QStringLiteral("以下是从图片中提取的题目内容：\n\n") + ocrResult + QStringLiteral("\n\n请帮我详细解答。");

    QSettings settings("AlgeMate", "AlgeMateApp");
    QString dsApiKey = settings.value("AI/DeepSeekApiKey", "").toString();

    if (dsApiKey.isEmpty()) {
        resultEdit_->moveCursor(QTextCursor::End);
        resultEdit_->insertPlainText(QStringLiteral("\n❌ 错误: 图片提取成功，但设置中没有填写 DeepSeek 密钥，无法进行解答。"));
        enableInputs(true);
        isLoading_ = false;
        updateStatusLabel(statusLabel_, QStringLiteral("就绪"), "#6b7280", "#C9C9DC");
        return;
    }

    sendToDeepSeek(finalPrompt);
}

void AiSolverPage::sendToDeepSeek(const QString& finalPrompt) {
    if (deepThinkEnabled_) {
        updateStatusLabel(statusLabel_, QStringLiteral("⏳ DeepSeek-Reasoner 深度思考中（仅输出最终结果，耗时较长）..."), "#f59e0b", "#ED8936");
    } else {
        updateStatusLabel(statusLabel_, QStringLiteral("⏳ DeepSeek 正在深度思考解题中..."), "#f59e0b", "#ED8936");
    }

    QString promptToSend = finalPrompt;
    if (deepThinkEnabled_) {
        promptToSend += QStringLiteral("\n\n请在内部充分思考、整理好结果后再输出。不要在输出里夹带自我修正的表述。");
    }

    QJsonObject userMsg;
    userMsg["role"] = "user";
    userMsg["content"] = promptToSend;
    chatHistory_.append(userMsg);

    QSettings settings("AlgeMate", "AlgeMateApp");
    QString dsApiKey = settings.value("AI/DeepSeekApiKey", "").toString();

    QNetworkRequest request{QUrl(DEEPSEEK_API_URL)};
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization", QStringLiteral("Bearer %1").arg(dsApiKey).toUtf8());

    QJsonObject requestBody;
    requestBody["model"] = deepThinkEnabled_ ? QStringLiteral("deepseek-reasoner") : QStringLiteral("deepseek-chat");
    requestBody["messages"] = chatHistory_;
    requestBody["stream"] = false;

    if (!deepThinkEnabled_) {
        requestBody["temperature"] = 0.7;
    }

    currentReply_ = networkManager_->post(request, QJsonDocument(requestBody).toJson());
    connect(currentReply_, &QNetworkReply::finished, this, &AiSolverPage::onReplyFinished);
}

void AiSolverPage::onReplyFinished() {
    if (!currentReply_) return;

    QString aiAnswer;
    bool ok = (currentReply_->error() == QNetworkReply::NoError);
    QString errorMsg;

    QByteArray respData = currentReply_->readAll();
    QJsonDocument doc = QJsonDocument::fromJson(respData);

    if (!ok) {
        errorMsg = currentReply_->errorString();
        if (doc.isObject() && doc.object().contains("error")) {
            QString apiError = doc.object()["error"].toObject()["message"].toString();
            if (!apiError.isEmpty()) errorMsg = apiError;
        }
    } else {
        if (doc.isObject()) {
            QJsonArray choices = doc.object()["choices"].toArray();
            if (!choices.isEmpty()) {
                aiAnswer = choices[0].toObject()["message"].toObject()["content"].toString();
            }
        }
        if (aiAnswer.trimmed().isEmpty()) {
            ok = false;
            errorMsg = QStringLiteral("DeepSeek 未返回有效内容");
        }
    }

    currentReply_->deleteLater();
    currentReply_ = nullptr;

    if (!ok) {
        resultEdit_->moveCursor(QTextCursor::End);
        resultEdit_->insertPlainText(QStringLiteral("\n\n❌ 错误: ") + errorMsg);
        rawMarkdown_ += QStringLiteral("\n\n❌ 错误: ") + errorMsg;
        updateStatusLabel(statusLabel_, QStringLiteral("❌ 错误: ") + errorMsg, "#ef4444", "#FC8181");
        enableInputs(true);
        isLoading_ = false;
        return;
    }

    pendingAiAnswer_ = aiAnswer;
    sendToPolish(aiAnswer);
}

void AiSolverPage::sendToPolish(const QString& aiAnswer) {
    QSettings settings("AlgeMate", "AlgeMateApp");
    QString dbApiKey = settings.value("AI/DoubaoApiKey", "").toString();

    if (dbApiKey.isEmpty()) {
        resultEdit_->moveCursor(QTextCursor::End);
        resultEdit_->insertPlainText(QStringLiteral("\n\n❌ 错误: 请先在【设置中心】配置【豆包 API Key】，否则无法整理输出。"));
        updateStatusLabel(statusLabel_, QStringLiteral("❌ 缺少豆包 API Key"), "#ef4444", "#FC8181");
        enableInputs(true);
        isLoading_ = false;
        pendingAiAnswer_.clear();
        return;
    }

    updateStatusLabel(statusLabel_, QStringLiteral("⏳ 正在整理输出..."), "#f59e0b", "#ED8936");

    rawMarkdown_ += QStringLiteral("\n【AlgeMate AI】\n");
    resultEdit_->moveCursor(QTextCursor::End);
    resultEdit_->insertPlainText(QStringLiteral("\n【AlgeMate AI】\n"));

    QNetworkRequest request{QUrl(DOUBAO_API_URL)};
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization", QStringLiteral("Bearer %1").arg(dbApiKey).toUtf8());

    QJsonObject requestBody;
    requestBody["model"] = QStringLiteral("doubao-seed-2-0-pro-260215");
    requestBody["stream"] = true;
    QJsonObject thinkingObj;
    thinkingObj["type"] = "disabled";
    requestBody["thinking"] = thinkingObj;

    QString sysPrompt = QStringLiteral(
        "你是一个严格的 LaTeX/Markdown 语法校对器，只输出修正后的 markdown 文本，不输出任何解释、绝对不要用代码块（``` ）包裹整段输出、不要追加说明。\n"
        "校对规则：\n"
        "1. 行内公式必须 $...$ 包裹，独立公式必须 $$...$$ 包裹；禁止裸出 \\begin{align*} / \\begin{equation} / \\begin{aligned} 等环境（必须放进 $$...$$ 内）。\n"
        "2. 矩阵和列向量统一写成 \\begin{pmatrix}...\\end{pmatrix}，行间用 \\\\ 分隔，列间用 & 分隔；禁止 \\begin{array}{c} 写单列。\n"
        "3. 检查并补全所有缺失的反斜杠（\\）、花括号（{}）、$ 符号、\\begin / \\end 配对、\\left / \\right 配对。\n"
        "4. 不允许 markdown 加粗与公式嵌套：**$x$** 改为 $\\mathbf{x}$。\n"
        "5. 保留原文所有解题步骤、文字、代码块和分点结构，只修语法错误，不改语义。\n"
        "6. 不要在文本中改写中文字符；只修改公式与 markdown 标记。\n"
        "现在请校对下面这段 markdown，并直接输出完整修正版："
    );

    QJsonArray messagesArray;
    QJsonObject sysMsg;
    sysMsg["role"] = "system";
    sysMsg["content"] = sysPrompt;
    messagesArray.append(sysMsg);

    QJsonObject userMsg;
    userMsg["role"] = "user";
    userMsg["content"] = aiAnswer;
    messagesArray.append(userMsg);

    requestBody["messages"] = messagesArray;

    polishReply_ = networkManager_->post(request, QJsonDocument(requestBody).toJson());
    connect(polishReply_, &QNetworkReply::readyRead, this, &AiSolverPage::onPolishReadyRead);
    connect(polishReply_, &QNetworkReply::finished, this, &AiSolverPage::onPolishFinished);
}

void AiSolverPage::onPolishReadyRead() {
    if (!polishReply_) return;

    QString strData = QString::fromUtf8(polishReply_->readAll());
    QStringList lines = strData.split("\n", Qt::SkipEmptyParts);

    for (const QString& line : lines) {
        if (!line.startsWith("data: ")) continue;

        QString jsonData = line.mid(6).trimmed();
        if (jsonData == "[DONE]") continue;

        QJsonDocument doc = QJsonDocument::fromJson(jsonData.toUtf8());
        if (doc.isObject()) {
            QJsonArray choices = doc.object()["choices"].toArray();
            if (!choices.isEmpty()) {
                QString content = choices[0].toObject()["delta"].toObject()["content"].toString();
                if (!content.isEmpty()) {
                    resultEdit_->moveCursor(QTextCursor::End);
                    resultEdit_->insertPlainText(content);
                    rawMarkdown_ += content;
                }
            }
        }
    }
}

void AiSolverPage::onPolishFinished() {
    if (!polishReply_) return;

    bool ok = (polishReply_->error() == QNetworkReply::NoError);
    QString errorMsg;

    if (!ok) {
        errorMsg = polishReply_->errorString();
        QJsonDocument doc = QJsonDocument::fromJson(polishReply_->readAll());
        if (doc.isObject() && doc.object().contains("error")) {
            QString apiError = doc.object()["error"].toObject()["message"].toString();
            if (!apiError.isEmpty()) errorMsg = apiError;
        }
    }

    polishReply_->deleteLater();
    polishReply_ = nullptr;

    if (!ok) {
        resultEdit_->moveCursor(QTextCursor::End);
        resultEdit_->insertPlainText(QStringLiteral("\n\n❌ 错误: ") + errorMsg + QStringLiteral("，请在【设置中心】检查【豆包 API Key】是否已正确配置。"));
        updateStatusLabel(statusLabel_, QStringLiteral("❌ 输出整理失败"), "#ef4444", "#FC8181");
        enableInputs(true);
        isLoading_ = false;
        pendingAiAnswer_.clear();
        return;
    }

    const QString marker = QStringLiteral("【AlgeMate AI】\n");
    int lastAiPos = rawMarkdown_.lastIndexOf(marker);
    if (lastAiPos != -1) {
        QString fullResponse = rawMarkdown_.mid(lastAiPos + marker.length()).trimmed();
        if (!fullResponse.isEmpty()) {
            QJsonObject assistantMsg;
            assistantMsg["role"] = "assistant";
            assistantMsg["content"] = fullResponse;
            chatHistory_.append(assistantMsg);
        }
    }

    renderer_->clearCache();
    QString html = renderer_->render(rawMarkdown_, resultEdit_->document());
    resultEdit_->setHtml(html);
    Latex::LatexRenderer::postProcessDocument(resultEdit_->document());
    resultEdit_->setSourceMarkdown(rawMarkdown_);

    QTextCursor cursor = resultEdit_->textCursor();
    cursor.movePosition(QTextCursor::End);
    resultEdit_->setTextCursor(cursor);

    updateStatusLabel(statusLabel_, QStringLiteral("✅ 完成"), "#10b981", "#48BB78");
    enableInputs(true);
    isLoading_ = false;
    pendingAiAnswer_.clear();
}

void AiSolverPage::onClearHistoryClicked() {
    resultEdit_->clear();
    currentImagePath_.clear();
    imageRow_->hide();
    rawMarkdown_.clear();

    // 仅保留 system prompt
    while (chatHistory_.size() > 1) {
        chatHistory_.removeAt(1);
    }
    updateStatusLabel(statusLabel_, QStringLiteral("✅ 对话历史已清空"), "#10b981", "#48BB78");
}

void AiSolverPage::onCopyAllClicked() {
    if (rawMarkdown_.trimmed().isEmpty()) {
        updateStatusLabel(statusLabel_, QStringLiteral("⚠️ 暂无内容可复制"), "#f59e0b", "#ED8936");
        return;
    }
    QApplication::clipboard()->setText(rawMarkdown_);
    updateStatusLabel(statusLabel_, QStringLiteral("✅ 已复制全部内容到剪贴板"), "#10b981", "#48BB78");
}

void AiSolverPage::enableInputs(bool enabled) {
    inputEdit_->setEnabled(enabled);
    sendButton_->setEnabled(enabled);
    clearButton_->setEnabled(enabled);
    uploadImgButton_->setEnabled(enabled);
    if (clearImageButton_) clearImageButton_->setEnabled(enabled);
    if (deepThinkButton_) deepThinkButton_->setEnabled(enabled);
}

void AiSolverPage::onClearImageClicked() {
    if (isLoading_) return;
    if (currentImagePath_.isEmpty()) {
        imageRow_->hide();
        return;
    }

    currentImagePath_.clear();
    imageRow_->hide();
    updateStatusLabel(statusLabel_, QStringLiteral("✅ 已取消挂载的图片"), "#10b981", "#48BB78");
}

} // namespace AlgeMate::AiSolver