#ifndef PASTEXAMS_H
#define PASTEXAMS_H

#include <QString>
#include <QVector>
#include "QuestionBank.h" // 复用 Question 结构体和 QuestionType

namespace AlgeMate::Learning {

class PastExams {
public:
    // 获取所有支持的试卷列表（用于 UI 下拉框）
    static QVector<QString> getExamList();

    // 根据试卷索引加载对应的真题
    static QVector<Question> getExamPaper(int examIndex);
};

} // namespace AlgeMate::Learning

#endif // PASTEXAMS_H