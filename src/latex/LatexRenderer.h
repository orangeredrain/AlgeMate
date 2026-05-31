#ifndef ALGEMATE_LATEX_RENDERER_H
#define ALGEMATE_LATEX_RENDERER_H

#include <QColor>
#include <QHash>
#include <QObject>
#include <QString>
#include <QTextBrowser>
#include <QTextDocument>
#include <QTimer>

#include <functional>
#include <memory>

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

    /// 全局查询: 该 URL 对应的公式是块公式（displayStyle 为 true）。
    /// LatexTextBrowser 复制时使用，决定还原为 $...$ 还是 $$...$$。
    static bool isUrlDisplay(const QString& url);

    void clearCache();

    /// 在调用方 setHtml() 之后调用：将 doc 中的“伪公式图片”
    /// （src=latex-vec://N）替换为矢量 inline object。调用后公式在
    /// 文档重绘时实时走 LatexInlineHandler，获得无损缩放。幂等。
    static void postProcessDocument(QTextDocument* doc);

    /// 通用矢量化接入接口：注册一个 LaTeX 公式占位图到 doc，返回可直接拼到
    /// HTML 字符串里的 <img src="latex-vec://N" .../> 片段。调用方在 setHtml
    /// 之后必须调用 postProcessDocument(doc) 把所有占位替换为矢量 inline object。
    /// 用于把计算助手 / 算法演示 / AI 解题等所有 LaTeX 渲染统一到矢量化路径。
    static QString embedAsImg(QTextDocument* doc, const QString& latex,
                              bool displayStyle = false, int fontSize = 14,
                              QColor color = QColor(0, 0, 0));

private:
    int m_fontSize = 14;
    QColor m_textColor = Qt::black;
    QHash<QString, QString> m_mathMacros;
    QHash<QString, CmdHandler> m_commands;
};

// =========================================================================
//  为一个 QTextBrowser 挂上“内容变化 → 自动 postProcessDocument”的 hook，一次性
//  让该 browser 所有 setHtml / append 之后都能把 latex-vec:// 占位替换为矢量
//  inline object。ctor 中调用一次即可，同一 doc 多次调用也不会出问题。
// =========================================================================
inline void attachLatexAutoPostProcess(QTextBrowser* browser)
{
    if (!browser) return;
    QTextDocument* doc = browser->document();
    if (!doc) return;

    // 幂等保护: 同一个 doc 重复 attach 会造成多个 lambda 同时调度 postProcess。
    // 在 doc 上设 dynamic property, 已绑定过则跳过。
    static const char kAttachedFlag[] = "_algemate_latex_auto_attached";
    if (doc->property(kAttachedFlag).toBool()) return;
    doc->setProperty(kAttachedFlag, true);

    // 同一事件循环连续多次 contentsChanged 合并为一次 postProcess；
    // postProcess 内部 cursor 修改 doc 又会发 contentsChanged，重入要拦住。
    auto pending = std::make_shared<bool>(false);
    auto inProc  = std::make_shared<bool>(false);
    QObject::connect(doc, &QTextDocument::contentsChanged, doc,
                     [doc, pending, inProc]() {
        if (*inProc || *pending) return;
        *pending = true;
        QTimer::singleShot(0, doc, [doc, pending, inProc]() {
            *pending = false;
            *inProc  = true;
            LatexRenderer::postProcessDocument(doc);
            *inProc  = false;
        });
    });
}

} // namespace AlgeMate::Latex

#endif // ALGEMATE_LATEX_RENDERER_H
