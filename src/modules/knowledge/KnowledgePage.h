#ifndef ALGEMATE_KNOWLEDGEPAGE_H
#define ALGEMATE_KNOWLEDGEPAGE_H

#include <QWidget>

class QSplitter;
class QTreeWidget;
class QTextBrowser;

namespace AlgeMate::Knowledge {

/**
 * 知识点学习页：左侧章节目录（树），右侧 Markdown 讲解（QTextBrowser）。
 * 模块间不引用其他 modules；内容由 resources/knowledge/*.md 经 qrc 打包加载。
 */
class KnowledgePage : public QWidget {
    Q_OBJECT
public:
    explicit KnowledgePage(QWidget* parent = nullptr);

private:
    /** 搭建 README 约定的左右分栏：QSplitter + 树 + 浏览器 */
    void composeContentArea(QSplitter* splitter);

    /**
     * 填充静态章节目录（第一步：写死在代码里；后续可改为读 JSON/数据库）。
     * 叶子节点在 Qt::UserRole 上绑定 qrc 路径，如 ":/knowledge/ch01_vectors.md"。
     */
    void buildChapterTree();

    /** 从 Qt 资源路径读取 UTF-8 Markdown 并交给 QTextBrowser 渲染 */
    void loadMarkdownFromResource(const QString& resourcePath);

    QSplitter*     splitter_ = nullptr;
    QTreeWidget*   chapterTree_ = nullptr;
    QTextBrowser*  contentView_ = nullptr;
};

}

#endif
