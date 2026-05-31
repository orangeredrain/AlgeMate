#pragma once

#include <QWidget>
#include <QSettings>

class QVBoxLayout;
class QLineEdit;
class QLabel;
class QPushButton;
class QKeySequenceEdit;

namespace AlgeMate::Settings {

class SettingsPage : public QWidget {
    Q_OBJECT

public:
    explicit SettingsPage(QWidget* parent = nullptr);
    ~SettingsPage() override = default;
signals:
    void shortcutsChanged();
private slots:
    void showProfileDialog();  // 唤起全局的个人资料弹窗
    void refreshUserStatus();  // 响应全局信号，同步账号UI状态
    void saveApiKey();
    void saveShortcuts();

private:
    void setupUI();
    void loadSettings();

    // 模块化创建 UI 卡片
    QWidget* createThemeCard();
    QWidget* createAccountCard();
    QWidget* createApiCard();
    QWidget* createShortcutCard();

private:
    // UI 控件引用，方便后续读取和更新
    QLabel* m_accountStatusLabel{nullptr};
    QPushButton* m_loginButton{nullptr};

    // API Keys 控件
    QLineEdit* m_apiKeyEdit{nullptr};
    QLineEdit* m_doubaoApiKeyEdit{nullptr};

    // 修改为你的新快捷键需求
    QKeySequenceEdit* m_shortcutAiChat{nullptr};     // AI 助手
    QKeySequenceEdit* m_shortcutWrongBook{nullptr};  // 错题本
    QKeySequenceEdit* m_shortcutUnfinished{nullptr}; // 未完成的目标
};

} // namespace AlgeMate::Settings