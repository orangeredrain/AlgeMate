#ifndef ALGEMATE_CALC_INTERACTIVE_LEXER_H
#define ALGEMATE_CALC_INTERACTIVE_LEXER_H

#include <string>
#include <vector>

namespace AlgeMate::Calculator::Interactive {

enum class Tok {
    End, Number, Ident,
    Plus, Minus, Star, Slash, Caret,
    LParen, RParen, LBracket, RBracket,
    Comma, Semi, Assign, Apos,
    Error
};

struct Token {
    Tok         kind = Tok::End;
    std::string text;         // Number/Ident 的字面, Error 的提示
    int         pos = 0;      // 在源中的起始字符位
    bool        spaceBefore = false;  // 前一个字符是空白 (用于矩阵行的空白分隔判断)
};

// 词法分析: 把表达式字符串切分为 token 序列 (末尾总是 Tok::End)
// 非法字符产出 Tok::Error, 调用方可据此报错
std::vector<Token> lex(const std::string& src);

}

#endif
