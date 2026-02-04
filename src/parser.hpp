#pragma once

#include <memory>

#include "lexer.hpp"
#include "expression.hpp"
#include "type_expression.hpp"
#include "statement.hpp"

struct SpectrError;

struct SyntaxError : SpectrError {
    using SpectrError::SpectrError;
};

class Parser {
private:
    const std::string& path;

    std::vector<Token> tokens;
    std::vector<size_t> lineOffsets;

    size_t index;
    Token* peek();
    Token* next();

    std::unique_ptr<Stmt> parseStatement();

    std::unique_ptr<Stmt> matchExplicitVarDecl(std::unique_ptr<IdentifierExpr> lhs, const Token& typeMarker);
    std::unique_ptr<Stmt> matchInferredVarDecl(std::unique_ptr<IdentifierExpr> lhs, const Token& assignmentOp);
    std::unique_ptr<Stmt> matchVarDecl(std::unique_ptr<IdentifierExpr> lhs, std::unique_ptr<TypeExpr> type, const Token& assignmentOp);
    std::unique_ptr<Stmt> matchAssignment(std::unique_ptr<Expr> lhs, const Token& assignmentSign);
    std::unique_ptr<Stmt> matchTypeStmt(const Token& tok);
    std::unique_ptr<Stmt> matchIf();
    std::unique_ptr<Stmt> matchExpr();
    std::unique_ptr<BlockExpr> matchBody();

    int rbp(TokenType type) const;
    int lbp(TokenType type) const;
    bool isApplStart(TokenType type) const;
    const int applBp = 70;
    std::unique_ptr<Expr> parseExpr() { return parseExpr(0); }
    std::unique_ptr<Expr> parseExpr(int rbp);
    std::unique_ptr<Expr> nud(const Token& tok);
    std::unique_ptr<Expr> led(std::unique_ptr<Expr> left, const Token& tok);

    std::unique_ptr<Expr> parseParen(size_t start);
    std::unique_ptr<ListExpr> parseList(size_t start);
    std::unique_ptr<LambdaExpr> parseParams(size_t start);
    std::unique_ptr<LambdaExpr> parseLambda(std::unique_ptr<Params> params);
    std::unique_ptr<BlockExpr> parseBlock(size_t start);

    std::unique_ptr<TypeExpr> parseTypeExpr() { return parseTypeExpr(0); }
    std::unique_ptr<TypeExpr> parseTypeExpr(int rbp);
    std::unique_ptr<TypeExpr> typeNud(const Token& tok);
    std::unique_ptr<TypeExpr> typeLed(std::unique_ptr<TypeExpr> left, const Token& tok);

    std::unique_ptr<TypeExpr> parseTypeParen(size_t start);
    std::unique_ptr<TypeExpr> parseLambdaType(size_t start, std::vector<std::unique_ptr<TypeExpr>> params);
    std::unique_ptr<TypeExpr> parseLambdaType(std::unique_ptr<TypeExpr> left);
    std::unique_ptr<TypeExpr> parseStructType(size_t start);

    Token& expect(TokenType type);
    void skipToLineBreak();
    void skipLineBreaks();

    std::vector<std::unique_ptr<Stmt>> parse();

public:
    std::vector<std::unique_ptr<Stmt>> parseToks(const std::vector<Token>& toks, const std::vector<size_t>& offsets);

    Parser(const std::string& path) : path(path) {}
};