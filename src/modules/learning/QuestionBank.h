#ifndef QUESTIONBANK_H
#define QUESTIONBANK_H
#include <QLabel>
#include <QString>

// 题目类型枚举
enum class QuestionType {
    Single,      // 单选题
    Fill,        // 填空题（数字）
    Subjective   // 解答题
};

// 题目结构体
struct Question {
    int id;                          // 题目ID
    QString contentPath;             // MD文件路径
    QString content;                 // 题目内容（从MD文件读取）
    QuestionType type;               // 题目类型
    int score;                       // 该题分数
    QString correctAnswer;           // 正确答案
    QVector<QString> choices;        // 选择项（仅用于单选题）
    int attempts;                    // 做题次数
    QString userAnswer;              // 用户答案
    bool isCorrect;                  // 是否正确
};


#endif // QUESTIONBANK_H
