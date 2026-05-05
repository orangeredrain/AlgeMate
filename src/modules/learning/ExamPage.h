#ifndef ALGEMATE_EXAM_PAGE_H
#define ALGEMATE_EXAM_PAGE_H

#include <QWidget>

namespace AlgeMate::Learning {

/// 考试模式
class ExamPage : public QWidget {
    Q_OBJECT
public:
    explicit ExamPage(QWidget* parent = nullptr);
signals:
    void backRequested();
};

} // namespace AlgeMate::Learning

#endif // ALGEMATE_EXAM_PAGE_H
