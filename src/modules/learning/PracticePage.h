#ifndef ALGEMATE_PRACTICE_PAGE_H
#define ALGEMATE_PRACTICE_PAGE_H

#include <QWidget>
#include <QLabel>
#include <QVector>
#include <QString>

#include "QuestionBank.h"

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

public slots: // 注意：改成 public slots 方便外部调用
    void onSubmitAnswer();
    void onNextQuestion();
    void onPreviousQuestion();
    void onLoadChapterQuestions();
    void onAiGradeSubjective();
    void loadQuestionsByMicroChapter(const QString& microMarkdownName); // 新增这个方法

private:
    void loadQuestion(int index);
    void updateUIForQuestion(const Question& q);
    void displayResult(bool isCorrect, const QString& feedback);

    QVector<Question> chapterQuestions;  // 从章节MD文件加载的题目
    int currentQuestionIndex;
    QWidget* answerWidget;
    QLabel* questionLabel;              // 需要 #include <QLabel>
    QLabel* feedbackLabel;              // 需要 #include <QLabel>
    QLabel* progressLabel;              // 需要 #include <QLabel>
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