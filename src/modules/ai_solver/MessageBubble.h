#ifndef ALGEMATE_MESSAGEBUBBLE_H
#define ALGEMATE_MESSAGEBUBBLE_H

#include <QWidget>

class QLabel;
class QHBoxLayout;

namespace AlgeMate::AiSolver {

/**
 * @brief A styled message bubble widget for chat display
 */
class MessageBubble : public QWidget {
    Q_OBJECT
public:
    /**
     * @brief Create a message bubble
     * @param text Message text content
     * @param isUser True if user message, false if AI response
     * @param parent Parent widget
     */
    explicit MessageBubble(const QString& text, bool isUser, QWidget* parent = nullptr);

    /**
     * @brief Set the message text
     */
    void setText(const QString& text);

    /**
     * @brief Get the message text
     */
    QString text() const;

    /**
     * @brief Check if this is a user message
     */
    bool isUser() const { return isUser_; }

    /**
     * @brief Set maximum width for text wrapping
     */
    void setMaxWidth(int width);

protected:
    void paintEvent(QPaintEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;

private:
    void applyStyles();
    void updateLayout();

    QLabel* textLabel_    = nullptr;
    QHBoxLayout* layout_  = nullptr;
    bool isUser_          = false;
    int maxWidth_         = 500;
};

}

#endif
