#include "ui/MainWindow.h"
#include "core/ThemeManager.h"
#include "core/AppPaths.h"

#include <QApplication>
#include <QIcon>

int main(int argc, char* argv[]) {
    QApplication::setHighDpiScaleFactorRoundingPolicy(
        Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);

    QApplication app(argc, argv);
    QFont font;
    font.setFamily(QStringLiteral("Microsoft YaHei"));
    font.setPointSize(10);
    app.setFont(font);
    app.setOrganizationName(QStringLiteral("AlgeMate"));
    app.setApplicationName(AlgeMate::AppPaths::appName());
    app.setApplicationDisplayName(AlgeMate::AppPaths::appDisplayName());
    app.setApplicationVersion(AlgeMate::AppPaths::appVersion());

    app.setWindowIcon(QIcon(AlgeMate::AppPaths::kAppIcon));

    AlgeMate::ThemeManager::instance().applyTheme(
        AlgeMate::ThemeManager::Theme::Light);

    app.setWindowIcon(QIcon(":/icon.ico"));

    AlgeMate::MainWindow w;
    w.show();
    return app.exec();
}
