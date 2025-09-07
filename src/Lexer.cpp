#include "Lexer.h"
#include "Types.h"
#include <iostream>
#include <cctype>

void Lexer::advance() {
    if (str[pos] == '\n') {
        ++line;
        column = 1;
    } else {
        ++column;
    }

    ++pos;
}

void Lexer::skipWhitespace() {
    while (pos < str.length() && std::isspace((unsigned char)str[pos]) && str[pos] != '\n') {
        advance();
    }
}

bool Lexer::skipComments() {
    if (inBlockComment) {
        while (pos < str.length()) {
            if (str[pos] == '#' && pos + 1 < str.length() && str[pos + 1] == '#') {
                advance(); // skip '#'
                advance(); // skip '#'
                inBlockComment = false;
                break;
            }
            advance();
        }
        return true;
    }

    if (str[pos] == '#') {
        if (pos + 1 < str.length() && str[pos + 1] == '#') {
            inBlockComment = true;
            advance();
            advance();
            skipComments();
            return true;
        }

        while (pos < str.length()) {
            advance();
            if (str[pos] == '\n')
                break;
        }

        advance();
        return true;
    }

    return false;
}

Token Lexer::tokenizeNumber() {
    size_t start = pos;
    size_t startCol = column;
    while (pos < str.length() && (std::isdigit(str[pos]) || str[pos] == '.' || str[pos] == 'i')) {
        advance();
        if (str[pos-1] == 'i')
            return Token{ TokenType::Integer, str.substr(start, pos-1 - start), line, startCol, start };
    }

    return Token{ TokenType::Number, str.substr(start, pos - start), line, startCol, start };
}

Token Lexer::tokenizeString() {
    size_t start = pos;
    size_t startCol = column;
    while (pos < str.length() && (str[pos] != '"')) {
        advance();
    }

    return Token{ TokenType::String, str.substr(start, pos - start), line, startCol, start };
}

Token Lexer::tokenizeName() {
    size_t start = pos;
    size_t startCol = column;
    while (pos < str.length() && (std::isalnum(str[pos]) || str[pos] == '_')) {
        advance();
    }

    std::string name = str.substr(start, pos - start);
    if (namedTypes.contains(name))
        return Token{ TokenType::Type, name, line, startCol, start };
    else if (name == "true")
        return Token{ TokenType::TRUE, name, line, startCol, start };
    else if (name == "false")
        return Token{ TokenType::FALSE, name, line, startCol, start };
    else if (name == "sin")
        return Token{ TokenType::Sine, name, line, startCol, start };
    else if (name == "sqr")
        return Token{ TokenType::Square, name, line, startCol, start };
    else if (name == "saw")
        return Token{ TokenType::Saw, name, line, startCol, start };
    else if (name == "play")
        return Token{ TokenType::PLAY, name, line, startCol, start };
    else if (name == "hold")
        return Token{ TokenType::HOLD, name, line, startCol, start };
    else if (name == "release")
        return Token{ TokenType::RELEASE, name, line, startCol, start };
    else if (name == "out")
        return Token{ TokenType::Out, name, line, startCol, start };

    return Token{ TokenType::Identifier, name, line, startCol, start };
}

Token Lexer::tokenizeLabel() {
    size_t start = pos;
    size_t startCol = column;
    // Dots . are not allowed, they are reserved for automated labelling, e.g. for midi labels
    while (pos < str.length() && (std::isalnum(str[pos]) || str[pos] == '_')) {
        advance();
    }

    std::string name = str.substr(start, pos - start);
    return Token{ TokenType::Label, name, line, startCol, start };
}

Token Lexer::makeToken(TokenType type, std::string text) {
    return Token{ type, text, line, column, pos };
}

std::vector<Token> Lexer::tokenize(const std::string& input) {
    str = input;
    pos = 0;
    line = 1;
    column = 1;

    std::vector<Token> tokens;
    while (pos < str.length()) {
        skipWhitespace();
        if (pos >= str.length())
            break;
        
        if (skipComments())
            continue;

        if (std::isdigit(str[pos])) {
            tokens.push_back(tokenizeNumber());
        } else if (std::isalpha(str[pos])) {
            tokens.push_back(tokenizeName());
        } else if (str[pos] == '\'') {
            advance();
            tokens.push_back(tokenizeLabel());
        } else if (str[pos] == '+') {
            tokens.push_back(makeToken(TokenType::Plus, "+"));
            advance();
        } else if (str[pos] == '-') {
            if (pos+1 < str.size() && str[pos+1] == '>') {
                tokens.push_back(makeToken(TokenType::Arrow, "->"));
                advance();
                advance();
            }
            else {
                tokens.push_back(makeToken(TokenType::Minus, "-"));
                advance();
            }
        } else if (str[pos] == '*') {
            tokens.push_back(makeToken(TokenType::Star, "*"));
            advance();
        } else if (str[pos] == '/') {
            tokens.push_back(makeToken(TokenType::Slash, "/"));
            advance();
        } else if (str[pos] == ':') {
            if (pos + 1 < str.length() && str[pos + 1] == '=') {
                tokens.push_back(makeToken(TokenType::LAssign, ":="));
                advance();
                advance();
                continue;
            }

            tokens.push_back(makeToken(TokenType::Colon, ":"));
            advance();
        } else if (str[pos] == '@') {
            tokens.push_back(makeToken(TokenType::At, "@"));
            advance();
        } else if (str[pos] == '=') {
            tokens.push_back(makeToken(TokenType::EAssign, "="));
            advance();
        } else if (str[pos] == '(') {
            tokens.push_back(makeToken(TokenType::LParen, "("));
            advance();
        } else if (str[pos] == ')') {
            tokens.push_back(makeToken(TokenType::RParen, ")"));
            advance();
        } else if (str[pos] == '{') {
            tokens.push_back(makeToken(TokenType::LBrace, "{"));
            advance();
        } else if (str[pos] == '}') {
            tokens.push_back(makeToken(TokenType::RBrace, "}"));
            advance();
        } else if (str[pos] == '[') {
            if (pos+1 < str.size() && !isblank(str[pos+1])) {
                tokens.push_back(tokenizeName());
            }
            else {
                tokens.push_back(makeToken(TokenType::LBracket, "["));
                advance();
            }
        } else if (str[pos] == ']') {
            tokens.push_back(makeToken(TokenType::RBracket, "]"));
            advance();
        } else if (str[pos] == '$') {
            tokens.push_back(makeToken(TokenType::Dollar, "$"));
            advance();
        } else if (str[pos] == ',') {
            tokens.push_back(makeToken(TokenType::Comma, ","));
            advance();
        } else if (str[pos] == '|') {
            tokens.push_back(makeToken(TokenType::Vert, "|"));
            advance();
        } else if (str[pos] == '\n') {
            tokens.push_back(makeToken(TokenType::LnBreak, "\\n"));
            advance();
        } else {
            throw std::runtime_error("Unexpected character '" + std::to_string(str[pos]) + "' at (" + std::to_string(line) + ", " + std::to_string(column) + ").");
            advance();
        }
    }

    return tokens;
}