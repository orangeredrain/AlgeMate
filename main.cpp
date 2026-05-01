#include "ui/MainWindow.h"
#include "core/ThemeManager.h"
#include "core/AppPaths.h"

#include <QApplication>

int main(int argc, char* argv[]) {
    QApplication::setHighDpiScaleFactorRoundingPolicy(
        Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);

    QApplication app(argc, argv);
    app.setOrganizationName(QStringLiteral("AlgeMate"));
    app.setApplicationName(AlgeMate::AppPaths::appName());
    app.setApplicationDisplayName(AlgeMate::AppPaths::appDisplayName());
    app.setApplicationVersion(AlgeMate::AppPaths::appVersion());

    AlgeMate::ThemeManager::instance().applyTheme(
        AlgeMate::ThemeManager::Theme::Light);

    AlgeMate::MainWindow w;
    w.show();
    return app.exec();
}
