#ifndef ALGEMATE_USERPROFILE_H
#define ALGEMATE_USERPROFILE_H

#include <QObject>
#include <QPixmap>
#include <QString>

namespace AlgeMate {

class UserProfile : public QObject {
    Q_OBJECT
public:
    static UserProfile& instance();

    QString userName()   const { return userName_; }
    QString avatarPath() const { return avatarPath_; }

    QPixmap avatarPixmap(int size) const;

    void setUserName(const QString& name);
    void setAvatarPath(const QString& path);

    static QString greetingByTime();

signals:
    void profileChanged();

private:
    explicit UserProfile(QObject* parent = nullptr);
    UserProfile(const UserProfile&) = delete;
    UserProfile& operator=(const UserProfile&) = delete;

    void load();
    void save() const;

    QString userName_;
    QString avatarPath_;
};

}

#endif
