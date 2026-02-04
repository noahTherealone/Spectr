#pragma once

#include <algorithm>
#include "statement.hpp"
#include "expression.hpp"
#include "type_expression.hpp"
#include "type.hpp"
#include "base.hpp"

struct SpectrError;

struct TypeError : SpectrError {
    using SpectrError::SpectrError;

    std::string show(const std::string& path, const std::vector<size_t>& offsets) const {
        auto it = std::upper_bound(offsets.begin(), offsets.end(), start);
        size_t line = it - offsets.begin() - 1;
        size_t column = start - offsets[line];
        return "\033[31mTypeError at " + sourcePos(path, offsets, start) + ": " + msg + "\033[0m\n";
    }
};

class TypeChecker : public StmtVisitor, public ExprVisitor, public TypeExprVisitor {
public:
    void typeCheckAST(const std::vector<std::unique_ptr<Stmt>>& stmts);
    explicit TypeChecker(const std::string& path, const std::vector<size_t>& offsets) :
        path(path), offsets(offsets) {}

private:
    friend class TypeError;
    friend class ExpectedGuard;

    const std::string& path;
    const std::vector<size_t>& offsets;

    TypePtr expected;
    TypePtr result;

    std::vector<TypeError> errors;

    void message(const std::string& msg) const;
    void report(const std::string& err, size_t start, size_t length);
    void report(const std::string& err, Stmt* expr);
    void report(const std::string& err, Expr* expr);
    void report(const std::string& err, TypeExpr* expr);

    void visit(IfStmt& stmt) override;
    void visit(VarDeclStmt& stmt) override;
    void visit(ReferenceDeclStmt& stmt) override;
    void visit(AssignmentStmt& stmt) override;
    void visit(AliasDeclStmt& stmt) override;
    void visit(ReturnStmt& stmt) override;
    void visit(ExprStmt& stmt) override;
    TypePtr visit(Stmt* stmt, TypePtr _expected); //may return nullptr

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
    TypePtr visit(Expr* expr, TypePtr _expected); //must always return a valid TypePtr or throw

    void visit(PrimTypeExpr& expr) override;
    void visit(AnyTypeExpr& expr) override;
    void visit(NamedTypeExpr& expr) override;
    void visit(ListTypeExpr& expr) override;
    void visit(TupleTypeExpr& expr) override;
    void visit(UnionTypeExpr& expr) override;
    void visit(LambdaTypeExpr& expr) override;
    void visit(StructTypeExpr& expr) override;
    TypePtr visit(TypeExpr* expr);
};

class ExpectedGuard {
    TypeChecker& tc;
    TypePtr saved;

public:
    ExpectedGuard(TypeChecker& tc, TypePtr expected) : tc(tc), saved(expected) {
        tc.expected = expected;
    }
    
    ~ExpectedGuard() {
        tc.expected = saved;
    }
};