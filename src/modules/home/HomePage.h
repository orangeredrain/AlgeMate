#ifndef ALGEMATE_HOMEPAGE_H
#define ALGEMATE_HOMEPAGE_H

#include <QWidget>
#include <QHash>

class QLabel;
class QEvent;

namespace AlgeMate::Home {

class HomePage : public QWidget {
    Q_OBJECT
public:
    enum NavTarget {
        Calculator = 0,
        AiSolver   = 1,
        Knowledge  = 2,
        Learning   = 3,
        Settings   = 4,
    };
    Q_ENUM(NavTarget)

    explicit HomePage(QWidget* parent = nullptr);

signals:
    void requestNavigate(int target);

protected:
    bool eventFilter(QObject* obj, QEvent* e) override;

private slots:
    void refreshGreeting();

private:
    QLabel* avatarLabel_   = nullptr;
    QLabel* greetingLabel_ = nullptr;
    QLabel* subtitleLabel_ = nullptr;
    QHash<QObject*, int> cardTargets_;
};

}

#endif
