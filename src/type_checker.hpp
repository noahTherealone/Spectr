#pragma once

#include "statement.hpp"
#include "expression.hpp"
#include "type_expression.hpp"
#include "type.hpp"

struct TypeError;

class TypeChecker : public StmtVisitor, public ExprVisitor, public TypeExprVisitor {
public:
    void typeCheckAST(const std::vector<std::unique_ptr<Stmt>>& stmts);
    explicit TypeChecker(const std::string& path, const std::vector<size_t>& offsets) :
        path(path), offsets(offsets) {}

private:
    const std::string& path;
    const std::vector<size_t>& offsets;

    TypePtr expected;
    TypePtr result;

    void message(const std::string& msg) const;

    void visit(IfStmt& stmt) override;
    void visit(VarDeclStmt& stmt) override;
    void visit(ReferenceDeclStmt& stmt) override;
    void visit(AssignmentStmt& stmt) override;
    void visit(AliasDeclStmt& stmt) override;
    void visit(ReturnStmt& stmt) override;
    void visit(ExprStmt& stmt) override;
    TypePtr visit(Stmt* stmt); //may return nullptr
    TypePtr visit(Stmt* stmt, TypePtr _expected);
    bool typeCheck(std::vector<std::unique_ptr<Stmt>> stmt, TypePtr _expected);

    void visit(IdentifierExpr& expr) override;
    void visit(AttributeExpr& expr) override;
    void visit(VoidExpr& expr) override;
    void visit(BooleanExpr& expr) override;
    void visit(IntExpr& expr) override;
    void visit(NumExpr& expr) override;
    void visit(StrExpr& expr) override;
    void visit(BinaryExpr& expr) override;
    void visit(TernaryExpr& expr) override;
    void visit(ListExpr& expr) override;
    void visit(TupleExpr& expr) override;
    void visit(BlockExpr& expr) override;
    void visit(LambdaExpr& expr) override;
    void visit(ApplExpr& expr) override;
    TypePtr visit(Expr* expr); //must always return a valid TypePtr or throw
    TypePtr visit(Expr* expr, TypePtr _expected);
    bool typeCheck(Expr* expr, TypePtr _expected);

    void visit(PrimTypeExpr& expr) override;
    void visit(AnyTypeExpr& expr) override;
    void visit(NamedTypeExpr& expr) override;
    void visit(ListTypeExpr& expr) override;
    void visit(TupleTypeExpr& expr) override;
    void visit(UnionTypeExpr& expr) override;
    void visit(LambdaTypeExpr& expr) override;
    TypePtr visit(TypeExpr* expr);
};