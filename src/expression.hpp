#pragma once

#include <string>
#include <memory>
#include <charconv>
#include "type.hpp"
#include "lexer.hpp"
//#include "name_resolution.hpp"

const std::string exprColor = "\033[36m";

enum class TokenType;
struct Token;
struct SyntaxError;

class ExprVisitor;

struct Expr {
    virtual ~Expr() = default;
    virtual std::string show() const = 0;

    size_t start()  const { return start_; }
    size_t length() const { return length_; }

    virtual void accept(ExprVisitor& v) = 0;

protected:
    size_t start_;
    size_t length_;

    Expr(size_t start, size_t length) : start_(start), length_(length) {}
    Expr(const Token& tok) : Expr(tok.index, tok.text.length()) {}
};

struct IdentifierExpr;
struct AttributeExpr;
struct VoidExpr;
struct BooleanExpr;
struct IntExpr;
struct NumExpr;
struct StrExpr;
struct BinaryExpr;
struct TernaryExpr;
struct ListExpr;
struct TupleExpr;
struct BlockExpr;
struct LambdaExpr;
struct ApplExpr;

class ExprVisitor {
public:
    virtual ~ExprVisitor() = default;

    virtual void visit(IdentifierExpr& expr) = 0;
    virtual void visit(AttributeExpr& expr)  = 0;
    virtual void visit(VoidExpr& expr)       = 0;
    virtual void visit(BooleanExpr& expr)    = 0;
    virtual void visit(IntExpr& expr)        = 0;
    virtual void visit(NumExpr& expr)        = 0;
    virtual void visit(StrExpr& expr)        = 0;
    virtual void visit(BinaryExpr& expr)     = 0;
    virtual void visit(TernaryExpr& expr)    = 0;
    virtual void visit(ListExpr& expr)       = 0;
    virtual void visit(TupleExpr& expr)      = 0;
    virtual void visit(BlockExpr& expr)      = 0;
    virtual void visit(LambdaExpr& expr)     = 0;
    virtual void visit(ApplExpr& expr)       = 0;
};

struct VarDecl;

struct IdentifierExpr : Expr {
    std::string name;
    VarDecl* decl;
    std::string show() const override {
        return name;
    }

    void accept(ExprVisitor& v) override { v.visit(*this); }

    IdentifierExpr(const Token& tok) : Expr(tok), name(tok.text) {}
    IdentifierExpr(size_t start, size_t length) : Expr(start, length) {}
};

struct AttributeExpr : Expr {
    std::unique_ptr<Expr> base;
    std::string name;
    std::string show() const override {
        return base->show() + "." + name;
    }

    void accept(ExprVisitor& v) override { v.visit(*this); }
    
    AttributeExpr(std::unique_ptr<Expr> base, const Token& tok) :
        Expr(tok.index-1, tok.text.length()+1),
        base(std::move(base)) {}
};

// Literals (once actual Spectr values exist, these may or may not be unified to LiteralExpr and hold that value already)

struct VoidExpr : Expr {
    std::string show() const override {
        return "nil";
    }

    void accept(ExprVisitor& v) override { v.visit(*this); }
    
    VoidExpr(const Token& tok) : Expr(tok) {}
    VoidExpr(size_t start, size_t length) : Expr(start, length) {}
};

struct BooleanExpr : Expr {
    bool value;
    std::string show() const override {
        return value ? "true" : "false";
    }

    void accept(ExprVisitor& v) override { v.visit(*this); }
    
    BooleanExpr(const Token& tok) : Expr(tok) {
        if (tok.type == TokenType::True)
            value = true;
        else if (tok.type == TokenType::False)
            value = false;
        else
            throw std::exception();
    }
};

struct IntExpr : Expr {
    int value;
    std::string show() const override {
        return std::to_string(value) + "i";
    }

    void accept(ExprVisitor& v) override { v.visit(*this); }
    
    IntExpr(const Token& tok) : Expr(tok) {
        std::string_view ss = tok.text.substr(0, tok.text.length() - 1);
        auto res = std::from_chars(ss.data(), ss.data() + ss.size(), value);
        if (res.ec != std::errc{}) {
            throw std::exception();
        }
    }
};

struct NumExpr : Expr {
    double value;
    std::string show() const override {
        return std::to_string(value);
    }

    void accept(ExprVisitor& v) override { v.visit(*this); }
    
    NumExpr(const Token& tok): Expr(tok) {
        auto res = std::from_chars(tok.text.data(), tok.text.data() + tok.text.size(), value);
        if (res.ec != std::errc{}) {
            throw std::exception();
        }
    }
};

struct StrExpr : Expr {
    std::string value;
    std::string show() const override {
        return "\"" + value + "\"";
    }

    void accept(ExprVisitor& v) override { v.visit(*this); }
    
    StrExpr(const Token& tok) :
        Expr(tok.index, tok.text.length() + 2), // because quotation marks are missing from the token text
        value(tok.text) {}
};

enum class BinaryOperator {
    NOT_AN_OPERATOR,
    
#define X(sym, tok, op, lbp, rbp) op,
    BINARY_OPERATORS
#undef X
};

constexpr BinaryOperator binaryOpFromToken(TokenType t) {
    switch (t) {
#define X(sym, tok, op, lbp, rbp) case TokenType::tok: return BinaryOperator::op;
        BINARY_OPERATORS
#undef X

        default: return BinaryOperator::NOT_AN_OPERATOR;
    }
}

inline std::string showBinaryOp(BinaryOperator op) {
    switch (op) {
#define X(sym, tok, op, lbp, rbp) case BinaryOperator::op: return sym;
        BINARY_OPERATORS
#undef X

        default: return "<!!!>";
    }
}

struct BinaryExpr : Expr {
    BinaryOperator op;
    std::unique_ptr<Expr> left;
    std::unique_ptr<Expr> right;
    std::string show() const override {
        return exprColor + "(\033[0m" + left->show() + exprColor + " " + showBinaryOp(op) + "\033[0m " + right->show() + exprColor + ")\033[0m";
    }

    void accept(ExprVisitor& v) override { v.visit(*this); }
    
    BinaryExpr(std::unique_ptr<Expr> left, std::unique_ptr<Expr> right, const Token& tok) :
        Expr(left->start(), right->start() - left->start() + right->length()),
        op(binaryOpFromToken(tok.type)),
        left(std::move(left)),
        right(std::move(right)) {
            if (op == BinaryOperator::NOT_AN_OPERATOR)
                throw std::exception(); // this shouldn't happen, if it does, check the parser
        }
};

struct TernaryExpr : Expr {
    std::unique_ptr<Expr> primary;
    std::unique_ptr<Expr> condition;
    std::unique_ptr<Expr> alternative;
    std::string show() const override {
        return exprColor + "(\033[0m" + primary->show() + exprColor + " if\033[0m " + condition->show() + exprColor + " else\033[0m " + alternative->show() + exprColor + ")\033[0m";
    }

    void accept(ExprVisitor& v) override { v.visit(*this); }
    
    TernaryExpr(std::unique_ptr<Expr> primary, std::unique_ptr<Expr> condition, std::unique_ptr<Expr> alternative) :
        Expr(primary->start(), alternative->start() - primary->start() + alternative->length()),
        primary(std::move(primary)),
        condition(std::move(condition)),
        alternative(std::move(alternative)) {}
};

struct ListExpr : Expr {
    std::vector<std::unique_ptr<Expr>> exprns;
    std::string show() const override;

    std::vector<std::unique_ptr<Expr>> takeExprns() {
        return std::move(exprns);
    }

    void accept(ExprVisitor& v) override { v.visit(*this); }
    
    ListExpr(std::vector<std::unique_ptr<Expr>> exprns, size_t start, size_t length) :
        Expr(start, length),
        exprns(std::move(exprns)) {}
};

struct TupleExpr : Expr {
    std::vector<std::unique_ptr<Expr>> exprns;
    std::string show() const override;

    std::vector<std::unique_ptr<Expr>> takeExprns() {
        return std::move(exprns);
    }

    void accept(ExprVisitor& v) override { v.visit(*this); }
    
    TupleExpr(std::vector<std::unique_ptr<Expr>> exprns, size_t start, size_t length) :
        Expr(start, length),
        exprns(std::move(exprns)) {}
};

struct Stmt;

struct BlockExpr : Expr {
    std::vector<std::unique_ptr<Stmt>> stmts;
    std::string show() const override;

    void accept(ExprVisitor& v) override { v.visit(*this); }
    
    BlockExpr(std::vector<std::unique_ptr<Stmt>> stmts, size_t start, size_t length) :
        Expr(start, length), stmts(std::move(stmts)) {}
    
    BlockExpr(std::unique_ptr<Stmt> stmt);
};

struct TypeExpr;

struct Param {
    VarDecl* decl;
    std::unique_ptr<IdentifierExpr> id;
    std::unique_ptr<TypeExpr> type;

    Param(std::unique_ptr<IdentifierExpr> id, std::unique_ptr<TypeExpr> type);
};

struct Params {
    size_t start;
    size_t length;
    std::vector<std::unique_ptr<Param>> params;

    Params() = default;
    Params(std::unique_ptr<TupleExpr> tuple);
    Params(std::unique_ptr<IdentifierExpr> id);
    Params(std::unique_ptr<Expr> expr);
};

struct LambdaExpr : Expr {
    std::unique_ptr<Params> params;
    std::unique_ptr<BlockExpr> body;
    std::string show() const override;

    void accept(ExprVisitor& v) override { v.visit(*this); }
    
    LambdaExpr(std::unique_ptr<Params> params, std::unique_ptr<BlockExpr> body);
    LambdaExpr(std::unique_ptr<TupleExpr> params, std::unique_ptr<BlockExpr> body);
};

struct ApplExpr : Expr {
    std::unique_ptr<Expr> fun;
    std::unique_ptr<Expr> arg;
    std::string show() const override;

    void accept(ExprVisitor& v) override { v.visit(*this); }

    ApplExpr(std::unique_ptr<Expr> fun, std::unique_ptr<Expr> arg);
};