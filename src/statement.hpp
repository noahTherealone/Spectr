#pragma once

#include <vector>
#include <utility>
#include <memory>
#include <string>
#include <optional>
#include "name_resolution.hpp"

struct Expr;
struct IdentifierExpr;
struct TypeExpr;
struct BlockExpr;

const std::string stmtColor = "\033[94m";

struct Stmt {
    virtual ~Stmt() = default;
    virtual std::string show() const = 0;

    size_t start()  const { return start_; }
    size_t length() const { return length_; }

protected:
    size_t start_;
    size_t length_;

    Stmt(size_t start, size_t length) : start_(start), length_(length) {}
};

using IfChain = std::vector<std::pair<std::unique_ptr<Expr>, std::unique_ptr<BlockExpr>>>;
struct IfStmt : Stmt {
    IfChain cases;
    std::unique_ptr<BlockExpr> elseCase;
    std::string show() const override;

    IfStmt(IfChain cases, std::unique_ptr<BlockExpr> elseCase, size_t start, size_t length);
};

struct VarDeclStmt : Stmt {
    std::unique_ptr<VarDecl> decl;
    std::unique_ptr<IdentifierExpr> lhs;
    std::unique_ptr<TypeExpr> type;
    std::unique_ptr<Expr> value;
    std::string show() const override;

    VarDeclStmt(std::unique_ptr<IdentifierExpr> lhs, std::unique_ptr<TypeExpr> type, std::unique_ptr<Expr> value);
};

struct TypeInferredDeclStmt : Stmt {
    std::unique_ptr<Expr> lhs;
    std::unique_ptr<Expr> value;
    std::string show() const override;

    TypeInferredDeclStmt(std::unique_ptr<Expr> lhs, std::unique_ptr<Expr> value);
};

struct ReferenceDeclStmt : Stmt {
    std::unique_ptr<Expr> lhs;
    std::unique_ptr<Expr> value;
    std::string show() const override;

    ReferenceDeclStmt(std::unique_ptr<Expr> lhs, std::unique_ptr<Expr> value);
};

struct AssignmentStmt : Stmt {
    std::unique_ptr<Expr> lhs;
    std::unique_ptr<Expr> value;
    std::string show() const override;

    AssignmentStmt(std::unique_ptr<Expr> lhs, std::unique_ptr<Expr> value);
};

struct TypeDeclStmt : Stmt {
    std::string* name;
};

struct ReturnStmt : Stmt {
    std::unique_ptr<Expr> value;
    std::string show() const override;

    ReturnStmt(std::unique_ptr<Expr> value, size_t start);
};

struct ExprStmt : Stmt {
    std::unique_ptr<Expr> expr;
    std::string show() const override;

    ExprStmt(std::unique_ptr<Expr> expr);
};