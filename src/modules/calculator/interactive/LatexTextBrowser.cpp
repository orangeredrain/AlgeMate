
#include "LatexTextBrowser.h"

#include "expr/Value.h"  
#include "latex/LatexInlineHandler.h"
#include "latex/LatexRenderer.h"

#include <QMimeData>
#include <QTextBlock>
#include <QTextCursor>
#include <QTextDocument>
#include <QTextFragment>

namespace AlgeMate::Calculator::Interactive {

QMimeData* LatexTextBrowser::createMimeDataFromSelection() const {
    QTextCursor cur = textCursor();
    if (!cur.hasSelection()) return QTextBrowser::createMimeDataFromSelection();

    const int selStart = cur.selectionStart();
    const int selEnd   = cur.selectionEnd();
    QString out;
    QTextBlock block = document()->findBlock(selStart);
    while (block.isValid() && block.position() < selEnd) {
        for (auto it = block.begin(); !it.atEnd(); ++it) {
            QTextFragment frag = it.fragment();
            const int fs = frag.position();
            const int fe = fs + frag.length();
            if (fe <= selStart || fs >= selEnd) continue;
            const QTextCharFormat cf = frag.charFormat();

            if (cf.objectType() == AlgeMate::Latex::kLatexObjectType) {
                QString src = cf.property(AlgeMate::Latex::kLatexOrigSourceProp).toString();
                if (src.isEmpty())
                    src = cf.property(AlgeMate::Latex::kLatexSourceProp).toString();
                if (!src.isEmpty()) {
                    out += src;
                    continue;
                }
            }
            if (cf.isImageFormat()) {
                const QString url = cf.toImageFormat().name();

                QString latex = AlgeMate::Latex::LatexRenderer::latexForUrl(url);
                if (latex.isEmpty()) latex = latexForImageUrl(url);
                if (!latex.isEmpty()) {
                    out += latex;
                    continue;
                }
            }
            const int chunkStart = qMax(fs, selStart) - fs;
            const int chunkEnd   = qMin(fe, selEnd) - fs;
            QString txt = frag.text().mid(chunkStart, chunkEnd - chunkStart);
            txt.remove(QChar::ObjectReplacementCharacter);
            out += txt;
        }
        block = block.next();
        if (block.isValid() && block.position() < selEnd) out += QChar('\n');
    }
    auto* md = new QMimeData;
    md->setText(out);
    return md;
}

} 
