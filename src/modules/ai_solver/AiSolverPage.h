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

private slots:
    void onSendButtonClicked();
    void onClearHistoryClicked();
    void onReplyReadyRead();
    void onReplyFinished();
    void onApiKeyChanged();

private:
    void setupUI();
    void saveApiKey();
    void loadApiKey();
    void enableInputs(bool enabled);

    // UI Components
    QVBoxLayout*            mainLayout_       = nullptr;
    QTextEdit*              resultEdit_       = nullptr;
    Latex::LatexRenderer*   renderer_         = nullptr;//newly added
    QLineEdit*              inputEdit_        = nullptr;
    QLineEdit*              apiKeyEdit_       = nullptr;
    QLabel*                 statusLabel_      = nullptr;
    QPushButton*            sendButton_       = nullptr;
    QPushButton*            clearButton_      = nullptr;

    // Network
    std::unique_ptr<QNetworkAccessManager> networkManager_;
    QNetworkReply* currentReply_ = nullptr;

    // State
    QString apiKey_;
    QJsonArray chatHistory_;
    bool isLoading_ = false;
};

}

#endif
