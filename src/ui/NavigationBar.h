#ifndef ALGEMATE_NAVIGATIONBAR_H
#define ALGEMATE_NAVIGATIONBAR_H

#include <QWidget>

class QListWidget;

namespace AlgeMate {

class NavigationBar : public QWidget {
    Q_OBJECT
public:
    explicit NavigationBar(QWidget* parent = nullptr);

    void addNavItem(const QString& icon, const QString& title);
    void setCurrentIndex(int index);
    int  currentIndex() const;

signals:
    void navigated(int index);

private:
    void buildUi();
    QListWidget* list_ = nullptr;
};

}

#endif
