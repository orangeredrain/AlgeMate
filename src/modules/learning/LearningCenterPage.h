#ifndef ALGEMATE_LEARNING_CENTER_PAGE_H
#define ALGEMATE_LEARNING_CENTER_PAGE_H

#include <QWidget>

namespace AlgeMate::Learning {

/// 学习管理中心: 数据统计与趋势分析
class LearningCenterPage : public QWidget {
    Q_OBJECT
public:
    explicit LearningCenterPage(QWidget* parent = nullptr);
signals:
    void backRequested();
};

} // namespace AlgeMate::Learning

#endif // ALGEMATE_LEARNING_CENTER_PAGE_H
