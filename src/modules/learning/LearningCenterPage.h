#ifndef ALGEMATE_LEARNING_CENTER_PAGE_H
#define ALGEMATE_LEARNING_CENTER_PAGE_H

#include <QWidget>

class QListWidget;
class QShowEvent;
class QLabel;
class QToolButton;

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
    QLabel* labelTomatoRecord_ = nullptr;
    QLabel* m_lblTomatoCount = nullptr;
    QToolButton* m_btnTomato = nullptr;
    void updateTomatoRecord();

    QListWidget* m_records = nullptr;
    QWidget* m_moduleChart = nullptr;
};

} // namespace AlgeMate::Learning，防止与项目中其他地方也有LearningCenterPage这个类

#endif // ALGEMATE_LEARNING_CENTER_PAGE_H