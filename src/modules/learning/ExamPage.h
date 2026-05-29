#ifndef EXAMPAGE_H
#define EXAMPAGE_H

#include <QWidget>
#include <QSpinBox>
#include <QLabel>
#include <QString>
#include <QVector>
#include <QMap>
#include <memory>
#include <QPushButton>
#include "QuestionBank.h"

namespace AlgeMate::Learning {

// 考试选择页面
class ExamSettingPage : public QWidget {
    Q_OBJECT
public:
    explicit ExamSettingPage(QWidget* parent = nullptr);

signals:
    void backRequested();
    void examStarted(int timeMinutes);

private:
    QSpinBox* timeSpinBox;
};

// 考试进行页面
class ExamProgressPage : public QWidget {
    Q_OBJECT
public:
    explicit ExamProgressPage(const QVector<Question>& questions, int timeMinutes, QWidget* parent = nullptr);

signals:
    void backRequested();
    void examFinished(const QVector<Question>& results);

private slots:
    void onSubmitAnswer();
    void onSubmitExam();
    void onTimeUpdate();
    void onNextQuestion();
    void onPreviousQuestion();

private:
    void loadQuestion(int index);
    void updateUIForQuestion(const Question& q);
    void updateNavButtons();
    QTimer* m_examTimer = nullptr;

    //错题本
    void saveWrongQuestions();

    void handleSingleChoiceAnswer(int choiceIndex);
    void handleFillAnswer(double value);
    void handleSubjectiveAnswer(const QString& answer);

    QVector<Question> questions;
    int currentQuestionIndex;
    int remainingSeconds;
    QLabel* timerLabel;
    QLabel* questionLabel;
    QLabel* scoreLabel;
    QWidget* answerWidget;  // 根据题目类型动态创建
    QVector<QPushButton*> navButtons;
};

// 考试结果页面
class ExamResultPage : public QWidget {
    Q_OBJECT
public:
    explicit ExamResultPage(const QVector<Question>& results, QWidget* parent = nullptr);

signals:
    void backRequested();

private:
    void displayResults(const QVector<Question>& results);
};

// 主考试页面
class ExamPage : public QWidget {
    Q_OBJECT
public:
    explicit ExamPage(QWidget* parent = nullptr);
    void resetToSettings();

signals:
    void backRequested();

private slots:
    void onStartExam(int timeMinutes);
    void onExamFinished(const QVector<Question>& results);

private:
    void loadQuestions();  // 从MD文件加载题目

    QVector<Question> examQuestions;
    QWidget* currentPage;  // 当前显示的页面
};

} // namespace AlgeMate::Learning

#endif // EXAMPAGE_H