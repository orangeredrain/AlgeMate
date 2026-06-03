#include "ui/MainWindow.h"
#include "core/ThemeManager.h"
#include "core/AppPaths.h"

#include <QApplication>
#include <QIcon>
#include <QSurfaceFormat> // 引入 QSurfaceFormat

int main(int argc, char* argv[]) {
#ifdef Q_OS_MAC
    QSurfaceFormat format;
    format.setVersion(3, 3);
    format.setProfile(QSurfaceFormat::CoreProfile);
    format.setDepthBufferSize(24);
    format.setStencilBufferSize(8);
    QSurfaceFormat::setDefaultFormat(format);
#endif

    QApplication::setHighDpiScaleFactorRoundingPolicy(
        Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);

    QApplication app(argc, argv);

    // Mac 适配字体：避免 Mac 上缺失微软雅黑导致排版错乱
    QFont font;
#ifdef Q_OS_MAC
    font.setFamily(QStringLiteral(".AppleSystemUIFont"));
#else
    font.setFamily(QStringLiteral("Microsoft YaHei"));
#endif
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