#ifndef ALGEMATE_PRACTICE_PAGE_H
#define ALGEMATE_PRACTICE_PAGE_H

#include <QWidget>
#include <QLabel>
#include <QVector>
#include <QString>
#include <QPushButton>
#include <QGridLayout>
#include <QStackedWidget>

#include "QuestionBank.h"

class QSplitter;
class QTreeWidget;

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
    // void topicPracticeRequested();
};

/// 计算题练习
class CalculationProblemPage : public QWidget {
    Q_OBJECT
public:
    explicit CalculationProblemPage(QWidget* parent = nullptr);

signals:
    void backRequested();

private slots:
    void onCardClicked(int type);        // 点击卡片进入对应题型
    void onReturnToCatalog();            // 从做题页返回目录
    void onGenerateCurrentType();        // 重新生成当前题型
    void onSubmitAnswer();
    void onAiGradeSubjective();

private:
    void buildCatalog();
    void buildProblemView();
    void addCard(QGridLayout* grid, int row, int col, const QString& icon,
                 const QString& title, const QString& desc, int type);

    void generateQuestionByType(int type);
    void updateUIForQuestion();
    void displayResult(bool isCorrect, const QString& feedback);
    void saveToWrongBook(const Question& q);

    QStackedWidget* m_stack = nullptr;
    QWidget* m_catalogWidget = nullptr;
    QWidget* m_problemWidget = nullptr;
    QLabel* m_problemTitleLabel = nullptr;

    int m_currentType = 0; // 当前选中的题型：0=秩, 1=迹, 2=行列式, 3=基础解系
    Question currentQuestion;
    int totalAttempted = 0;
    int totalCorrect = 0;

    QLabel* progressLabel = nullptr;
    QWidget* answerWidget = nullptr;
    QLabel* questionLabel = nullptr;
    QLabel* feedbackLabel = nullptr;
    QPushButton* submitBtn = nullptr;
};

/// 对应章节练习
class ChapterPracticePage : public QWidget {
    Q_OBJECT
public:
    explicit ChapterPracticePage(QWidget* parent = nullptr);
protected:
    bool eventFilter(QObject* watched, QEvent* event) override;
signals:
    void backRequested();

public slots: // 注意：改成 public slots 方便外部调用
    void onSubmitAnswer();
    void onNextQuestion();
    void onPreviousQuestion();
    void onLoadChapterQuestions();
    void onAiGradeSubjective();
    void loadQuestionsByMicroChapter(const QString& microMarkdownName);
    void selectChapterByResourcePath(const QString& path);

private:
    void loadQuestion(int index);
    void updateUIForQuestion(const Question& q);
    void displayResult(bool isCorrect, const QString& feedback);
    void buildChapterTree(); // 目录树构建函数
    void saveToWrongBook(const Question& q); // 保存错题到本地的私有方法

    QVector<Question> chapterQuestions;
    int currentQuestionIndex;
    QWidget* answerWidget;
    QLabel* questionLabel;
    QLabel* feedbackLabel;
    QLabel* progressLabel;

    // 分栏导航和指示标头
    class QSplitter* m_splitter     = nullptr;
    class QTreeWidget* m_chapterTree  = nullptr;
    QLabel* m_chapterTitleLabel = nullptr;
};

// /// 专题模式练习
// class TopicPracticePage : public QWidget {
//     Q_OBJECT
// public:
//     explicit TopicPracticePage(QWidget* parent = nullptr);

// signals:
//     void backRequested();
// };

} // namespace AlgeMate::Learning

#endif // ALGEMATE_PRACTICE_PAGE_H