#include <iostream>
#include "type_checker.hpp"
#include "name_resolution.hpp"
#include "base.hpp"

void TypeChecker::typeCheckAST(const std::vector<std::unique_ptr<Stmt>>& stmts) {
    for (auto& stmt : stmts) {
        try {
            stmt->accept(*this);
        }
        catch (const TypeError& err) {
            std::cout << err.show(path, offsets);
        }
    }
}

void TypeChecker::message(const std::string& msg) const {    
    std::cout << msg + "\n";
}

void TypeChecker::report(const std::string& msg, size_t start, size_t length) {
    TypeError err(msg, start, length);
    std::cout << err.show(path, offsets);
    errors.push_back(err);
}

void TypeChecker::report(const std::string& msg, Stmt* stmt) {
    report(msg, stmt->start(), stmt->length());
}

void TypeChecker::report(const std::string& msg, Expr* expr) {
    report(msg, expr->start(), expr->length());
}

void TypeChecker::report(const std::string& msg, TypeExpr* expr) {
    report(msg, expr->start(), expr->length());
}

#pragma region Statement visitors

void TypeChecker::visit(IfStmt& stmt) {
    result = nullptr;
}

void TypeChecker::visit(VarDeclStmt& stmt) {
    if (stmt.type) {
        TypePtr type = visit(stmt.type.get());
        stmt.decl->type = type;
        if (stmt.value) {
            TypePtr value = visit(stmt.value.get(), type);
            if (!(value <= type))
                report(
                    "Tried assigning value of type " + value->show()
                        + "\033[31m to " + stmt.lhs->show()
                        + "\033[31m of type " + type->show(),
                    stmt.value.get()
                );
        }
    }
    else {
        stmt.decl->type = visit(stmt.value.get(), nullptr);
    }

    result = nullptr;
}

void TypeChecker::visit(ReferenceDeclStmt& stmt) {
    result = nullptr;
}

void TypeChecker::visit(AssignmentStmt& stmt) {
    result = nullptr;
}

void TypeChecker::visit(AliasDeclStmt& stmt) {
    result = nullptr;
}

void TypeChecker::visit(ReturnStmt& stmt) {
    if (!stmt.value) {
        result = nullptr;
        return;
    }
    
    result = visit(stmt.value.get(), expected);
}

void TypeChecker::visit(ExprStmt& stmt) {
    visit(stmt.expr.get(), nullptr);
    result = nullptr;
}

TypePtr TypeChecker::visit(Stmt* stmt, TypePtr _expected) {
    ExpectedGuard guard(*this, _expected);
    stmt->accept(*this);
    return result;
}

#pragma endregion

#pragma region Expression visitors

void TypeChecker::visit(IdentifierExpr& expr) {
    result = expr.decl->type;
}

void TypeChecker::visit(AttributeExpr& expr) {
    std::cout << "Attributes are not supported yet!\n";
    result = INVALID_TYPE;
}

void TypeChecker::visit(VoidExpr& expr) {
    result = VOID_TYPE;
}

void TypeChecker::visit(BooleanExpr& expr) {
    result = BOOL_TYPE;
}

void TypeChecker::visit(IntExpr& expr) {
    result = INT_TYPE;
}

void TypeChecker::visit(NumExpr& expr) {
    result = NUM_TYPE;
}

void TypeChecker::visit(StrExpr& expr) {
    result = STR_TYPE;
}

void TypeChecker::visit(BinaryExpr& expr) {
    TypePtr left = visit(expr.left.get(), nullptr);
    TypePtr right = visit(expr.right.get(), nullptr);

    if (left->compare(*right) == 0) {
        if (auto prim = dynamic_cast<const PrimType*>(left.get())) {
            if (prim->prim != Prim::Void) {
                result = left;
                return;
            }
        }
    }

    report(
        showBinaryOp(expr.op) + " is not defined for operands of types " + left->show() + "\033[31m and " + right->show(),
        &expr
    );

    result = INVALID_TYPE;
}

void TypeChecker::visit(TernaryExpr& expr) {
    TypePtr primary     = visit(expr.primary.get(), expected);
    TypePtr alternative = visit(expr.alternative.get(), expected);
    TypePtr condition   = visit(expr.condition.get(), BOOL_TYPE);
    
    if (expected && !(primary <= expected))
        report(
            "Primary type " + primary->show() + "\033[31m is incompatible with expected type " + expected->show(),
            expr.primary.get()
        );
    
    if (expected && !(alternative <= expected))
        report(
            "Alternative type " + alternative->show() + "\033[31m is incompatible with expected type " + expected->show(),
            expr.alternative.get()
        );
    
    if (condition->compare(*BOOL_TYPE) != 0)
        report(
            "Condition type " + condition->show() + "\033[31m is incompatible with expected type " + BOOL_TYPE->show(),
            expr.condition.get()
        );
    
    result = UnionType::fromOptions( {primary, alternative} );
}

void TypeChecker::visit(ListExpr& expr) {
    auto list = expected ? dynamic_cast<const ListType*>(expected.get()) : nullptr;

    if (expected && !list)
        report(
            "Expected " + expected->show() + "\033[31m, received list instead",
            &expr
        );

    if (list) {
        std::vector<TypePtr> options;
        for (auto& elem : expr.exprns) {
            auto t = visit(elem.get(), list->type);
            if (!(t <= list->type))
                report(
                    "List element type " + t->show() + "\033[31m is incompatible with " + list->type->show(),
                    &expr
                );
            
            options.push_back(t);
        }

        result = std::make_shared<const ListType>(UnionType::fromOptions(options));
        return;
    }

    std::vector<TypePtr> options;
    for (auto& elem : expr.exprns)
        options.push_back(visit(elem.get(), nullptr));
    
    result = std::make_shared<const ListType>(UnionType::fromOptions(options));

}

void TypeChecker::visit(TupleExpr& expr) {
    auto tuple = expected ? dynamic_cast<const TupleType*>(expected.get()) : nullptr;
    if (expected && !tuple)
        report(
            "Expected " + expected->show() + "\033[31m, got a tuple instead",
            &expr
        );
    
    bool expectationCompatible = tuple && expr.exprns.size() == tuple->types.size();
    if (tuple && !expectationCompatible)
        report(
            "Expected tuple of length " + std::to_string(tuple->types.size())
            + ", received tuple of length " + std::to_string(expr.exprns.size()),
            &expr
        );

    std::vector<TypePtr> types;
    for (size_t i = 0; i < expr.exprns.size(); i++)
        types.push_back(visit(expr.exprns[i].get(), expectationCompatible ? tuple->types[i] : nullptr));
    
    result = std::make_shared<const TupleType>(types);
}

void TypeChecker::visit(BlockExpr& expr) {
    std::vector<TypePtr> results;
    for (auto& stmt : expr.stmts) {
        if (auto rs = dynamic_cast<ReturnStmt*>(stmt.get())) {
            TypePtr _result = visit(rs->value.get(), expected);
            if (expected && !(_result <= expected))
                report(
                    "Block returns type " + _result->show() + "\033[31m incompatible with expected type " + expected->show(),
                    rs
                );
            
            if (_result)
                results.push_back(_result);
        }
        else
            results.push_back(visit(stmt.get(), expected));
    }

    result = UnionType::fromOptions(results);
}

void TypeChecker::visit(LambdaExpr& expr) {
    auto lambda = dynamic_cast<const LambdaType*>(expected.get());
    if (expected && !lambda)
        report(
            "Lambda expression cannot be of type " + expected->show() + "\033[31m",
            &expr
        );
    
    std::vector<TypePtr> expectedParams;
    if (lambda) {
        if (auto tupleArg = dynamic_cast<const TupleType*>(lambda->arg.get())) {
            for (auto t : tupleArg->types)
                expectedParams.push_back(t);
        }
        else
            expectedParams.push_back(lambda->arg);
    }

    bool expectationCompatible = lambda && expr.params->params.size() == expectedParams.size();
    if (lambda && !expectationCompatible)
        report(
            "Lambda expression has " + std::to_string(expr.params->params.size()) + " parameters, expected " + std::to_string(expectedParams.size()),
            expr.params->start,
            expr.params->length
        );

    std::vector<TypePtr> params;
    for (size_t i = 0; i < expr.params->params.size(); i++) {
        auto& param = expr.params->params[i];
        
        if (param->type) {
            TypePtr annotation = visit(param->type.get());
            if (expectationCompatible && !(expectedParams[i] <= annotation))
                report(
                    "Lambda parameter annotation " + annotation->show() + "\033[31m doesn't generalize " + param->decl->type->show() + "\033[0m",
                    param->type.get()
                );

            param->decl->type = annotation;
            params.push_back(annotation);
            continue;
        }
        
        if (!expectationCompatible)
            report(
                "Lambda parameter type for " + param->id->show() + " could not be inferred",
                param->id.get()
            );
        
        param->decl->type = expectationCompatible ? expectedParams[i] : INVALID_TYPE;
        params.push_back(param->decl->type);
    }

    TypePtr out = visit(expr.body.get(), lambda ? lambda->out : nullptr);
    if (lambda && !(out <= lambda->out))
        report(
            "Lambda body returns wrong type",
            expr.body.get()
        );
    
    result = std::make_shared<const LambdaType>(TupleType::toTuple(params), out);
}

void TypeChecker::visit(ApplExpr& expr) {
    TypePtr fun = visit(expr.fun.get(), nullptr);
    auto lambda = dynamic_cast<const LambdaType*>(fun.get());
    if (!lambda)
        report(
            "Called expression of type " + fun->show() + "\033[31m must be a function",
            expr.fun.get()
        );

    TypePtr arg = visit(expr.arg.get(), lambda ? lambda->arg : nullptr);
    if (lambda && !(arg <= lambda->arg))
        report(
            "Function argument type " + arg->show() + "\033[31m is incompatible with " + lambda->arg->show(),
            expr.arg.get()
        );
    
    result = lambda ? lambda->out : fun;
}

TypePtr TypeChecker::visit(Expr* expr, TypePtr _expected) {
    ExpectedGuard guard(*this, _expected);
    expr->accept(*this);
    message(
        primTypeColor + sourcePos(path, offsets, expr->start()) + ":\033[0m "
        + expr->show() + ": " + result->show() + (_expected ? " (expected: " + _expected->show() + ")" : "")
    );

    return result;
}

#pragma endregion

#pragma region Type expression visitors

void TypeChecker::visit(PrimTypeExpr& expr) {
    result = std::make_shared<const PrimType>(expr.prim);
}

void TypeChecker::visit(AnyTypeExpr& expr) {
    result = std::make_shared<const AnyType>();
}

void TypeChecker::visit(NamedTypeExpr& expr) {
    result = expr.decl->type;
}

void TypeChecker::visit(ListTypeExpr& expr) {
    result = std::make_shared<const ListType>(visit(expr.type.get()));
}

void TypeChecker::visit(TupleTypeExpr& expr) {
    std::vector<TypePtr> types;
    for (auto& type : expr.types)
        types.push_back(visit(type.get()));
    
    result = std::make_shared<const TupleType>(types);
}

void TypeChecker::visit(UnionTypeExpr& expr) {
    std::vector<TypePtr> options;
    for (auto& option : expr.options)
        options.push_back(visit(option.get()));
    
    result = UnionType::fromOptions(options);
}

void TypeChecker::visit(LambdaTypeExpr& expr) {
    result = std::make_shared<const LambdaType>(visit(expr.arg.get()), visit(expr.out.get()));
}

TypePtr TypeChecker::visit(TypeExpr* expr) {
    ExpectedGuard guard(*this, nullptr);
    expr->accept(*this);
    return result;
}

#pragma endregion