#include "Lexer.h"

#include <cctype>

namespace AlgeMate::Calculator::Interactive {

static bool isIdentStart(char c) {
    unsigned char u = static_cast<unsigned char>(c);
    // UTF-8 多字节字符 (≥ 0x80) 允许作为标识符 (如希腊字母 α/β/λ/σ)
    return std::isalpha(u) || c == '_' || u >= 0x80;
}
static bool isIdentCont (char c) {
    unsigned char u = static_cast<unsigned char>(c);
    return std::isalnum(u) || c == '_' || u >= 0x80;
}
static bool isSpace     (char c) { return c == ' ' || c == '\t' || c == '\r' || c == '\n'; }

std::vector<Token> lex(const std::string& src) {
    std::vector<Token> out;
    const int N = (int)src.size();
    int i = 0;
    bool pendingSpace = false;
    while (i < N) {
        char c = src[i];
        if (isSpace(c)) { pendingSpace = true; ++i; continue; }

        Token t;
        t.pos = i;
        t.spaceBefore = pendingSpace;
        pendingSpace = false;

        // 数字: 整数或小数 (.5 / 5. / 5.5)
        if (std::isdigit((unsigned char)c) || (c == '.' && i + 1 < N && std::isdigit((unsigned char)src[i + 1]))) {
            int start = i;
            bool sawDot = false;
            while (i < N) {
                if (std::isdigit((unsigned char)src[i])) { ++i; continue; }
                if (src[i] == '.' && !sawDot) { sawDot = true; ++i; continue; }
                break;
            }
            t.kind = Tok::Number;
            t.text = src.substr(start, i - start);
            out.push_back(t);
            continue;
        }

        // 标识符
        if (isIdentStart(c)) {
            int start = i;
            while (i < N && isIdentCont(src[i])) ++i;
            t.kind = Tok::Ident;
            t.text = src.substr(start, i - start);
            out.push_back(t);
            continue;
        }

        // 单字符符号
        switch (c) {
            case '+':  t.kind = Tok::Plus;     break;
            case '-':  t.kind = Tok::Minus;    break;
            case '*':  t.kind = Tok::Star;     break;
            case '/':  t.kind = Tok::Slash;    break;
            case '^':  t.kind = Tok::Caret;    break;
            case '(':  t.kind = Tok::LParen;   break;
            case ')':  t.kind = Tok::RParen;   break;
            case '[':  t.kind = Tok::LBracket; break;
            case ']':  t.kind = Tok::RBracket; break;
            case ',':  t.kind = Tok::Comma;    break;
            case ';':  t.kind = Tok::Semi;     break;
            case '=':  t.kind = Tok::Assign;   break;
            case '\'': t.kind = Tok::Apos;     break;
            default:
                t.kind = Tok::Error;
                t.text = std::string("非法字符 '") + c + "'";
                out.push_back(t);
                ++i;
                continue;
        }
        t.text = std::string(1, c);
        ++i;
        out.push_back(t);
    }
    Token end;
    end.kind = Tok::End;
    end.pos = N;
    end.spaceBefore = pendingSpace;
    out.push_back(end);
    return out;
}

}
