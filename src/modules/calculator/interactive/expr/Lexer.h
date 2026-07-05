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
    std::string text;         
    int         pos = 0;      
    bool        spaceBefore = false;  
};

std::vector<Token> lex(const std::string& src);

}

#endif
