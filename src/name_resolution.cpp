#include <iostream>
#include <algorithm>
#include "name_resolution.hpp"
#include "expression.hpp"

void NameResolver::pushScope() {
    currentScope = new Scope(currentScope);
}

void NameResolver::popScope() {
    currentScope = currentScope->parent;
}

void NameResolver::resolveAST(const std::vector<std::unique_ptr<Stmt>>& ast, const std::string& path, const std::vector<size_t>& offsets) {

    std::cout << "\n\033[1m\033[36mName resolution>\033[0m\n";

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

#pragma region Statement visitors

void NameResolver::visit(IfStmt& stmt) {
    for (auto& c : stmt.cases) {
        
    }
}

void NameResolver::visit(VarDeclStmt& stmt) {
    if (stmt.value) {
        stmt.value->accept(*this);
    }

    VarDecl* decl = declare<VarDecl>(
        stmt.lhs->name,
        stmt.lhs->name,
        stmt.lhs->start(),
        stmt.value->start() - stmt.lhs->start() + stmt.value->length()
    );

    std::cout << ">> " + decl->name + "\n";
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

}

#pragma endregion

#pragma region Expression visitors

void NameResolver::visit(IdentifierExpr& stmt) {

}

void NameResolver::visit(AttributeExpr& stmt) {

}

void NameResolver::visit(VoidExpr& stmt) {

}

void NameResolver::visit(BooleanExpr& stmt) {

}

void NameResolver::visit(IntExpr& stmt) {

}

void NameResolver::visit(NumExpr& stmt) {

}

void NameResolver::visit(StrExpr& stmt) {

}

void NameResolver::visit(BinaryExpr& stmt) {

}

void NameResolver::visit(TernaryExpr& stmt) {

}

void NameResolver::visit(TupleExpr& stmt) {

}

void NameResolver::visit(BlockExpr& stmt) {

}

void NameResolver::visit(ParamsExpr& stmt) {

}

void NameResolver::visit(LambdaExpr& stmt) {

}

#pragma endregion