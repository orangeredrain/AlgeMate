#include "MainWindow.h"
#include "ui_MainWindow.h"

#include "TitleBar.h"
#include "NavigationBar.h"
#include "core/AppPaths.h"

#include "modules/home/HomePage.h"
#include "modules/calculator/CalculatorPage.h"
#include "modules/ai_solver/AiSolverPage.h"
#include "modules/learning/LearningPage.h"
#include "modules/settings/SettingsPage.h"
#include "modules/test_center/TestCenterPage.h"

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
    auto* learningPage = new Learning::LearningPage(this);
    const QList<Item> items{
        { QStringLiteral("🏠"), QStringLiteral("首页"),         home },
        { QStringLiteral("🧮"), QStringLiteral("计算助手"),     new Calculator::CalculatorPage(this) },
        { QStringLiteral("🤖"), QStringLiteral("AI 智能解题"), new AiSolver::AiSolverPage(this) },
        { QStringLiteral("📈"), QStringLiteral("学习中心"),     learningPage },
        { QStringLiteral("🧪"), QStringLiteral("测试中心"),     new TestCenter::TestCenterPage(this) },
        { QStringLiteral("⚙"),  QStringLiteral("设置中心"),     new Settings::SettingsPage(this) },
    };

    for (const auto& it : items) {
        nav_->addNavItem(it.icon, it.title);
        stack_->addWidget(it.page);
    }
    nav_->setCurrentIndex(0);
    stack_->setCurrentIndex(0);
    learningPage_ = learningPage;

    connect(home, &Home::HomePage::requestNavigate,
            this, [this](int target) {
                int index = 0;
                switch (target) {
                case Home::HomePage::Calculator: index = 1; break;
                case Home::HomePage::AiSolver:   index = 2; break;
                case Home::HomePage::Knowledge:  index = 3; break;
                case Home::HomePage::Learning:   index = 3; break;
                case Home::HomePage::Settings:   index = 5; break;
                default: return;
                }
                nav_->setCurrentIndex(index);
                stack_->setCurrentIndex(index);
                if (target == Home::HomePage::Knowledge)
                    learningPage_->showKnowledge();
            });
}

}
