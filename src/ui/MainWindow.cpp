#include "MainWindow.h"
#include "ui_MainWindow.h"

#include "TitleBar.h"
#include "NavigationBar.h"
#include "core/AppPaths.h"

#include "modules/home/HomePage.h"
#include "modules/calculator/CalculatorPage.h"
#include "modules/ai_solver/AiSolverPage.h"
#include "modules/knowledge/KnowledgePage.h"
#include "modules/learning/LearningPage.h"
#include "modules/settings/SettingsPage.h"

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QStackedWidget>
#include <QStatusBar>

namespace AlgeMate {

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent), ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    setWindowTitle(AppPaths::appDisplayName() + QStringLiteral("：") + AppPaths::appSubtitle());
    resize(1280, 800);
    setMinimumSize(1080, 680);

    composeLayout();
    registerModules();

    statusBar()->showMessage(QStringLiteral("就绪 · ") + AppPaths::appVersion());
}

MainWindow::~MainWindow() { delete ui; }

void MainWindow::composeLayout() {
    titleBar_ = new TitleBar(this);
    setMenuWidget(titleBar_);

    auto* central = ui->centralwidget;
    auto* root = new QHBoxLayout(central);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    nav_   = new NavigationBar(central);
    stack_ = new QStackedWidget(central);
    stack_->setContentsMargins(0, 0, 0, 0);

    root->addWidget(nav_);
    root->addWidget(stack_, 1);

    connect(nav_, &NavigationBar::navigated,
            stack_, &QStackedWidget::setCurrentIndex);
}

void MainWindow::registerModules() {
    struct Item { QString icon; QString title; QWidget* page; };
    auto* home = new Home::HomePage(this);
    const QList<Item> items{
        { QStringLiteral("🏠"), QStringLiteral("首页"),         home },
        { QStringLiteral("🧮"), QStringLiteral("计算助手"),     new Calculator::CalculatorPage(this) },
        { QStringLiteral("🤖"), QStringLiteral("AI 智能解题"), new AiSolver::AiSolverPage(this) },
        { QStringLiteral("📘"), QStringLiteral("知识点学习"),   new Knowledge::KnowledgePage(this) },
        { QStringLiteral("📈"), QStringLiteral("学习中心"),     new Learning::LearningPage(this) },
        { QStringLiteral("⚙"),  QStringLiteral("设置中心"),     new Settings::SettingsPage(this) },
    };

    for (const auto& it : items) {
        nav_->addNavItem(it.icon, it.title);
        stack_->addWidget(it.page);
    }
    nav_->setCurrentIndex(0);
    stack_->setCurrentIndex(0);

    connect(home, &Home::HomePage::requestNavigate,
            this, [this](int target) {
                const int index = target + 1;
                nav_->setCurrentIndex(index);
                stack_->setCurrentIndex(index);
            });
}

}
