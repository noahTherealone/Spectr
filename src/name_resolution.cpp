#include <iostream>
#include <algorithm>
#include <assert.h>
#include "name_resolution.hpp"
#include "expression.hpp"

void NameResolver::pushScope() {
    scopes.push_back(std::make_unique<Scope>(currentScope));
    currentScope = scopes.back().get();
    depth++;
}

void NameResolver::popScope() {
    assert(!scopes.empty());
    assert(currentScope == scopes.back().get());

    currentScope = currentScope->parent;
    scopes.pop_back();
    depth--;
}

void NameResolver::resolveAST(const std::vector<std::unique_ptr<Stmt>>& ast) {
    for (const auto& stmt : ast) {
        try {
            stmt->accept(*this);
        }
        catch (const NameError& err) {
            auto it = std::upper_bound(offsets.begin(), offsets.end(), err.start);
            size_t line = it - offsets.begin() - 1;
            size_t column = err.start - offsets[line];
            std::cout << "\033[31mNameError at " + path + " (" + std::to_string(line+1) + ":" + std::to_string(column+1) + "): " + err.msg + "\033[0m\n";
        }
    }
}

void NameResolver::message(const std::string& msg) const {
    for (size_t i = 0; i < depth; i++)
        std::cout << "  ";
    
    std::cout << msg + "\n";
}

#pragma region Statement visitors

void NameResolver::visit(IfStmt& stmt) {
    for (auto& c : stmt.cases) {
        
    }
}

void NameResolver::visit(VarDeclStmt& stmt) {
    if (stmt.value) {
        stmt.value->accept(*this);
    }

    bool shadows = currentScope->lookup(stmt.lhs->name);

    VarDecl* decl = declare<VarDecl>(
        stmt.lhs->name,
        stmt.lhs->name,
        stmt.lhs->start(),
        stmt.value->start() - stmt.lhs->start() + stmt.value->length()
    );

    message((shadows ? "~>" : "-> ") + decl->name);
}

void NameResolver::visit(ReferenceDeclStmt& stmt) {

}

void NameResolver::visit(AssignmentStmt& stmt) {

}

void NameResolver::visit(TypeDeclStmt& stmt) {

}

void NameResolver::visit(ReturnStmt& stmt) {

}

void NameResolver::visit(ExprStmt& stmt) {
    stmt.expr->accept(*this);
}

#pragma endregion

#pragma region Expression visitors

void NameResolver::visit(IdentifierExpr& expr) {
    Decl* decl = currentScope->lookup(expr.name);
    if (!decl)
        throw NameError("Undeclared identifier '" + expr.name + "'", expr.start(), expr.length());
    
    if (auto varDecl = dynamic_cast<VarDecl*>(decl)) {
        expr.decl = varDecl;
        message(" - " + varDecl->name);
    }
    else {
        throw NameError("Identifier '" + expr.name + "' is not a variable", expr.start(), expr.length());
    }
}

void NameResolver::visit(AttributeExpr& expr) {

}

void NameResolver::visit(VoidExpr& expr) {

}

void NameResolver::visit(BooleanExpr& expr) {

}

void NameResolver::visit(IntExpr& expr) {

}

void NameResolver::visit(NumExpr& expr) {

}

void NameResolver::visit(StrExpr& expr) {

}

void NameResolver::visit(BinaryExpr& expr) {
    expr.left->accept(*this);
    expr.right->accept(*this);
}

void NameResolver::visit(TernaryExpr& expr) {
    expr.primary->accept(*this);
    expr.condition->accept(*this);
    expr.alternative->accept(*this);
}

void NameResolver::visit(TupleExpr& expr) {
    for (auto& elem : expr.exprns) {
        elem->accept(*this);
    }
}

void NameResolver::visit(BlockExpr& expr) {
    pushScope();
    resolveAST(expr.stmts);
    popScope();
}

void NameResolver::visit(ParamsExpr& expr) {

}

void NameResolver::visit(LambdaExpr& expr) {

}

#pragma endregion