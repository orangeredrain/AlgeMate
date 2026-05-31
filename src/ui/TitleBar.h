#ifndef ALGEMATE_TITLEBAR_H
#define ALGEMATE_TITLEBAR_H

#include <QWidget>

class QPushButton;
class QLabel;
class QAbstractButton;

namespace AlgeMate {

class TitleBar : public QWidget {
    Q_OBJECT
public:
    explicit TitleBar(QWidget* parent = nullptr);

private slots:
    void onLightClicked();
    void onDarkClicked();
    void onUserClicked();
    void refreshUser();

private:
    void buildUi();
    void syncThemeButtons();

    QPushButton* btnLight_ = nullptr;
    QPushButton* btnDark_  = nullptr;
    QPushButton* btnFontDec_ = nullptr;
    QPushButton* btnFontNorm_= nullptr;
    QPushButton* btnFontInc_ = nullptr;
    QAbstractButton* btnUser_ = nullptr;
    QLabel*      avatarLabel_ = nullptr;
    QLabel*      nameLabel_   = nullptr;
protected:
    bool eventFilter(QObject* obj, QEvent* e) override;
};

}

#endif
