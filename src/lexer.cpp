#include <iostream>
#include "lexer.hpp"

void Lexer::skipWhitespace() {
    while (index < code.length() && std::isspace(code[index]) && code[index] != '\n')
        advance();
}

bool Lexer::skipComments() {
    if (code[index] != '#')
        return false;
    
    advance();
    if (index < code.length()) {
        if (code[index] == '[') { // Comment block
            while (index < code.length()) {
                if (code[index] == ']') {
                    advance();
                    if (index < code.length() && code[index] == '#') {
                        advance();
                        return true;
                    }
                }

                advance();
            }
        } else { // Single line comment
            while (index < code.length()) {
                if (code[index] == '\n') {
                    advance();
                    return true;
                }

                advance();
            }
        }
    }

    return true;
}

void Lexer::advance() {
    if (code[index] == '\n') {
        ++line;
        column = 1;
    } else
        ++column;
    
    ++index;
}

void Lexer::tokenizeNumber() {
    size_t start = index;
    size_t startCol = column;

    bool hasDot = false;

    while (index < code.length() && !std::isspace(code[index])) {
        if (std::isdigit(code[index])) {
            advance();
            continue;
        }

        if (code[index] == '.') {
            if (hasDot)
                throw LexerError("Unexpected second dot inside numeric literal", index, 1);
            
            hasDot = true;
            advance();
            continue;
        }

        if (code[index] == 'i') {
            advance();

            if (hasDot)
                throw LexerError("Integer suffix is invalid for numeric literals with a decimal point", index-1, 1);

            if (std::isalpha(code[index])) {
                size_t first = index;
                while (std::isalpha(code[index])) advance();
                throw LexerError("Unexpected letters after integer literal", first, index-first);
            }

            tokens->push_back(Token{ TokenType::IntLiteral, code.substr(start, index-start), start });
            return;
        }

        if (std::isalpha(code[index])) {
            size_t first = index;
            while (std::isalpha(code[index])) advance();
            throw LexerError("Unexpected letters after numeric literal", first, index-first);
        }
        
        break;
    }

    tokens->push_back(Token{ TokenType::NumLiteral, code.substr(start, index-start), start });
}

void Lexer::tokenizeString() {
    size_t start = index;
    size_t startCol = column;

    advance();

    while (index < code.length() && code[index] != '\n') {
        if (code[index] == '"') {
            tokens->push_back(Token{ TokenType::StrLiteral, code.substr(start+1, index-start-1), start });
            advance();
            return;
        }

        advance();
    }

    throw LexerError("Unclosed string literal", index, 0);
}

void Lexer::tokenizeWord() {
    size_t start = index;
    size_t startCol = column;

    while (index < code.length() && std::isalnum(code[index])) {
        advance();
    }

    std::string_view word = code.substr(start, index-start);

    tokens->push_back(Token{ wordToTokenType(word), code.substr(start, index-start), start });

    if (index + 1 < code.length() && code[index] == ':' && code[index+1] == ' ') {
        tokens->push_back(Token{ TokenType::TypeMarker, ": ", index });
        advance();
        advance();
    }
}

void Lexer::tokenizeSymbol() {
    size_t start    = index;
    size_t startCol = column;

    while (index < code.length() && !std::isalnum(code[index]) && !std::isspace(code[index])) {
        advance();
    }

    std::string_view symbol = code.substr(start, index-start);
    
    for (size_t l = symbol.length(); l > 0; l--) {
        std::string_view subsymbol = symbol.substr(0, l);
        if (auto type = checkSymbol(subsymbol)) {
            tokens->push_back(Token{ *type, std::string(subsymbol), start });
            index = start + l;
            return;
        }
    }

    throw LexerError("Unknown symbol \"" + std::string(symbol) + "\"", start, index-start);
}

void Lexer::tokenize(const std::string& input, std::vector<Token>& tokens, std::vector<size_t>& lineOffsets) {
    code   = input;
    index  = 0;
    line   = 1;
    column = 1;
    this->tokens = &tokens;
    lineOffsets = {0};
    
    while (index < code.length()) {
        try {
            skipWhitespace();
            if (skipComments()) continue;

            char c = code[index];

            if (std::isdigit(c) || c == '.')
                tokenizeNumber();
            else if (c == '"')
                tokenizeString();
            else if (std::isalpha(c))
                tokenizeWord();
            else if (c == '\n') {
                tokens.push_back(Token{ TokenType::LineBreak, "\n", index });
                lineOffsets.push_back(index+1);
                advance();
            } else
                tokenizeSymbol();
                
        } catch (LexerError err) {
            std::cout << "LexerError at " + path + "(" + std::to_string(err.start) + "): " + err.msg + "\n";
        }
    }
}