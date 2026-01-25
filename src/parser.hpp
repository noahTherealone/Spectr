#pragma once

#include <memory>

#include "lexer.hpp"
#include "expression.hpp"
#include "type_expression.hpp"
#include "statement.hpp"

struct SyntaxError : SpectrError {
    using SpectrError::SpectrError;
};

class Parser {
private:
    std::string path;

    std::vector<Token> tokens;
    std::vector<size_t> lineOffsets;

    size_t index;
    Token* peek();
    Token* next();

    std::unique_ptr<Stmt> parseStatement();

    std::unique_ptr<Stmt> matchVarDecl(std::unique_ptr<Expr> lhs, const Token& typeMarker);
    std::unique_ptr<Stmt> matchAssignment(std::unique_ptr<Expr> lhs, const Token& assignmentSign);
    std::unique_ptr<Stmt> matchIf();
    std::unique_ptr<Stmt> matchExpr();
    std::unique_ptr<BlockExpr> matchBody();

    std::unique_ptr<Expr> parseExpr() { return parseExpr(0); }
    std::unique_ptr<Expr> parseExpr(int rbp);
    std::unique_ptr<Expr> nud(const Token& tok);
    std::unique_ptr<Expr> led(std::unique_ptr<Expr> left, const Token& tok);

    std::unique_ptr<Expr> parseParen(size_t start);
    std::unique_ptr<Expr> parseParams(size_t start);
    std::unique_ptr<Expr> parseLambda(std::unique_ptr<Expr> left);
    std::unique_ptr<BlockExpr> parseBlock(size_t start);

    std::unique_ptr<TypeExpr> parseTypeExpr() { return parseTypeExpr(0); }
    std::unique_ptr<TypeExpr> parseTypeExpr(int rbp);
    std::unique_ptr<TypeExpr> typeNud(const Token& tok);
    std::unique_ptr<TypeExpr> typeLed(std::unique_ptr<TypeExpr> left, const Token& tok);

    Token& expect(TokenType type);
    void skipToLineBreak();
    void skipLineBreaks();

public:
    std::vector<std::unique_ptr<Stmt>> parseCode(const std::string& code);
    std::vector<std::unique_ptr<Stmt>> parseFile(const std::string& path);
};