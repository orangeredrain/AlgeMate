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
    // 自动接入矢量化后处理: setHtml() 后会在本事件循环末尾
    // 把 latex-vec:// 占位替换为渲染器质量的 inline object。
    // 避免调用方忘调 postProcessDocument 导致公式丢失。
    attachLatexAutoPostProcess(this);
}

void LatexTextBrowser::setSourceMarkdown(const QString& markdown)
{
    m_sourceMarkdown = markdown;
}

QMimeData* LatexTextBrowser::createMimeDataFromSelection() const
{
    QTextCursor cursor = textCursor();

    // 全选 / 空选 → 直接给原始 markdown 源
    // characterCount 含末尾隐含换行，所以判定 end >= total - 1
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

    // 按 block 收集，每个 block 内部按 fragment 处理；
    // block 之间补换行，块公式（独占一个 block 的 image）用 $$..$$，
    // 行内公式用 $..$。
    QStringList blockTexts;
    QTextBlock block = document()->findBlock(start);
    while (block.isValid()) {
        const int blockPos = block.position();
        if (blockPos >= end) break;

        QString blockText;
        // 检测：本 block 是否仅含一个 image / latex object fragment（判定块公式）
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
            // 矢量化路径：inline ObjectFormat，参数都在 charFormat property
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
                // 旧路径兑底（如果某些地方没调 postProcessDocument，image 仍存在）
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

} // namespace AlgeMate::Latex
