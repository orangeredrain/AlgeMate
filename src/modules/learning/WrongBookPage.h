#ifndef ALGEMATE_WRONG_BOOK_PAGE_H
#define ALGEMATE_WRONG_BOOK_PAGE_H

#include <QWidget>
#include <QDialog>
#include <QGridLayout>
#include "QuestionBank.h"

namespace AlgeMate::Learning {

/// 错题详情弹窗
class WrongDetailDialog : public QDialog {
    Q_OBJECT
public:
    WrongDetailDialog(const Question& q, const QString& time, int wrongCount, void* sharedRenderer, QWidget* parent = nullptr);
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
    // 升级为：迷你方形题干卡片
    void addWrongQuestionCard(const Question& q, const QString& time, int wrongCount, int index);

private:
    QGridLayout* contentLayout; // 升级为网格布局实现方形卡片矩阵
};

} // namespace AlgeMate::Learning

#endif // ALGEMATE_WRONG_BOOK_PAGE_H