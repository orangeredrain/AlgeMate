#include "UserProfileDialog.h"
#include "core/UserProfile.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QFileDialog>
#include <QDialogButtonBox>
#include <QPainter>
#include <QPainterPath>
#include <QFileInfo>
#include <QPixmap>

namespace AlgeMate {

static QPixmap roundedPreview(const QString& path, const QString& fallbackName, int size) {
    const int dpr = 2;
    const int px = size * dpr;
    QPixmap out(px, px);
    out.fill(Qt::transparent);
    out.setDevicePixelRatio(dpr);
    QPainter p(&out);
    p.setRenderHint(QPainter::Antialiasing, true);
    p.setRenderHint(QPainter::SmoothPixmapTransform, true);
    QPainterPath clip;
    clip.addEllipse(0, 0, size, size);
    p.setClipPath(clip);

    QPixmap src;
    if (!path.isEmpty()) src.load(path);

    if (!src.isNull()) {
        const QPixmap scaled = src.scaled(size, size,
            Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
        const int x = (scaled.width()  - size) / 2;
        const int y = (scaled.height() - size) / 2;
        p.drawPixmap(0, 0, scaled, x, y, size, size);
    } else {
        QLinearGradient g(0, 0, size, size);
        g.setColorAt(0.0, QColor("#8B7BFF"));
        g.setColorAt(1.0, QColor("#6A5AE0"));
        p.fillRect(0, 0, size, size, g);
        QFont f = p.font();
        f.setPointSizeF(size * 0.42);
        f.setBold(true);
        p.setFont(f);
        p.setPen(Qt::white);
        p.drawText(QRect(0, 0, size, size), Qt::AlignCenter,
            fallbackName.isEmpty() ? QStringLiteral("A") : fallbackName.left(1).toUpper());
    }
    return out;
}

UserProfileDialog::UserProfileDialog(QWidget* parent) : QDialog(parent) {
    setWindowTitle(QStringLiteral("个人资料"));
    setModal(true);
    setMinimumWidth(420);

    pendingAvatarPath_ = UserProfile::instance().avatarPath();

    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(28, 24, 28, 20);
    root->setSpacing(18);

    auto* title = new QLabel(QStringLiteral("个人资料"));
    title->setStyleSheet("font-size:18px; font-weight:700;");

    avatarView_ = new QLabel;
    avatarView_->setFixedSize(96, 96);
    avatarView_->setAlignment(Qt::AlignCenter);

    btnChoose_ = new QPushButton(QStringLiteral("选择头像…"));
    btnChoose_->setProperty("primary", true);
    btnRemove_ = new QPushButton(QStringLiteral("移除头像"));

    auto* avatarRow = new QHBoxLayout;
    avatarRow->setSpacing(16);
    auto* btnCol = new QVBoxLayout;
    btnCol->setSpacing(8);
    btnCol->addWidget(btnChoose_);
    btnCol->addWidget(btnRemove_);
    btnCol->addStretch();
    avatarRow->addWidget(avatarView_);
    avatarRow->addLayout(btnCol, 1);

    auto* nameLabel = new QLabel(QStringLiteral("用户名"));
    nameLabel->setStyleSheet("color:#8A8FA3; font-size:12px;");
    editName_ = new QLineEdit(UserProfile::instance().userName());
    editName_->setPlaceholderText(QStringLiteral("请输入昵称"));
    editName_->setMaxLength(24);

    auto* nameCol = new QVBoxLayout;
    nameCol->setSpacing(6);
    nameCol->addWidget(nameLabel);
    nameCol->addWidget(editName_);

    auto* box = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    box->button(QDialogButtonBox::Ok)->setText(QStringLiteral("保存"));
    box->button(QDialogButtonBox::Ok)->setProperty("primary", true);
    box->button(QDialogButtonBox::Cancel)->setText(QStringLiteral("取消"));
    connect(box, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(box, &QDialogButtonBox::rejected, this, &QDialog::reject);

    root->addWidget(title);
    root->addLayout(avatarRow);
    root->addLayout(nameCol);
    root->addStretch();
    root->addWidget(box);

    connect(btnChoose_, &QPushButton::clicked, this, &UserProfileDialog::chooseAvatar);
    connect(btnRemove_, &QPushButton::clicked, this, &UserProfileDialog::removeAvatar);

    refreshAvatarPreview();
}

void UserProfileDialog::refreshAvatarPreview() {
    avatarView_->setPixmap(roundedPreview(pendingAvatarPath_, editName_->text(), 96));
    btnRemove_->setEnabled(!pendingAvatarPath_.isEmpty());
}

void UserProfileDialog::chooseAvatar() {
    const QString path = QFileDialog::getOpenFileName(
        this,
        QStringLiteral("选择头像图片"),
        QString(),
        QStringLiteral("图片文件 (*.png *.jpg *.jpeg *.bmp *.webp)"));
    if (path.isEmpty()) return;
    if (!QFileInfo::exists(path)) return;
    pendingAvatarPath_ = path;
    refreshAvatarPreview();
}

void UserProfileDialog::removeAvatar() {
    pendingAvatarPath_.clear();
    refreshAvatarPreview();
}

void UserProfileDialog::accept() {
    UserProfile::instance().setUserName(editName_->text());
    UserProfile::instance().setAvatarPath(pendingAvatarPath_);
    QDialog::accept();
}

}
