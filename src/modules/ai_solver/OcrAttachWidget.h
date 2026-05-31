#ifndef ALGEMATE_OCR_ATTACH_WIDGET_H
#define ALGEMATE_OCR_ATTACH_WIDGET_H

#include <QWidget>
#include <QString>

class QPushButton;
class QLabel;
class QNetworkAccessManager;
class QNetworkReply;
class QDragEnterEvent;
class QDragMoveEvent;
class QDropEvent;
class QMimeData;

namespace AlgeMate::AiSolver {

/// 通用「上传题目/解答图片 → 豆包 OCR 识别 → emit 文本」组件。
/// 复用于 AI 解题、考试主观题、练习主观题。
/// 豆包 API Key 从 algemate_ai.conf 第二行读取（与 AiSolverPage 共用）。
class OcrAttachWidget : public QWidget {
    Q_OBJECT
public:
    /// hintWhenIdle: 空闲态显示在按钮旁边的提示（如「上传题目图片自动识别」）
    explicit OcrAttachWidget(const QString& hintWhenIdle, QWidget* parent = nullptr);
    ~OcrAttachWidget() override;

    /// 外部业务繁忙时（例如正在 AI 判卷）禁用所有交互
    void setBusy(bool busy);

    /// 当前是否有图片挂载或正在识别
    bool hasPending() const;

signals:
    /// OCR 识别成功，附带识别到的文字
    void ocrTextReady(const QString& text);
    /// OCR 失败 / 网络错误
    void ocrFailed(const QString& reason);
    /// 状态变化通知（用于状态栏显示，可选监听）
    void statusChanged(const QString& msg, bool isError);

protected:
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dragMoveEvent(QDragMoveEvent* event) override;
    void dropEvent(QDropEvent* event) override;

private slots:
    void onUploadClicked();
    void onClearClicked();
    void onOcrFinished();

private:
    void setupUi(const QString& hintWhenIdle);
    void mountImagePath(const QString& path, const QString& displayName);
    bool extractImageFromMime(const QMimeData* mime);
    void startOcr();
    QString loadDoubaoKey() const;
    void setHint(const QString& text, bool isError = false);
    void resetUploadButtonState();

    QPushButton* uploadBtn_   = nullptr;
    QPushButton* clearBtn_    = nullptr;
    QLabel*      thumbLabel_  = nullptr;
    QLabel*      hintLabel_   = nullptr;
    QWidget*     thumbRow_    = nullptr;

    QNetworkAccessManager* netMgr_ = nullptr;
    QNetworkReply*         reply_  = nullptr;

    QString currentImagePath_;
    QString idleHint_;
    bool    busy_ = false;
};

} // namespace AlgeMate::AiSolver

#endif // ALGEMATE_OCR_ATTACH_WIDGET_H
