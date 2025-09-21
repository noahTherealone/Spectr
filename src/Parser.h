#pragma once

#include "Lexer.h"
#include "Interpreter.hpp"
#include "Types.h"
#include "LogSettings.h"
#include <memory>
#include <string>
#include <vector>
#include <iostream>

class _Interpreter;
class Interpreter;

class Parser {
private:
    _Interpreter* backend;

    std::vector<Token> tokens;
    size_t pos = 0, lastPos = 0;

    Token peek() { return tokens[pos]; }
    Token next() { return tokens[pos++]; }

    std::unique_ptr<Expr> parseExpression(int rbp = 0);
    std::unique_ptr<Type> parseTypeExpr(int rbp = 0);

    std::unique_ptr<Expr> nud(Token tok);
    std::unique_ptr<Expr> led(Token tok, std::unique_ptr<Expr> left);
    Type typeNud(Token tok);
    Type typeLed(Token tok, Type left);

    int lbp(TokenType type);
    int typeLbp(TokenType type);

    void parserError(const std::string& msg, size_t line, size_t col);

public:
    LogSettings* logSettings;

    void parseCode(const std::string& code);
    void parseFile(const std::string& path);

    Parser() = default;
    Parser(_Interpreter* b, LogSettings* ls)
        : backend(b), logSettings(ls) { }
};