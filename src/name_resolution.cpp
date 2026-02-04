#include <iostream>
#include <algorithm>
#include <assert.h>
#include "name_resolution.hpp"
#include "expression.hpp"
#include "type_expression.hpp"
#include "base.hpp"

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
            std::cout << "\033[31mNameError at " + sourcePos(path, offsets, err.start) + ": " + err.msg + "\033[0m\n";
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
        c.first->accept(*this);
        c.second->accept(*this);
    }

    if (stmt.elseCase) {
        stmt.elseCase->accept(*this);
    }
}

void NameResolver::visit(VarDeclStmt& stmt) {
    if (stmt.type) {
        stmt.type->accept(*this);
    }

    if (stmt.value) {
        stmt.value->accept(*this);
    }

    Decl* shadows = currentScope->lookup(stmt.lhs->name);
    if (shadows && dynamic_cast<TypeDecl*>(shadows)) {
        throw NameError("Cannot shadow type name '" + stmt.lhs->name + "'", stmt.start(), stmt.length());
    }

    VarDecl* decl = declare<VarDecl>(
        stmt.lhs->name,
        stmt.lhs->name,
        stmt.lhs->start(),
        stmt.value
            ? stmt.value->start() - stmt.lhs->start() + stmt.value->length()
            : stmt.type->start()  - stmt.lhs->start() + stmt.type->length()
    );

    stmt.decl = decl;
    message((shadows ? "~> " : "-> ") + decl->name);
}

void NameResolver::visit(ReferenceDeclStmt& stmt) {

}

void NameResolver::visit(AssignmentStmt& stmt) {
    stmt.lhs->accept(*this);
    stmt.value->accept(*this);
}

void NameResolver::visit(AliasDeclStmt& stmt) {
    stmt.value->accept(*this);
    
    if (currentScope->lookup(stmt.name->name))
        throw NameError("Type names may not shadow declared name '" + stmt.name->name + "'", stmt.start(), stmt.length());
    
    TypeDecl* decl = declare<TypeDecl>(
        stmt.name->name,
        stmt.name->name,
        stmt.name->start(),
        stmt.value->start() - stmt.name->start() + stmt.value->length()
    );

    stmt.name->decl = decl;

    message(primTypeColor + "-> " + decl->name + "\033[0m");
}

void NameResolver::visit(ReturnStmt& stmt) {
    stmt.value->accept(*this);
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
    expr.base->accept(*this);
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
    expr.condition->accept(*this);
    expr.primary->accept(*this);
    expr.alternative->accept(*this);
}

void NameResolver::visit(ListExpr& expr) {
    for (auto& elem : expr.exprns) {
        elem->accept(*this);
    }
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

void NameResolver::visit(LambdaExpr& expr) {
    pushScope();
    for (auto& param : expr.params->params) {
        // parameter type may not be explicit
        if (param->type)
            param->type->accept(*this);
        
        Decl* shadow = currentScope->lookup(param->id->name);
        VarDecl* decl = declare<VarDecl>(
            param->id->name,
            param->id->name,
            param->id->start(),
            param->id->length()
        );

        param->decl = decl;
        message((shadow ? "~> " : "-> ") + param->id->name);
    }
    
    resolveAST(expr.body->stmts);
    popScope();
}

void NameResolver::visit(ApplExpr& expr) {
    expr.fun->accept(*this);
    expr.arg->accept(*this);
}

#pragma endregion

#pragma region Type expression visitors

void NameResolver::visit(PrimTypeExpr& expr) {

}

void NameResolver::visit(AnyTypeExpr& expr) {

}

void NameResolver::visit(NamedTypeExpr& expr) {
    Decl* decl = currentScope->lookup(expr.name);
    if (!decl)
        throw NameError("Undeclared type name '" + expr.name + "'", expr.start(), expr.length());
    
    if (auto typeDecl = dynamic_cast<TypeDecl*>(decl)) {
        expr.decl = typeDecl;
        message(primTypeColor + " - " + typeDecl->name + "\033[0m");
    }
    else {
        throw NameError("Identifier '" + expr.name + "' is not type name", expr.start(), expr.length());
    }
}

void NameResolver::visit(ListTypeExpr& expr) {
    expr.type->accept(*this);
}

void NameResolver::visit(TupleTypeExpr& expr) {
    for (auto& type : expr.types)
        type->accept(*this);
}

void NameResolver::visit(UnionTypeExpr& expr) {
    for (auto& option : expr.options)
        option->accept(*this);
}

void NameResolver::visit(LambdaTypeExpr& expr) {
    //for (auto& param : expr.params)
    //    param->accept(*this);
    
    expr.arg->accept(*this);
    expr.out->accept(*this);
}

void NameResolver::visit(StructTypeExpr& expr) {
    pushScope();
    for (auto &stmt : expr.stmts)
        stmt->accept(*this);
    
    popScope();
}

#pragma endregion