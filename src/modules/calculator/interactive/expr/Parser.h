#ifndef ALGEMATE_CALC_INTERACTIVE_PARSER_H
#define ALGEMATE_CALC_INTERACTIVE_PARSER_H

#include "Lexer.h"

#include <memory>
#include <string>
#include <vector>

namespace AlgeMate::Calculator::Interactive {

// AST 节点 (简易 tagged union 风格)
struct Node {
    enum class Kind {
        Number, Ident, Matrix, Call, Binary, Unary, Assign
    };

    Kind kind;

    // 公用字段 (按 Kind 解释):
    std::string         name;     // Ident/Call(fn 名)/Assign(左值)/Binary(op)/Unary(op)
    std::shared_ptr<Node>              child;   // Unary.operand / Assign.value
    std::shared_ptr<Node>              lhs;     // Binary.left
    std::shared_ptr<Node>              rhs;     // Binary.right
    std::vector<std::shared_ptr<Node>> args;    // Call.args
    std::vector<std::vector<std::shared_ptr<Node>>> rows;  // Matrix rows
};

using NodePtr = std::shared_ptr<Node>;

// 解析结果
struct ParseResult {
    bool    ok    = true;
    NodePtr root;           // 成功时为 AST 根
    std::string error;      // 失败时的中文错误信息
    int     errorPos = 0;   // 出错位置 (字符位)
};

// 主入口: 解析一行表达式 / 赋值
ParseResult parse(const std::string& src);

}

#endif
