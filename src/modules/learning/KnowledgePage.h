#ifndef ALGEMATE_KNOWLEDGE_PAGE_H
#define ALGEMATE_KNOWLEDGE_PAGE_H

#include <QWidget>

class QSplitter;
class QTreeWidget;
class QTreeWidgetItem;

namespace AlgeMate::Latex {
class LatexRenderer;
class LatexTextBrowser;
}

namespace AlgeMate::Learning {

/// 知识点学习页：左侧章节目录树，右侧 LaTeX 渲染讲解.
class KnowledgePage : public QWidget {
    Q_OBJECT
public:
    explicit KnowledgePage(QWidget* parent = nullptr);
    ~KnowledgePage() override;

signals:
    void backRequested();
    void enterChapterPractice();

private:
    void buildChapterTree();
    void loadContentFromResource(const QString& resourcePath);

    QSplitter*              m_splitter     = nullptr;
    QTreeWidget*            m_chapterTree  = nullptr;
    Latex::LatexTextBrowser* m_contentView  = nullptr;
    Latex::LatexRenderer*   m_renderer     = nullptr;
};

} // namespace AlgeMate::Learning

#endif // ALGEMATE_KNOWLEDGE_PAGE_H
