#include "KnowledgePage.h"

#include "latex/LatexRenderer.h"
#include "latex/LatexTextBrowser.h"
#include "core/ThemeManager.h"

#include <QFile>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QRegularExpression>
#include <QSplitter>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QUrl>
#include <QVBoxLayout>

namespace AlgeMate::Learning {

namespace {

constexpr int kRoleMarkdownResource = Qt::UserRole + 1;

QPushButton* makeBackBtn(QWidget* parent = nullptr) {
    auto* btn = new QPushButton(QStringLiteral("← 返回"), parent);
    btn->setObjectName(QStringLiteral("LearnBackBtn"));
    return btn;
}

QPushButton* makePrimaryBtn(const QString& text, QWidget* parent = nullptr) {
    auto* btn = new QPushButton(text, parent);
    btn->setProperty("primary", true);
    btn->setCursor(Qt::PointingHandCursor);
    btn->setMinimumHeight(36);
    btn->setStyleSheet(btn->styleSheet());
    return btn;
}

} // namespace

KnowledgePage::KnowledgePage(QWidget* parent) : QWidget(parent)
{
    m_renderer = new Latex::LatexRenderer;
    // LaTeX 公式图片的颜色需要随主题切换 (默认黑色, 暗色背景下不可读).
    // 参考计算交互/演示模块 RenderTheme::forCurrent + setTextColor 的做法.
    auto themeTextColor = []() {
        const bool dark = AlgeMate::ThemeManager::instance().currentTheme()
                          == AlgeMate::ThemeManager::Theme::Dark;
        return dark ? QColor("#F3F3FA") : QColor("#1F2033");
    };
    m_renderer->setTextColor(themeTextColor());
    m_renderer->addMathMacro(QStringLiteral("F"),  QStringLiteral("\\mathbb{F}"));
    m_renderer->addMathMacro(QStringLiteral("R"),  QStringLiteral("\\mathbb{R}"));
    m_renderer->addMathMacro(QStringLiteral("C"),  QStringLiteral("\\mathbb{C}"));
    m_renderer->addMathMacro(QStringLiteral("Q"),  QStringLiteral("\\mathbb{Q}"));
    m_renderer->addMathMacro(QStringLiteral("Z"),  QStringLiteral("\\mathbb{Z}"));
    m_renderer->addMathMacro(QStringLiteral("N"),  QStringLiteral("\\mathbb{N}"));

    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(24, 16, 24, 24);
    root->setSpacing(14);

    // 顶栏
    auto* top = new QHBoxLayout;
    auto* back = makeBackBtn(this);
    connect(back, &QPushButton::clicked, this, &KnowledgePage::backRequested);
    auto* title = new QLabel(QStringLiteral("知识点学习"));
    title->setObjectName(QStringLiteral("PageTitle"));
    auto* toPrac = makePrimaryBtn(QStringLiteral("进入章节练习 →"), this);
    connect(toPrac, &QPushButton::clicked, this, &KnowledgePage::enterChapterPractice);
    top->addWidget(back);
    top->addWidget(title);
    top->addStretch();
    top->addWidget(toPrac);

    // 中间主体: 可拖拽左右分栏
    m_splitter = new QSplitter(Qt::Horizontal);
    m_splitter->setChildrenCollapsible(false);

    m_chapterTree = new QTreeWidget;
    m_chapterTree->setObjectName(QStringLiteral("KnowledgeTree"));
    m_chapterTree->setHeaderHidden(true);
    m_chapterTree->setMinimumWidth(240);

    m_contentView = new Latex::LatexTextBrowser;
    m_contentView->setObjectName(QStringLiteral("KnowledgeContent"));
    // 关闭 QTextBrowser 默认的链接导航, 错开我们在 anchorClicked 里自己处理 "切换答案" 伪链接
    m_contentView->setOpenLinks(false);
    m_contentView->setOpenExternalLinks(true);
    connect(m_contentView, &QTextBrowser::anchorClicked,
            this, &KnowledgePage::onAnchorClicked);

    m_splitter->addWidget(m_chapterTree);
    m_splitter->addWidget(m_contentView);
    m_splitter->setStretchFactor(0, 0);
    m_splitter->setStretchFactor(1, 1);

    buildChapterTree();

    connect(m_chapterTree, &QTreeWidget::currentItemChanged,
            this, [this](QTreeWidgetItem* current, QTreeWidgetItem*) {
                if (!current) return;
                const QString path =
                    current->data(0, kRoleMarkdownResource).toString();
                if (path.isEmpty()) {
                    // 不 clearCache: 提示文本不含公式, 且清池会影响其他页面已注册的 latex-vec:// 映射。
                    m_currentResource.clear();
                    m_contentView->setHtml(QStringLiteral(
                        "<div style='font-family:Microsoft YaHei,sans-serif; "
                        "font-size:14pt; color:#8A8FA3;'>"
                        "请从左侧展开并选择具体小节查看讲解。</div>"));
                    return;
                }
                loadContentFromResource(path);
            });

    // 主题切换: 更新 LaTeX 渲染色, 并清缓存重新渲染当前小节内容.
    connect(&AlgeMate::ThemeManager::instance(),
            &AlgeMate::ThemeManager::themeChanged,
            this, [this, themeTextColor](AlgeMate::ThemeManager::Theme){
                if (!m_renderer) return;
                m_renderer->setTextColor(themeTextColor());
                m_renderer->clearCache();
                if (!m_currentResource.isEmpty()) {
                    loadContentFromResource(m_currentResource);
                }
            });

    // 默认选中第一个叶子
    const auto items = m_chapterTree->findItems(
        QStringLiteral("*"), Qt::MatchWildcard | Qt::MatchRecursive);
    for (auto* it : items) {
        if (!it->data(0, kRoleMarkdownResource).toString().isEmpty()) {
            m_chapterTree->setCurrentItem(it);
            break;
        }
    }

    root->addLayout(top);
    root->addWidget(m_splitter, 1);
}

KnowledgePage::~KnowledgePage()
{
    delete m_renderer;
}

void KnowledgePage::buildChapterTree()
{
    m_chapterTree->clear();

    auto addChapter = [this](const QString& title) {
        auto* node = new QTreeWidgetItem(m_chapterTree);
        node->setText(0, title);
        node->setExpanded(true);
        return node;
    };

    auto addSection = [](QTreeWidgetItem* chapter, const QString& title,
                         const QString& resourcePath) {
        auto* leaf = new QTreeWidgetItem(chapter);
        leaf->setText(0, title);
        leaf->setData(0, kRoleMarkdownResource, resourcePath);
        return leaf;
    };

    QTreeWidgetItem* ch1 = addChapter(QStringLiteral("第 1 章 多项式"));
    // addSection(ch1, QStringLiteral("1.1 整除与带余除法"),
    //            QStringLiteral(":/knowledge/ch01_vectors.md"));
    addSection(ch1, QStringLiteral("1.1 整除与带余除法"),
               QStringLiteral(":/knowledge/ch01_1.md"));
    addSection(ch1, QStringLiteral("1.2 最大公因式"),
               QStringLiteral(":/knowledge/ch01_2.md"));
    addSection(ch1, QStringLiteral("1.3 不可约多项式与唯一分解定理"),
               QStringLiteral(":/knowledge/ch01_3.md"));
    addSection(ch1, QStringLiteral("1.4 重因式"),
               QStringLiteral(":/knowledge/ch01_4.md"));
    addSection(ch1, QStringLiteral("1.5 n元多项式环与对称多项式"),
               QStringLiteral(":/knowledge/ch01_5.md"));

    QTreeWidgetItem* ch2 = addChapter(QStringLiteral("第 2 章 行列式"));
    addSection(ch2, QStringLiteral("2.1 行列式的定义"),
               QStringLiteral(":/knowledge/ch02_1.md"));
    addSection(ch2, QStringLiteral("2.2 克拉默法则与拉普拉斯定理"),
               QStringLiteral(":/knowledge/ch02_2.md"));

    QTreeWidgetItem* ch3 = addChapter(QStringLiteral("第 3 章 n维向量与向量空间"));
    addSection(ch3, QStringLiteral("3.1 n维向量与向量空间"),
               QStringLiteral(":/knowledge/ch03_1.md"));
    addSection(ch3, QStringLiteral("3.2 极大线性无关组"),
               QStringLiteral(":/knowledge/ch03_2.md"));
    addSection(ch3, QStringLiteral("3.3 向量组的秩"),
               QStringLiteral(":/knowledge/ch03_3.md"));
    addSection(ch3, QStringLiteral("3.4 矩阵的秩"),
               QStringLiteral(":/knowledge/ch03_4.md"));
    addSection(ch3, QStringLiteral("3.5 线性方程组的解"),
               QStringLiteral(":/knowledge/ch03_5.md"));

    QTreeWidgetItem* ch4 = addChapter(QStringLiteral("第 4 章 矩阵的运算"));
    addSection(ch4, QStringLiteral("4.1 矩阵的加法、数乘与乘法"),
               QStringLiteral(":/knowledge/ch04_1.md"));
    addSection(ch4, QStringLiteral("4.2 可逆矩阵"),
               QStringLiteral(":/knowledge/ch04_2.md"));
    addSection(ch4, QStringLiteral("4.3 分块矩阵"),
               QStringLiteral(":/knowledge/ch04_3.md"));

    QTreeWidgetItem* ch5 = addChapter(QStringLiteral("第 5 章 矩阵的相抵与相似"));
    addSection(ch5, QStringLiteral("5.1 矩阵的相抵"),
               QStringLiteral(":/knowledge/ch05_1.md"));
    addSection(ch5, QStringLiteral("5.2 矩阵的相似"),
               QStringLiteral(":/knowledge/ch05_2.md"));
    addSection(ch5, QStringLiteral("5.3 特征向量与矩阵可对角化"),
               QStringLiteral(":/knowledge/ch05_3.md"));
    addSection(ch5, QStringLiteral("5.4 实对称矩阵的正交对角化"),
               QStringLiteral(":/knowledge/ch05_4.md"));

    QTreeWidgetItem* ch6 = addChapter(QStringLiteral("第 6 章 二次型"));
    addSection(ch6, QStringLiteral("6.1 二次型的定义、规范形"),
               QStringLiteral(":/knowledge/ch06_1.md"));
    addSection(ch6, QStringLiteral("6.2 正定二次型与正定矩阵"),
               QStringLiteral(":/knowledge/ch06_2.md"));

    QTreeWidgetItem* ch7 = addChapter(QStringLiteral("第 7 章 线性空间"));
    addSection(ch7, QStringLiteral("7.1 基与维数"),
               QStringLiteral(":/knowledge/ch07_1.md"));
    addSection(ch7, QStringLiteral("7.2 子空间的交、和与直和"),
               QStringLiteral(":/knowledge/ch07_2.md"));
    addSection(ch7, QStringLiteral("7.3 线性空间的同构"),
               QStringLiteral(":/knowledge/ch07_3.md"));
    addSection(ch7, QStringLiteral("7.4 商空间"),
               QStringLiteral(":/knowledge/ch07_4.md"));

    QTreeWidgetItem* ch8 = addChapter(QStringLiteral("第 8 章 线性映射"));
    addSection(ch8, QStringLiteral("8.1 线性映射的定义"),
               QStringLiteral(":/knowledge/ch08_1.md"));
    addSection(ch8, QStringLiteral("8.2 核与像"),
               QStringLiteral(":/knowledge/ch08_2.md"));
    addSection(ch8, QStringLiteral("8.3 线性映射的矩阵表示"),
               QStringLiteral(":/knowledge/ch08_3.md"));
    addSection(ch8, QStringLiteral("8.4 不变子空间与 Cayley–Hamilton定理"),
               QStringLiteral(":/knowledge/ch08_4.md"));
    addSection(ch8, QStringLiteral("8.5 Jordan标准形"),
               QStringLiteral(":/knowledge/ch08_5.md"));

    QTreeWidgetItem* ch9 = addChapter(QStringLiteral("第 9 章 lambda-矩阵"));
    addSection(ch9, QStringLiteral("9.1 lambda-矩阵的定义"),
               QStringLiteral(":/knowledge/ch09_1.md"));
    addSection(ch9, QStringLiteral("9.2 Smith 标准形"),
               QStringLiteral(":/knowledge/ch09_2.md"));
    addSection(ch9, QStringLiteral("9.3 不变因子与 Jordan 标准形"),
               QStringLiteral(":/knowledge/ch09_3.md"));

    QTreeWidgetItem* ch10 = addChapter(QStringLiteral("第 10 章 具有度量的线性空间"));
    addSection(ch10, QStringLiteral("10.1 内积与欧几里得空间"),
               QStringLiteral(":/knowledge/ch10_1.md"));
    addSection(ch10, QStringLiteral("10.2 正交变换与对称变换"),
               QStringLiteral(":/knowledge/ch10_2.md"));
    addSection(ch10, QStringLiteral("10.3 酉空间与 Hermite 矩阵"),
               QStringLiteral(":/knowledge/ch10_3.md"));
    addSection(ch10, QStringLiteral("10.4 最小二乘法"),
               QStringLiteral(":/knowledge/ch10_4.md"));

    m_chapterTree->expandToDepth(1);
}

void KnowledgePage::loadContentFromResource(const QString& resourcePath)
{
    m_currentResource = resourcePath;
    QFile file(resourcePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        m_mainHtml = QStringLiteral("<p style='color:red;'>无法打开资源文件：%1</p>")
                         .arg(resourcePath);
        m_answerHtml.clear();
        m_answerVisible = false;
        m_contentView->setHtml(m_mainHtml);
        return;
    }
    // 不能 clearCache: 会清空全局 latex-vec:// 映射, 影响其他页面
    // 未异步替换的公式; 同公式重复渲染会命中实例池。
    QString source = QString::fromUtf8(file.readAll());

    // 拆分 ### 思考题答案 标记: 主体与答案分别预渲染为 HTML, 答案默认隐藏
    static const QRegularExpression kAnswerMarker(
        QStringLiteral("^\\s*###\\s*思考题答案\\s*$"),
        QRegularExpression::MultilineOption);
    QString mainPart = source;
    QString answerPart;
    auto mm = kAnswerMarker.match(source);
    if (mm.hasMatch()) {
        const int markerStart = mm.capturedStart();
        mainPart = source.left(markerStart);
        answerPart = source.mid(markerStart); // 保留 ### 思考题答案 标题
    }

    m_mainHtml = m_renderer->render(mainPart, m_contentView->document());
    m_answerHtml = answerPart.isEmpty()
                       ? QString()
                       : m_renderer->render(answerPart, m_contentView->document());
    m_answerVisible = false;
    refreshContentHtml();
}

void KnowledgePage::refreshContentHtml()
{
    QString html = m_mainHtml;
    if (!m_answerHtml.isEmpty()) {
        const QString linkText = m_answerVisible
            ? QStringLiteral("▲ 收起思考题答案")
            : QStringLiteral("▼ 查看思考题答案");
        // 小号灰色文字链接: 紧跟在 思考题 后面, 嵌入主面板同一个框内
        html += QStringLiteral(
                    "<p style=\"margin:6px 0 0; "
                    "font-size:12pt; "
                    "color:#6b7280;\">"
                    "<a href=\"algemate://toggle-answer\" "
                    "style=\"color:#6b7280; text-decoration:none;\">%1</a></p>")
                    .arg(linkText);
        if (m_answerVisible) {
            html += m_answerHtml;
        }
    }
    m_contentView->setHtml(html);
    Latex::LatexRenderer::postProcessDocument(m_contentView->document());
}

void KnowledgePage::onAnchorClicked(const QUrl& url)
{
    if (url.toString() == QLatin1String("algemate://toggle-answer")) {
        if (m_answerHtml.isEmpty()) return;
        m_answerVisible = !m_answerVisible;
        refreshContentHtml();
    }
}

} // namespace AlgeMate::Learning
