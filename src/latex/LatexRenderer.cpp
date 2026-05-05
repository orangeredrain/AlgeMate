#include "latex/LatexRenderer.h"

#include <jkqtmathtext/jkqtmathtext.h>

#include <QGuiApplication>
#include <QPainter>
#include <QPixmap>
#include <QScreen>
#include <QTextDocument>
#include <QUrl>

namespace AlgeMate::Latex {

// 全局: 所有渲染器的 URL→LaTeX 映射共享, LatexTextBrowser 可直接查询
static QHash<QString, QString> g_urlToLatex;
static long long g_imageCounter = 0;

// 构造 / 配置

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

void LatexRenderer::clearCache()
{
    g_urlToLatex.clear();
}

// 内部: 去注释

static QString stripComments(const QString& src)
{
    QString out;
    out.reserve(src.size());
    const int n = src.size();
    int i = 0;
    while (i < n) {
        QChar ch = src[i];
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

// 内部: 数学宏展开

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

// 内部: JKQTMathText → QPixmap

static QPixmap renderMathPixmap(const QString& latex, int fontSize,
                                const QColor& color, bool displayStyle)
{
    qreal dpr = 1.0;
    if (auto* scr = QGuiApplication::primaryScreen())
        dpr = scr->devicePixelRatio();
    if (dpr < 1.0) dpr = 1.0;

    const int ss = 3;

    JKQTMathText mt;
    mt.useXITS();
    mt.setFontRoman(QStringLiteral("Microsoft YaHei")); // \text{中文} 需要中文字体
    mt.setFontSans(QStringLiteral("Microsoft YaHei"));
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

    const int marginSS = 3 * ss;
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

// 内部: 数学公式 → <img> HTML

static QString mathToImg(const QString& latex, QTextDocument* doc,
                         int fontSize, const QColor& color,
                         bool displayStyle)
{
    QPixmap px = renderMathPixmap(latex, fontSize, color, displayStyle);

    ++g_imageCounter;
    const QString url = QStringLiteral("latex-tex://%1").arg(g_imageCounter);
    doc->addResource(QTextDocument::ImageResource, QUrl(url), px);
    g_urlToLatex[url] = latex;

    int w = int(px.width() / px.devicePixelRatio());
    int h = int(px.height() / px.devicePixelRatio());

    if (displayStyle) {
        return QStringLiteral(
            "<div align=\"center\"><img src=\"%1\" width=\"%2\" height=\"%3\" "
            "style=\"margin:1px 0;\" /></div>"
        ).arg(url).arg(w).arg(h);
    } else {
        return QStringLiteral(
            "<img src=\"%1\" width=\"%2\" height=\"%3\" "
            "style=\"vertical-align:middle; margin:0 1px;\" />"
        ).arg(url).arg(w).arg(h);
    }
}

// 内部: 纯文本 → HTML 转义

static QString escapeText(const QString& s)
{
    QString r = s.toHtmlEscaped();
    r.replace(QLatin1Char('\n'), QStringLiteral("<br/>"));
    return r;
}

// 内部: 匹配括号 / 花括号 (支持嵌套)

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

// 核心

QString LatexRenderer::render(const QString& rawSource, QTextDocument* doc)
{
    if (!doc) return rawSource.toHtmlEscaped();

    QString src = stripComments(rawSource);

    QString html;
    html += QStringLiteral(
        "<div style=\"font-family:'Microsoft YaHei','SimHei','Noto Sans CJK SC',"
        "sans-serif; font-size:10.5pt; line-height:1.35;\">");

    const int n = src.size();
    int i = 0;
    QString textBuf;

    auto flushText = [&]() {
        if (!textBuf.isEmpty()) {
            html += escapeText(textBuf);
            textBuf.clear();
        }
    };

    while (i < n) {
        // 块级公式: $$...$$
        if (src[i] == QLatin1Char('$') && i + 1 < n &&
            src[i + 1] == QLatin1Char('$')) {
            flushText();
            i += 2;
            int end = src.indexOf(QStringLiteral("$$"), i);
            if (end < 0) {
                textBuf += QStringLiteral("$$");
                continue;
            }
            QString latex = src.mid(i, end - i).trimmed();
            if (!latex.isEmpty()) {
                latex = expandMathMacros(latex, m_mathMacros);
                html += mathToImg(latex, doc, m_fontSize, m_textColor,
                                  /*displayStyle=*/true);
            }
            i = end + 2;
            continue;
        }

        // 块级公式: \[...\]
        if (src[i] == QLatin1Char('\\') && i + 1 < n &&
            src[i + 1] == QLatin1Char('[')) {
            flushText();
            i += 2;
            int end = src.indexOf(QStringLiteral("\\]"), i);
            if (end < 0) {
                textBuf += QStringLiteral("\\[");
                continue;
            }
            QString latex = src.mid(i, end - i).trimmed();
            if (!latex.isEmpty()) {
                latex = expandMathMacros(latex, m_mathMacros);
                html += mathToImg(latex, doc, m_fontSize, m_textColor,
                                  /*displayStyle=*/true);
            }
            i = end + 2;
            continue;
        }

        // 行内公式: $...$
        if (src[i] == QLatin1Char('$')) {
            flushText();
            ++i;
            int end = src.indexOf(QLatin1Char('$'), i);
            if (end < 0) {
                textBuf += QLatin1Char('$');
                continue;
            }
            QString latex = src.mid(i, end - i);
            if (!latex.isEmpty()) {
                latex = expandMathMacros(latex, m_mathMacros);
                html += mathToImg(latex, doc, m_fontSize, m_textColor,
                                  /*displayStyle=*/false);
            }
            i = end + 1;
            continue;
        }

        // 自定义文档命令: \cmd[opt]{arg} 或 \cmd{arg}
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

        // 普通文本
        textBuf += src[i];
        ++i;
    }

    flushText();
    html += QStringLiteral("</div>");
    return html;
}

} // namespace AlgeMate::Latex
