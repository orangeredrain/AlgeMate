#ifndef ALGEMATE_USERPROFILEDIALOG_H
#define ALGEMATE_USERPROFILEDIALOG_H

#include <QDialog>

class QLineEdit;
class QLabel;
class QPushButton;

namespace AlgeMate {

class UserProfileDialog : public QDialog {
    Q_OBJECT
public:
    explicit UserProfileDialog(QWidget* parent = nullptr);

private slots:
    void chooseAvatar();
    void removeAvatar();
    void accept() override;

private:
    void refreshAvatarPreview();

    QLineEdit*   editName_    = nullptr;
    QLabel*      avatarView_  = nullptr;
    QPushButton* btnChoose_   = nullptr;
    QPushButton* btnRemove_   = nullptr;
    QString      pendingAvatarPath_;
};

}

#endif
