#include "ApiClient.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QUrl>
#include <QDebug>

ApiClient::ApiClient(QObject* parent) : QObject(parent) {
    m_network = new QNetworkAccessManager(this);
    connect(m_network, &QNetworkAccessManager::finished, this, &ApiClient::onReplyFinished);
}

void ApiClient::sendQuestionToAI(const QString& prompt) {
    // 可根据自己AI服务端接口修改URL及POST体结构
    QUrl url("http://127.0.0.1:8000/ai_grading"); // 改为你的实际AI判题API
    QNetworkRequest req(url);
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QJsonObject obj;
    obj["prompt"] = prompt;

    m_network->post(req, QJsonDocument(obj).toJson());
}

void ApiClient::onReplyFinished(QNetworkReply* reply) {
    QByteArray ba = reply->readAll();
    QString res;
    if (reply->error() == QNetworkReply::NoError) {
        QJsonDocument doc = QJsonDocument::fromJson(ba);
        if (doc.isObject() && doc.object().contains("result"))
            res = doc.object()["result"].toString();
        else
            res = QString::fromUtf8(ba); // 兜底逻辑
    } else {
        res = QStringLiteral("AI判题请求失败：") + reply->errorString();
    }
    emit responseReady(res);
    reply->deleteLater();
}