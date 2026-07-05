#ifndef ALGEMATE_TITLEBAR_H
#define ALGEMATE_TITLEBAR_H

#include <QWidget>

class QPushButton;
class QLabel;
class QAbstractButton;
class QToolButton;

namespace AlgeMate {

class TitleBar : public QWidget {
    Q_OBJECT
public:
    explicit TitleBar(QWidget* parent = nullptr);

    // 1. 确保 private slots 里声明了点击的槽函数
private slots:
    void onLightClicked();
    void onDarkClicked();
    void onUserClicked();
    void onTomatoClicked();

protected:
    bool eventFilter(QObject* obj, QEvent* e) override;

private:
    void buildUi();
    void syncThemeButtons();
    void refreshUser();

    // 2. 在 private 变量区域声明按钮指针
private:
    QPushButton* btnLight_ = nullptr;
    QPushButton* btnDark_ = nullptr;
    QToolButton* btnUser_ = nullptr;
    QToolButton* btnTomato_ = nullptr;

    QLabel* avatarLabel_ = nullptr;
    QLabel* nameLabel_ = nullptr;
};

} // namespace AlgeMate

#endif
