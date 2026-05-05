#ifndef ALGEMATE_WRONG_BOOK_PAGE_H
#define ALGEMATE_WRONG_BOOK_PAGE_H

#include <QWidget>

namespace AlgeMate::Learning {

/// 错题本
class WrongBookPage : public QWidget {
    Q_OBJECT
public:
    explicit WrongBookPage(QWidget* parent = nullptr);
signals:
    void backRequested();
};

} // namespace AlgeMate::Learning

#endif // ALGEMATE_WRONG_BOOK_PAGE_H
