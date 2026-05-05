#ifndef ALGEMATE_LATEX_TEXT_BROWSER_H
#define ALGEMATE_LATEX_TEXT_BROWSER_H

#include <QTextBrowser>

namespace AlgeMate::Latex {

/// QTextBrowser 子类: 复制选区时将 LaTeX 图片自动还原为 $...$ 源码.
/// 利用 LatexRenderer 的全局 URL→LaTeX 映射, 无需手动绑定.
class LatexTextBrowser : public QTextBrowser {
    Q_OBJECT
public:
    explicit LatexTextBrowser(QWidget* parent = nullptr);

protected:
    QMimeData* createMimeDataFromSelection() const override;
};

} // namespace AlgeMate::Latex

#endif // ALGEMATE_LATEX_TEXT_BROWSER_H
