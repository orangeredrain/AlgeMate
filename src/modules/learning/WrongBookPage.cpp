#include "WrongBookPage.h"
#include "latex/LatexRenderer.h"
#include "latex/LatexTextBrowser.h"
#include "core/ThemeManager.h" // <-- 引入主题管理器

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
#include <random>
#include <QGraphicsDropShadowEffect>
#include <QInputDialog>

namespace AlgeMate::Learning {

// ==================== 详情弹窗实现 ====================
WrongDetailDialog::WrongDetailDialog(const Question& q, const QString& time, int wrongCount, void* sharedRenderer, bool isRedoMode, QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle(isRedoMode ? QStringLiteral("🎯 错题盲盒重做中...") : QStringLiteral("错题解析精研报告"));
    resize(620, 560);

    auto* lay = new QVBoxLayout(this);
    lay->setContentsMargins(20, 20, 20, 20);
    lay->setSpacing(14);

    QLabel* metaLabel = nullptr;
    if (!isRedoMode) {
        metaLabel = new QLabel(QStringLiteral("🕒 错误时间：%1   |   🔥 累计错误：%2 次").arg(time).arg(wrongCount), this);
        lay->addWidget(metaLabel);
    } else {
        metaLabel = new QLabel(QStringLiteral("📝 正在进行全本错题打乱重做，请认真作答："), this);
        lay->addWidget(metaLabel);
    }

    auto* renderer = static_cast<Latex::LatexRenderer*>(sharedRenderer);

    // 1. 题干区
    auto* qBrowser = new Latex::LatexTextBrowser(this);
    qBrowser->setFrameShape(QFrame::NoFrame);
    qBrowser->setMinimumHeight(100);
    lay->addWidget(qBrowser);

    // 2. 选项区
    Latex::LatexTextBrowser* optBrowser = nullptr;
    if (q.type == QuestionType::Single) {
        optBrowser = new Latex::LatexTextBrowser(this);
        optBrowser->setFrameShape(QFrame::NoFrame);
        optBrowser->setMinimumHeight(60);
        lay->addWidget(optBrowser);
    }

    // 3. 解析对照区
    Latex::LatexTextBrowser* ansBrowser = nullptr;
    if (!isRedoMode) {
        ansBrowser = new Latex::LatexTextBrowser(this);
        ansBrowser->setFrameShape(QFrame::NoFrame);
        ansBrowser->setMinimumHeight(140);
        lay->addWidget(ansBrowser);
    }

    // 4. 底部按钮控制区
    QPushButton* actionBtn = nullptr;
    QPushButton* deleteBtn = nullptr;
    QPushButton* closeBtn = nullptr;

    if (isRedoMode) {
        actionBtn = new QPushButton(QStringLiteral("⌨️ 输入答案并提交"), this);
        actionBtn->setFixedHeight(40);
        int currentId = q.id;
        connect(actionBtn, &QPushButton::clicked, this, [this, currentId]() {
            bool ok;
            QString text = QInputDialog::getText(this, QStringLiteral("错题重做"), QStringLiteral("请输入您的新答案:"), QLineEdit::Normal, "", &ok);
            if (ok) { emit redoRequested(currentId, text); accept(); }
        });
        lay->addWidget(actionBtn);
    } else {
        deleteBtn = new QPushButton(QStringLiteral("🗑️ 从错题本中移除此题"), this);
        deleteBtn->setFixedHeight(36);
        int targetId = q.id;
        connect(deleteBtn, &QPushButton::clicked, this, [this, targetId]() {
            if (QMessageBox::question(this, QStringLiteral("确认移除"), QStringLiteral("确定要将这道错题移除吗？")) == QMessageBox::Yes) {
                emit deleteRequested(targetId); accept();
            }
        });
        lay->addWidget(deleteBtn);

        closeBtn = new QPushButton(QStringLiteral("完成精研，返回错题本"), this);
        closeBtn->setFixedHeight(36);
        connect(closeBtn, &QPushButton::clicked, this, &QDialog::accept);
        lay->addWidget(closeBtn);
    }

    // 动态渲染及换色函数
    auto applyThemeToDialog = [=]() {
        bool isDark = AlgeMate::ThemeManager::instance().currentTheme() == AlgeMate::ThemeManager::Theme::Dark;
        this->setStyleSheet(isDark
                                ? "QDialog { background-color: #1C1B2E; color: #E6E7F0; } QLabel { color: #E6E7F0; } LatexTextBrowser { background: #28263F; color: #E6E7F0; border: 1px solid #3B395A; }"
                                : "QDialog { background-color: #ffffff; color: #1f2937; } QLabel { color: #1f2937; } LatexTextBrowser { background: #ffffff; color: #1f2937; border: none; }");

        if (!isRedoMode && metaLabel) {
            metaLabel->setStyleSheet(isDark ? "color: #7B7B96; font-size: 12px; background: #28263F; padding: 6px; border-radius: 4px; border: 1px solid #3B395A;"
                                            : "color: #718096; font-size: 12px; background: #f7fafc; padding: 6px; border-radius: 4px;");
        } else if (metaLabel) {
            metaLabel->setStyleSheet(isDark ? "color: #8FA1FF; font-size: 13px; font-weight: bold; background: #312F4A; padding: 6px; border-radius: 4px;"
                                            : "color: #3182ce; font-size: 13px; font-weight: bold; background: #ebf8ff; padding: 6px; border-radius: 4px;");
        }

        if (renderer) {
            renderer->setTextColor(isDark ? QColor("#E6E7F0") : QColor("#1f2937"));
            qBrowser->setHtml(renderer->render(QStringLiteral("### 📌 【题目题干】\n") + q.content, qBrowser->document()));

            if (optBrowser) {
                optBrowser->setStyleSheet(isDark ? "background: #28263F; color: #E6E7F0; padding: 8px; border-radius: 6px;" : "background: #f8fafc; padding: 8px; border-radius: 6px;");
                QString opts; for (int i = 0; i < q.choices.size(); ++i) opts += QString("%1. %2<br/>").arg(QChar('A' + i)).arg(q.choices[i]);
                optBrowser->setHtml(renderer->render(opts, optBrowser->document()));
            }

            if (ansBrowser && !isRedoMode) {
                QString userAnsStr = q.userAnswer.isEmpty() ? QStringLiteral("未作答") : q.userAnswer;
                QString correctAnsStr = q.correctAnswer;
                if (q.type == QuestionType::Single) {
                    bool okU, okC; int uIdx = q.userAnswer.toInt(&okU); int cIdx = q.correctAnswer.toInt(&okC);
                    if (okU && !q.userAnswer.isEmpty()) userAnsStr = QString(QChar('A' + uIdx));
                    if (okC && !q.correctAnswer.isEmpty()) correctAnsStr = QString(QChar('A' + cIdx));
                }
                QString renderedUser = renderer->render(userAnsStr, ansBrowser->document());
                QString renderedCorrect = renderer->render(correctAnsStr, ansBrowser->document());
                QString reportHtml = isDark ? QString(
                                                  "<div style='border-top: 1px dashed #4B4970; padding-top:10px; line-height:1.6; font-size:13px;'>"
                                                  "  <p style='color:#FC8181;'><b>❌ 我的历史作答:</b></p>"
                                                  "  <blockquote style='background:#3B2230; padding:8px; border-left:4px solid #FC8181;'>%1</blockquote>"
                                                  "  <p style='color:#48BB78; margin-top:12px;'><b>✅ 官方标准解题参考:</b></p>"
                                                  "  <blockquote style='background:#22543D; padding:8px; border-left:4px solid #48BB78;'>%2</blockquote>"
                                                  "</div>").arg(renderedUser, renderedCorrect)
                                            : QString(
                                                  "<div style='border-top: 1px dashed #cbd5e0; padding-top:10px; line-height:1.6; font-size:13px;'>"
                                                  "  <p style='color:#e53e3e;'><b>❌ 我的历史作答:</b></p>"
                                                  "  <blockquote style='background:#fff5f5; padding:8px; border-left:4px solid #e53e3e;'>%1</blockquote>"
                                                  "  <p style='color:#38a169; margin-top:12px;'><b>✅ 官方标准解题参考:</b></p>"
                                                  "  <blockquote style='background:#f0fff4; padding:8px; border-left:4px solid #38a169;'>%2</blockquote>"
                                                  "</div>").arg(renderedUser, renderedCorrect);
                ansBrowser->setHtml(reportHtml);
            }
        }

        if (actionBtn) actionBtn->setStyleSheet(isDark ? "QPushButton { background: #312F4A; color: #8FA1FF; border-radius: 6px; font-weight: bold; font-size: 14px; } QPushButton:hover { background: #3B395A; }"
                                            : "QPushButton { background: #3182ce; color: white; border-radius: 6px; font-weight: bold; font-size: 14px; } QPushButton:hover { background: #2b6cb0; }");
        if (deleteBtn) deleteBtn->setStyleSheet(isDark ? "QPushButton { background: #3B2230; color: #FC8181; border: 1px solid #6B2A3A; border-radius: 6px; font-weight: bold; } QPushButton:hover { background: #4A2935; }"
                                            : "QPushButton { background: #fff5f5; color: #c53030; border: 1px solid #fed7d7; border-radius: 6px; font-weight: bold; } QPushButton:hover { background: #feb2b2; }");
        if (closeBtn)  closeBtn->setStyleSheet(isDark  ? "QPushButton { background: #3B395A; color: #E6E7F0; border-radius: 6px; } QPushButton:hover { background: #4B4970; }"
                                           : "QPushButton { background: #4a5568; color: white; border-radius: 6px; } QPushButton:hover { background: #2d3748; }");
    };

    applyThemeToDialog();
    connect(&AlgeMate::ThemeManager::instance(), &AlgeMate::ThemeManager::themeChanged, this, [applyThemeToDialog](AlgeMate::ThemeManager::Theme){ applyThemeToDialog(); });
}

// ==================== 错题本主页实现 ====================
WrongBookPage::WrongBookPage(QWidget* parent)
    : QWidget(parent)
{
    auto* renderer = new Latex::LatexRenderer;
    renderer->addMathMacro(QStringLiteral("F"),  QStringLiteral("\\mathbb{F}"));
    renderer->addMathMacro(QStringLiteral("R"),  QStringLiteral("\\mathbb{R}"));
    renderer->addMathMacro(QStringLiteral("C"),  QStringLiteral("\\mathbb{C}"));
    renderer->addMathMacro(QStringLiteral("Q"),  QStringLiteral("\\mathbb{Q}"));
    renderer->addMathMacro(QStringLiteral("Z"),  QStringLiteral("\\mathbb{Z}"));
    renderer->addMathMacro(QStringLiteral("N"),  QStringLiteral("\\mathbb{N}"));
    this->setProperty("latex_renderer", QVariant::fromValue(static_cast<void*>(renderer)));

    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(24, 16, 24, 24);
    mainLayout->setSpacing(16);

    auto* top = new QHBoxLayout;

    auto* back = new QPushButton(QStringLiteral("← 返回"), this);
    connect(back, &QPushButton::clicked, this, &WrongBookPage::backRequested);

    auto* title = new QLabel(QStringLiteral("错题本"), this);

    top->addWidget(back); top->addWidget(title); top->addStretch();

    auto* sortGroup = new QHBoxLayout;
    auto* sortTimeBtn = new QPushButton(QStringLiteral("🕒 按错误时间"), this);
    auto* sortCountBtn = new QPushButton(QStringLiteral("📊 按错误次数"), this);
    auto* redoAllBtn = new QPushButton(QStringLiteral("🔄 错题重做 (乱序)"), this);
    auto* clearAllBtn = new QPushButton(QStringLiteral("🗑️ 清空所有错题"), this);

    this->setProperty("sort_mode", "time");
    connect(sortTimeBtn, &QPushButton::clicked, this, [this]() { this->setProperty("sort_mode", "time"); reload(); });
    connect(sortCountBtn, &QPushButton::clicked, this, [this]() { this->setProperty("sort_mode", "count"); reload(); });
    connect(redoAllBtn, &QPushButton::clicked, this, &WrongBookPage::startShuffleRedoWorkflow);
    connect(clearAllBtn, &QPushButton::clicked, this, [this]() {
        if (QMessageBox::warning(this, QStringLiteral("警告"), QStringLiteral("确定要永久清空错题本里的所有题目吗？"), QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes) {
            QFile file("wrong_questions.json");
            if (file.open(QIODevice::WriteOnly)) { file.write(QJsonDocument(QJsonArray()).toJson()); file.close(); }
            reload();
        }
    });

    sortGroup->addWidget(sortTimeBtn); sortGroup->addWidget(sortCountBtn);
    sortGroup->addStretch();
    sortGroup->addWidget(redoAllBtn); sortGroup->addWidget(clearAllBtn);

    auto* scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);

    // 【修正点】：使用局部变量 container，并放入 Lambda 捕获列表中
    auto* container = new QWidget;
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

    // ==============================================================
    // ==================== 统一应用暗色主题样式的 Lambda 函数 =========
    // 捕获局部的 container
    auto applyTheme = [this, back, title, sortTimeBtn, sortCountBtn, redoAllBtn, clearAllBtn, container]() {
        bool isDark = AlgeMate::ThemeManager::instance().currentTheme() == AlgeMate::ThemeManager::Theme::Dark;

        this->setStyleSheet(isDark ? "WrongBookPage { background-color: #1C1B2E; }" : "WrongBookPage { background-color: #f8fafc; }");

        back->setStyleSheet(isDark
                                ? "QPushButton { background: transparent; border: 1px solid #3B395A; padding: 6px 12px; border-radius: 6px; color: #C9C9DC;} QPushButton:hover { background: #28263F; }"
                                : "QPushButton { background: transparent; border: 1px solid #cbd5e0; padding: 6px 12px; border-radius: 6px; color: #4a5568;} QPushButton:hover { background: #edf2f7; }");

        title->setStyleSheet(isDark ? "font-size: 26px; font-weight: 800; color: #E6E7F0;" : "font-size: 26px; font-weight: 800; color: #111827;");

        QString sortStyle = isDark
                                ? "QPushButton { background: #28263F; border: 1px solid #3B395A; padding: 6px 12px; border-radius: 6px; font-size: 12px; color: #C9C9DC; } QPushButton:hover { background: #312F4A; }"
                                : "QPushButton { background: white; border: 1px solid #cbd5e0; padding: 6px 12px; border-radius: 6px; font-size: 12px; color: #4a5568; } QPushButton:hover { background: #f7fafc; }";
        sortTimeBtn->setStyleSheet(sortStyle); sortCountBtn->setStyleSheet(sortStyle);

        redoAllBtn->setStyleSheet(isDark
                                      ? "QPushButton { background: #312F4A; border: 1px solid #3B395A; padding: 6px 14px; border-radius: 6px; font-size: 12px; color: #8FA1FF; font-weight: bold; } QPushButton:hover { background: #3B395A; }"
                                      : "QPushButton { background: #ebf8ff; border: 1px solid #bee3f8; padding: 6px 14px; border-radius: 6px; font-size: 12px; color: #2b6cb0; font-weight: bold; } QPushButton:hover { background: #e2e8f0; }");

        clearAllBtn->setStyleSheet(isDark
                                       ? "QPushButton { background: #3B2230; border: 1px solid #6B2A3A; padding: 6px 14px; border-radius: 6px; font-size: 12px; color: #FC8181; font-weight: 500; } QPushButton:hover { background: #4A2935; }"
                                       : "QPushButton { background: #fff5f5; border: 1px solid #feb2b2; padding: 6px 14px; border-radius: 6px; font-size: 12px; color: #c53030; font-weight: 500; } QPushButton:hover { background: #fee2e2; }");

        container->setStyleSheet(isDark
                                     ? QStringLiteral("background:qlineargradient(x1:0,y1:0,x2:1,y2:1,stop:0 #1F1E33,stop:1 #1C1B2E);")
                                     : QStringLiteral("background:qlineargradient(x1:0,y1:0,x2:1,y2:1,stop:0 #f8fafc,stop:1 #eef2ff);"));

        auto* renderer = static_cast<Latex::LatexRenderer*>(this->property("latex_renderer").value<void*>());
        if(renderer) {
            renderer->setTextColor(isDark ? QColor("#E6E7F0") : QColor("#1f2937"));
        }
        reload();
    };

    applyTheme();
    connect(&AlgeMate::ThemeManager::instance(), &AlgeMate::ThemeManager::themeChanged, this, [applyTheme](AlgeMate::ThemeManager::Theme){ applyTheme(); });
    // ==============================================================
}

void WrongBookPage::startShuffleRedoWorkflow()
{
    QFile file("wrong_questions.json");
    if (!file.exists() || !file.open(QIODevice::ReadOnly)) {
        QMessageBox::information(this, QStringLiteral("提示"), QStringLiteral("错题本空空如也，无需重做！"));
        return;
    }
    QJsonArray arr = QJsonDocument::fromJson(file.readAll()).array();
    file.close();

    if (arr.isEmpty()) { QMessageBox::information(this, QStringLiteral("提示"), QStringLiteral("错题本空空如也，无需重做！")); return; }

    std::vector<Question> redoList;
    for (const auto& val : arr) {
        QJsonObject obj = val.toObject(); Question q;
        q.id = obj["id"].toInt(); q.content = obj["content"].toString(); q.userAnswer = obj["userAnswer"].toString(); q.correctAnswer = obj["correctAnswer"].toString();
        QString type = obj["type"].toString(); q.type = (type == "single") ? QuestionType::Single : ((type == "fill") ? QuestionType::Fill : QuestionType::Subjective);
        QJsonArray choicesArray = obj["choices"].toArray(); for (const auto& v : choicesArray) q.choices.append(v.toString());
        redoList.push_back(q);
    }

    std::random_device rd; std::mt19937 g(rd()); std::shuffle(redoList.begin(), redoList.end(), g);
    QMessageBox::information(this, QStringLiteral("开始重做"), QStringLiteral("已为您成功打乱 %1 道错题顺序，下面开始闭卷重做！").arg(redoList.size()));

    auto* renderer = static_cast<Latex::LatexRenderer*>(this->property("latex_renderer").value<void*>());
    int correctCount = 0;
    for (size_t i = 0; i < redoList.size(); ++i) {
        const auto& q = redoList[i];
        WrongDetailDialog dlg(q, "", 0, renderer, true, this);
        connect(&dlg, &WrongDetailDialog::redoRequested, this, [this, &correctCount](int targetId, const QString& newAns) {
            bool ok = executeRedoAnswerCheck(targetId, newAns); if (ok) correctCount++;
        });
        dlg.exec();
    }

    QMessageBox::information(this, QStringLiteral("重做结束"), QStringLiteral("重做大作战完成！本次正确: %1/%2 道。答对的题目已自动从错题本移出。").arg(correctCount).arg(redoList.size()));
    reload();
}

bool WrongBookPage::executeRedoAnswerCheck(int id, const QString& newAns)
{
    QFile file("wrong_questions.json");
    if (!file.open(QIODevice::ReadOnly)) return false;
    QJsonArray arr = QJsonDocument::fromJson(file.readAll()).array(); file.close();
    bool isCorrect = false; QJsonArray newArr;

    for (const auto& val : arr) {
        QJsonObject obj = val.toObject();
        if (obj["id"].toInt() == id) {
            if (obj["correctAnswer"].toString().trimmed() == newAns.trimmed()) { isCorrect = true; }
            else { obj["wrongCount"] = obj["wrongCount"].toInt() + 1; obj["time"] = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss"); obj["userAnswer"] = newAns; newArr.append(obj); }
        } else newArr.append(val);
    }

    if (file.open(QIODevice::WriteOnly)) { file.write(QJsonDocument(newArr).toJson()); file.close(); }
    if (isCorrect) QMessageBox::information(this, QStringLiteral("对啦"), QStringLiteral("✅ 回答正确！此题已踢出本本。"), QMessageBox::Ok);
    else QMessageBox::critical(this, QStringLiteral("错啦"), QStringLiteral("❌ 答案还是不对哦，继续留在本本里积累经验。"), QMessageBox::Ok);
    return isCorrect;
}

void WrongBookPage::loadWrongQuestions()
{
    QFile file("wrong_questions.json");
    auto showEmptyLabel = [this]() {
        m_wrongQuestionCount = 0; emit wrongCountChanged(m_wrongQuestionCount);
        auto* emptyLabel = new QLabel(QStringLiteral("🎉 还没有任何错题记录，继续保持！"), this);
        emptyLabel->setAlignment(Qt::AlignCenter);
        bool isDark = AlgeMate::ThemeManager::instance().currentTheme() == AlgeMate::ThemeManager::Theme::Dark;
        emptyLabel->setStyleSheet(isDark ? "font-size: 18px; color: #7B7B96; padding: 80px;" : "font-size: 18px; color: #6b7280; padding: 80px;");
        contentLayout->addWidget(emptyLabel, 0, 0);
    };

    if (!file.exists() || !file.open(QIODevice::ReadOnly)) { showEmptyLabel(); return; }

    QJsonArray arr = QJsonDocument::fromJson(file.readAll()).array(); file.close();
    m_wrongQuestionCount = arr.size(); emit wrongCountChanged(m_wrongQuestionCount);

    if (arr.isEmpty()) { showEmptyLabel(); return; }

    std::vector<QJsonObject> sortedItems;
    for (const auto& val : arr) sortedItems.push_back(val.toObject());

    QString currentMode = this->property("sort_mode").toString();
    if (currentMode == "count") { std::sort(sortedItems.begin(), sortedItems.end(), [](const QJsonObject& a, const QJsonObject& b) { return a["wrongCount"].toInt() > b["wrongCount"].toInt(); }); }
    else { std::sort(sortedItems.begin(), sortedItems.end(), [](const QJsonObject& a, const QJsonObject& b) { return QDateTime::fromString(a["time"].toString(), "yyyy-MM-dd hh:mm:ss") > QDateTime::fromString(b["time"].toString(), "yyyy-MM-dd hh:mm:ss"); }); }

    int index = 0;
    for (const auto& obj : sortedItems) {
        Question q; q.id = obj["id"].toInt(); q.content = obj["content"].toString(); q.userAnswer = obj["userAnswer"].toString(); q.correctAnswer = obj["correctAnswer"].toString(); q.score = obj["score"].toInt();
        QString type = obj["type"].toString(); q.type = (type == "single") ? QuestionType::Single : ((type == "fill") ? QuestionType::Fill : QuestionType::Subjective);
        QJsonArray choicesArray = obj["choices"].toArray(); for (const auto& v : choicesArray) q.choices.append(v.toString());
        addWrongQuestionCard(q, obj["time"].toString(), obj["wrongCount"].toInt(), index++);
    }
}

void WrongBookPage::addWrongQuestionCard(const Question& q, const QString& time, int wrongCount, int index)
{
    const bool isDark = AlgeMate::ThemeManager::instance().currentTheme() == AlgeMate::ThemeManager::Theme::Dark;

    auto* card = new QFrame(this);
    card->setMinimumHeight(200);
    card->setMaximumWidth(520);
    card->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    card->setStyleSheet(isDark
                            ? "QFrame { background: #28263F; border-radius: 22px; border: 1px solid #3B395A; } QFrame:hover { border: 1px solid #6F77FF; background: #312F4A; }"
                            : "QFrame { background: rgba(255,255,255,0.96); border-radius: 22px; border: 1px solid #e5e7eb; } QFrame:hover { border: 1px solid #c7d2fe; background: white; }");

    auto* shadow = new QGraphicsDropShadowEffect(card);
    shadow->setBlurRadius(30); shadow->setOffset(0, 8); shadow->setColor(QColor(15, 23, 42, 20));
    card->setGraphicsEffect(shadow);

    auto* lay = new QVBoxLayout(card);
    lay->setContentsMargins(16, 12, 16, 12); lay->setSpacing(8);

    auto* topRow = new QHBoxLayout;
    auto* typeLabel = new QLabel(QString("[%1]").arg((q.type == QuestionType::Single) ? "单选题" : ((q.type == QuestionType::Fill) ? "填空题" : "解答题")));
    typeLabel->setStyleSheet(isDark ? "QLabel{ background:#312F4A; color:#8FA1FF; border-radius:10px; padding:4px 10px; font-weight:700; font-size:11px; }"
                                    : "QLabel{ background:#eef2ff; color:#4f46e5; border-radius:10px; padding:4px 10px; font-weight:700; font-size:11px; }");
    auto* countLabel = new QLabel(QStringLiteral("🔥 错误 %1 次").arg(wrongCount));
    countLabel->setStyleSheet(isDark ? "QLabel{ color:#C9C9DC; font-size:12px; font-weight:600; }"
                                     : "QLabel{ color:#64748b; font-size:12px; font-weight:600; }");
    topRow->addWidget(typeLabel); topRow->addStretch(); topRow->addWidget(countLabel);
    lay->addLayout(topRow);

    auto* qBrowser = new Latex::LatexTextBrowser(card);
    qBrowser->setFrameShape(QFrame::NoFrame);
    qBrowser->setStyleSheet(isDark ? "background:#1F1E33; color:#E6E7F0; border-radius:14px; padding:12px;"
                                   : "background:#f8fafc; color:#1f2937; border-radius:14px; padding:12px;");
    qBrowser->setMinimumHeight(95);
    qBrowser->setAttribute(Qt::WA_TransparentForMouseEvents, false);
    qBrowser->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    qBrowser->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    auto* renderer = static_cast<Latex::LatexRenderer*>(this->property("latex_renderer").value<void*>());
    if (renderer) qBrowser->setHtml(renderer->render(q.content, qBrowser->document()));
    lay->addWidget(qBrowser, 1);

    auto* timeLabel = new QLabel(QString("🕒 %1").arg(time), card);
    timeLabel->setStyleSheet(isDark ? "QLabel{ color:#7B7B96; font-size:11px; }" : "QLabel{ color:#94a3b8; font-size:11px; }");
    lay->addWidget(timeLabel);

    auto* detailBtn = new QPushButton(QStringLiteral("查看解析 →"), card);
    detailBtn->setFixedHeight(30);
    detailBtn->setCursor(Qt::PointingHandCursor);
    detailBtn->setStyleSheet(isDark
                                 ? "QPushButton{ background:qlineargradient(x1:0,y1:0,x2:1,y2:0,stop:0 #312F4A,stop:1 #3B395A); color:#B0BBFF; border:1px solid #4B4970; border-radius:14px; font-size:13px; font-weight:700; padding:9px 12px; } QPushButton:hover{ background:#3B395A; border:1px solid #6F77FF; }"
                                 : "QPushButton{ background:qlineargradient(x1:0,y1:0,x2:1,y2:0,stop:0 #f5f3ff,stop:1 #ede9fe); color:#7c3aed; border:1px solid #ddd6fe; border-radius:14px; font-size:13px; font-weight:700; padding:9px 12px; } QPushButton:hover{ background:#ede9fe; border:1px solid #c4b5fd; }");
    lay->addWidget(detailBtn);

    int currentCardId = q.id;
    connect(detailBtn, &QPushButton::clicked, this, [this, currentCardId, time, wrongCount, renderer]() {
        QFile file("wrong_questions.json");
        if (file.open(QIODevice::ReadOnly)) {
            QJsonArray arr = QJsonDocument::fromJson(file.readAll()).array(); file.close();
            for (const auto& val : arr) {
                QJsonObject obj = val.toObject();
                if (obj["id"].toInt() == currentCardId) {
                    Question detailsQ; detailsQ.id = currentCardId; detailsQ.content = obj["content"].toString(); detailsQ.userAnswer = obj["userAnswer"].toString(); detailsQ.correctAnswer = obj["correctAnswer"].toString();
                    QString type = obj["type"].toString(); detailsQ.type = (type == "single") ? QuestionType::Single : ((type == "fill") ? QuestionType::Fill : QuestionType::Subjective);
                    QJsonArray choicesArray = obj["choices"].toArray(); for (const auto& v : choicesArray) detailsQ.choices.append(v.toString());
                    WrongDetailDialog realDlg(detailsQ, time, wrongCount, renderer, false, this);
                    connect(&realDlg, &WrongDetailDialog::deleteRequested, this, [this](int delId){ removeWrongQuestionById(delId); });
                    realDlg.exec();
                    return;
                }
            }
        }
    });

    int row = index / 2; int col = index % 2;
    contentLayout->addWidget(card, row, col);
}

void WrongBookPage::removeWrongQuestionById(int id)
{
    QFile file("wrong_questions.json");
    if (!file.open(QIODevice::ReadOnly)) return;
    QJsonArray arr = QJsonDocument::fromJson(file.readAll()).array(); file.close();

    QJsonArray newArr;
    for (const auto& val : arr) { if (val.toObject()["id"].toInt() != id) newArr.append(val); }

    if (file.open(QIODevice::WriteOnly)) { file.write(QJsonDocument(newArr).toJson()); file.close(); }
    reload();
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

int WrongBookPage::getWrongCount() const
{
    return m_wrongQuestionCount;
}

} // namespace AlgeMate::Learning