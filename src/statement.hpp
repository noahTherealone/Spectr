#pragma once

#include <vector>
#include <utility>
#include <memory>
#include <string>
#include <optional>

struct Expr;
struct IdentifierExpr;
struct BlockExpr;
struct TypeExpr;
struct NamedTypeExpr;

const std::string stmtColor = "\033[94m";

class StmtVisitor;

struct Stmt {
    virtual ~Stmt() = default;
    virtual std::string show() const = 0;

    size_t start()  const { return start_; }
    size_t length() const { return length_; }

    virtual void accept(class StmtVisitor& visitor) = 0;

protected:
    size_t start_;
    size_t length_;

    Stmt(size_t start, size_t length) : start_(start), length_(length) {}
};

struct IfStmt;
struct VarDeclStmt;
struct ReferenceDeclStmt;
struct AssignmentStmt;
struct AliasDeclStmt;
struct ReturnStmt;
struct ExprStmt;

class StmtVisitor {
public:
    virtual ~StmtVisitor() = default;

    virtual void visit(IfStmt& stmt) = 0;
    virtual void visit(VarDeclStmt& stmt) = 0;
    virtual void visit(ReferenceDeclStmt& stmt) = 0;
    virtual void visit(AssignmentStmt& stmt) = 0;
    virtual void visit(AliasDeclStmt& stmt) = 0;
    virtual void visit(ReturnStmt& stmt) = 0;
    virtual void visit(ExprStmt& stmt) = 0;
};

using IfChain = std::vector<std::pair<std::unique_ptr<Expr>, std::unique_ptr<BlockExpr>>>;
struct IfStmt : Stmt {
    IfChain cases;
    std::unique_ptr<BlockExpr> elseCase;
    std::string show() const override;

    void accept(class StmtVisitor& visitor) { visitor.visit(*this); }
    IfStmt(IfChain cases, std::unique_ptr<BlockExpr> elseCase, size_t start, size_t length);
};

struct VarDecl;

struct VarDeclStmt : Stmt {
    VarDecl* decl;
    std::unique_ptr<IdentifierExpr> lhs;
    std::unique_ptr<TypeExpr> type;
    std::unique_ptr<Expr> value;
    std::string show() const override;

    void accept(class StmtVisitor& visitor) { visitor.visit(*this); }
    VarDeclStmt(std::unique_ptr<IdentifierExpr> lhs, std::unique_ptr<TypeExpr> type, std::unique_ptr<Expr> value);
};

struct ReferenceDeclStmt : Stmt {
    std::unique_ptr<Expr> lhs;
    std::unique_ptr<Expr> value;
    std::string show() const override;

    void accept(class StmtVisitor& visitor) { visitor.visit(*this); }
    ReferenceDeclStmt(std::unique_ptr<Expr> lhs, std::unique_ptr<Expr> value);
};

struct AssignmentStmt : Stmt {
    std::unique_ptr<Expr> lhs;
    std::unique_ptr<Expr> value;
    std::string show() const override;

    void accept(class StmtVisitor& visitor) { visitor.visit(*this); }
    AssignmentStmt(std::unique_ptr<Expr> lhs, std::unique_ptr<Expr> value);
};

struct AliasDeclStmt : Stmt {
    std::unique_ptr<NamedTypeExpr> name;
    std::unique_ptr<TypeExpr> value;
    std::string show() const override;

    void accept(class StmtVisitor& visitor) { visitor.visit(*this); }
    AliasDeclStmt(std::unique_ptr<NamedTypeExpr> name, std::unique_ptr<TypeExpr> value);
};

struct ReturnStmt : Stmt {
    std::unique_ptr<Expr> value;
    std::string show() const override;

    void accept(class StmtVisitor& visitor) { visitor.visit(*this); }
    ReturnStmt(std::unique_ptr<Expr> value, size_t start);
};

struct ExprStmt : Stmt {
    std::unique_ptr<Expr> expr;
    std::string show() const override;

    void accept(class StmtVisitor& visitor) { visitor.visit(*this); }
    ExprStmt(std::unique_ptr<Expr> expr);
};