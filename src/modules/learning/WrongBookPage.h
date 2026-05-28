#ifndef ALGEMATE_WRONG_BOOK_PAGE_H
#define ALGEMATE_WRONG_BOOK_PAGE_H

#include <QWidget>
#include <QVector>
#include "QuestionBank.h"

class QVBoxLayout;
namespace AlgeMate::Learning {

/// 错题本
class WrongBookPage : public QWidget {
    Q_OBJECT
public:
    explicit WrongBookPage(QWidget* parent = nullptr);
signals:
    void backRequested();

private:
    void loadWrongQuestions();
    void addWrongQuestionCard(const Question& q,
                              const QString& time,
                              int wrongCount);

private:
    QVBoxLayout* contentLayout;
};

} // namespace AlgeMate::Learning

#endif // ALGEMATE_WRONG_BOOK_PAGE_H