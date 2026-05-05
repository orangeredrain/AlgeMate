#ifndef ALGEMATE_CLICKABLE_CARD_H
#define ALGEMATE_CLICKABLE_CARD_H

#include <QFrame>
#include <QMouseEvent>

namespace AlgeMate::Learning {

/// QFrame 子类: 点击发出 clicked() 信号.
/// 配合 QSS #Card 规则使用, 兼具卡片外观和点击能力.
class ClickableCard : public QFrame {
    Q_OBJECT
public:
    explicit ClickableCard(QWidget* parent = nullptr) : QFrame(parent) {
        setObjectName(QStringLiteral("Card"));
        setCursor(Qt::PointingHandCursor);
    }
signals:
    void clicked();
protected:
    void mouseReleaseEvent(QMouseEvent* e) override {
        if (e->button() == Qt::LeftButton && rect().contains(e->pos()))
            emit clicked();
        QFrame::mouseReleaseEvent(e);
    }
};

} // namespace AlgeMate::Learning

#endif // ALGEMATE_CLICKABLE_CARD_H
