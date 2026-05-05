#ifndef ALGEMATE_PRACTICE_PAGE_H
#define ALGEMATE_PRACTICE_PAGE_H

#include <QWidget>

namespace AlgeMate::Learning {

/// 练习模式入口: 显示 3 个子模式卡片.
class PracticePage : public QWidget {
    Q_OBJECT
public:
    explicit PracticePage(QWidget* parent = nullptr);

signals:
    void backRequested();
    void calculationProblemRequested();
    void chapterPracticeRequested();
    void topicPracticeRequested();
};

/// 计算题练习
class CalculationProblemPage : public QWidget {
    Q_OBJECT
public:
    explicit CalculationProblemPage(QWidget* parent = nullptr);
signals:
    void backRequested();
};

/// 对应章节练习
class ChapterPracticePage : public QWidget {
    Q_OBJECT
public:
    explicit ChapterPracticePage(QWidget* parent = nullptr);
signals:
    void backRequested();
};

/// 专题模式练习
class TopicPracticePage : public QWidget {
    Q_OBJECT
public:
    explicit TopicPracticePage(QWidget* parent = nullptr);
signals:
    void backRequested();
};

} // namespace AlgeMate::Learning

#endif // ALGEMATE_PRACTICE_PAGE_H
