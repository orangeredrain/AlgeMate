#include "Parser.h"

#include <stdexcept>

namespace AlgeMate::Calculator::Interactive {

namespace {

// 异常: 携带位置, 由顶层 parse 捕获并填 ParseResult
struct ParseError : std::runtime_error {
    int pos;
    ParseError(const std::string& msg, int p) : std::runtime_error(msg), pos(p) {}
};

class Parser {
public:
    explicit Parser(const std::vector<Token>& toks) : toks_(toks) {}

    NodePtr parseStmt() {
        // 赋值: IDENT '=' expr
        if (toks_.size() >= 3
            && toks_[0].kind == Tok::Ident
            && toks_[1].kind == Tok::Assign) {
            auto n = std::make_shared<Node>();
            n->kind = Node::Kind::Assign;
            n->name = toks_[0].text;
            pos_ = 2;
            n->child = parseExpr(false);
            expectEnd_();
            return n;
        }
        // 多项式函数定义: IDENT '(' IDENT ')' '=' expr  ——
        //   语义等同 IDENT '=' expr; 形参名由 Evaluator 的自由变量机制自动识别.
        //   例: f(x) = x^2 + 1  存为 f → 多项式(x^2 + 1).
        if (toks_.size() >= 6
            && toks_[0].kind == Tok::Ident
            && toks_[1].kind == Tok::LParen
            && toks_[2].kind == Tok::Ident
            && toks_[3].kind == Tok::RParen
            && toks_[4].kind == Tok::Assign) {
            auto n = std::make_shared<Node>();
            n->kind = Node::Kind::Assign;
            n->name = toks_[0].text;
            pos_ = 5;
            n->child = parseExpr(false);
            expectEnd_();
            return n;
        }
        auto e = parseExpr(false);
        expectEnd_();
        return e;
    }

private:
    const std::vector<Token>& toks_;
    std::size_t pos_ = 0;

    const Token& peek() const { return toks_[pos_]; }
    const Token& consume() { return toks_[pos_++]; }

    // inMatrix: 矩阵行上下文, '+' '-' 前若有空格则视为新元素起点
    NodePtr parseExpr(bool inMatrix) {
        return parseAdd(inMatrix);
    }

    NodePtr parseAdd(bool inMatrix) {
        auto left = parseMul(inMatrix);
        while (peek().kind == Tok::Plus || peek().kind == Tok::Minus) {
            // 矩阵行内: 若 +/- 前有空格, 视为新元素起点, 不消费
            if (inMatrix && peek().spaceBefore) break;
            std::string op = peek().text;
            consume();
            auto right = parseMul(inMatrix);
            auto n = std::make_shared<Node>();
            n->kind = Node::Kind::Binary;
            n->name = op;
            n->lhs = left;
            n->rhs = right;
            left = n;
        }
        return left;
    }

    NodePtr parseMul(bool inMatrix) {
        auto left = parseUnary(inMatrix);
        while (true) {
            // 显式 * / 
            if (peek().kind == Tok::Star || peek().kind == Tok::Slash) {
                std::string op = peek().text;
                consume();
                auto right = parseUnary(inMatrix);
                auto n = std::make_shared<Node>();
                n->kind = Node::Kind::Binary;
                n->name = op;
                n->lhs = left;
                n->rhs = right;
                left = n;
                continue;
            }
            // 隐式乘法: 左端已是完整值表达式, 若紧跟 Number/Ident/'(' 则自动插入 '*'.
            // 矩阵行内 spaceBefore 为真时跟过, 依然由空白分隔逻辑将其视为新元素.
            const Tok k = peek().kind;
            const bool canStartPrimary =
                (k == Tok::Number || k == Tok::Ident || k == Tok::LParen);
            if (canStartPrimary && !(inMatrix && peek().spaceBefore)) {
                auto right = parseUnary(inMatrix);
                auto n = std::make_shared<Node>();
                n->kind = Node::Kind::Binary;
                n->name = "*";
                n->lhs = left;
                n->rhs = right;
                left = n;
                continue;
            }
            break;
        }
        return left;
    }

    NodePtr parseUnary(bool inMatrix) {
        if (peek().kind == Tok::Minus || peek().kind == Tok::Plus) {
            std::string op = peek().text;
            consume();
            auto operand = parseUnary(inMatrix);
            if (op == "+") return operand;
            auto n = std::make_shared<Node>();
            n->kind = Node::Kind::Unary;
            n->name = "-";
            n->child = operand;
            return n;
        }
        return parsePow(inMatrix);
    }

    // 幂 ^: 右结合
    NodePtr parsePow(bool inMatrix) {
        auto base = parsePostfix(inMatrix);
        if (peek().kind == Tok::Caret) {
            consume();
            auto exp = parseUnary(inMatrix);
            auto n = std::make_shared<Node>();
            n->kind = Node::Kind::Binary;
            n->name = "^";
            n->lhs = base;
            n->rhs = exp;
            return n;
        }
        return base;
    }

    // 后缀: '  (转置, 可重复)
    NodePtr parsePostfix(bool inMatrix) {
        auto p = parsePrimary(inMatrix);
        while (peek().kind == Tok::Apos) {
            consume();
            auto n = std::make_shared<Node>();
            n->kind = Node::Kind::Unary;
            n->name = "'";
            n->child = p;
            p = n;
        }
        return p;
    }

    NodePtr parsePrimary(bool inMatrix) {
        const Token& t = peek();
        if (t.kind == Tok::Number) {
            consume();
            auto n = std::make_shared<Node>();
            n->kind = Node::Kind::Number;
            n->name = t.text;
            return n;
        }
        if (t.kind == Tok::Ident) {
            std::string name = t.text;
            consume();
            if (peek().kind == Tok::LParen) {
                consume();
                auto n = std::make_shared<Node>();
                n->kind = Node::Kind::Call;
                n->name = name;
                if (peek().kind != Tok::RParen) {
                    n->args.push_back(parseExpr(false));
                    while (peek().kind == Tok::Comma) {
                        consume();
                        n->args.push_back(parseExpr(false));
                    }
                }
                expect_(Tok::RParen, "缺少 ')'");
                return n;
            }
            auto n = std::make_shared<Node>();
            n->kind = Node::Kind::Ident;
            n->name = name;
            return n;
        }
        if (t.kind == Tok::LParen) {
            consume();
            auto e = parseExpr(false);
            expect_(Tok::RParen, "缺少 ')'");
            return e;
        }
        if (t.kind == Tok::LBracket) {
            return parseMatrix();
        }
        if (t.kind == Tok::Error) {
            throw ParseError(t.text, t.pos);
        }
        (void)inMatrix;
        throw ParseError("期望一个数、变量、矩阵或括号表达式", t.pos);
    }

    NodePtr parseMatrix() {
        const Token& lb = peek();
        consume();  // '['
        auto n = std::make_shared<Node>();
        n->kind = Node::Kind::Matrix;
        std::size_t expectedCols = 0;
        // 空矩阵 []
        if (peek().kind == Tok::RBracket) {
            consume();
            return n;
        }
        while (true) {
            std::vector<NodePtr> row;
            row.push_back(parseExpr(true));
            while (true) {
                const Token& t = peek();
                if (t.kind == Tok::Comma) {
                    consume();
                    row.push_back(parseExpr(true));
                    continue;
                }
                // 空白分隔: 下一个 token 为行内新元素的起始之一
                if (t.spaceBefore &&
                    (t.kind == Tok::Number || t.kind == Tok::Ident ||
                     t.kind == Tok::LParen || t.kind == Tok::LBracket ||
                     t.kind == Tok::Minus  || t.kind == Tok::Plus)) {
                    row.push_back(parseExpr(true));
                    continue;
                }
                break;
            }
            if (n->rows.empty()) expectedCols = row.size();
            else if (row.size() != expectedCols)
                throw ParseError(
                    "矩阵各行元素个数不一致 (第 " + std::to_string(n->rows.size() + 1)
                    + " 行有 " + std::to_string(row.size())
                    + " 个, 期望 " + std::to_string(expectedCols) + ")", peek().pos);
            n->rows.push_back(std::move(row));

            if (peek().kind == Tok::Semi) { consume(); continue; }
            if (peek().kind == Tok::RBracket) { consume(); return n; }
            throw ParseError("矩阵中期望 ';' 或 ']'", peek().pos);
        }
        (void)lb;
    }

    void expect_(Tok k, const char* msg) {
        if (peek().kind != k) throw ParseError(msg, peek().pos);
        consume();
    }
    void expectEnd_() {
        if (peek().kind != Tok::End)
            throw ParseError("多余的字符", peek().pos);
    }
};

}  // anonymous ns

ParseResult parse(const std::string& src) {
    ParseResult r;
    auto toks = lex(src);
    try {
        Parser p(toks);
        r.root = p.parseStmt();
        r.ok = true;
    } catch (const ParseError& e) {
        r.ok = false;
        r.error = e.what();
        r.errorPos = e.pos;
    } catch (const std::exception& e) {
        r.ok = false;
        r.error = e.what();
        r.errorPos = 0;
    }
    return r;
}

}
