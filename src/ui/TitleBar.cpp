#include "TitleBar.h"
#include "UserProfileDialog.h"
#include "core/ThemeManager.h"
#include "core/AppPaths.h"
#include "core/UserProfile.h"
#include "TomatoClockDialog.h"
#include "TomatoManager.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QToolButton>
#include <QIcon>
#include <QFrame>
#include <QPixmap>
#include <QEvent>
#include <QMouseEvent>
#include <QDialog>
#include <QVBoxLayout>

namespace AlgeMate {

TitleBar::TitleBar(QWidget* parent) : QWidget(parent) {
    setObjectName(QStringLiteral("TitleBar"));
    setFixedHeight(60);
    setAttribute(Qt::WA_StyledBackground, true);
    buildUi();
    syncThemeButtons();

    connect(&ThemeManager::instance(), &ThemeManager::themeChanged,
            this, [this](ThemeManager::Theme){ syncThemeButtons(); });
}

static QPushButton* makeIconBtn(const QString& text, const QString& tip) {
    auto* b = new QPushButton(text);
    b->setProperty("iconBtn", true);
    b->setCursor(Qt::PointingHandCursor);
    b->setToolTip(tip);
    b->setCheckable(true);

    return b;
}

void TitleBar::buildUi() {
    auto* root = new QHBoxLayout(this);
    root->setContentsMargins(20, 8, 20, 8);
    root->setSpacing(14);

    auto* logo = new QLabel;
    logo->setObjectName("AppLogo");
    logo->setFixedSize(36, 36);
    logo->setAlignment(Qt::AlignCenter);
    logo->setCursor(Qt::PointingHandCursor);

    QPixmap logoPix(AppPaths::kAppIcon);
    logo->setPixmap(logoPix);
    logo->setScaledContents(true);

    logo->installEventFilter(this);

    auto* titleBox = new QVBoxLayout;
    titleBox->setSpacing(0);
    auto* title = new QLabel(AppPaths::appDisplayName());
    title->setObjectName(QStringLiteral("AppTitle"));
    auto* subtitle = new QLabel(AppPaths::appSubtitle());
    subtitle->setObjectName(QStringLiteral("AppSubtitle"));
    titleBox->addWidget(title);
    titleBox->addWidget(subtitle);

    auto* version = new QLabel(AppPaths::appVersion());
    version->setObjectName(QStringLiteral("AppVersion"));

    root->addWidget(logo);
    root->addLayout(titleBox);
    root->addWidget(version);
    root->addStretch();

    // 主题切换部分
    auto* themeLabel = new QLabel(QStringLiteral("主题"));
    themeLabel->setStyleSheet("color:#8A8FA3; font-size:12px;");
    themeLabel->setVisible(false);
    btnLight_ = makeIconBtn(QStringLiteral("☀"), QStringLiteral("亮色主题"));
    btnDark_  = makeIconBtn(QStringLiteral("☾"), QStringLiteral("暗色主题"));
    btnLight_->setAutoExclusive(true);
    btnDark_->setAutoExclusive(true);
    btnLight_->setVisible(false);
    btnDark_->setVisible(false);

    root->addWidget(themeLabel);
    root->addWidget(btnLight_);
    root->addWidget(btnDark_);

    auto* sep1 = new QFrame;
    sep1->setFrameShape(QFrame::VLine);
    sep1->setFixedHeight(24);
    sep1->setVisible(false);
    root->addSpacing(4); root->addWidget(sep1); root->addSpacing(4);

    btnTomato_ = new QToolButton;
    btnTomato_->setObjectName(QStringLiteral("TomatoButton"));
    btnTomato_->setCursor(Qt::PointingHandCursor);
    btnTomato_->setToolTip(QStringLiteral("开启番茄钟"));
    btnTomato_->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    btnTomato_->setText(QStringLiteral(" 🍅 番茄钟"));
    btnTomato_->setAutoRaise(true);
    // 复用你的精美圆角样式，并微调
    // btnTomato_->setStyleSheet(
    //     "QToolButton#TomatoButton { background:transparent; border:1px solid transparent; "
    //     "border-radius:14px; padding:4px 10px; font-size:13px; font-weight:600; color:#E05A47; }"
    //     "QToolButton#TomatoButton:hover { background:#FFEBE9; border-color:#FFC4C0; }");

    root->addWidget(btnTomato_);

    auto* sep2 = new QFrame;
    sep2->setFrameShape(QFrame::VLine);
    sep2->setFixedHeight(24);
    root->addSpacing(4); root->addWidget(sep2); root->addSpacing(4);

    // 用户按钮部分
    auto* userBtn = new QToolButton;
    userBtn->setObjectName(QStringLiteral("UserButton"));
    userBtn->setCursor(Qt::PointingHandCursor);
    userBtn->setToolTip(QStringLiteral("点击设置个人资料"));
    userBtn->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    userBtn->setIconSize(QSize(32, 32));
    userBtn->setAutoRaise(true);
    // userBtn->setStyleSheet(
    //     "QToolButton#UserButton { background:transparent; border:1px solid transparent; "
    //     "border-radius:18px; padding:2px 12px 2px 3px; font-size:13px; font-weight:600; }"
    //     "QToolButton#UserButton:hover { background:#F1EEFF; border-color:#D7CCFF; }");

    btnUser_ = userBtn;
    avatarLabel_ = nullptr;
    nameLabel_   = nullptr;
    root->addWidget(btnUser_);

    connect(btnLight_, &QPushButton::clicked, this, &TitleBar::onLightClicked);
    connect(btnDark_,  &QPushButton::clicked, this, &TitleBar::onDarkClicked);
    connect(btnUser_, &QToolButton::clicked, this, &TitleBar::onUserClicked);
    connect(btnTomato_, &QToolButton::clicked, this, &TitleBar::onTomatoClicked);

    connect(&TomatoManager::instance(), &TomatoManager::tick, this, [this](int seconds, const QString& timeStr) {
        auto& manager = TomatoManager::instance();
        if (manager.currentState() != TomatoManager::State::Idle) {
            QString statusText;
            switch (manager.currentState()) {
            case TomatoManager::State::Focus:
                statusText = QStringLiteral("专注中");
                break;
            case TomatoManager::State::ShortBreak:
                statusText = QStringLiteral("短休中");
                break;
            case TomatoManager::State::LongBreak:
                statusText = QStringLiteral("长休中");
                break;
            default:
                break;
            }
            btnTomato_->setText(QString(" 🍅 %1 %2").arg(statusText, timeStr));
        }
    });

    connect(&TomatoManager::instance(), &TomatoManager::statusChanged, this, [this](TomatoManager::State state) {
        if (state == TomatoManager::State::Idle) {
            btnTomato_->setText(QStringLiteral(" 🍅 番茄钟"));
        }
    });

    refreshUser();
    connect(&UserProfile::instance(), &UserProfile::profileChanged,
            this, &TitleBar::refreshUser);
}

void TitleBar::refreshUser() {
    if (auto* tb = qobject_cast<QToolButton*>(btnUser_)) {
        tb->setIcon(QIcon(UserProfile::instance().avatarPixmap(32)));
        tb->setText(QStringLiteral("  ") + UserProfile::instance().userName());
    }
}

void TitleBar::onUserClicked() {
    UserProfileDialog dlg(this);
    dlg.exec();
}


void TitleBar::onTomatoClicked() {
    auto& manager = TomatoManager::instance();

    // 如果当前番茄钟是闲置状态，点击标题栏按钮自动拉起并直接开始专注
    if (manager.currentState() == TomatoManager::State::Idle) {
        manager.startFocus();
    }

    // 弹出番茄钟对话框，它会通过信号槽自动同步全局 Manager 的状态和倒计时
    TomatoClockDialog dlg(this);
    dlg.exec();
}

void TitleBar::onLightClicked() { ThemeManager::instance().applyTheme(ThemeManager::Theme::Light); }
void TitleBar::onDarkClicked()  { ThemeManager::instance().applyTheme(ThemeManager::Theme::Dark); }

void TitleBar::syncThemeButtons() {
    const bool dark = ThemeManager::instance().currentTheme() == ThemeManager::Theme::Dark;
    btnLight_->setChecked(!dark);
    btnDark_->setChecked(dark);

    // 根据主题动态更新番茄钟按钮和用户按钮的样式
    if (dark) {
        // 暗色模式：采用深色背景+暗红边框，点击时颜色加深或变亮作为反馈
        btnTomato_->setStyleSheet(
            "QToolButton#TomatoButton { background:transparent; border:1px solid transparent; "
            "border-radius:14px; padding:4px 10px; font-size:13px; font-weight:600; color:#FC6E5B; }"
            "QToolButton#TomatoButton:hover { background:#3A1E1C; border-color:#5E2F2C; }"
            "QToolButton#TomatoButton:pressed { background:#4F2724; border-color:#753935; }"
            );

        btnUser_->setStyleSheet(
            "QToolButton#UserButton { background:transparent; border:1px solid transparent; color:#D9D9D9; "
            "border-radius:18px; padding:2px 12px 2px 3px; font-size:13px; font-weight:600; }"
            "QToolButton#UserButton:hover { background:#2A263D; border-color:#433C63; }"
            "QToolButton#UserButton:pressed { background:#35304C; border-color:#534A7A; }"
            );
    } else {
        // 亮色模式：保留你原本精美的样式，并增加点击时的 :pressed 状态反馈
        btnTomato_->setStyleSheet(
            "QToolButton#TomatoButton { background:transparent; border:1px solid transparent; "
            "border-radius:14px; padding:4px 10px; font-size:13px; font-weight:600; color:#E05A47; }"
            "QToolButton#TomatoButton:hover { background:#FFEBE9; border-color:#FFC4C0; }"
            "QToolButton#TomatoButton:pressed { background:#FFD6D1; border-color:#FFB1A8; }"
            );

        btnUser_->setStyleSheet(
            "QToolButton#UserButton { background:transparent; border:1px solid transparent; color:#333333; "
            "border-radius:18px; padding:2px 12px 2px 3px; font-size:13px; font-weight:600; }"
            "QToolButton#UserButton:hover { background:#F1EEFF; border-color:#D7CCFF; }"
            "QToolButton#UserButton:pressed { background:#E2DAFF; border-color:#C1AEFF; }"
            );
    }
}

bool TitleBar::eventFilter(QObject* obj, QEvent* e) {
    if (obj->objectName() == "AppLogo" && e->type() == QEvent::MouseButtonRelease) {
        auto* me = static_cast<QMouseEvent*>(e);
        if (me->button() == Qt::LeftButton) {
            QDialog dlg(this);
            dlg.setWindowTitle(QStringLiteral("应用图标"));
            dlg.setWindowFlags(dlg.windowFlags() & ~Qt::WindowContextHelpButtonHint);
            dlg.setMinimumSize(200, 200);

            auto* lay = new QVBoxLayout(&dlg);
            lay->setContentsMargins(20, 20, 20, 20);

            auto* imgLabel = new QLabel;
            imgLabel->setAlignment(Qt::AlignCenter);

            QPixmap bigPix(AppPaths::kAppIcon);
            if (bigPix.width() > 400 || bigPix.height() > 400) {
                bigPix = bigPix.scaled(400, 400, Qt::KeepAspectRatio, Qt::SmoothTransformation);
            }
            imgLabel->setPixmap(bigPix);

            lay->addWidget(imgLabel);
            dlg.exec();
            return true;
        }
    }
    return QWidget::eventFilter(obj, e);
}

} // namespace AlgeMate