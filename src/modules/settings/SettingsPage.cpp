#include "SettingsPage.h"
#include "core/ThemeManager.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QFrame>
#include <QPushButton>

namespace AlgeMate::Settings {

SettingsPage::SettingsPage(QWidget* parent) : QWidget(parent) {
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(24, 24, 24, 24);
    root->setSpacing(16);

    auto* title = new QLabel(QStringLiteral("设置中心"));
    title->setObjectName(QStringLiteral("PageTitle"));
    auto* subtitle = new QLabel(QStringLiteral("个性化外观 · 账号 · API · 快捷键"));
    subtitle->setObjectName(QStringLiteral("PageSubtitle"));

    auto* themeCard = new QFrame;
    themeCard->setObjectName(QStringLiteral("Card"));
    {
        auto* lay = new QVBoxLayout(themeCard);
        lay->setContentsMargins(24, 20, 24, 20);
        lay->setSpacing(12);

        auto* head = new QLabel(QStringLiteral("外观"));
        head->setStyleSheet("font-size:16px; font-weight:700;");

        auto* row = new QHBoxLayout;
        row->setSpacing(12);
        auto* lbl = new QLabel(QStringLiteral("主题模式："));

        auto* btnLight = new QPushButton(QStringLiteral("☀  亮色"));
        auto* btnDark  = new QPushButton(QStringLiteral("☾  暗色"));
        btnLight->setCheckable(true);
        btnDark->setCheckable(true);
        btnLight->setAutoExclusive(true);
        btnDark->setAutoExclusive(true);
        btnLight->setCursor(Qt::PointingHandCursor);
        btnDark->setCursor(Qt::PointingHandCursor);

        auto sync = [=]() {
            bool dark = ThemeManager::instance().currentTheme() == ThemeManager::Theme::Dark;
            btnLight->setChecked(!dark);
            btnDark->setChecked(dark);
        };
        sync();

        connect(btnLight, &QPushButton::clicked, []{
            ThemeManager::instance().applyTheme(ThemeManager::Theme::Light);
        });
        connect(btnDark, &QPushButton::clicked, []{
            ThemeManager::instance().applyTheme(ThemeManager::Theme::Dark);
        });
        connect(&ThemeManager::instance(), &ThemeManager::themeChanged,
                this, [sync](ThemeManager::Theme) { sync(); });

        row->addWidget(lbl);
        row->addWidget(btnLight);
        row->addWidget(btnDark);
        row->addStretch();

        lay->addWidget(head);
        lay->addLayout(row);
    }

    auto* card = new QFrame;
    card->setObjectName(QStringLiteral("Card"));
    auto* cardLayout = new QVBoxLayout(card);
    cardLayout->setContentsMargins(32, 48, 32, 48);
    auto* placeholder = new QLabel(
        QStringLiteral("设置中心模块\n\n"
                       "规划：QSettings 持久化 / 账号登录 / AI API Key / 快捷键自定义"));
    placeholder->setObjectName(QStringLiteral("PlaceholderLabel"));
    placeholder->setAlignment(Qt::AlignCenter);
    placeholder->setWordWrap(true);
    cardLayout->addWidget(placeholder);

    root->addWidget(title);
    root->addWidget(subtitle);
    root->addWidget(themeCard);
    root->addWidget(card, 1);
}

}
