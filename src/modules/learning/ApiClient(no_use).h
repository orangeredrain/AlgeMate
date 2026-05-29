#pragma once
#include <QObject>
#include <QString>
#include <QNetworkAccessManager>
#include <QNetworkReply>

// 核心接口: sendQuestionToAI(prompt), done后emit responseReady(result)
class ApiClient : public QObject {
    Q_OBJECT
public:
    explicit ApiClient(QObject* parent = nullptr);

    // prompt建议为题目+学生答案
    void sendQuestionToAI(const QString& prompt);

signals:
    void responseReady(const QString& result);

private slots:
    void onReplyFinished(QNetworkReply* reply);

private:
    QNetworkAccessManager* m_network;
};