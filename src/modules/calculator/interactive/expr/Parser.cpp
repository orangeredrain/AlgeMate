#include "Parser.h"

#include <stdexcept>

namespace AlgeMate::Calculator::Interactive {

namespace {

struct ParseError : std::runtime_error {
    int pos;
    ParseError(const std::string& msg, int p) : std::runtime_error(msg), pos(p) {}
};

class Parser {
public:
    explicit Parser(const std::vector<Token>& toks) : toks_(toks) {}

    NodePtr parseStmt() {

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

    NodePtr parseExpr(bool inMatrix) {
        return parseAdd(inMatrix);
    }

    NodePtr parseAdd(bool inMatrix) {
        auto left = parseMul(inMatrix);
        while (peek().kind == Tok::Plus || peek().kind == Tok::Minus) {

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
        consume();  
        auto n = std::make_shared<Node>();
        n->kind = Node::Kind::Matrix;
        std::size_t expectedCols = 0;

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

}  

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
