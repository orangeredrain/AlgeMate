#include "ThemeManager.h"
#include "AppPaths.h"

#include <QApplication>
#include <QFile>
#include <QFontDatabase>
#include <QTextStream>
#include <QWidget>
#include <algorithm>

namespace AlgeMate {

ThemeManager& ThemeManager::instance() {
    static ThemeManager inst;
    return inst;
}

ThemeManager::ThemeManager(QObject* parent)
    : QObject(parent)
{
    const QStringList candidates{
        QStringLiteral("Microsoft YaHei UI"),
        QStringLiteral("Microsoft YaHei"),
        QStringLiteral("PingFang SC"),
        QStringLiteral("Segoe UI"),
        QStringLiteral("Noto Sans CJK SC")
    };
    QString chosen;
    const QStringList installed = QFontDatabase::families();
    for (const auto& f : candidates) {
        if (installed.contains(f, Qt::CaseInsensitive)) { chosen = f; break; }
    }
    baseFont_ = QApplication::font();
    if (!chosen.isEmpty()) baseFont_.setFamily(chosen);
    baseFont_.setPointSizeF(10.0);
    QApplication::setFont(baseFont_);
}

QString ThemeManager::loadQss(const QString& path) const {
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) return {};
    QTextStream ts(&f);
    ts.setEncoding(QStringConverter::Utf8);
    return ts.readAll();
}

void ThemeManager::applyTheme(Theme t) {
    const char* path = (t == Theme::Dark) ? AppPaths::kDarkQss : AppPaths::kLightQss;
    const QString qss = loadQss(QString::fromUtf8(path));
    if (auto* app = qobject_cast<QApplication*>(QCoreApplication::instance())) {
        app->setStyleSheet(qss);
    }
    theme_ = t;
    emit themeChanged(theme_);
}

void ThemeManager::toggleTheme() {
    applyTheme(theme_ == Theme::Light ? Theme::Dark : Theme::Light);
}

void ThemeManager::applyFontScale() {
    QFont f = baseFont_;
    f.setPointSizeF(baseFont_.pointSizeF() * fontScale_);
    QApplication::setFont(f);
    const auto topLevels = QApplication::topLevelWidgets();
    for (QWidget* w : topLevels) {
        w->setFont(f);
        const auto children = w->findChildren<QWidget*>();
        for (QWidget* c : children) c->setFont(f);
    }
}

void ThemeManager::setFontScale(double s) {
    s = std::clamp(s, 0.85, 1.35);
    if (qFuzzyCompare(s, fontScale_)) return;
    fontScale_ = s;
    applyFontScale();
    emit fontScaleChanged(fontScale_);
}

void ThemeManager::increaseFont() { setFontScale(fontScale_ + 0.10); }
void ThemeManager::decreaseFont() { setFontScale(fontScale_ - 0.10); }
void ThemeManager::resetFont()    { setFontScale(1.0); }

}