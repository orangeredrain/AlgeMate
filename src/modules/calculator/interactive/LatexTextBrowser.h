// LatexTextBrowser.h
// 通用的 LaTeX 感知 QTextBrowser: 选区 Ctrl+C / 右键复制时,
// 会把公式图片 (src=calc-tex://N) 还原为 LaTeX 源码文本.
// 依赖 Value.cpp 中维护的 URL -> LaTeX 映射表 (latexForImageUrl).
// 适用于所有展示 LaTeX 公式的 QTextBrowser: 输出区历史、帮助对话框等.
#ifndef ALGEMATE_CALC_LATEX_TEXT_BROWSER_H
#define ALGEMATE_CALC_LATEX_TEXT_BROWSER_H

#include <QTextBrowser>

class QMimeData;

namespace AlgeMate::Calculator::Interactive {

class LatexTextBrowser : public QTextBrowser {
    Q_OBJECT
public:
    using QTextBrowser::QTextBrowser;

protected:
    // 重写选区 -> MIME 生成: 遇到公式图片时插入其 LaTeX 源码
    QMimeData* createMimeDataFromSelection() const override;
};

} // namespace AlgeMate::Calculator::Interactive

#endif
