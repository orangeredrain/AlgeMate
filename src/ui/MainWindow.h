#ifndef ALGEMATE_MAINWINDOW_H
#define ALGEMATE_MAINWINDOW_H

#include <QMainWindow>

class QStackedWidget;

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

namespace AlgeMate::Learning { class LearningPage; }

namespace AlgeMate {

class TitleBar;
class NavigationBar;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;

private:
    void composeLayout();
    void registerModules();

    Ui::MainWindow* ui = nullptr;
    TitleBar*       titleBar_ = nullptr;
    NavigationBar*  nav_      = nullptr;
    QStackedWidget* stack_    = nullptr;
    Learning::LearningPage* learningPage_ = nullptr;
};

}

#endif
