#ifndef ALGEMATE_KNOWLEDGE_PAGE_H
#define ALGEMATE_KNOWLEDGE_PAGE_H

#include <QWidget>
#include <QString>

class QSplitter;
class QTreeWidget;
class QTreeWidgetItem;
class QUrl;

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
    // void enterChapterPractice(const QString& chapterPath);// 直接把章节的路径发出去
    void enterChapterPractice();

private slots:
    void onAnchorClicked(const QUrl& url);

private:
    void buildChapterTree();
    void loadContentFromResource(const QString& resourcePath);
    void refreshContentHtml();

    QSplitter*               m_splitter        = nullptr;
    QTreeWidget*             m_chapterTree     = nullptr;
    Latex::LatexTextBrowser* m_contentView     = nullptr;
    Latex::LatexRenderer*    m_renderer        = nullptr;

    // 思考题答案: 作为内嵌 HTML 链接嵌入到主内容面板, 点击切换可见
    QString                  m_mainHtml;            // 不含答案的主体 HTML
    QString                  m_answerHtml;          // 答案 HTML（为空表示本小节无答案）
    bool                     m_answerVisible   = false;

    // 当前正在显示的小节资源路径, 主题切换时重新渲染。
    QString                  m_currentResource;
};

} // namespace AlgeMate::Learning

#endif // ALGEMATE_KNOWLEDGE_PAGE_H
