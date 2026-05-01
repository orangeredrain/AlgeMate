#include "UserProfile.h"

#include <QSettings>
#include <QPainter>
#include <QPainterPath>
#include <QFileInfo>
#include <QTime>

namespace AlgeMate {

namespace {
constexpr const char* kKeyUser   = "profile/userName";
constexpr const char* kKeyAvatar = "profile/avatarPath";
}

UserProfile& UserProfile::instance() {
    static UserProfile inst;
    return inst;
}

UserProfile::UserProfile(QObject* parent) : QObject(parent) { load(); }

void UserProfile::load() {
    QSettings s;
    userName_   = s.value(kKeyUser,   QStringLiteral("学习者")).toString();
    avatarPath_ = s.value(kKeyAvatar, QString()).toString();
    if (!avatarPath_.isEmpty() && !QFileInfo::exists(avatarPath_))
        avatarPath_.clear();
}

void UserProfile::save() const {
    QSettings s;
    s.setValue(kKeyUser,   userName_);
    s.setValue(kKeyAvatar, avatarPath_);
}

void UserProfile::setUserName(const QString& name) {
    const QString trimmed = name.trimmed();
    if (trimmed.isEmpty() || trimmed == userName_) return;
    userName_ = trimmed;
    save();
    emit profileChanged();
}

void UserProfile::setAvatarPath(const QString& path) {
    if (path == avatarPath_) return;
    avatarPath_ = path;
    save();
    emit profileChanged();
}

QPixmap UserProfile::avatarPixmap(int size) const {
    const int dpr = 2;
    const int px  = size * dpr;

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
    if (!avatarPath_.isEmpty()) src.load(avatarPath_);

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

        QString initial = userName_.isEmpty() ? QStringLiteral("A")
                                              : userName_.left(1).toUpper();
        QFont f = p.font();
        f.setPointSizeF(size * 0.42);
        f.setBold(true);
        p.setFont(f);
        p.setPen(Qt::white);
        p.drawText(QRect(0, 0, size, size), Qt::AlignCenter, initial);
    }
    return out;
}

QString UserProfile::greetingByTime() {
    const int h = QTime::currentTime().hour();
    if (h < 6)  return QStringLiteral("夜深了");
    if (h < 11) return QStringLiteral("早上好");
    if (h < 13) return QStringLiteral("中午好");
    if (h < 18) return QStringLiteral("下午好");
    if (h < 23) return QStringLiteral("晚上好");
    return QStringLiteral("夜深了");
}

}
