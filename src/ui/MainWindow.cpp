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

#include <QShortcut>
#include <QSettings>

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

    // 把主页、学习中心和设置页面单独提出来，方便后面绑定快捷键和信号
    auto* home = new Home::HomePage(this);
    auto* learningPage = new Learning::LearningPage(this);
    auto* settingsPage = new Settings::SettingsPage(this); // <-- 单独拿出设置页

    const QList<Item> items{
        { QStringLiteral("🏠"), QStringLiteral("首页"),         home },
        { QStringLiteral("🧮"), QStringLiteral("计算助手"),     new Calculator::CalculatorPage(this) },
        { QStringLiteral("🤖"), QStringLiteral("AI 智能解题"), new AiSolver::AiSolverPage(this) },
        { QStringLiteral("📈"), QStringLiteral("学习中心"),     learningPage },
        { QStringLiteral("🧪"), QStringLiteral("测试中心"),     new TestCenter::TestCenterPage(this) },
        { QStringLiteral("⚙"),  QStringLiteral("设置中心"),     settingsPage }, // <-- 用刚刚创建的变量
    };

    for (const auto& it : items) {
        nav_->addNavItem(it.icon, it.title);
        stack_->addWidget(it.page);
    }
    nav_->setCurrentIndex(0);
    stack_->setCurrentIndex(0);
    learningPage_ = learningPage;

    // --- 页面原有跳转逻辑 ---
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

    connect(learningPage, &Learning::LearningPage::requestNavigateToHomeGoalDetail,
            this, [this, home]() {
                nav_->setCurrentIndex(0);
                stack_->setCurrentIndex(0);
                home->showGoalDetail();
            });


    // ================== 新增：全局快捷键逻辑 ==================

    auto* shortcutAi = new QShortcut(this);
    auto* shortcutWrongBook = new QShortcut(this);
    auto* shortcutUnfinished = new QShortcut(this);

    // 1. 快捷键：唤醒 AI 助手 (索引 2)
    connect(shortcutAi, &QShortcut::activated, this, [this]() {
        nav_->setCurrentIndex(2);
        stack_->setCurrentIndex(2);
    });

    // 2. 快捷键：错题本 (跳转到学习中心，索引 3)
    connect(shortcutWrongBook, &QShortcut::activated, this, [this]() {
        nav_->setCurrentIndex(3);
        stack_->setCurrentIndex(3);
        if (learningPage_) {
            learningPage_->showWrongBook();
        }
    });

    // 3. 快捷键：未完成的目标 (跳转到首页并展开目标列表)
    connect(shortcutUnfinished, &QShortcut::activated, this, [this, home]() {
        nav_->setCurrentIndex(0);
        stack_->setCurrentIndex(0);
        home->showGoalDetail(); // 完美复用你已有的展现目标方法
    });

    // 定义一个刷新快捷键的 Lambda 函数
    auto reloadShortcuts = [=]() {
        QSettings settings("AlgeMate", "AlgeMateApp");
        shortcutAi->setKey(QKeySequence::fromString(settings.value("Shortcuts/AiChat").toString()));
        shortcutWrongBook->setKey(QKeySequence::fromString(settings.value("Shortcuts/WrongBook").toString()));
        shortcutUnfinished->setKey(QKeySequence::fromString(settings.value("Shortcuts/Unfinished").toString()));
    };

    // 软件启动时加载一次快捷键
    reloadShortcuts();

    // 监听设置页面发出的保存信号，动态刷新快捷键
    connect(settingsPage, &Settings::SettingsPage::shortcutsChanged, this, reloadShortcuts);

    // ========================================================
}

}
