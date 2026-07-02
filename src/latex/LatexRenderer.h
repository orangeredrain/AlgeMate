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

class LatexRenderer {
public:
    LatexRenderer();

    void setFontSize(int pt);
    void setTextColor(QColor c);

    void addMathMacro(const QString& cmd, const QString& expansion);

    using CmdHandler = std::function<QString(const QString& opt,
                                             const QString& arg)>;
    void addCommand(const QString& name, CmdHandler handler);

    QString render(const QString& source, QTextDocument* doc);

    static QString latexForUrl(const QString& url);

    static bool isUrlDisplay(const QString& url);

    void clearCache();

    static void postProcessDocument(QTextDocument* doc);

    static QString embedAsImg(QTextDocument* doc, const QString& latex,
                              bool displayStyle = false, int fontSize = 14,
                              QColor color = QColor(0, 0, 0));

private:
    int m_fontSize = 14;
    QColor m_textColor = Qt::black;
    QHash<QString, QString> m_mathMacros;
    QHash<QString, CmdHandler> m_commands;
};

inline void attachLatexAutoPostProcess(QTextBrowser* browser)
{
    if (!browser) return;
    QTextDocument* doc = browser->document();
    if (!doc) return;

    static const char kAttachedFlag[] = "_algemate_latex_auto_attached";
    if (doc->property(kAttachedFlag).toBool()) return;
    doc->setProperty(kAttachedFlag, true);

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

} 

#endif 
