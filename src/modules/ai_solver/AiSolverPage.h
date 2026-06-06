#ifndef ALGEMATE_AISOLVERPAGE_H
#define ALGEMATE_AISOLVERPAGE_H

#include <QWidget>
#include <QJsonArray>
#include <memory>

class QVBoxLayout;
class QPushButton;
class QTextEdit;
namespace AlgeMate::Latex { class LatexTextBrowser; }
class QNetworkAccessManager;
class QNetworkReply;
class QLabel;
class QMimeData;
class QDragEnterEvent;
class QDragMoveEvent;
class QDropEvent;

namespace AlgeMate::Latex {
class LatexRenderer;
class LatexTextBrowser;
}

namespace AlgeMate::AiSolver {

class AiSolverPage : public QWidget {
    Q_OBJECT
public:
    explicit AiSolverPage(QWidget* parent = nullptr);
    ~AiSolverPage() override;
protected:
    // 用于拦截 QTextEdit 的 Enter 回车按键
    bool eventFilter(QObject *obj, QEvent *event) override;
    // 拖拽图片支持（桌面文件 / 微信图片等）
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dragMoveEvent(QDragMoveEvent *event) override;
    void dropEvent(QDropEvent *event) override;
private slots:
    void onSendButtonClicked();
    void onClearHistoryClicked();
    void onReplyFinished();
    void onDoubaoOcrFinished(); // 豆包 OCR 完成的槽函数
    void onPolishReadyRead();
    void onPolishFinished();
    void onUploadImageClicked();
    void onClearImageClicked(); // 点 × 取消已挂载的图片
    void onCopyAllClicked();    // 一键复制全部 markdown（类似豆包复制按钮）

private:
    void setupUI();
    void enableInputs(bool enabled);
    void sendToDeepSeek(const QString& finalPrompt); // 豆包发给 deepseek
    void sendToPolish(const QString& aiAnswer);

    // 拖拽辅助：从 QMimeData 中提取图片并挂载到 currentImagePath_
    bool extractImageFromMime(const QMimeData *mime);
    void mountImagePath(const QString &path, const QString &displayName);

    // UI Components
    QVBoxLayout*            mainLayout_       = nullptr;
    Latex::LatexTextBrowser*  resultEdit_       = nullptr;
    Latex::LatexRenderer*   renderer_         = nullptr; // latex渲染
    QTextEdit*              inputEdit_        = nullptr;
    QLabel*                 statusLabel_      = nullptr;
    QPushButton*            sendButton_       = nullptr;
    QPushButton*            clearButton_      = nullptr;
    QPushButton*            copyAllButton_    = nullptr; // 一键复制全部 markdown

    // 图片上传相关组件
    QPushButton* uploadImgButton_  = nullptr;
    QWidget*     imageRow_         = nullptr; // 缩略图 + 删除按钮的外层容器
    QLabel*      imageLabel_       = nullptr;
    QPushButton* clearImageButton_ = nullptr; // × 取消已挂载图片

    // 深度思考模式（deepseek-reasoner）
    QPushButton* deepThinkButton_  = nullptr;
    bool         deepThinkEnabled_ = false;

    // Network
    std::unique_ptr<QNetworkAccessManager> networkManager_;
    QNetworkReply* currentReply_ = nullptr;
    QNetworkReply* ocrReply_ = nullptr; // 专门管理豆包的请求
    QNetworkReply* polishReply_ = nullptr;

    // State
    QJsonArray chatHistory_;
    bool isLoading_ = false;
    QString currentImagePath_; // 保存当前选中的图片路径
    QString rawMarkdown_; // 用于保存完整且未被破坏的 Markdown 源码
    QString pendingUserInput_; // 在豆包识图期间，暂存用户的打字输入
    QString pendingAiAnswer_;
};

}

#endif