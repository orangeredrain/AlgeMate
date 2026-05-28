#include "WrongBookPage.h"
#include "latex/LatexRenderer.h"
#include "latex/LatexTextBrowser.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QScrollArea>
#include <QFrame>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QMessageBox>
#include <QDateTime>
#include <algorithm>
#include <QGraphicsDropShadowEffect>

namespace AlgeMate::Learning {

static QPushButton* makeBackBtn(QWidget* parent = nullptr) {
    auto* btn = new QPushButton(QStringLiteral("← 返回"), parent);
    btn->setObjectName(QStringLiteral("LearnBackBtn"));
    btn->setStyleSheet("QPushButton { background: transparent; border: 1px solid #cbd5e0; padding: 6px 12px; border-radius: 6px; color: #4a5568;} QPushButton:hover { background: #edf2f7; }");
    return btn;
}

// ==================== 详情弹窗实现 ====================
WrongDetailDialog::WrongDetailDialog(const Question& q, const QString& time, int wrongCount, void* sharedRenderer, QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle(QStringLiteral("错题解析精研报告"));
    resize(620, 500);
    setStyleSheet("QDialog { background-color: #ffffff; }");

    auto* lay = new QVBoxLayout(this);
    lay->setContentsMargins(20, 20, 20, 20);
    lay->setSpacing(14);

    auto* metaLabel = new QLabel(QStringLiteral("🕒 错误时间：%1   |   🔥 累计错误：%2 次").arg(time).arg(wrongCount), this);
    metaLabel->setStyleSheet("color: #718096; font-size: 12px; background: #f7fafc; padding: 6px; border-radius: 4px;");
    lay->addWidget(metaLabel);

    auto* renderer = static_cast<Latex::LatexRenderer*>(sharedRenderer);

    // 1. 完整题干区
    auto* qBrowser = new Latex::LatexTextBrowser(this);
    qBrowser->setFrameShape(QFrame::NoFrame);
    qBrowser->setMinimumHeight(100);
    if (renderer) {
        renderer->clearCache();
        // 改用标准的三级 Markdown 标题进行传导渲染，界面字体柔和漂亮
        qBrowser->setHtml(renderer->render(QStringLiteral("### 📌 【核心题干】\n") + q.content, qBrowser->document()));
    }
    lay->addWidget(qBrowser);

    // 2. 选项渲染（针对单选题）
    if (q.type == QuestionType::Single) {
        auto* optBrowser = new Latex::LatexTextBrowser(this);
        optBrowser->setFrameShape(QFrame::NoFrame);
        optBrowser->setMinimumHeight(60);
        optBrowser->setStyleSheet("background: #f8fafc; padding: 8px; border-radius: 6px;");
        QString opts;
        for (int i = 0; i < q.choices.size(); ++i) {
            opts += QString("%1. %2<br/>").arg(QChar('A' + i)).arg(q.choices[i]);
        }
        if (renderer) optBrowser->setHtml(renderer->render(opts, optBrowser->document()));
        lay->addWidget(optBrowser);
    }

    // 3. 对照精析区
    auto* ansBrowser = new Latex::LatexTextBrowser(this);
    ansBrowser->setFrameShape(QFrame::NoFrame);
    ansBrowser->setMinimumHeight(140);

    if (renderer) {
        QString userAnsStr = q.userAnswer.isEmpty() ? QStringLiteral("未作答") : q.userAnswer;
        QString renderedUser = renderer->render(userAnsStr, ansBrowser->document());
        QString renderedCorrect = renderer->render(q.correctAnswer, ansBrowser->document());

        QString reportHtml = QString(
                                 "<div style='border-top: 1px dashed #cbd5e0; padding-top:10px; line-height:1.6; font-size:13px;'>"
                                 "  <p style='color:#e53e3e;'><b>❌ 我的历史作答:</b></p>"
                                 "  <blockquote style='background:#fff5f5; padding:8px; border-left:4px solid #e53e3e;'>%1</blockquote>"
                                 "  <p style='color:#38a169; margin-top:12px;'><b>✅ 官方标准解题参考:</b></p>"
                                 "  <blockquote style='background:#f0fff4; padding:8px; border-left:4px solid #38a169;'>%2</blockquote>"
                                 "</div>"
                                 ).arg(renderedUser, renderedCorrect);
        ansBrowser->setHtml(reportHtml);
    }
    lay->addWidget(ansBrowser);

    auto* closeBtn = new QPushButton(QStringLiteral("完成精研，返回错题本"), this);
    closeBtn->setFixedHeight(38);
    closeBtn->setStyleSheet("QPushButton { background: #3182ce; color: white; border-radius: 6px; font-weight: bold; } QPushButton:hover { background: #2b6cb0; }");
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::accept);
    lay->addWidget(closeBtn);
}

// ==================== 错题本主页实现 ====================
WrongBookPage::WrongBookPage(QWidget* parent)
    : QWidget(parent)
{
    auto* renderer = new Latex::LatexRenderer;
    renderer->addMathMacro(QStringLiteral("R"), QStringLiteral("\\mathbb{R}"));
    renderer->addMathMacro(QStringLiteral("C"), QStringLiteral("\\mathbb{C}"));
    this->setProperty("latex_renderer", QVariant::fromValue(static_cast<void*>(renderer)));

    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(24, 16, 24, 24);
    mainLayout->setSpacing(16);

    auto* top = new QHBoxLayout;
    auto* back = makeBackBtn(this);
    connect(back, &QPushButton::clicked, this, &WrongBookPage::backRequested);

    auto* title = new QLabel(QStringLiteral("错题本"), this);
    title->setStyleSheet("font-size: 26px; font-weight: 800; color: #111827;");

    top->addWidget(back); top->addWidget(title); top->addStretch();

    auto* sortGroup = new QHBoxLayout;
    auto* sortTimeBtn = new QPushButton(QStringLiteral("🕒 按错误时间排序"), this);
    auto* sortCountBtn = new QPushButton(QStringLiteral("📊 按错误次数排序"), this);

    QString sortStyle = "QPushButton { background: white; border: 1px solid #cbd5e0; padding: 6px 14px; border-radius: 6px; font-size: 12px; color: #4a5568; font-weight: 500; } QPushButton:hover { background: #f7fafc; }";
    sortTimeBtn->setStyleSheet(sortStyle); sortCountBtn->setStyleSheet(sortStyle);

    this->setProperty("sort_mode", "time");
    connect(sortTimeBtn, &QPushButton::clicked, this, [this]() { this->setProperty("sort_mode", "time"); reload(); });
    connect(sortCountBtn, &QPushButton::clicked, this, [this]() { this->setProperty("sort_mode", "count"); reload(); });

    sortGroup->addWidget(sortTimeBtn); sortGroup->addWidget(sortCountBtn); sortGroup->addStretch();

    auto* scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);

    auto* container = new QWidget;
    container->setStyleSheet(R"(background:qlineargradient(x1:0,y1:0,x2:1,y2:1,stop:0 #f8fafc,stop:1 #eef2ff);)");
    // 关键升级：更换为网格布局
    contentLayout = new QGridLayout(container);
    contentLayout->setContentsMargins(4, 4, 4, 4);
    contentLayout->setHorizontalSpacing(24);
    contentLayout->setVerticalSpacing(24);
    contentLayout->setColumnStretch(0, 1);
    contentLayout->setColumnStretch(1, 1);

    scrollArea->setWidget(container);
    mainLayout->addLayout(top);
    mainLayout->addLayout(sortGroup);
    mainLayout->addWidget(scrollArea, 1);

    loadWrongQuestions();
}

void WrongBookPage::loadWrongQuestions()
{
    QFile file("wrong_questions.json");
    if (!file.exists() || !file.open(QIODevice::ReadOnly)) {
        auto* emptyLabel = new QLabel(QStringLiteral("🎉 还没有任何错题记录，继续保持！"), this);
        emptyLabel->setAlignment(Qt::AlignCenter); emptyLabel->setStyleSheet("font-size: 18px; color: #6b7280; padding: 80px;");
        contentLayout->addWidget(emptyLabel, 0, 0); return;
    }

    QJsonArray arr = QJsonDocument::fromJson(file.readAll()).array(); file.close();
    if (arr.isEmpty()) {
        auto* emptyLabel = new QLabel(QStringLiteral("🎉 还没有任何错题记录，继续保持！"), this);
        emptyLabel->setAlignment(Qt::AlignCenter); emptyLabel->setStyleSheet("font-size: 18px; color: #6b7280; padding: 80px;");
        contentLayout->addWidget(emptyLabel, 0, 0); return;
    }

    std::vector<QJsonObject> sortedItems;
    for (const auto& val : arr) sortedItems.push_back(val.toObject());

    QString currentMode = this->property("sort_mode").toString();
    if (currentMode == "count") {
        std::sort(sortedItems.begin(), sortedItems.end(), [](const QJsonObject& a, const QJsonObject& b) {
            return a["wrongCount"].toInt() > b["wrongCount"].toInt();
        });
    } else {
        std::sort(sortedItems.begin(), sortedItems.end(), [](const QJsonObject& a, const QJsonObject& b) {
            QDateTime dtA = QDateTime::fromString(a["time"].toString(), "yyyy-MM-dd hh:mm:ss");
            QDateTime dtB = QDateTime::fromString(b["time"].toString(), "yyyy-MM-dd hh:mm:ss");
            return dtA > dtB;
        });
    }

    int index = 0;
    for (const auto& obj : sortedItems) {
        Question q;
        q.id = obj["id"].toInt();
        q.content = obj["content"].toString();
        q.userAnswer = obj["userAnswer"].toString();
        q.correctAnswer = obj["correctAnswer"].toString();
        q.score = obj["score"].toInt();

        QString type = obj["type"].toString();
        q.type = (type == "single") ? QuestionType::Single : ((type == "fill") ? QuestionType::Fill : QuestionType::Subjective);

        QJsonArray choicesArray = obj["choices"].toArray();
        for (const auto& v : choicesArray) q.choices.append(v.toString());

        addWrongQuestionCard(q, obj["time"].toString(), obj["wrongCount"].toInt(), index++);
    }
}

// 核心改造：纯题干方形卡片视图
void WrongBookPage::addWrongQuestionCard(const Question& q, const QString& time, int wrongCount, int index)
{
    auto* card = new QFrame(this);
    card->setMinimumHeight(200);
    card->setMaximumWidth(520);
    card->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    card->setStyleSheet(R"(
    QFrame {
        background: rgba(255,255,255,0.96);
        border-radius: 22px;
        border: 1px solid #e5e7eb;
    }

    QFrame:hover {
        border: 1px solid #c7d2fe;
        background: white;
    }
    )");
    auto* shadow = new QGraphicsDropShadowEffect(card);
    shadow->setBlurRadius(30);
    shadow->setOffset(0, 8);
    shadow->setColor(QColor(15, 23, 42, 20));
    card->setGraphicsEffect(shadow);

    auto* lay = new QVBoxLayout(card);
    lay->setContentsMargins(16, 12, 16, 12); lay->setSpacing(8);

    auto* topRow = new QHBoxLayout;
    auto* typeLabel = new QLabel(QString("[%1]").arg((q.type == QuestionType::Single) ? "单选题" : ((q.type == QuestionType::Fill) ? "填空题" : "解答题")));
    typeLabel->setStyleSheet(R"(
    QLabel{
        background:#eef2ff;
        color:#4f46e5;
        border-radius:10px;
        padding:4px 10px;
        font-weight:700;
        font-size:11px;
    }
    )");
    auto* countLabel = new QLabel(QStringLiteral("🔥 错误 %1 次").arg(wrongCount));
    countLabel->setStyleSheet(R"(
    QLabel{
        color:#64748b;
        font-size:12px;
        font-weight:600;
    }
    )");
    topRow->addWidget(typeLabel); topRow->addStretch(); topRow->addWidget(countLabel);
    lay->addLayout(topRow);

    // 仅预览题干
    auto* qBrowser = new Latex::LatexTextBrowser(card);
    qBrowser->setFrameShape(QFrame::NoFrame);
    qBrowser->setStyleSheet(R"(
    background:#f8fafc;
    border-radius:14px;
    padding:12px;
    )");
    qBrowser->setMinimumHeight(95);
    qBrowser->setAttribute(Qt::WA_TransparentForMouseEvents, true);

    auto* renderer = static_cast<Latex::LatexRenderer*>(this->property("latex_renderer").value<void*>());
    if (renderer) {
        renderer->clearCache();
        QString previewText = q.content;
        if (previewText.length() > 60) previewText = previewText.left(60) + "...";
        qBrowser->setHtml(renderer->render(previewText, qBrowser->document()));
    }
    lay->addWidget(qBrowser, 1);

    //时间信息
    auto* timeLabel = new QLabel(QString("🕒 %1").arg(time), card);
    timeLabel->setStyleSheet(R"(
    QLabel{
        color:#94a3b8;
        font-size:11px;
    }
    )");
    lay->addWidget(timeLabel);


    auto* detailBtn = new QPushButton(QStringLiteral("查看解析 →"), card);
    detailBtn->setFixedHeight(30);
    detailBtn->setCursor(Qt::PointingHandCursor);
    detailBtn->setStyleSheet(R"(
    QPushButton{
        background:qlineargradient(
            x1:0,y1:0,x2:1,y2:0,
            stop:0 #f5f3ff,
            stop:1 #ede9fe
        );

        color:#7c3aed;
        border:1px solid #ddd6fe;
        border-radius:14px;

        font-size:13px;
        font-weight:700;

        padding:9px 12px;
    }

    QPushButton:hover{
        background:#ede9fe;
        border:1px solid #c4b5fd;
    }

    QPushButton:pressed{
        background:#ddd6fe;
    }
    )");
    lay->addWidget(detailBtn);

    connect(detailBtn, &QPushButton::clicked, this, [this, q, time, wrongCount, renderer]() {
        WrongDetailDialog dlg(q, time, wrongCount, renderer, this);
        dlg.exec();
    });

    // 计算网格坐标（双列排布）
    int row = index / 2;
    int col = index % 2;
    contentLayout->addWidget(card, row, col);
}

void WrongBookPage::reload()
{
    QLayoutItem* child;
    while ((child = contentLayout->takeAt(0)) != nullptr) {
        if (child->widget()) { child->widget()->hide(); child->widget()->deleteLater(); }
        delete child;
    }
    loadWrongQuestions();
}

} // namespace AlgeMate::Learning