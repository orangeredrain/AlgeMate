#ifndef ALGEMATE_AISOLVERPAGE_H
#define ALGEMATE_AISOLVERPAGE_H

#include <QWidget>
#include <QJsonArray>
#include <memory>

class QVBoxLayout;
class QLineEdit;
class QPushButton;
class QScrollArea;
class QTextEdit;
class QNetworkAccessManager;
class QNetworkReply;
class QLabel;

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
private slots:
    void onSendButtonClicked();
    void onClearHistoryClicked();
    void onReplyReadyRead();
    void onReplyFinished();
    void onApiKeyChanged(); // deepseek api
    void onDoubaoApiKeyChanged(); // 豆包 api
    void onDoubaoOcrFinished();//（豆包 OCR 完成）的槽函数
    void onUploadImageClicked();

private:
    void setupUI();
    void saveApiKey();
    void loadApiKey();
    void enableInputs(bool enabled);
    void sendToDeepSeek(const QString& finalPrompt);//豆包发给deepseek

    // UI Components
    QVBoxLayout*            mainLayout_       = nullptr;
    QTextEdit*              resultEdit_       = nullptr;
    Latex::LatexRenderer*   renderer_         = nullptr;//latex渲染
    QTextEdit*              inputEdit_        = nullptr;
    QLineEdit*              apiKeyEdit_       = nullptr;//deepseek api
    QLineEdit*              doubaoApiKeyEdit_ = nullptr; //豆包 api
    QLabel*                 statusLabel_      = nullptr;
    QPushButton*            sendButton_       = nullptr;
    QPushButton*            clearButton_      = nullptr;

    //图片上传相关组件
    QPushButton* uploadImgButton_  = nullptr;
    QLabel* imageLabel_       = nullptr;

    // Network
    std::unique_ptr<QNetworkAccessManager> networkManager_;
    QNetworkReply* currentReply_ = nullptr;
    QNetworkReply* ocrReply_ = nullptr; // 专门管理豆包的请求

    // State
    QString apiKey_;
    QString doubaoApiKey_;    // 对应豆包 Key
    QJsonArray chatHistory_;
    bool isLoading_ = false;
    QString currentImagePath_;// 保存当前选中的图片路径
    QString rawMarkdown_; // 用于保存完整且未被破坏的 Markdown 源码
    QString pendingUserInput_; // 在豆包识图期间，暂存用户的打字输入
};

}

#endif