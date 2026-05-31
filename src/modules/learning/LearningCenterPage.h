#ifndef ALGEMATE_LEARNING_CENTER_PAGE_H
#define ALGEMATE_LEARNING_CENTER_PAGE_H

#include <QWidget>

class QListWidget;
class QShowEvent;

namespace AlgeMate::Learning {

/// 学习管理中心: 数据统计与趋势分析
class LearningCenterPage : public QWidget {
    Q_OBJECT
public:
    explicit LearningCenterPage(QWidget* parent = nullptr);
    void refreshData();

protected:
    void showEvent(QShowEvent* event) override;

signals:
    void backRequested();

private:
    void refreshRecords();

    QListWidget* m_records = nullptr;
    QWidget* m_moduleChart = nullptr;
};

} // namespace AlgeMate::Learning

#endif // ALGEMATE_LEARNING_CENTER_PAGE_H