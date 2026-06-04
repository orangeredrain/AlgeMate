#include "OcrAttachWidget.h"

#include <QBuffer>
#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QDropEvent>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QImage>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QMimeData>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QPushButton>
#include <QSettings>
#include <QStandardPaths>
#include <QTextStream>
#include <QUrl>
#include <QVBoxLayout>

namespace AlgeMate::AiSolver {

OcrAttachWidget::OcrAttachWidget(const QString& hintWhenIdle, QWidget* parent)
    : QWidget(parent), idleHint_(hintWhenIdle)
{
    netMgr_ = new QNetworkAccessManager(this);
    setupUi(hintWhenIdle);
    setAcceptDrops(true);
}

OcrAttachWidget::~OcrAttachWidget() = default;

void OcrAttachWidget::setupUi(const QString& hintWhenIdle)
{
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(4);

    // —— 第一行：上传按钮 + 文字提示 ——
    auto* btnRow = new QHBoxLayout;
    btnRow->setContentsMargins(0, 0, 0, 0);
    btnRow->setSpacing(8);

    uploadBtn_ = new QPushButton(QStringLiteral("📷 上传图片识别"), this);
    uploadBtn_->setCursor(Qt::PointingHandCursor);
    uploadBtn_->setMinimumHeight(34);
    uploadBtn_->setStyleSheet(QStringLiteral(
        "QPushButton { background:#f3e8ff; color:#7e22ce; border:1px solid #d8b4e2; border-radius:8px; padding:0 14px; font-size:13px; font-weight:bold; }"
        "QPushButton:hover { background:#e9d5ff; }"
        "QPushButton:disabled { background:#f3f4f6; color:#9ca3af; border-color:#e5e7eb; }"
    ));
    connect(uploadBtn_, &QPushButton::clicked, this, &OcrAttachWidget::onUploadClicked);

    hintLabel_ = new QLabel(hintWhenIdle, this);
    hintLabel_->setStyleSheet(QStringLiteral("color:#6b7280; font-size:12px;"));
    hintLabel_->setTextInteractionFlags(Qt::TextSelectableByMouse);

    btnRow->addWidget(uploadBtn_);
    btnRow->addWidget(hintLabel_, 1);

    // —— 第二行：缩略图 + × 删除（默认隐藏，挂载图片后显示）——
    thumbRow_ = new QWidget(this);
    auto* thumbLayout = new QHBoxLayout(thumbRow_);
    thumbLayout->setContentsMargins(0, 0, 0, 0);
    thumbLayout->setSpacing(8);

    thumbLabel_ = new QLabel(thumbRow_);
    thumbLabel_->setStyleSheet(QStringLiteral("color:#7e22ce; font-size:12px;"));
    thumbLayout->addWidget(thumbLabel_, 1, Qt::AlignVCenter);

    clearBtn_ = new QPushButton(QStringLiteral("\u00d7"), thumbRow_);
    clearBtn_->setToolTip(QStringLiteral("取消挂载的图片"));
    clearBtn_->setCursor(Qt::PointingHandCursor);
    clearBtn_->setFixedSize(22, 22);
    clearBtn_->setStyleSheet(QStringLiteral(
        "QPushButton { background:#f3f4f6; color:#6b7280; border:1px solid #e5e7eb; border-radius:11px; font-size:13px; font-weight:bold; padding:0; }"
        "QPushButton:hover { background:#fee2e2; color:#dc2626; border-color:#fecaca; }"
        "QPushButton:pressed { background:#fecaca; }"
    ));
    connect(clearBtn_, &QPushButton::clicked, this, &OcrAttachWidget::onClearClicked);
    thumbLayout->addWidget(clearBtn_, 0, Qt::AlignVCenter);

    thumbRow_->hide();

    root->addLayout(btnRow);
    root->addWidget(thumbRow_);
}

void OcrAttachWidget::setBusy(bool busy)
{
    busy_ = busy;
    uploadBtn_->setEnabled(!busy);
    clearBtn_->setEnabled(!busy);
    setAcceptDrops(!busy);
}

bool OcrAttachWidget::hasPending() const
{
    return !currentImagePath_.isEmpty() || reply_ != nullptr;
}

QString OcrAttachWidget::loadDoubaoKey() const
{
    QSettings settings(QStringLiteral("AlgeMate"), QStringLiteral("AlgeMateApp"));
    return settings.value(QStringLiteral("AI/DoubaoApiKey"), QString()).toString().trimmed();
}

void OcrAttachWidget::setHint(const QString& text, bool isError)
{
    hintLabel_->setText(text);
    hintLabel_->setStyleSheet(isError
                              ? QStringLiteral("color:#dc2626; font-size:12px;")
                              : QStringLiteral("color:#6b7280; font-size:12px;"));
    emit statusChanged(text, isError);
}

void OcrAttachWidget::resetUploadButtonState()
{
    uploadBtn_->setEnabled(!busy_);
    uploadBtn_->setText(QStringLiteral("📷 上传图片识别"));
}

void OcrAttachWidget::onUploadClicked()
{
    if (busy_) return;
    const QString filePath = QFileDialog::getOpenFileName(
        this, QStringLiteral("选择图片"), {},
        QStringLiteral("Images (*.png *.jpg *.jpeg *.bmp *.gif *.webp)"));
    if (filePath.isEmpty()) return;
    QFileInfo fi(filePath);
    mountImagePath(filePath, fi.fileName());
    startOcr();
}

void OcrAttachWidget::onClearClicked()
{
    if (busy_) return;
    if (reply_) {
        // 取消正在进行的 OCR
        reply_->abort();
    }
    currentImagePath_.clear();
    thumbRow_->hide();
    resetUploadButtonState();
    setHint(idleHint_);
}

void OcrAttachWidget::mountImagePath(const QString& path, const QString& displayName)
{
    currentImagePath_ = path;

    QImage img(path);
    QString thumbHtml;
    if (!img.isNull()) {
        QImage thumb = img.scaled(48, 48, Qt::KeepAspectRatio, Qt::SmoothTransformation);
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

    thumbLabel_->setText(QStringLiteral(
        "<table cellpadding='0' cellspacing='0'><tr>"
        "<td>%1</td>"
        "<td style='padding-left:8px;'>"
        "<span style='color:#7e22ce;font-weight:bold;'>已挂载</span> "
        "<span style='color:#6b7280;'>%2</span>"
        "</td></tr></table>"
    ).arg(thumbHtml, displayName.toHtmlEscaped()));
    thumbRow_->show();
}

bool OcrAttachWidget::extractImageFromMime(const QMimeData* mime)
{
    if (!mime) return false;
    static const QStringList kImageSuffixes = {
        QStringLiteral("png"),  QStringLiteral("jpg"),  QStringLiteral("jpeg"),
        QStringLiteral("bmp"),  QStringLiteral("gif"),  QStringLiteral("webp"),
        QStringLiteral("tif"),  QStringLiteral("tiff")
    };

    if (mime->hasUrls()) {
        const auto urls = mime->urls();
        for (const QUrl& url : urls) {
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
    if (mime->hasImage()) {
        QImage img = qvariant_cast<QImage>(mime->imageData());
        if (img.isNull()) return false;
        QString cacheDir = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
        if (cacheDir.isEmpty()) cacheDir = QDir::tempPath();
        QDir().mkpath(cacheDir);
        const QString fname = QStringLiteral("algemate_ocr_%1.png")
                                  .arg(QDateTime::currentMSecsSinceEpoch());
        const QString fullPath = QDir(cacheDir).filePath(fname);
        if (!img.save(fullPath, "PNG")) return false;
        mountImagePath(fullPath, fname);
        return true;
    }
    return false;
}

void OcrAttachWidget::dragEnterEvent(QDragEnterEvent* event)
{
    if (busy_) { event->ignore(); return; }
    const QMimeData* mime = event->mimeData();
    if (mime && (mime->hasUrls() || mime->hasImage())) {
        event->setDropAction(Qt::CopyAction);
        event->acceptProposedAction();
        return;
    }
    event->ignore();
}

void OcrAttachWidget::dragMoveEvent(QDragMoveEvent* event)
{
    if (busy_) { event->ignore(); return; }
    const QMimeData* mime = event->mimeData();
    if (mime && (mime->hasUrls() || mime->hasImage())) {
        event->setDropAction(Qt::CopyAction);
        event->acceptProposedAction();
        return;
    }
    event->ignore();
}

void OcrAttachWidget::dropEvent(QDropEvent* event)
{
    if (busy_) { event->ignore(); return; }
    if (extractImageFromMime(event->mimeData())) {
        event->setDropAction(Qt::CopyAction);
        event->acceptProposedAction();
        startOcr();
    } else {
        event->ignore();
        setHint(QStringLiteral("⚠️ 未识别到可用图片（仅 png/jpg/jpeg/bmp/gif/webp）"), true);
    }
}

void OcrAttachWidget::startOcr()
{
    if (currentImagePath_.isEmpty()) return;
    if (reply_) return; // 防重入

    const QString key = loadDoubaoKey();
    if (key.isEmpty()) {
        setHint(QStringLiteral("❌ 未找到豆包 API Key，请先在【设置中心】配置并保存"), true);
        emit ocrFailed(QStringLiteral("未配置豆包 API Key"));
        // 保留缩略图，让用户看到挂载状态；用户可以 × 取消
        return;
    }

    QImage image(currentImagePath_);
    if (image.isNull()) {
        setHint(QStringLiteral("❌ 图片无法读取"), true);
        emit ocrFailed(QStringLiteral("图片无法读取"));
        return;
    }

    setHint(QStringLiteral("⏳ 豆包正在识别图片中的文字与公式..."));
    uploadBtn_->setEnabled(false);
    uploadBtn_->setText(QStringLiteral("🔍 识别中..."));

    QUrl url(QStringLiteral("https://ark.cn-beijing.volces.com/api/v3/chat/completions"));
    QNetworkRequest request{url};
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization", QStringLiteral("Bearer %1").arg(key).toUtf8());

    QJsonObject requestBody;
    requestBody["model"] = QStringLiteral("doubao-seed-2-0-pro-260215");
    requestBody["stream"] = false;
    QJsonObject thinkingObj;
    thinkingObj["type"] = "disabled";
    requestBody["thinking"] = thinkingObj;

    QJsonArray messagesArray;
    QJsonObject userMsg;
    userMsg["role"] = "user";

    QByteArray ba;
    QBuffer buffer(&ba);
    buffer.open(QIODevice::WriteOnly);
    image.scaled(1024, 1024, Qt::KeepAspectRatio, Qt::SmoothTransformation)
         .save(&buffer, "JPEG", 85);

    QJsonArray contentArray;
    QJsonObject imageObj;
    imageObj["type"] = "image_url";
    QJsonObject imageUrlObj;
    imageUrlObj["url"] = QStringLiteral("data:image/jpeg;base64,")
                       + QString::fromLatin1(ba.toBase64());
    imageObj["image_url"] = imageUrlObj;
    contentArray.append(imageObj);

    QJsonObject textObj;
    textObj["type"] = "text";
    textObj["text"] = QStringLiteral(
        "请提取图片中的所有文字和数学公式，把内容复写为 LaTeX 格式，一字不差。"
        "你是一个无情的 OCR 机器，不要尝试解答或评论，只输出提取到的文字。");
    contentArray.append(textObj);

    userMsg["content"] = contentArray;
    messagesArray.append(userMsg);
    requestBody["messages"] = messagesArray;

    reply_ = netMgr_->post(request, QJsonDocument(requestBody).toJson());
    connect(reply_, &QNetworkReply::finished, this, &OcrAttachWidget::onOcrFinished);
}

void OcrAttachWidget::onOcrFinished()
{
    if (!reply_) return;

    const bool aborted = (reply_->error() == QNetworkReply::OperationCanceledError);
    QString resultText;
    QString errorMsg;

    if (reply_->error() == QNetworkReply::NoError) {
        const QByteArray data = reply_->readAll();
        QJsonDocument doc = QJsonDocument::fromJson(data);
        if (doc.isObject()) {
            const QJsonArray choices = doc.object()["choices"].toArray();
            if (!choices.isEmpty()) {
                const QJsonObject msg = choices[0].toObject()["message"].toObject();
                resultText = msg["content"].toString().trimmed();
            }
        }
        if (resultText.isEmpty()) {
            errorMsg = QStringLiteral("豆包返回内容为空");
        }
    } else if (!aborted) {
        errorMsg = reply_->errorString();
    }

    reply_->deleteLater();
    reply_ = nullptr;

    // 复位 UI（无论成功失败）
    currentImagePath_.clear();
    thumbRow_->hide();
    resetUploadButtonState();

    if (aborted) {
        setHint(QStringLiteral("已取消识别"), false);
        return;
    }
    if (!errorMsg.isEmpty()) {
        setHint(QStringLiteral("❌ OCR 失败：%1").arg(errorMsg), true);
        emit ocrFailed(errorMsg);
        return;
    }

    setHint(QStringLiteral("✅ 已识别 %1 个字符并写入答题区").arg(resultText.size()));
    emit ocrTextReady(resultText);
}

} // namespace AlgeMate::AiSolver
