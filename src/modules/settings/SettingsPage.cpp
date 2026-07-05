#include "SettingsPage.h"
#include "core/ThemeManager.h"
#include "core/UserProfile.h"
#include "ui/UserProfileDialog.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QLabel>
#include <QFrame>
#include <QPushButton>
#include <QScrollArea>
#include <QLineEdit>
#include <QKeySequenceEdit>
#include <QMessageBox>

namespace AlgeMate::Settings {

// 定义 QSettings 的组织名和应用名
const QString ORG_NAME = QStringLiteral("AlgeMate");
const QString APP_NAME = QStringLiteral("AlgeMateApp");

SettingsPage::SettingsPage(QWidget* parent) : QWidget(parent) {
    setupUI();
    loadSettings();

    // 初始化加载时同步一次当前全局账号状态
    refreshUserStatus();

    // 核心：监听全局账号变化信号（保证 TitleBar 和 设置中心 的状态永远一致）
    connect(&UserProfile::instance(), &UserProfile::profileChanged,
            this, &SettingsPage::refreshUserStatus);
}

void SettingsPage::setupUI() {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);

    // 1. 顶部标题区域
    auto* headerWidget = new QWidget;
    auto* headerLayout = new QVBoxLayout(headerWidget);
    headerLayout->setContentsMargins(24, 24, 24, 0);

    auto* title = new QLabel(QStringLiteral("设置中心"));
    title->setObjectName(QStringLiteral("PageTitle"));
    auto* subtitle = new QLabel(QStringLiteral("个性化外观 · 个人资料 · API · 快捷键"));
    subtitle->setObjectName(QStringLiteral("PageSubtitle"));

    headerLayout->addWidget(title);
    headerLayout->addWidget(subtitle);
    mainLayout->addWidget(headerWidget);

    // 2. 滚动区域
    auto* scrollArea = new QScrollArea;
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setStyleSheet("QScrollArea { background: transparent; }");

    auto* scrollContent = new QWidget;
    auto* contentLayout = new QVBoxLayout(scrollContent);
    contentLayout->setContentsMargins(24, 16, 24, 24);
    contentLayout->setSpacing(16);

    // 3. 添加各个设置模块卡片
    // contentLayout->addWidget(createThemeCard());
    contentLayout->addWidget(createAccountCard());
    contentLayout->addWidget(createApiCard());
    contentLayout->addWidget(createShortcutCard());
    contentLayout->addStretch(1);

    scrollArea->setWidget(scrollContent);
    mainLayout->addWidget(scrollArea);
}

QWidget* SettingsPage::createThemeCard() {
    auto* themeCard = new QFrame;
    themeCard->setObjectName(QStringLiteral("Card"));
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

    // ================== 【新增】设置专属的高亮样式 ==================
    // 亮色模式按钮选中：白底 + 亮黄字
    QString lightStyle =
        "QPushButton { padding: 6px 16px; border-radius: 6px; border: 1px solid #8A8FA3; color: #8A8FA3; background: transparent; font-weight: bold; }"
        "QPushButton:hover { background: rgba(138, 143, 163, 0.15); }"
        "QPushButton:checked { background: #FFFFFF; color: #F59E0B; border: 1px solid #F59E0B; }";

    // 暗色模式按钮选中：黑底 + 亮紫/蓝字
    QString darkStyle =
        "QPushButton { padding: 6px 16px; border-radius: 6px; border: 1px solid #8A8FA3; color: #8A8FA3; background: transparent; font-weight: bold; }"
        "QPushButton:hover { background: rgba(138, 143, 163, 0.15); }"
        "QPushButton:checked { background: #1C1B2E; color: #8FA1FF; border: 1px solid #1C1B2E; }";

    btnLight->setStyleSheet(lightStyle);
    btnDark->setStyleSheet(darkStyle);
    // =============================================================

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

    return themeCard;
}

QWidget* SettingsPage::createAccountCard() {
    auto* card = new QFrame;
    card->setObjectName(QStringLiteral("Card"));
    auto* lay = new QVBoxLayout(card);
    lay->setContentsMargins(24, 20, 24, 20);
    lay->setSpacing(12);

    auto* head = new QLabel(QStringLiteral("个人资料"));
    head->setStyleSheet("font-size:16px; font-weight:700;");

    auto* row = new QHBoxLayout;
    m_accountStatusLabel = new QLabel(QStringLiteral("读取状态中..."));

    m_loginButton = new QPushButton(QStringLiteral("个人资料"));
    m_loginButton->setCursor(Qt::PointingHandCursor);
    m_loginButton->setStyleSheet("QPushButton { padding: 6px 16px; }");

    // 点击按钮直接唤起弹窗
    connect(m_loginButton, &QPushButton::clicked, this, &SettingsPage::showProfileDialog);

    row->addWidget(m_accountStatusLabel);
    row->addWidget(m_loginButton);
    row->addStretch();

    lay->addWidget(head);
    lay->addLayout(row);

    return card;
}

QWidget* SettingsPage::createApiCard() {
    auto* card = new QFrame;
    card->setObjectName(QStringLiteral("Card"));
    auto* lay = new QVBoxLayout(card);
    lay->setContentsMargins(24, 20, 24, 20);
    lay->setSpacing(12);

    auto* head = new QLabel(QStringLiteral("AI 服务配置 (API Keys)"));
    head->setStyleSheet("font-size:16px; font-weight:700;");

    // DeepSeek 输入行
    auto* dsRow = new QHBoxLayout;
    m_apiKeyEdit = new QLineEdit;
    m_apiKeyEdit->setPlaceholderText(QStringLiteral("sk-..."));
    m_apiKeyEdit->setEchoMode(QLineEdit::PasswordEchoOnEdit);
    dsRow->addWidget(new QLabel(QStringLiteral("DeepSeek 密钥：")));
    dsRow->addWidget(m_apiKeyEdit, 1);

    // 豆包 输入行
    auto* dbRow = new QHBoxLayout;
    m_doubaoApiKeyEdit = new QLineEdit;
    m_doubaoApiKeyEdit->setPlaceholderText(QStringLiteral("用于图片 OCR 识别..."));
    m_doubaoApiKeyEdit->setEchoMode(QLineEdit::PasswordEchoOnEdit);
    dbRow->addWidget(new QLabel(QStringLiteral("豆包 OCR 密钥：")));
    dbRow->addWidget(m_doubaoApiKeyEdit, 1);

    auto* saveBtn = new QPushButton(QStringLiteral("保存全部密钥"));
    saveBtn->setCursor(Qt::PointingHandCursor);
    connect(saveBtn, &QPushButton::clicked, this, &SettingsPage::saveApiKey);

    lay->addWidget(head);
    lay->addLayout(dsRow);
    lay->addLayout(dbRow);
    lay->addWidget(saveBtn, 0, Qt::AlignRight);

    return card;
}

QWidget* SettingsPage::createShortcutCard() {
    auto* card = new QFrame;
    card->setObjectName(QStringLiteral("Card"));
    auto* lay = new QVBoxLayout(card);
    lay->setContentsMargins(24, 20, 24, 20);
    lay->setSpacing(12);
    auto* head = new QLabel(QStringLiteral("快捷键设置"));
    head->setStyleSheet("font-size:16px; font-weight:700;");
    auto* formLayout = new QFormLayout;
    formLayout->setHorizontalSpacing(24);
    m_shortcutAiChat = new QKeySequenceEdit;
    m_shortcutWrongBook = new QKeySequenceEdit;
    m_shortcutUnfinished = new QKeySequenceEdit;
    // 提示信息，方便用户知道作用
    m_shortcutAiChat->setToolTip("按下快捷键跳转到 AI智能解题 界面");
    m_shortcutWrongBook->setToolTip("按下快捷键跳转到 错题本 界面");
    m_shortcutUnfinished->setToolTip("按下快捷键跳转到 未完成的目标 界面");
    formLayout->addRow(QStringLiteral("唤醒 AI 助手："), m_shortcutAiChat);
    formLayout->addRow(QStringLiteral("错题本："), m_shortcutWrongBook);
    formLayout->addRow(QStringLiteral("未完成的目标："), m_shortcutUnfinished);
    // 增加：专门的保存按钮
    auto* saveBtn = new QPushButton(QStringLiteral("保存快捷键"));
    saveBtn->setCursor(Qt::PointingHandCursor);
    connect(saveBtn, &QPushButton::clicked, this, &SettingsPage::saveShortcuts);
    lay->addWidget(head);
    lay->addLayout(formLayout);
    lay->addWidget(saveBtn, 0, Qt::AlignRight); // 放在右下角
    return card;
}

void SettingsPage::loadSettings() {
    QSettings settings(ORG_NAME, APP_NAME);
    m_apiKeyEdit->setText(settings.value("AI/DeepSeekApiKey", "").toString());
    m_doubaoApiKeyEdit->setText(settings.value("AI/DoubaoApiKey", "").toString());
    // 加载快捷键（如果没设置过，可以给个默认值，比如空字符串）
    m_shortcutAiChat->setKeySequence(QKeySequence::fromString(settings.value("Shortcuts/AiChat", "").toString()));
    m_shortcutWrongBook->setKeySequence(QKeySequence::fromString(settings.value("Shortcuts/WrongBook", "").toString()));
    m_shortcutUnfinished->setKeySequence(QKeySequence::fromString(settings.value("Shortcuts/Unfinished", "").toString()));
}

void SettingsPage::saveApiKey() {
    QSettings settings(ORG_NAME, APP_NAME);
    settings.setValue("AI/DeepSeekApiKey", m_apiKeyEdit->text().trimmed());
    settings.setValue("AI/DoubaoApiKey", m_doubaoApiKeyEdit->text().trimmed());
    settings.sync();

    QMessageBox::information(this, QStringLiteral("成功"), QStringLiteral("API Keys 已保存！\nAI 助手将自动应用新配置。"));
}

void SettingsPage::saveShortcuts() {
    QSettings settings(ORG_NAME, APP_NAME);
    settings.setValue("Shortcuts/AiChat", m_shortcutAiChat->keySequence().toString());
    settings.setValue("Shortcuts/WrongBook", m_shortcutWrongBook->keySequence().toString());
    settings.setValue("Shortcuts/Unfinished", m_shortcutUnfinished->keySequence().toString());
    settings.sync();
    QMessageBox::information(this, QStringLiteral("成功"), QStringLiteral("快捷键已保存！"));

    // 发出信号通知 MainWindow 更新全局快捷键
    emit shortcutsChanged();
}

// 唤起全局现有的 UserProfileDialog
void SettingsPage::showProfileDialog() {
    UserProfileDialog dlg(this);
    dlg.exec();
    // 关闭弹窗后，UserProfile 自身发出的 profileChanged 信号会自动触发 refreshUserStatus()
}

// 槽函数：同步全局用户信息 UI
void SettingsPage::refreshUserStatus() {
    QString userName = UserProfile::instance().userName();

    // 假设未登录时的用户名为 "Guest" 或空，你可以根据 UserProfile 的实际逻辑来修改判断条件
    if (userName.isEmpty() || userName == QStringLiteral("Guest") || userName == QStringLiteral("未登录")) {
        m_accountStatusLabel->setText(QStringLiteral("当前未登录"));
        m_accountStatusLabel->setStyleSheet("color: #6b7280;");
        m_loginButton->setText(QStringLiteral("登录 / 注册"));
    } else {
        m_accountStatusLabel->setText(QStringLiteral("当前用户名: ") + userName);
        m_accountStatusLabel->setStyleSheet("color: #10b981; font-weight: bold;"); // 绿色，提示成功
        m_loginButton->setText(QStringLiteral("编辑个人资料"));
    }
}

} // namespace AlgeMate::Settings