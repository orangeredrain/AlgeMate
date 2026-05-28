#ifndef ALGEMATE_WRONG_BOOK_PAGE_H
#define ALGEMATE_WRONG_BOOK_PAGE_H

#include <QWidget>
#include <QDialog>
#include <QGridLayout>
#include "QuestionBank.h"

namespace AlgeMate::Learning {

/// 错题详情弹窗（兼顾重做模式）
class WrongDetailDialog : public QDialog {
    Q_OBJECT
signals:
    void deleteRequested(int id);
    void redoRequested(int id, const QString& newAnswer);
public:
    // 增加了 isRedoMode 参数，用于智能判断是“练习状态”还是“看解析状态”
    WrongDetailDialog(const Question& q, const QString& time, int wrongCount, void* sharedRenderer, bool isRedoMode = false, QWidget* parent = nullptr);
};

/// 错题本主页
class WrongBookPage : public QWidget {
    Q_OBJECT
public:
    explicit WrongBookPage(QWidget* parent = nullptr);
    void reload();
signals:
    void backRequested();

private:
    void loadWrongQuestions();
    void addWrongQuestionCard(const Question& q, const QString& time, int wrongCount, int index);

    // 内部业务管理
    void removeWrongQuestionById(int id);
    void startShuffleRedoWorkflow(); // 新增：全局乱序串联做题核心控制
    bool executeRedoAnswerCheck(int id, const QString& newAns); // 新增：判断及更新错题本

private:
    QGridLayout* contentLayout;
};

} // namespace AlgeMate::Learning

#endif // ALGEMATE_WRONG_BOOK_PAGE_H