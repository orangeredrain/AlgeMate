#include "MessageBubble.h"

#include <QLabel>
#include <QHBoxLayout>
#include <QPainter>
#include <QStyleOption>
#include <QResizeEvent>

namespace AlgeMate::AiSolver {

MessageBubble::MessageBubble(const QString& text, bool isUser, QWidget* parent)
    : QWidget(parent), isUser_(isUser) {

    setObjectName(QStringLiteral("MessageBubble"));

    layout_ = new QHBoxLayout(this);
    layout_->setContentsMargins(0, 0, 0, 0);
    layout_->setSpacing(8);

    textLabel_ = new QLabel(text);
    textLabel_->setWordWrap(true);
    textLabel_->setObjectName(isUser ? QStringLiteral("UserMessage") : QStringLiteral("AiMessage"));

    if (isUser) {
        // User message - right aligned
        layout_->addStretch();
        layout_->addWidget(textLabel_, 0, Qt::AlignRight | Qt::AlignTop);
    } else {
        // AI message - left aligned
        layout_->addWidget(textLabel_, 0, Qt::AlignLeft | Qt::AlignTop);
        layout_->addStretch();
    }

    applyStyles();
    setMinimumHeight(60);
}

void MessageBubble::setText(const QString& text) {
    if (textLabel_) {
        textLabel_->setText(text);
        updateLayout();
    }
}

QString MessageBubble::text() const {
    return textLabel_ ? textLabel_->text() : QString();
}

void MessageBubble::setMaxWidth(int width) {
    maxWidth_ = width;
    if (textLabel_) {
        textLabel_->setMaximumWidth(width);
    }
    updateLayout();
}

void MessageBubble::applyStyles() {
    if (isUser_) {
        // User message - blue background
        textLabel_->setStyleSheet(
            QStringLiteral("QLabel#UserMessage {"
                          "    background-color: #0078d4;"
                          "    color: white;"
                          "    padding: 10px 14px;"
                          "    border-radius: 14px;"
                          "    font-size: 13px;"
                          "    max-width: %1px;"
                          "}").arg(maxWidth_)
        );
    } else {
        // AI message - gray background
        textLabel_->setStyleSheet(
            QStringLiteral("QLabel#AiMessage {"
                          "    background-color: #e8e8e8;"
                          "    color: #333333;"
                          "    padding: 10px 14px;"
                          "    border-radius: 14px;"
                          "    font-size: 13px;"
                          "    max-width: %1px;"
                          "}").arg(maxWidth_)
        );
    }

    textLabel_->setMaximumWidth(maxWidth_);
}

void MessageBubble::updateLayout() {
    if (layout_) {
        layout_->invalidate();
    }
    adjustSize();
}

void MessageBubble::paintEvent(QPaintEvent* event) {
    QStyleOption opt;
    opt.initFrom(this);
    QPainter p(this);
    style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);

    QWidget::paintEvent(event);
}

void MessageBubble::resizeEvent(QResizeEvent* event) {
    QWidget::resizeEvent(event);
}

}