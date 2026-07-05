#ifndef ALGEMATE_LATEX_INLINE_HANDLER_H
#define ALGEMATE_LATEX_INLINE_HANDLER_H

#include <QObject>
#include <QTextFormat>
#include <QTextObjectInterface>

namespace AlgeMate::Latex {

constexpr int kLatexObjectType   = QTextFormat::UserObject + 41;

constexpr int kLatexSourceProp   = QTextFormat::UserProperty + 41; 
constexpr int kLatexDisplayProp  = QTextFormat::UserProperty + 42; 
constexpr int kLatexFontSizeProp = QTextFormat::UserProperty + 43; 
constexpr int kLatexColorProp    = QTextFormat::UserProperty + 44; 

constexpr int kLatexOrigSourceProp = QTextFormat::UserProperty + 45; 

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

    static void clearPool();

private:
    LatexInlineHandler() = default;
};

} 

#endif 
