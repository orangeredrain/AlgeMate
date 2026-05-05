#include "latex/LatexTextBrowser.h"
#include "latex/LatexRenderer.h"

#include <QMimeData>
#include <QTextBlock>
#include <QTextFragment>

namespace AlgeMate::Latex {

LatexTextBrowser::LatexTextBrowser(QWidget* parent)
    : QTextBrowser(parent)
{
}

QMimeData* LatexTextBrowser::createMimeDataFromSelection() const
{
    QTextCursor cursor = textCursor();
    if (!cursor.hasSelection())
        return QTextBrowser::createMimeDataFromSelection();

    int start = cursor.selectionStart();
    int end   = cursor.selectionEnd();

    QStringList parts;
    QTextBlock block = document()->findBlock(start);
    while (block.isValid()) {
        int blockPos = block.position();
        if (blockPos >= end) break;

        for (auto it = block.begin(); it != block.end(); ++it) {
            QTextFragment frag = it.fragment();
            if (!frag.isValid()) continue;

            int fragStart = blockPos + frag.position();
            int fragEnd   = fragStart + frag.length();
            if (fragEnd <= start || fragStart >= end) continue;

            int clipStart = qMax(0, start - fragStart);
            int clipLen   = qMin(frag.length(), end - qMax(start, fragStart))
                            - clipStart;

            if (frag.charFormat().isImageFormat()) {
                QString url = frag.charFormat().toImageFormat().name();
                QString src = LatexRenderer::latexForUrl(url);
                if (!src.isEmpty())
                    parts << (QStringLiteral("$") + src + QStringLiteral("$"));
                else
                    parts << frag.text().mid(clipStart, clipLen);
            } else {
                parts << frag.text().mid(clipStart, clipLen);
            }
        }
        block = block.next();
    }

    auto* md = new QMimeData;
    md->setText(parts.join(QString()));
    return md;
}

} // namespace AlgeMate::Latex
