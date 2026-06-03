#ifndef ALGEMATE_THEMEMANAGER_H
#define ALGEMATE_THEMEMANAGER_H

#include <QObject>
#include <QFont>

namespace AlgeMate {

class ThemeManager : public QObject {
    Q_OBJECT
public:
    enum class Theme { Light, Dark };
    Q_ENUM(Theme)

    static ThemeManager& instance();

    Theme  currentTheme() const { return theme_; }
    double fontScale()    const { return fontScale_; }

    void applyTheme(Theme t);
    void toggleTheme();

    void setFontScale(double s);
    void increaseFont();
    void decreaseFont();
    void resetFont();

signals:
    void themeChanged(Theme t);
    void fontScaleChanged(double s);

private:
    explicit ThemeManager(QObject* parent = nullptr);
    ThemeManager(const ThemeManager&) = delete;
    ThemeManager& operator=(const ThemeManager&) = delete;

    QString loadQss(const QString& resourcePath) const;
    void    applyFontScale();

    Theme   theme_       = Theme::Light;
    double  fontScale_   = 1.0;
    QFont   baseFont_;
    QString m_lightQssCache;
    QString m_darkQssCache;
    bool m_qssLoaded = false;
};

}

#endif
