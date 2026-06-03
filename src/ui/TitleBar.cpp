#include "TitleBar.h"
#include "UserProfileDialog.h"
#include "core/ThemeManager.h"
#include "core/AppPaths.h"
#include "core/UserProfile.h"

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
    logo->setObjectName("AppLogo");            // 核心：给它起个名字，方便等下识别
    logo->setFixedSize(36, 36);
    logo->setAlignment(Qt::AlignCenter);
    logo->setCursor(Qt::PointingHandCursor);   // 核心：鼠标悬停时变成小手，提示可点击

    QPixmap logoPix(AppPaths::kAppIcon);
    logo->setPixmap(logoPix);
    logo->setScaledContents(true);

    logo->installEventFilter(this);            // 核心：开启事件监听

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

    auto* themeLabel = new QLabel(QStringLiteral("主题"));
    themeLabel->setStyleSheet("color:#8A8FA3; font-size:12px;");
    btnLight_ = makeIconBtn(QStringLiteral("☀"), QStringLiteral("亮色主题"));
    btnDark_  = makeIconBtn(QStringLiteral("☾"), QStringLiteral("暗色主题"));
    btnLight_->setAutoExclusive(true);
    btnDark_->setAutoExclusive(true);

    root->addWidget(themeLabel);
    root->addWidget(btnLight_);
    root->addWidget(btnDark_);

    auto* sep1 = new QFrame;
    sep1->setFrameShape(QFrame::VLine);
    sep1->setFixedHeight(24);
    root->addSpacing(4); root->addWidget(sep1); root->addSpacing(4);

    auto* fontLabel = new QLabel(QStringLiteral("字号"));
    fontLabel->setStyleSheet("color:#8A8FA3; font-size:12px;");
    btnFontDec_  = makeIconBtn(QStringLiteral("A-"), QStringLiteral("缩小字号"));
    btnFontNorm_ = makeIconBtn(QStringLiteral("A"),  QStringLiteral("标准字号"));
    btnFontInc_  = makeIconBtn(QStringLiteral("A+"), QStringLiteral("放大字号"));
    btnFontDec_->setCheckable(false);
    btnFontNorm_->setCheckable(false);
    btnFontInc_->setCheckable(false);

    root->addWidget(fontLabel);
    root->addWidget(btnFontDec_);
    root->addWidget(btnFontNorm_);
    root->addWidget(btnFontInc_);

    auto* sep2 = new QFrame;
    sep2->setFrameShape(QFrame::VLine);
    sep2->setFixedHeight(24);
    root->addSpacing(4); root->addWidget(sep2); root->addSpacing(4);

    auto* userBtn = new QToolButton;
    userBtn->setObjectName(QStringLiteral("UserButton"));
    userBtn->setCursor(Qt::PointingHandCursor);
    userBtn->setToolTip(QStringLiteral("点击设置个人资料"));
    userBtn->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    userBtn->setIconSize(QSize(32, 32));
    userBtn->setAutoRaise(true);
    userBtn->setStyleSheet(
        "QToolButton#UserButton { background:transparent; border:1px solid transparent; "
        "border-radius:18px; padding:2px 12px 2px 3px; font-size:13px; font-weight:600; }"
        "QToolButton#UserButton:hover { background:#F1EEFF; border-color:#D7CCFF; }");

    btnUser_ = userBtn;
    avatarLabel_ = nullptr;
    nameLabel_   = nullptr;
    root->addWidget(btnUser_);

    connect(btnLight_, &QPushButton::clicked, this, &TitleBar::onLightClicked);
    connect(btnDark_,  &QPushButton::clicked, this, &TitleBar::onDarkClicked);
    connect(btnFontDec_,  &QPushButton::clicked, []{ ThemeManager::instance().decreaseFont(); });
    connect(btnFontNorm_, &QPushButton::clicked, []{ ThemeManager::instance().resetFont(); });
    connect(btnFontInc_,  &QPushButton::clicked, []{ ThemeManager::instance().increaseFont(); });
    connect(btnUser_, &QToolButton::clicked, this, &TitleBar::onUserClicked);

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

void TitleBar::onLightClicked() { ThemeManager::instance().applyTheme(ThemeManager::Theme::Light); }
void TitleBar::onDarkClicked()  { ThemeManager::instance().applyTheme(ThemeManager::Theme::Dark); }

void TitleBar::syncThemeButtons() {
    const bool dark = ThemeManager::instance().currentTheme() == ThemeManager::Theme::Dark;
    btnLight_->setChecked(!dark);
    btnDark_->setChecked(dark);
}

bool TitleBar::eventFilter(QObject* obj, QEvent* e) {
    // 如果被点击的是我们的 Logo
    if (obj->objectName() == "AppLogo" && e->type() == QEvent::MouseButtonRelease) {
        auto* me = static_cast<QMouseEvent*>(e);
        if (me->button() == Qt::LeftButton) {

            // 创建一个弹窗来显示大图
            QDialog dlg(this);
            dlg.setWindowTitle(QStringLiteral("应用图标"));
            // 隐藏 Windows 弹窗自带的问号按钮
            dlg.setWindowFlags(dlg.windowFlags() & ~Qt::WindowContextHelpButtonHint);
            dlg.setMinimumSize(200, 200);

            auto* lay = new QVBoxLayout(&dlg);
            lay->setContentsMargins(20, 20, 20, 20);

            auto* imgLabel = new QLabel;
            imgLabel->setAlignment(Qt::AlignCenter);

            // 加载原比例的大图
            QPixmap bigPix(AppPaths::kAppIcon);
            // 如果图片分辨率太大，限制一下最大宽高（比如最大 400x400）防止撑满屏幕
            if (bigPix.width() > 400 || bigPix.height() > 400) {
                bigPix = bigPix.scaled(400, 400, Qt::KeepAspectRatio, Qt::SmoothTransformation);
            }
            imgLabel->setPixmap(bigPix);

            lay->addWidget(imgLabel);

            // 展现弹窗（阻塞式，关掉后才能点别的）
            dlg.exec();

            return true; // 事件已处理
        }
    }
    // 其他事件交给父类处理
    return QWidget::eventFilter(obj, e);
}

}
