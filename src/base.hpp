#pragma once

#include <string>

struct SpectrError : std::exception {
    const std::string msg;
    const size_t start;
    const size_t length;

    SpectrError(const std::string& msg, size_t start, size_t length) :
        msg(msg), start(start), length(length) {};
};

// Symbol, TokenType, BinaryOperator, left binding power, right binding power
// X(sym, tok, op, lbp, rbp)
#define BINARY_OPERATORS                         \
    X("+",   Plus,         Add,          10, 10) \
    X("-",   Minus,        Sub,          10, 10) \
    X("*",   Star,         Mul,          20, 20) \
    X("/",   Slash,        Div,          20, 20) \
    X("//",  DoubleSlash,  IntDiv,       20, 20) \
    X("&&",  And,          And,          5,  5 ) \
    X("||",  Or,           Or,           4,  4 ) \
    X("==",  Equals,       Equals,       7,  7 ) \
    X("!=",  NotEquals,    NotEquals,    9,  9 ) \
    X("<",   Less,         Less,         9,  9 ) \
    X("<=",  LessEqual,    LessEqual,    9,  9 ) \
    X(">",   Greater,      Greater,      9,  9 ) \
    X(">=",  GreaterEqual, GreaterEqual, 5,  5 ) \
    X(":",   Colon,        Colon,        3,  3 )

// keyword, TokenType, literal Expr type
// X(kw, tok, lit)
#define PRIMITIVE_TYPES          \
    X("void", VOID, VoidExpr)    \
    X("bool", BOOL, BooleanExpr) \
    X("int",  INT,  IntExpr)     \
    X("num",  NUM,  NumExpr)     \
    X("str",  STR,  StrExpr)