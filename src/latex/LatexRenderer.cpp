#include "latex/LatexRenderer.h"
#include "latex/LatexInlineHandler.h"

#include <jkqtmathtext/jkqtmathtext.h>

#include <QAbstractTextDocumentLayout>
#include <QGuiApplication>
#include <QPainter>
#include <QPixmap>
#include <QScreen>
#include <QTextBlock>
#include <QTextCursor>
#include <QTextDocument>
#include <QTextFragment>
#include <QUrl>
#include <QRegularExpression>
#include <QFontDatabase>
#include <QSet>

namespace AlgeMate::Latex {

static QHash<QString, QString> g_urlToLatex;

static QSet<QString> g_displayUrls;

static QHash<QString, int>    g_urlFontSize;
static QHash<QString, QColor> g_urlColor;
static QHash<QString, QString> g_urlToFixed;
static long long g_imageCounter = 0;

LatexRenderer::LatexRenderer() = default;

void LatexRenderer::setFontSize(int pt) { m_fontSize = pt; }
void LatexRenderer::setTextColor(QColor c) { m_textColor = c; }

void LatexRenderer::addMathMacro(const QString& cmd, const QString& expansion)
{
    m_mathMacros[cmd] = expansion;
}

void LatexRenderer::addCommand(const QString& name, CmdHandler handler)
{
    m_commands[name] = handler;
}

QString LatexRenderer::latexForUrl(const QString& url)
{
    return g_urlToLatex.value(url);
}

bool LatexRenderer::isUrlDisplay(const QString& url)
{
    return g_displayUrls.contains(url);
}

void LatexRenderer::clearCache()
{
    g_urlToLatex.clear();
    g_displayUrls.clear();
    g_urlFontSize.clear();
    g_urlColor.clear();
    g_urlToFixed.clear();
    LatexInlineHandler::clearPool();
}

void LatexRenderer::postProcessDocument(QTextDocument* doc)
{
    if (!doc) return;

    if (auto* layout = doc->documentLayout()) {
        layout->registerHandler(kLatexObjectType, LatexInlineHandler::instance());
    }

    struct Hit { int pos; int len; QString url; };
    QList<Hit> hits;
    for (QTextBlock block = doc->begin(); block.isValid(); block = block.next()) {
        for (auto it = block.begin(); it != block.end(); ++it) {
            QTextFragment frag = it.fragment();
            if (!frag.isValid()) continue;
            if (!frag.charFormat().isImageFormat()) continue;
            const QString url = frag.charFormat().toImageFormat().name();
            if (!url.startsWith(QStringLiteral("latex-vec://"))) continue;
            hits.append({frag.position(), frag.length(), url});
        }
    }
    if (hits.isEmpty()) return;

    QTextCursor cursor(doc);
    cursor.beginEditBlock();
    for (int i = hits.size() - 1; i >= 0; --i) {
        const Hit& h = hits[i];
        const QString fixed = g_urlToFixed.value(h.url);
        const QString orig  = g_urlToLatex.value(h.url);
        if (fixed.isEmpty()) continue;

        QTextCharFormat fmt;
        fmt.setObjectType(kLatexObjectType);

        fmt.setVerticalAlignment(QTextCharFormat::AlignMiddle);
        fmt.setProperty(kLatexSourceProp,     fixed);
        fmt.setProperty(kLatexOrigSourceProp, orig);
        fmt.setProperty(kLatexDisplayProp,    g_displayUrls.contains(h.url));
        fmt.setProperty(kLatexFontSizeProp,   g_urlFontSize.value(h.url, 14));
        fmt.setProperty(kLatexColorProp,      g_urlColor.value(h.url, QColor(Qt::black)));

        cursor.setPosition(h.pos);
        cursor.setPosition(h.pos + h.len, QTextCursor::KeepAnchor);
        cursor.removeSelectedText();
        cursor.insertText(QString(QChar::ObjectReplacementCharacter), fmt);
    }
    cursor.endEditBlock();
}

static QString stripComments(const QString& src)
{
    QString out;
    out.reserve(src.size());
    const int n = src.size();
    int i = 0;
    while (i < n) {
        QChar ch = src[i];

        const ushort u = ch.unicode();
        const bool isCtrl =
            (u < 0x20 && u != 0x09 && u != 0x0A) ||
            u == 0x7F ||
            u == 0x00AD ||
            u == 0x200B || u == 0x200C || u == 0x200D ||
            u == 0x202A || u == 0x202B || u == 0x202C || u == 0x202D || u == 0x202E ||
            u == 0x2060 ||
            u == 0xFEFF;
        if (isCtrl) { ++i; continue; }
        if (ch == QLatin1Char('%')) {
            ++i;
            while (i < n && src[i] != QLatin1Char('\n'))
                ++i;
            if (i < n && src[i] == QLatin1Char('\n')) {
                out += QLatin1Char('\n');
                ++i;
            }
        } else {
            out += ch;
            ++i;
        }
    }
    return out;
}

static QString expandMathMacros(const QString& latex,
                                const QHash<QString, QString>& macros)
{
    if (macros.isEmpty()) return latex;

    QStringList names;
    for (auto it = macros.begin(); it != macros.end(); ++it)
        names.append(it.key());
    std::sort(names.begin(), names.end(),
              [](const QString& a, const QString& b) { return a.size() > b.size(); });

    QString result = latex;
    for (const QString& cmd : names) {
        const QString pattern = QLatin1Char('\\') + cmd;
        const int patLen = pattern.size();
        int pos = 0;
        while (pos < result.size()) {
            int found = result.indexOf(pattern, pos);
            if (found < 0) break;

            int after = found + patLen;
            if (after < result.size() && result[after].isLetter()) {
                pos = after;
                continue;
            }
            result.replace(found, patLen, macros[cmd]);
            pos = found + macros[cmd].size();
        }
    }
    return result;
}

static QPixmap renderMathPixmap(const QString& latex, int fontSize,
                                const QColor& color, bool displayStyle)
{
    qreal dpr = 1.0;
    if (auto* scr = QGuiApplication::primaryScreen())
        dpr = scr->devicePixelRatio();
    if (dpr < 1.0) dpr = 1.0;

    const int ss = 2;

    static const QString kCjkFont = []() -> QString {
        static const QStringList preferred = {
            QStringLiteral("Source Han Serif SC"),
            QStringLiteral("Source Han Serif CN"),
            QStringLiteral("Noto Serif CJK SC"),
            QStringLiteral("PingFang SC"),
            QStringLiteral("Hiragino Sans GB"),
            QStringLiteral("STSong"),
            QStringLiteral("Heiti SC"),
            QStringLiteral("STHeiti"),
            QStringLiteral("SimSun"),
            QStringLiteral("宋体"),
            QStringLiteral("Microsoft YaHei")
        };
        const QStringList all = QFontDatabase::families();
        for (const QString& p : preferred) {
            for (const QString& f : all) {
                if (f.compare(p, Qt::CaseInsensitive) == 0) return p;
            }
        }
        return QGuiApplication::font().family();
    }();

    JKQTMathText mt;
    mt.useXITS();
    mt.setFontRoman(kCjkFont); 
    mt.setFontSans(kCjkFont);
    mt.setFontSize(fontSize * ss);
    mt.setFontColor(color);

    QString wrapped;
    if (displayStyle)
        wrapped = QStringLiteral("$\\displaystyle ") + latex + QStringLiteral("$");
    else
        wrapped = QStringLiteral("$") + latex + QStringLiteral("$");

    mt.parse(wrapped);

    QPixmap probe(4, 4);
    QPainter pm(&probe);
    QSizeF sz = mt.getSize(pm);
    pm.end();

    const int marginSS = 1 * ss;
    int wSS = qCeil(sz.width()) + marginSS * 2;
    int hSS = qCeil(sz.height()) + marginSS * 2;
    if (wSS < 8) wSS = 8;
    if (hSS < 8) hSS = 8;

    QPixmap big(wSS, hSS);
    big.fill(Qt::transparent);
    {
        QPainter p(&big);
        p.setRenderHint(QPainter::Antialiasing, true);
        p.setRenderHint(QPainter::TextAntialiasing, true);
        p.setRenderHint(QPainter::SmoothPixmapTransform, true);
        mt.draw(p, Qt::AlignCenter, QRectF(0, 0, wSS, hSS), false);
    }

    int wP = int((wSS / ss) * dpr);
    int hP = int((hSS / ss) * dpr);
    if (wP < 2) wP = 2;
    if (hP < 2) hP = 2;

    QPixmap scaled = big.scaled(wP, hP, Qt::IgnoreAspectRatio,
                                Qt::SmoothTransformation);
    scaled.setDevicePixelRatio(dpr);
    return scaled;
}

static QString normalizeSingleColumnMatrices(QString latex)
{

    auto extractBraceArg = [](const QString& s, int pos, int& outEnd) -> QString {
        if (pos < 0 || pos >= s.size() || s[pos] != QLatin1Char('{')) {
            outEnd = -1;
            return QString();
        }
        int depth = 1;
        int i = pos + 1;
        while (i < s.size() && depth > 0) {
            const QChar c = s[i];
            if (c == QLatin1Char('\\') && i + 1 < s.size()) {

                i += 2;
                continue;
            }
            if (c == QLatin1Char('{')) ++depth;
            else if (c == QLatin1Char('}')) {
                --depth;
                if (depth == 0) {
                    outEnd = i + 1;
                    return s.mid(pos + 1, i - pos - 1);
                }
            }
            ++i;
        }
        outEnd = -1;
        return QString();
    };

    auto skipSpaces = [](const QString& s, int from) {
        while (from < s.size() && s[from].isSpace()) ++from;
        return from;
    };

    for (const QString& cmd : { QStringLiteral("\\boldsymbol"), QStringLiteral("\\bm") }) {
        int p = 0;
        while ((p = latex.indexOf(cmd, p)) >= 0) {

            const int after = p + cmd.size();
            if (after < latex.size() && latex[after].isLetter()) { p = after; continue; }
            const int b = skipSpaces(latex, after);
            if (b >= latex.size() || latex[b] != QLatin1Char('{')) { p = after; continue; }
            int end = -1;
            const QString arg = extractBraceArg(latex, b, end);
            if (end < 0) { p = after; continue; }
            const QString repl = QStringLiteral("\\mathbf{%1}").arg(arg);
            latex.replace(p, end - p, repl);
            p += repl.size();
        }
    }

    {
        const QString cmd = QStringLiteral("\\binom");
        int p = 0;
        while ((p = latex.indexOf(cmd, p)) >= 0) {
            const int after = p + cmd.size();
            if (after < latex.size() && latex[after].isLetter()) { p = after; continue; }
            int b = skipSpaces(latex, after);
            if (b >= latex.size() || latex[b] != QLatin1Char('{')) { p = after; continue; }
            int end1 = -1;
            const QString a = extractBraceArg(latex, b, end1);
            if (end1 < 0) { p = after; continue; }
            int b2 = skipSpaces(latex, end1);
            if (b2 >= latex.size() || latex[b2] != QLatin1Char('{')) { p = after; continue; }
            int end2 = -1;
            const QString c = extractBraceArg(latex, b2, end2);
            if (end2 < 0) { p = after; continue; }
            const QString repl = QStringLiteral(
                "\\left(\\begin{array}{rcl} & %1 & \\\\ & %2 & \\end{array}\\right)"
            ).arg(a, c);
            latex.replace(p, end2 - p, repl);
            p += repl.size();
        }
    }

    struct EnvSpec {
        const char* env;
        const char* leftDelim;
        const char* rightDelim;
    };
    static const EnvSpec kSpecs[] = {
        {"pmatrix", "(",   ")"},
        {"bmatrix", "[",   "]"},
        {"vmatrix", "|",   "|"},
        {"Vmatrix", "\\|", "\\|"},
    };

    auto rewriteSingleCol = [](QString content) -> QString {

        if (!content.contains(QStringLiteral("\\\\"))) {
            QString fixed;
            fixed.reserve(content.size() + 8);
            for (int i = 0; i < content.size(); ++i) {
                if (content[i] == QLatin1Char('\\') &&
                    i + 1 < content.size() && content[i + 1].isSpace()) {
                    fixed += QStringLiteral("\\\\");
                } else {
                    fixed += content[i];
                }
            }
            content = fixed;
        }

        QStringList rows;
        int q = 0;
        while (q < content.size()) {
            int sp = content.indexOf(QStringLiteral("\\\\"), q);
            if (sp < 0) { rows << content.mid(q); break; }
            rows << content.mid(q, sp - q);
            q = sp + 2;
        }
        QString out;
        for (int i = 0; i < rows.size(); ++i) {
            if (i) out += QStringLiteral(" \\\\ ");
            out += QStringLiteral(" & ");
            out += rows[i].trimmed();
            out += QStringLiteral(" & ");
        }
        return out;
    };

    for (const auto& s : kSpecs) {
        const QString begin = QStringLiteral("\\begin{%1}").arg(QString::fromLatin1(s.env));
        const QString end   = QStringLiteral("\\end{%1}").arg(QString::fromLatin1(s.env));
        int p = 0;
        while ((p = latex.indexOf(begin, p)) >= 0) {
            const int cs = p + begin.size();
            const int e  = latex.indexOf(end, cs);
            if (e < 0) break;
            const QString content = latex.mid(cs, e - cs);
            if (!content.contains(QLatin1Char('&'))) {
                const QString newContent = rewriteSingleCol(content);
                const QString repl = QStringLiteral("\\left%1\\begin{array}{rcl}%2\\end{array}\\right%3")
                    .arg(QString::fromLatin1(s.leftDelim), newContent,
                         QString::fromLatin1(s.rightDelim));
                latex.replace(p, e + end.size() - p, repl);
                p += repl.size();
            } else {
                p = e + end.size();
            }
        }
    }

    {
        const QString begin = QStringLiteral("\\begin{array}");
        const QString end   = QStringLiteral("\\end{array}");
        int p = 0;
        while ((p = latex.indexOf(begin, p)) >= 0) {
            int ab = p + begin.size();
            if (ab >= latex.size() || latex[ab] != QLatin1Char('{')) { p = ab; continue; }
            int cb = ab + 1, depth = 1;
            while (cb < latex.size() && depth > 0) {
                if (latex[cb] == QLatin1Char('{')) ++depth;
                else if (latex[cb] == QLatin1Char('}')) --depth;
                if (depth > 0) ++cb;
            }
            if (depth != 0) break;
            const int cs = cb + 1;
            const int e  = latex.indexOf(end, cs);
            if (e < 0) break;
            const QString content = latex.mid(cs, e - cs);
            if (!content.contains(QLatin1Char('&'))) {
                const QString newContent = rewriteSingleCol(content);
                const QString repl = QStringLiteral("\\begin{array}{rcl}%1\\end{array}")
                    .arg(newContent);
                latex.replace(p, e + end.size() - p, repl);
                p += repl.size();
            } else {
                p = e + end.size();
            }
        }
    }

    return latex;
}

static QString mathToImg(const QString& latex, QTextDocument* doc,
                         int fontSize, const QColor& color,
                         bool displayStyle)
{
    return LatexRenderer::embedAsImg(doc, latex, displayStyle, fontSize, color);
}

QString LatexRenderer::embedAsImg(QTextDocument* doc, const QString& latex,
                                  bool displayStyle, int fontSize, QColor color)
{
    if (!doc) return QString();

    QString clean = latex;
    clean.replace(QChar(0x01), QString());
    clean.replace(QChar(0x02), QString());
    clean.replace(QChar(0x07), QString());

    {
        const QString s = clean.trimmed();
        bool hasMathSyntax = false;
        bool hasCJK = false;
        for (QChar c : s) {
            const ushort u = c.unicode();
            if (u == '\\' || u == '_' || u == '^' || u == '{' || u == '}' ||
                u == '=' || u == '+' || u == '*' || u == '/' || u == '<' || u == '>' ||
                (u >= '0' && u <= '9')) {
                hasMathSyntax = true;
            }
            if ((u >= 0x4E00 && u <= 0x9FFF) ||
                (u >= 0x3000 && u <= 0x303F) ||
                (u >= 0xFF00 && u <= 0xFFEF)) {
                hasCJK = true;
            }
        }
        if (hasCJK && !hasMathSyntax) {
            return latex.toHtmlEscaped();
        }
    }

    const QString fixed = normalizeSingleColumnMatrices(clean);

    ++g_imageCounter;
    const QString url = QStringLiteral("latex-vec://%1").arg(g_imageCounter);
    QPixmap placeholder(8, 8);
    placeholder.fill(Qt::transparent);
    doc->addResource(QTextDocument::ImageResource, QUrl(url), placeholder);
    g_urlToLatex[url] = latex;          
    g_urlToFixed[url] = fixed;          
    g_urlFontSize[url] = fontSize;
    g_urlColor[url] = color;
    if (displayStyle) g_displayUrls.insert(url);

    if (displayStyle) {
        return QStringLiteral(
            "<div align=\"center\"><img src=\"%1\" width=\"8\" height=\"8\" /></div>"
        ).arg(url);
    } else {
        return QStringLiteral(
            "<img src=\"%1\" width=\"8\" height=\"8\" style=\"vertical-align:middle;\" />"
        ).arg(url);
    }
}

static QString escapeText(const QString& s)
{
    QString r = s.toHtmlEscaped();
    r.replace(QLatin1Char('\n'), QStringLiteral("<br/>"));
    return r;
}

static QString renderMarkdownText(QString s)
{

    QRegularExpression h1(R"((^|\n)# ([^\n]+))");
    s.replace(h1, "\\1<h1 style=\"font-size:22px; font-weight:bold; margin:6px 0;\">\\2</h1>");

    QRegularExpression h2(R"((^|\n)## ([^\n]+))");
    s.replace(h2, "\\1<h2 style=\"font-size:18px; font-weight:bold; margin:4px 0;\">\\2</h2>");

    QRegularExpression h3(R"((^|\n)### ([^\n]+))");
    s.replace(h3, "\\1<h3 style=\"font-size:15px; font-weight:bold; margin:2px 0;\">\\2</h3>");

    QRegularExpression h4(R"((^|\n)#### ([^\n]+))");
    s.replace(h4, "\\1<h4 style=\"font-size:14px; font-weight:bold; margin:2px 0;\">\\2</h4>");

    QRegularExpression bold(R"(\*\*(.*?)\*\*)");
    s.replace(bold, "<b>\\1</b>");

    QRegularExpression italic(R"(\*(.*?)\*)");
    s.replace(italic, "<i>\\1</i>");

    QRegularExpression hr(R"((^|\n)---(\n|$))");
    s.replace(hr, "\\1<hr style=\"border:none; border-top:1px solid #cbd5e0; margin:8px 0;\">\\2");

    s.replace('\n', "<br/>");

    return s;
}

static QString maskMathPipesGlobal(const QString& src)
{
    QString out;
    out.reserve(src.size());
    bool inMath = false;
    bool inBlock = false;
    const int n = src.size();
    int i = 0;
    while (i < n) {
        const QChar c = src[i];

        if (c == QLatin1Char('\n') && inMath) {
            const bool nextIsLF =
                (i + 1 < n && src[i + 1] == QLatin1Char('\n'));
            if (!inBlock) {
                inMath = false;
            } else if (nextIsLF) {
                inMath = false;
                inBlock = false;
            }
        }

        if (c == QLatin1Char('$')) {
            if (i + 1 < n && src[i + 1] == QLatin1Char('$')) {
                inMath = !inMath;
                inBlock = inMath;
                out += QLatin1Char('$');
                out += QLatin1Char('$');
                i += 2;
                continue;
            }
            inMath = !inMath;
            if (inMath) inBlock = false;
            out += c;
            ++i;
            continue;
        }
        if (inMath && c == QLatin1Char('|')) {
            out += QChar(0x07);
        } else {
            out += c;
        }
        ++i;
    }
    return out;
}

static QString unmaskMathPipes(QString s)
{
    s.replace(QChar(0x07), QLatin1Char('|'));
    return s;
}

static bool isTableSeparatorMasked(const QString& maskedLine)
{
    QString t = maskedLine.trimmed();
    if (!t.contains(QLatin1Char('|'))) return false;
    if (t.startsWith(QLatin1Char('|'))) t = t.mid(1);
    if (t.endsWith(QLatin1Char('|'))) t.chop(1);
    const QStringList parts = t.split(QLatin1Char('|'));
    if (parts.isEmpty()) return false;
    static const QRegularExpression sepRe(QStringLiteral("^\\s*:?-{2,}:?\\s*$"));
    for (const QString& p : parts) {
        if (!sepRe.match(p).hasMatch()) return false;
    }
    return true;
}

static QStringList splitTableRowMasked(const QString& maskedLine)
{
    QString t = maskedLine.trimmed();
    if (t.startsWith(QLatin1Char('|'))) t = t.mid(1);
    if (t.endsWith(QLatin1Char('|'))) t.chop(1);
    QStringList cells = t.split(QLatin1Char('|'));
    for (QString& c : cells) c = unmaskMathPipes(c.trimmed());
    return cells;
}

static QString renderCell(const QString& cell, QTextDocument* doc,
                          int fontSize, const QColor& color,
                          const QHash<QString, QString>& macros)
{
    QString out;
    QString textBuf;
    auto flushText = [&]() {
        if (textBuf.isEmpty()) return;
        QString r = textBuf.toHtmlEscaped();
        static const QRegularExpression bold(QStringLiteral("\\*\\*(.*?)\\*\\*"));
        r.replace(bold, QStringLiteral("<b>\\1</b>"));
        static const QRegularExpression italic(QStringLiteral("\\*(.*?)\\*"));
        r.replace(italic, QStringLiteral("<i>\\1</i>"));
        out += r;
        textBuf.clear();
    };
    const int n = cell.size();
    int i = 0;
    while (i < n) {
        if (cell[i] == QLatin1Char('$')) {
            flushText();
            ++i;
            int end = cell.indexOf(QLatin1Char('$'), i);
            if (end < 0) { textBuf += QLatin1Char('$'); continue; }
            QString latex = cell.mid(i, end - i);
            if (!latex.isEmpty()) {
                latex = expandMathMacros(latex, macros);
                out += mathToImg(latex, doc, fontSize, color, false);
            }
            i = end + 1;
            continue;
        }
        textBuf += cell[i];
        ++i;
    }
    flushText();
    return out;
}

struct TableData {
    QStringList align;
    QStringList header;
    QList<QStringList> rows;
};

static QString extractTables(const QString& src, QList<TableData>& tables)
{

    const QString masked = maskMathPipesGlobal(src);
    const QStringList lines = src.split(QLatin1Char('\n'));
    const QStringList maskedLines = masked.split(QLatin1Char('\n'));
    if (lines.size() != maskedLines.size()) {

        return src;
    }
    QString out;
    out.reserve(src.size());
    int i = 0;
    const int total = lines.size();
    while (i < total) {
        const QString& m = maskedLines[i];
        if (i + 1 < total && m.contains(QLatin1Char('|')) &&
            isTableSeparatorMasked(maskedLines[i + 1])) {
            TableData td;
            td.header = splitTableRowMasked(maskedLines[i]);
            const QStringList sep = splitTableRowMasked(maskedLines[i + 1]);
            for (const QString& s2 : sep) {
                const bool l = s2.startsWith(QLatin1Char(':'));
                const bool r = s2.endsWith(QLatin1Char(':'));
                if (l && r) td.align << QStringLiteral("center");
                else if (r) td.align << QStringLiteral("right");
                else td.align << QStringLiteral("left");
            }
            int j = i + 2;
            while (j < total) {
                if (lines[j].trimmed().isEmpty()) break;
                if (!maskedLines[j].contains(QLatin1Char('|'))) break;
                td.rows << splitTableRowMasked(maskedLines[j]);
                ++j;
            }
            const int idx = tables.size();
            tables << td;
            out += QStringLiteral("\x01TBL%1\x02").arg(idx);
            if (j < total) out += QLatin1Char('\n');
            i = j;
        } else {
            out += lines[i];
            if (i + 1 < total) out += QLatin1Char('\n');
            ++i;
        }
    }
    return out;
}

static QString renderTableHtml(const TableData& td, QTextDocument* doc,
                               int fontSize, const QColor& color,
                               const QHash<QString, QString>& macros)
{
    QString html;
    html += QStringLiteral(
        "<table border='1' cellspacing='0' cellpadding='4' "
        "style='border-collapse:collapse; margin:8px 0;'>");
    html += QStringLiteral("<thead><tr>");
    for (int k = 0; k < td.header.size(); ++k) {
        const QString a = (k < td.align.size() ? td.align[k] : QStringLiteral("left"));
        html += QStringLiteral(
            "<th style='border:1px solid #888; background:#f3f4f6; "
            "padding:4px 10px; text-align:%1;'>").arg(a);
        html += renderCell(td.header[k], doc, fontSize, color, macros);
        html += QStringLiteral("</th>");
    }
    html += QStringLiteral("</tr></thead><tbody>");
    for (const QStringList& row : td.rows) {
        html += QStringLiteral("<tr>");
        for (int k = 0; k < row.size(); ++k) {
            const QString a = (k < td.align.size() ? td.align[k] : QStringLiteral("left"));
            html += QStringLiteral(
                "<td style='border:1px solid #888; padding:4px 10px; "
                "text-align:%1;'>").arg(a);
            html += renderCell(row[k], doc, fontSize, color, macros);
            html += QStringLiteral("</td>");
        }
        html += QStringLiteral("</tr>");
    }
    html += QStringLiteral("</tbody></table>");
    return html;
}

static int findMathClose(const QString& src, int from,
                         const QString& closeTok, bool blockLevel)
{
    const int n = src.size();
    const int tokLen = closeTok.size();
    int p = from;
    while (p + tokLen <= n) {
        if (blockLevel) {

            if (src[p] == QLatin1Char('\n') &&
                p + 1 < n && src[p + 1] == QLatin1Char('\n')) {
                return -1;
            }
        } else {

            if (src[p] == QLatin1Char('\n')) return -1;
        }
        if (QStringView(src).mid(p, tokLen) == closeTok) return p;
        ++p;
    }
    return -1;
}

static int findClosing(const QString& s, int openPos,
                       QChar openCh, QChar closeCh)
{
    int depth = 0;
    for (int i = openPos; i < s.size(); ++i) {
        QChar ch = s[i];
        if (ch == openCh)
            ++depth;
        else if (ch == closeCh) {
            --depth;
            if (depth == 0) return i;
        }
    }
    return -1;
}

QString LatexRenderer::render(const QString& rawSource, QTextDocument* doc)
{
    if (!doc) return rawSource.toHtmlEscaped();

    QString src = stripComments(rawSource);

    QList<TableData> tables;
    src = extractTables(src, tables);

    QString html;
    html += QStringLiteral(
        "<div style=\"font-family:'Microsoft YaHei','SimHei','Noto Sans CJK SC',"
        "sans-serif; font-size:10.5pt; line-height:1.35;\">");

    const int n = src.size();
    int i = 0;
    QString textBuf;

    auto flushText = [&]() {
        if (!textBuf.isEmpty()) {
            html += renderMarkdownText(textBuf);
            textBuf.clear();
        }
    };

    while (i < n) {

        if (src[i].unicode() == 0x01) {
            int end = src.indexOf(QChar(0x02), i + 1);
            if (end > i) {
                const QString token = src.mid(i + 1, end - i - 1);
                if (token.startsWith(QStringLiteral("TBL"))) {
                    bool ok = false;
                    const int idx = token.mid(3).toInt(&ok);
                    if (ok && idx >= 0 && idx < tables.size()) {
                        flushText();
                        html += renderTableHtml(tables[idx], doc, m_fontSize,
                                                m_textColor, m_mathMacros);
                        i = end + 1;
                        continue;
                    }
                }
            }

            ++i;
            continue;
        }

        if (src[i] == QLatin1Char('$') && i + 1 < n &&
            src[i + 1] == QLatin1Char('$')) {
            int end = findMathClose(src, i + 2, QStringLiteral("$$"),
                                    true);
            if (end < 0) {

                const bool isLineEnd = (i + 2 == n) ||
                                       (i + 2 < n && src[i + 2] == QLatin1Char('\n'));
                if (isLineEnd) {
                    const int lineStart = textBuf.lastIndexOf(QLatin1Char('\n'));
                    QString lineCandidate = textBuf.mid(lineStart + 1);
                    const QString tc = lineCandidate.trimmed();
                    const bool looksLikeLatex =
                        !tc.isEmpty() &&
                        (tc.contains(QLatin1Char('\\')) ||
                         tc.contains(QLatin1Char('^')) ||
                         tc.contains(QLatin1Char('_')) ||
                         tc.contains(QLatin1Char('{')) ||
                         tc.contains(QLatin1Char('=')));
                    if (looksLikeLatex) {
                        textBuf.chop(lineCandidate.size());
                        flushText();
                        QString latex = expandMathMacros(tc, m_mathMacros);
                        html += mathToImg(latex, doc, m_fontSize, m_textColor,
                                          true);
                        i += 2;
                        continue;
                    }
                }

                textBuf += QStringLiteral("$$");
                i += 2;
                continue;
            }
            flushText();
            QString latex = src.mid(i + 2, end - i - 2).trimmed();
            if (!latex.isEmpty()) {
                latex = expandMathMacros(latex, m_mathMacros);
                html += mathToImg(latex, doc, m_fontSize, m_textColor,
                                  true);
            }
            i = end + 2;
            continue;
        }

        if (src[i] == QLatin1Char('\\') && i + 1 < n &&
            src[i + 1] == QLatin1Char('[')) {
            int end = findMathClose(src, i + 2, QStringLiteral("\\]"),
                                    true);
            if (end < 0) {
                textBuf += QStringLiteral("\\[");
                i += 2;
                continue;
            }
            flushText();
            QString latex = src.mid(i + 2, end - i - 2).trimmed();
            if (!latex.isEmpty()) {
                latex = expandMathMacros(latex, m_mathMacros);
                html += mathToImg(latex, doc, m_fontSize, m_textColor,
                                  true);
            }
            i = end + 2;
            continue;
        }

        if (src[i] == QLatin1Char('\\') && i + 1 < n &&
            src[i + 1] == QLatin1Char('(')) {
            int end = findMathClose(src, i + 2, QStringLiteral("\\)"),
                                    false);
            if (end < 0) {
                textBuf += QStringLiteral("\\(");
                i += 2;
                continue;
            }
            flushText();
            QString latex = src.mid(i + 2, end - i - 2).trimmed();
            if (!latex.isEmpty()) {
                latex = expandMathMacros(latex, m_mathMacros);
                html += mathToImg(latex, doc, m_fontSize, m_textColor,
                                  false);
            }
            i = end + 2;
            continue;
        }

        if (src[i] == QLatin1Char('$')) {
            int end = findMathClose(src, i + 1, QStringLiteral("$"),
                                    false);
            if (end < 0) {
                textBuf += QLatin1Char('$');
                ++i;
                continue;
            }
            flushText();
            QString latex = src.mid(i + 1, end - i - 1);
            if (!latex.trimmed().isEmpty()) {
                latex = expandMathMacros(latex, m_mathMacros);
                html += mathToImg(latex, doc, m_fontSize, m_textColor,
                                  false);
            }
            i = end + 1;
            continue;
        }

        if (src[i] == QLatin1Char('\\') && i + 1 < n &&
            src[i + 1].isLetter()) {
            int nameStart = i + 1;
            int nameEnd = nameStart;
            while (nameEnd < n && src[nameEnd].isLetter())
                ++nameEnd;
            QString cmdName = src.mid(nameStart, nameEnd - nameStart);

            if (m_commands.contains(cmdName)) {
                flushText();
                int pos = nameEnd;
                QString opt, arg;

                if (pos < n && src[pos] == QLatin1Char('[')) {
                    int close = findClosing(src, pos, QLatin1Char('['),
                                            QLatin1Char(']'));
                    if (close >= 0) {
                        opt = src.mid(pos + 1, close - pos - 1);
                        pos = close + 1;
                    }
                }
                if (pos < n && src[pos] == QLatin1Char('{')) {
                    int close = findClosing(src, pos, QLatin1Char('{'),
                                            QLatin1Char('}'));
                    if (close >= 0) {
                        arg = src.mid(pos + 1, close - pos - 1);
                        pos = close + 1;
                    }
                }

                html += m_commands[cmdName](opt, arg);
                i = pos;
                continue;
            }
        }

        textBuf += src[i];
        ++i;
    }

    flushText();
    html += QStringLiteral("</div>");

    html.replace(QChar(0x01), QString());
    html.replace(QChar(0x02), QString());
    html.replace(QChar(0x07), QString());
    return html;
}

} 
