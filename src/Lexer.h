#pragma once

#include <string>
#include <vector>

enum class TokenType {
    Number,
    Integer,
    String,
    Identifier,
    TRUE,
    FALSE,
    Type,
    Label,
    Plus,
    Minus,
    Star,
    Slash,
    Colon,
    At,
    LParen,
    RParen,
    LBrace,
    RBrace,
    LBracket,
    RBracket,
    Dollar,
    Comma,
    Vert,
    Arrow,
    Out,
    //NUM,
    //SIGNAL,
    //SPECTR,
    //OSC,
    EAssign,
    LAssign,
    Sine,
    Square,
    Saw,
    PLAY,
    HOLD,
    RELEASE,
    LnBreak,
};

constexpr std::string_view view(TokenType type) {
    switch (type) {
        case TokenType::Number:     return "<n>";
        case TokenType::Integer:    return "<i>";
        case TokenType::String:     return "<str>";
        case TokenType::Identifier: return "<id>";
        case TokenType::TRUE:       return "true";
        case TokenType::FALSE:      return "false";
        case TokenType::Type:       return "<T>";
        case TokenType::Label:      return "<'l>";
        case TokenType::Plus:       return "+";
        case TokenType::Minus:      return "-";
        case TokenType::Star:       return "*";
        case TokenType::Slash:      return "/";
        case TokenType::Colon:      return ":";
        case TokenType::At:         return "@";
        case TokenType::EAssign:    return "=";
        case TokenType::LAssign:    return ":=";
        case TokenType::LParen:     return "(";
        case TokenType::RParen:     return ")";
        case TokenType::LBrace:     return "{";
        case TokenType::RBrace:     return "}";
        case TokenType::LBracket:   return "[";
        case TokenType::RBracket:   return "]";
        case TokenType::Dollar:     return "$";
        case TokenType::Comma:      return ",";
        case TokenType::Vert:       return "|";
        case TokenType::Arrow:      return "->";
        case TokenType::Out:        return "[out]";
        //case TokenType::NUM:        return "$num";
        //case TokenType::SIGNAL:     return "$signal";
        //case TokenType::SPECTR:     return "$spectr";
        //case TokenType::OSC:        return "$osc";
        case TokenType::Sine:       return "~sin~";
        case TokenType::Square:     return "~sqr~";
        case TokenType::Saw:        return "~saw~";
        case TokenType::PLAY:       return "|P>";
        case TokenType::HOLD:       return "|H>";
        case TokenType::RELEASE:    return "|R>";
        case TokenType::LnBreak:    return ";";
        default:                    return "<?>"; // fallback
    }
}

struct Token {
    TokenType type;
    std::string text;
    size_t line, column, pos;

    std::string to_string() {
        return "\033[30m[\033[0m" + std::string(view(type)) + "\033[30m]\033[0m";
    }
};

class Lexer {
public:
    std::vector<Token> tokenize(const std::string& input);

private:
    std::string str;
    size_t pos = 0, line = 1, column = 1;
    bool inBlockComment = false;

    void advance();
    void skipWhitespace();
    bool skipComments();
    Token tokenizeNumber();
    Token tokenizeString();
    Token tokenizeName();
    Token tokenizeLabel();
    Token makeToken(TokenType type, std::string text);
};