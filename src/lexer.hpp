#pragma once

#include <string>
#include <vector>
#include <array>
#include <utility>
#include <string_view>
#include <optional>

struct SpectrError : std::exception {
    const std::string msg;
    const size_t start;
    const size_t length;

    SpectrError(const std::string& msg, size_t start, size_t length) :
        msg(msg), start(start), length(length) {};
};

struct LexerError : SpectrError {
    using SpectrError::SpectrError;
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
    X(":",   Colon,        Colon,        3,  3 ) \
    X("->",  RightArrow,   Map,          2,  2 )

// keyword, TokenType, literal Expr type, Prim
// X(kw, tok, lit, pt)
#define PRIMITIVE_TYPES          \
    X("void", VOID, VoidExpr,    Void) \
    X("bool", BOOL, BooleanExpr, Bool) \
    X("int",  INT,  IntExpr,     Int ) \
    X("num",  NUM,  NumExpr,     Num ) \
    X("str",  STR,  StrExpr,     Str )

enum class TokenType {

//  Values

    Identifier,
    Nil,
    True,
    False,

//  Literals

    NumLiteral,
    IntLiteral,
    StrLiteral,

//  Primitive types

#define X(kw, tok, lit, pt) tok,
    PRIMITIVE_TYPES
#undef X

//  Binary Operators

#define X(sym, tok, op, lbp, rbp) tok,
    BINARY_OPERATORS
#undef X

//  Grouping

    LParen,
    RParen,
    LBrace,
    RBrace,
    LBracket,
    RBracket,
    
    AttrAccess,
    Indexing,
    FunApp, // function application
    Comma,

//  Statements

    IF,
    ELIF,
    ELSE,
    WHILE,
    WHERE,
    TYPE,
    INTERFACE,
    IMPL,
    RETURN,

//  Assignment and typing (also affect statement shape)

    Assign,
    TypeInferredAssign,
    ReferenceAssign,
    LazyAssign,
    MemoLazyAssign,
    PlusAssign,
    MinusAssign,
    StarAssign,
    SlashAssign,
    DoubleSlashAssign,
    TypeMarker,
    Union,
    Maybe,
    Reference,

    LineBreak
};

constexpr std::array<TokenType, 5> primTypes = {
#define X(kw, tok, lit, pt) TokenType::tok,
    PRIMITIVE_TYPES
#undef X
};

constexpr std::array<TokenType, 15> binaryOps = {
#define X(sym, tok, op, lbp, rbp) TokenType::tok,
    BINARY_OPERATORS
#undef X
};

constexpr std::array<std::pair<std::string_view, TokenType>, 17> keywords = {{
#define X(kw, tok, lit, pt) {kw, TokenType::tok},
    PRIMITIVE_TYPES
#undef X
    {"nil",       TokenType::Nil},
    {"true",      TokenType::True},
    {"false",     TokenType::False},
    {"if",        TokenType::IF},
    {"elif",      TokenType::ELIF},
    {"else",      TokenType::ELSE},
    {"while",     TokenType::WHILE},
    {"where",     TokenType::WHERE},
    {"type",      TokenType::TYPE},
    {"interface", TokenType::INTERFACE},
    {"impl",      TokenType::IMPL},
    {"return",    TokenType::RETURN}
}};

constexpr TokenType wordToTokenType(std::string_view s) {
    for (auto [kw, tt] : keywords) {
        if (kw == s)
            return tt;
    }

    return TokenType::Identifier;
}

constexpr std::array<std::pair<std::string_view, TokenType>, 35> symbols = {{
    {"(",    TokenType::LParen},
    {")",    TokenType::RParen},
    {"[",    TokenType::LBracket},
    {"]",    TokenType::RBracket},
    {"{",    TokenType::LBrace},
    {"}",    TokenType::RBrace},
    {"=",    TokenType::Assign},
    {":=",   TokenType::TypeInferredAssign},
    {"&=",   TokenType::ReferenceAssign},
    {"::=",  TokenType::LazyAssign},
    {":=:",  TokenType::MemoLazyAssign},
    {",",    TokenType::Comma},

#define X(sym, tok, op, lbp, rbp) {sym, TokenType::tok},
    BINARY_OPERATORS
#undef X

    {"+=",   TokenType::PlusAssign},
    {"-=",   TokenType::MinusAssign},
    {"*=",   TokenType::StarAssign},
    {"/=",   TokenType::SlashAssign},
    {"//=",  TokenType::DoubleSlashAssign},
    {"|",    TokenType::Union},
    {"?",    TokenType::Maybe},
    {"&",    TokenType::Reference}
}};

constexpr std::optional<TokenType> checkSymbol(std::string_view s) {
    for (auto [sym, tt] : symbols) {
        if (sym == s)
            return tt;
    }

    return std::nullopt;
};

struct Token {
    TokenType type;
    std::string text;
    size_t index;

    std::string show() const {
        if (type == TokenType::LineBreak)  return " ";
        if (type == TokenType::StrLiteral) return "\"" + text + "\"";
        return text;
    }
};

class Lexer {
// The Lexer transforms raw code into tokens comprehensible to the Parser

public:
    void tokenize(const std::string& input, std::vector<Token>& tokens, std::vector<size_t>& lineOffsets);
    Lexer(const std::string& path) : path(path) {};

private:
    const std::string& path;
    std::string code;
    std::vector<Token>* tokens;
    size_t index  = 0;
    size_t line   = 1;
    size_t column = 1;
    bool insideBlockComment = false;

    void advance();
    void skipWhitespace();
    bool skipComments();
    void tokenizeNumber();
    void tokenizeString();
    void tokenizeWord();
    void tokenizeSymbol();
};