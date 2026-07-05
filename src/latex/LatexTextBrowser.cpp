#include "latex/LatexTextBrowser.h"
#include "latex/LatexInlineHandler.h"
#include "latex/LatexRenderer.h"

#include <QMimeData>
#include <QTextBlock>
#include <QTextDocument>
#include <QTextFragment>

namespace AlgeMate::Latex {

LatexTextBrowser::LatexTextBrowser(QWidget* parent)
    : QTextBrowser(parent)
{

    attachLatexAutoPostProcess(this);
}

void LatexTextBrowser::setSourceMarkdown(const QString& markdown)
{
    m_sourceMarkdown = markdown;
}

QMimeData* LatexTextBrowser::createMimeDataFromSelection() const
{
    QTextCursor cursor = textCursor();

    if (!m_sourceMarkdown.isEmpty()) {
        const int total = document()->characterCount();
        const int start = cursor.selectionStart();
        const int end   = cursor.selectionEnd();
        const bool isAllOrEmpty =
            !cursor.hasSelection() ||
            (start <= 0 && end >= total - 1);
        if (isAllOrEmpty) {
            auto* md = new QMimeData;
            md->setText(m_sourceMarkdown);
            return md;
        }
    }

    if (!cursor.hasSelection())
        return QTextBrowser::createMimeDataFromSelection();

    const int start = cursor.selectionStart();
    const int end   = cursor.selectionEnd();

    QStringList blockTexts;
    QTextBlock block = document()->findBlock(start);
    while (block.isValid()) {
        const int blockPos = block.position();
        if (blockPos >= end) break;

        QString blockText;

        int imageFragCount = 0;
        int textCharCount  = 0;
        for (auto it = block.begin(); it != block.end(); ++it) {
            QTextFragment frag = it.fragment();
            if (!frag.isValid()) continue;
            const auto& cf = frag.charFormat();
            const bool isLatexObj = (cf.objectType() == kLatexObjectType);
            if (cf.isImageFormat() || isLatexObj) ++imageFragCount;
            else textCharCount += frag.text().trimmed().size();
        }
        const bool blockIsSoloImage = (imageFragCount == 1 && textCharCount == 0);

        for (auto it = block.begin(); it != block.end(); ++it) {
            QTextFragment frag = it.fragment();
            if (!frag.isValid()) continue;

            const int fragStart = blockPos + frag.position();
            const int fragEnd   = fragStart + frag.length();
            if (fragEnd <= start || fragStart >= end) continue;

            const int clipStart = qMax(0, start - fragStart);
            const int clipLen   = qMin(frag.length(), end - qMax(start, fragStart))
                                  - clipStart;

            const auto& cf = frag.charFormat();

            if (cf.objectType() == kLatexObjectType) {
                const QString src = cf.property(kLatexOrigSourceProp).toString();
                const bool isDis  = cf.property(kLatexDisplayProp).toBool();
                if (!src.isEmpty()) {
                    if (isDis || blockIsSoloImage)
                        blockText += QStringLiteral("$$") + src + QStringLiteral("$$");
                    else
                        blockText += QStringLiteral("$") + src + QStringLiteral("$");
                }
            } else if (cf.isImageFormat()) {

                const QString url = cf.toImageFormat().name();
                const QString src = LatexRenderer::latexForUrl(url);
                if (!src.isEmpty()) {
                    const bool isDisplay =
                        LatexRenderer::isUrlDisplay(url) || blockIsSoloImage;
                    if (isDisplay)
                        blockText += QStringLiteral("$$") + src + QStringLiteral("$$");
                    else
                        blockText += QStringLiteral("$") + src + QStringLiteral("$");
                } else {
                    blockText += frag.text().mid(clipStart, clipLen);
                }
            } else {
                blockText += frag.text().mid(clipStart, clipLen);
            }
        }

        blockTexts << blockText;
        block = block.next();
    }

    auto* md = new QMimeData;
    md->setText(blockTexts.join(QStringLiteral("\n")));
    return md;
}

} 
