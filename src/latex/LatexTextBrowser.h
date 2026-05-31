#ifndef ALGEMATE_LATEX_TEXT_BROWSER_H
#define ALGEMATE_LATEX_TEXT_BROWSER_H

#include <QString>
#include <QTextBrowser>

namespace AlgeMate::Latex {

/// QTextBrowser 子类: 复制选区时将 LaTeX 图片自动还原为 $...$ / $$...$$ 源码.
/// 利用 LatexRenderer 的全局 URL→LaTeX 映射, 无需手动绑定.
///
/// 进阶用法: 通过 setSourceMarkdown() 注入"原始 markdown 源",
/// 复制全选时直接返回该 markdown, 实现"所见即所复制为源码".
class LatexTextBrowser : public QTextBrowser {
    Q_OBJECT
public:
    explicit LatexTextBrowser(QWidget* parent = nullptr);

    /// 注入当前显示的原始 markdown 源（包含 $..$, $$..$$, **bold** 等标记）。
    /// 复制全选/空选时直接返回此源；部分选区仍走反向拼接。
    void setSourceMarkdown(const QString& markdown);

protected:
    QMimeData* createMimeDataFromSelection() const override;

private:
    QString m_sourceMarkdown;
};

} // namespace AlgeMate::Latex

#endif // ALGEMATE_LATEX_TEXT_BROWSER_H
