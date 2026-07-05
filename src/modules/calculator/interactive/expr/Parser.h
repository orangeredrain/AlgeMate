#ifndef ALGEMATE_CALC_INTERACTIVE_PARSER_H
#define ALGEMATE_CALC_INTERACTIVE_PARSER_H

#include "Lexer.h"

#include <memory>
#include <string>
#include <vector>

namespace AlgeMate::Calculator::Interactive {

struct Node {
    enum class Kind {
        Number, Ident, Matrix, Call, Binary, Unary, Assign
    };

    Kind kind;

    std::string         name;     
    std::shared_ptr<Node>              child;   
    std::shared_ptr<Node>              lhs;     
    std::shared_ptr<Node>              rhs;     
    std::vector<std::shared_ptr<Node>> args;    
    std::vector<std::vector<std::shared_ptr<Node>>> rows;  
};

using NodePtr = std::shared_ptr<Node>;

struct ParseResult {
    bool    ok    = true;
    NodePtr root;           
    std::string error;      
    int     errorPos = 0;   
};

ParseResult parse(const std::string& src);

}

#endif
