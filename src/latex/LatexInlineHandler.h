#ifndef ALGEMATE_LATEX_INLINE_HANDLER_H
#define ALGEMATE_LATEX_INLINE_HANDLER_H

#include <QObject>
#include <QTextFormat>
#include <QTextObjectInterface>

namespace AlgeMate::Latex {

// 自定义 inline object type：避开 Qt 内置 ImageObject(=1) 等。
constexpr int kLatexObjectType   = QTextFormat::UserObject + 41;

// 公式参数挂载到 QTextCharFormat 的 property（drawObject 时读取）
constexpr int kLatexSourceProp   = QTextFormat::UserProperty + 41; // QString 原始 LaTeX
constexpr int kLatexDisplayProp  = QTextFormat::UserProperty + 42; // bool 块公式
constexpr int kLatexFontSizeProp = QTextFormat::UserProperty + 43; // int  字号(pt)
constexpr int kLatexColorProp    = QTextFormat::UserProperty + 44; // QColor 文本色
// kLatexSourceProp 存 normalize 后的 LaTeX（用于渲染）；kLatexOrigSourceProp 存
// 用户原始输入（用于 LatexTextBrowser 复制还原为用户原文）。
constexpr int kLatexOrigSourceProp = QTextFormat::UserProperty + 45; // QString 原始 LaTeX（未 normalize）

/// LaTeX 矢量化 inline object 渲染器。
/// 由 QTextDocument 的 layout 在每次重绘时调用 drawObject() 直接走 QPainter
/// 矢量绘制（非位图缓存），实现窗口缩放 / HiDPI 切屏始终锐利的效果。
///
/// 内部维护一个 (latex, displayStyle, fontSize, color) → JKQTMathText* 实例池：
/// 第一次访问时 parse 一次，后续重绘直接 draw。clearPool() 在 LatexRenderer
/// clearCache() 时调用。
class LatexInlineHandler : public QObject, public QTextObjectInterface {
    Q_OBJECT
    Q_INTERFACES(QTextObjectInterface)
public:
    static LatexInlineHandler* instance();

    QSizeF intrinsicSize(QTextDocument* doc, int posInDocument,
                         const QTextFormat& format) override;

    void drawObject(QPainter* painter, const QRectF& rect,
                    QTextDocument* doc, int posInDocument,
                    const QTextFormat& format) override;

    /// 释放所有缓存的 JKQTMathText 实例
    static void clearPool();

private:
    LatexInlineHandler() = default;
};

} // namespace AlgeMate::Latex

#endif // ALGEMATE_LATEX_INLINE_HANDLER_H
