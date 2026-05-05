#include "KnowledgePage.h"

#include <QFile>
#include <QLabel>
#include <QSplitter>
#include <QTextBrowser>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QVBoxLayout>
#include <QFont>

namespace AlgeMate::Knowledge {

namespace {

/** 树节点上存储「对应 Markdown 的 qrc 路径」；无路径表示仅分组，不加载文档 */
constexpr int kRoleMarkdownResource = Qt::UserRole + 1;

} // namespace

KnowledgePage::KnowledgePage(QWidget* parent) : QWidget(parent) {
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(24, 24, 24, 24);
    root->setSpacing(12);

    auto* title = new QLabel(QStringLiteral("知识点学习"));
    title->setObjectName(QStringLiteral("PageTitle"));
    title->setStyleSheet(
        "font-size:28px;"
        "font-weight:700;"
        "color:#1F2430;"
        );
    auto* subtitle = new QLabel(QStringLiteral("章节目录 · 图文讲解 · 经典例题"));
    subtitle->setObjectName(QStringLiteral("PageSubtitle"));
    subtitle->setStyleSheet(
        "font-size:14px;"
        "color:#8A8FA3;"
        );

    root->addWidget(title);
    root->addWidget(subtitle);

    // 中间主体：可拖拽调整左右宽度，左侧目录、右侧正文
    splitter_ = new QSplitter(Qt::Horizontal, this);
    splitter_->setChildrenCollapsible(false);
    composeContentArea(splitter_);
    root->addWidget(splitter_, 1);

    // 选中目录项时，若有绑定的资源路径则加载 Markdown
    connect(chapterTree_, &QTreeWidget::currentItemChanged,
            this, [this](QTreeWidgetItem* current, QTreeWidgetItem* /*previous*/) {
                if (!current)
                    return;
                const QString path =
                    current->data(0, kRoleMarkdownResource).toString();
                if (path.isEmpty()) {
                    contentView_->setPlainText(
                        QStringLiteral("请从左侧展开并选择具体小节查看讲解。"));
                    return;
                }
                loadMarkdownFromResource(path);

            });

    // 默认选中第一个「带文档」的叶子，避免右侧空白
    const auto items = chapterTree_->findItems(
        QStringLiteral("*"), Qt::MatchWildcard | Qt::MatchRecursive);
    for (auto* it : items) {
        if (!it->data(0, kRoleMarkdownResource).toString().isEmpty()) {
            chapterTree_->setCurrentItem(it);
            break;
        }
    }
}

void KnowledgePage::composeContentArea(QSplitter* splitter) {
    // 左侧：章节树（README：QTreeView / QListWidget；此处用 QTreeWidget 便于静态数据）
    chapterTree_ = new QTreeWidget(splitter);
    chapterTree_->setHeaderHidden(true);
    chapterTree_->setMinimumWidth(240);
    chapterTree_->setAlternatingRowColors(true);
    chapterTree_->setStyleSheet(R"(
        QTreeWidget {
            background: white;
            border: none;
            border-radius: 18px;
            padding: 12px;
            font-size: 14px;
        }

        QTreeWidget::item {
            height: 36px;
            padding-left: 8px;
            border-radius: 8px;
        }

        QTreeWidget::item:selected {
            background: #E9EEFF;
            color: #3D5AFE;
            font-weight: 600;
        }

        QTreeWidget::item:hover {
            background: #F5F7FB;
        }
        )");

    // 右侧：图文区（后续公式可走 LaTeX 图或 WebEngine；当前用 setMarkdown MVP）
    contentView_ = new QTextBrowser(splitter);
    contentView_->setOpenExternalLinks(true);
    contentView_->setStyleSheet(R"(
        QTextBrowser {
            background: white;
            border: none;
            border-radius: 18px;
            padding: 24px;
            font-size: 15px;
            line-height: 1.7;
        }
        )");

    splitter->addWidget(chapterTree_);
    splitter->addWidget(contentView_);
    splitter->setStretchFactor(0, 0);
    splitter->setStretchFactor(1, 1);

    buildChapterTree();
}

void KnowledgePage::buildChapterTree() {
    chapterTree_->clear();

    // 辅助：添加仅有分组作用的章节节点（不绑定 md）
    auto addChapter = [this](const QString& title) {
        auto* node = new QTreeWidgetItem(chapterTree_);
        node->setText(0, title);
        node->setExpanded(true);
        return node;
    };

    // 辅助：在某章下添加小节，resourcePath 形如 ":/knowledge/xxx.md"
    auto addSection = [](QTreeWidgetItem* chapter, const QString& title,
                         const QString& resourcePath) {
        auto* leaf = new QTreeWidgetItem(chapter);
        leaf->setText(0, title);
        leaf->setData(0, kRoleMarkdownResource, resourcePath);
        return leaf;
    };

    // ---------- 静态目录（按课程大纲继续扩充）----------
    QTreeWidgetItem* ch1 = addChapter(QStringLiteral("第 1 章 向量与线性方程组"));
    addSection(ch1, QStringLiteral("1.1 向量与线性组合"),
               QStringLiteral(":/knowledge/ch01_vectors.md"));
    addSection(ch1, QStringLiteral("1.2 矩阵与方程组简介"),
               QStringLiteral(":/knowledge/ch01_matrices_intro.md"));

    QTreeWidgetItem* ch2 = addChapter(QStringLiteral("第 2 章 矩阵运算（占位）"));
    addSection(ch2, QStringLiteral("2.1 矩阵乘法"),
               QStringLiteral(":/knowledge/ch02_matrix_multiply.md"));

    chapterTree_->expandToDepth(1);
}

void KnowledgePage::loadMarkdownFromResource(const QString& resourcePath) {
    QFile file(resourcePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        contentView_->setPlainText(
            QStringLiteral("无法打开资源文件：\n%1").arg(resourcePath));
        return;
    }
    const QByteArray raw = file.readAll();
    contentView_->setMarkdown(QString::fromUtf8(raw));
}

} // namespace AlgeMate::Knowledge
