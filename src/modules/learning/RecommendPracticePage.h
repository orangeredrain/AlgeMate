#ifndef ALGEMATE_RECOMMEND_PRACTICE_PAGE_H
#define ALGEMATE_RECOMMEND_PRACTICE_PAGE_H

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QVector>
#include "QuestionBank.h"
#include <QStackedWidget>
#include <QVBoxLayout>
#include <QScrollArea>
#include <QJsonArray>

namespace AlgeMate::Learning {

class RecommendPracticePage : public QWidget {
    Q_OBJECT
public:
    explicit RecommendPracticePage(QWidget* parent = nullptr);
    void reloadQuestions();
    void showLoadingPlaceholder();

signals:
    void requestRegenerate();
    void backRequested(); // 新增：返回上一级的信号

private slots:
    void loadBatch(int batchIndex);
    void backToList();
    void markCurrentQuestionCompleted();
    void onNextQuestion();       // 新增：下一题
    void onPreviousQuestion();   // 新增：上一题
    void onSubmitAnswer();       // 新增：提交
    void onAiGradeSubjective();  // 新增：AI评分

private:
    QStackedWidget* m_internalStack;
    QWidget* m_batchListPage;
    QWidget* m_practicePage;
    QVBoxLayout* m_batchListLayout;
    QJsonArray m_allBatches;
    int m_currentBatchIndex;

    void loadQuestion(int index);
    void updateUIForQuestion(const Question& q);
    void displayResult(bool isCorrect, const QString& feedback);
    void saveToWrongBook(const Question& q);

    QVector<Question> m_questions;
    int m_currentIndex = 0;

    QWidget* m_answerWidget = nullptr;
    QLabel* m_questionLabel = nullptr;
    QLabel* m_feedbackLabel = nullptr;
    QLabel* m_progressLabel = nullptr;
};

} // namespace AlgeMate::Learning
#endif // ALGEMATE_RECOMMEND_PRACTICE_PAGE_H