#ifndef ALGEMATE_LATEX_RENDERER_H
#define ALGEMATE_LATEX_RENDERER_H

#include <QColor>
#include <QHash>
#include <QString>

#include <functional>

class QTextDocument;

namespace AlgeMate::Latex {

/// 通用 LaTeX 渲染器.
/// 输入含中文 + $...$ / $$...$$ / \[...\] 的 LaTeX 源码,
/// 输出可直接 setHtml() 到 QTextBrowser 的 HTML.
///
/// 渲染的公式图片自动注册到全局 URL→LaTeX 映射表,
/// LatexTextBrowser 复制时可还原为 LaTeX 源码, 无需手动绑定.
class LatexRenderer {
public:
    LatexRenderer();

    // ---- 可选配置 ----

    void setFontSize(int pt);
    void setTextColor(QColor c);

    void addMathMacro(const QString& cmd, const QString& expansion);

    using CmdHandler = std::function<QString(const QString& opt,
                                             const QString& arg)>;
    void addCommand(const QString& name, CmdHandler handler);

    // ---- 核心 ----

    QString render(const QString& source, QTextDocument* doc);

    /// 全局查询: 根据图片 URL 反查 LaTeX 源码.
    static QString latexForUrl(const QString& url);

    void clearCache();

private:
    int m_fontSize = 14;
    QColor m_textColor = Qt::black;
    QHash<QString, QString> m_mathMacros;
    QHash<QString, CmdHandler> m_commands;
};

} // namespace AlgeMate::Latex

#endif // ALGEMATE_LATEX_RENDERER_H
