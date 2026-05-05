#include "KnowledgePage.h"

#include "latex/LatexRenderer.h"
#include "latex/LatexTextBrowser.h"

#include <QFile>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QSplitter>
#include <QTreeWidget>
#include <QTreeWidgetItem>
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
    // 不设 textColor, 数学颜色走 QSS, 文本颜色由 QSS #KnowledgeContent 控制
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
    m_contentView->setOpenExternalLinks(true);
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
                    m_renderer->clearCache();
                    m_contentView->setHtml(QStringLiteral(
                        "<div style='font-family:Microsoft YaHei,sans-serif; "
                        "font-size:14pt; color:#8A8FA3;'>"
                        "请从左侧展开并选择具体小节查看讲解。</div>"));
                    return;
                }
                loadContentFromResource(path);
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

    QTreeWidgetItem* ch1 = addChapter(QStringLiteral("第 1 章 向量与线性方程组"));
    addSection(ch1, QStringLiteral("1.1 向量与线性组合"),
               QStringLiteral(":/knowledge/ch01_vectors.md"));
    addSection(ch1, QStringLiteral("1.2 矩阵与方程组简介"),
               QStringLiteral(":/knowledge/ch01_matrices_intro.md"));

    QTreeWidgetItem* ch2 = addChapter(QStringLiteral("第 2 章 矩阵运算"));
    addSection(ch2, QStringLiteral("2.1 矩阵乘法"),
               QStringLiteral(":/knowledge/ch02_matrix_multiply.md"));

    m_chapterTree->expandToDepth(1);
}

void KnowledgePage::loadContentFromResource(const QString& resourcePath)
{
    QFile file(resourcePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        m_contentView->setHtml(
            QStringLiteral("<p style='color:red;'>无法打开资源文件：%1</p>")
            .arg(resourcePath));
        return;
    }
    m_renderer->clearCache();
    QString source = QString::fromUtf8(file.readAll());
    QString html = m_renderer->render(source, m_contentView->document());
    m_contentView->setHtml(html);
}

} // namespace AlgeMate::Learning
