#include "latex/LatexInlineHandler.h"

#include <jkqtmathtext/jkqtmathtext.h>

#include <QFontDatabase>
#include <QGuiApplication>
#include <QHash>
#include <QPainter>
#include <QPixmap>
#include <QStringList>
#include <memory>

namespace AlgeMate::Latex {

static const QString& cjkFontName()
{
    static const QString name = []() -> QString {
        const QStringList preferred = {
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
    return name;
}

struct PoolKey {
    QString latex;
    bool    display;
    int     fontSize;
    QRgb    color;
    bool operator==(const PoolKey& o) const {
        return display == o.display && fontSize == o.fontSize
            && color == o.color && latex == o.latex;
    }
};

inline uint qHash(const PoolKey& k, uint seed = 0)
{
    return qHash(k.latex, seed) ^ uint(k.fontSize)
         ^ uint(k.color) ^ (k.display ? 0x55aau : 0u);
}

static QHash<PoolKey, std::shared_ptr<JKQTMathText>> g_pool;

static std::shared_ptr<JKQTMathText> getOrCreateMt(
    const QString& latex, bool display, int fontSize, const QColor& color)
{
    PoolKey key{latex, display, fontSize, color.rgba()};
    auto it = g_pool.find(key);
    if (it != g_pool.end()) return it.value();

    auto mt = std::make_shared<JKQTMathText>();
    mt->useXITS();
    mt->setFontRoman(cjkFontName());
    mt->setFontSans(cjkFontName());
    mt->setFontSize(fontSize);
    mt->setFontColor(color);

    QString wrapped = display
        ? (QStringLiteral("$\\displaystyle ") + latex + QStringLiteral("$"))
        : (QStringLiteral("$") + latex + QStringLiteral("$"));
    mt->parse(wrapped);

    g_pool.insert(key, mt);
    return mt;
}

LatexInlineHandler* LatexInlineHandler::instance()
{
    static LatexInlineHandler s;
    return &s;
}

QSizeF LatexInlineHandler::intrinsicSize(QTextDocument*, int,
                                        const QTextFormat& fmt)
{
    const QString src = fmt.property(kLatexSourceProp).toString();
    if (src.isEmpty()) return QSizeF(8, 8);

    const bool   dis = fmt.property(kLatexDisplayProp).toBool();
    const int    fs  = fmt.property(kLatexFontSizeProp).toInt();
    const QColor col = fmt.property(kLatexColorProp).value<QColor>();

    auto mt = getOrCreateMt(src, dis, fs <= 0 ? 14 : fs,
                            col.isValid() ? col : QColor(Qt::black));
    QPixmap probe(4, 4);
    QPainter p(&probe);
    QSizeF sz = mt->getSize(p);
    p.end();

    return QSizeF(sz.width() + 4.0, sz.height() + 2.0);
}

void LatexInlineHandler::drawObject(QPainter* painter, const QRectF& rect,
                                    QTextDocument*, int,
                                    const QTextFormat& fmt)
{
    if (!painter) return;
    const QString src = fmt.property(kLatexSourceProp).toString();
    if (src.isEmpty()) return;

    const bool   dis = fmt.property(kLatexDisplayProp).toBool();
    const int    fs  = fmt.property(kLatexFontSizeProp).toInt();
    const QColor col = fmt.property(kLatexColorProp).value<QColor>();

    auto mt = getOrCreateMt(src, dis, fs <= 0 ? 14 : fs,
                            col.isValid() ? col : QColor(Qt::black));

    painter->save();
    painter->setRenderHint(QPainter::Antialiasing,         true);
    painter->setRenderHint(QPainter::TextAntialiasing,     true);
    painter->setRenderHint(QPainter::SmoothPixmapTransform, true);

    const QRectF inner = rect.adjusted(2.0, 1.0, -2.0, -1.0);
    mt->draw(*painter, Qt::AlignCenter, inner, false);

    painter->restore();
}

void LatexInlineHandler::clearPool()
{
    g_pool.clear();
}

} 
