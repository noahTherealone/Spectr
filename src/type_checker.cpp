#include <iostream>
#include <algorithm>
#include "type_checker.hpp"
#include "name_resolution.hpp"

void TypeChecker::typeCheckAST(const std::vector<std::unique_ptr<Stmt>>& stmts) {
    for (auto& stmt : stmts) {
        try {
            stmt->accept(*this);
        }
        catch (const TypeError& err) {
            auto it = std::upper_bound(offsets.begin(), offsets.end(), err.start);
            size_t line = it - offsets.begin() - 1;
            size_t column = err.start - offsets[line];
            std::cout << "\033[31mTypeError at " + path + " (" + std::to_string(line+1) + ":" + std::to_string(column+1) + "): " + err.msg + "\033[0m\n";
        }
    }
}

void TypeChecker::message(const std::string& msg) const {    
    std::cout << msg + "\n";
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
                throw TypeError(
                    "Tried assigning value of type " + value->show()
                        + "\033[31m to " + stmt.lhs->show()
                        + "\033[31m of type " + type->show(),
                    stmt.value->start(), stmt.value->length()
                );
        }
    }
    else {
        expected = nullptr;
        stmt.decl->type = visit(stmt.value.get());
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
    
    result = visit(stmt.value.get());
}

void TypeChecker::visit(ExprStmt& stmt) {
    visit(stmt.expr.get(), nullptr);
    result = nullptr;
}

TypePtr TypeChecker::visit(Stmt* stmt) {
    stmt->accept(*this);
    return result;
}

TypePtr TypeChecker::visit(Stmt* stmt, TypePtr _expected) {
    TypePtr previous = expected;
    expected = _expected;
    TypePtr _result = visit(stmt);
    expected = previous;
    return _result;
}

bool TypeChecker::typeCheck(std::vector<std::unique_ptr<Stmt>> stmts, TypePtr _expected) {
    TypePtr previous = expected;
    expected = _expected;
    std::vector<TypePtr> results;
    for (auto& stmt : stmts) {
        TypePtr r = visit(stmt.get(), _expected);
        if (r) results.push_back(r);
    }

    bool match = UnionType::fromOptions(results) <= _expected;
    expected = previous;
    return match;
}

#pragma endregion

#pragma region Expression visitors

void TypeChecker::visit(IdentifierExpr& expr) {
    result = expr.decl->type;
}

void TypeChecker::visit(AttributeExpr& expr) {
    std::cout << "Attributes are not supported yet!\n";
    result = nullptr;
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

    throw TypeError(
        showBinaryOp(expr.op) + " is not defined for operands of types " + left->show() + "\033[31m and " + right->show(),
        expr.start(),
        expr.length()
    );

    result = VOID_TYPE;
}

void TypeChecker::visit(TernaryExpr& expr) {
    TypePtr primary     = visit(expr.primary.get());
    TypePtr alternative = visit(expr.alternative.get());
    TypePtr condition   = visit(expr.condition.get(), BOOL_TYPE);
    
    if (expected && !(primary <= expected))
        throw TypeError(
            "Primary type " + primary->show() + "\033[31m is incompatible with expected type " + expected->show(),
            expr.primary->start(),
            expr.primary->length()
        );
    
    if (expected && !(alternative <= expected))
        throw TypeError(
            "Alternative type " + alternative->show() + "\033[31m is incompatible with expected type " + expected->show(),
            expr.alternative->start(),
            expr.alternative->length()
        );
    
    if (condition->compare(*BOOL_TYPE) != 0)
        throw TypeError(
            "Condition type " + condition->show() + "\033[31m is incompatible with expected type " + BOOL_TYPE->show(),
            expr.condition->start(),
            expr.condition->length()
        );
    
    result = UnionType::fromOptions( {primary, alternative} );
}

void TypeChecker::visit(ListExpr& expr) {
    if (expected) {
        for (auto& elem : expr.exprns) {
            if (!typeCheck(elem.get(), expected))
                throw TypeError("List element didn't match expected type '" + expected->show() + "'", elem->start(), elem->length());
        }

        return;
    }

    std::vector<TypePtr> options;
    for (auto& elem : expr.exprns)
        options.push_back(visit(elem.get()));
    
    result = UnionType::fromOptions(options);
}

void TypeChecker::visit(TupleExpr& expr) {
    std::vector<TypePtr> types;
    for (auto& elem : expr.exprns)
        types.push_back(visit(elem.get()));
    
    result = std::make_shared<const TupleType>(types);
}

void TypeChecker::visit(BlockExpr& expr) {
    std::vector<TypePtr> results;
    for (auto& stmt : expr.stmts) {
        if (auto rs = dynamic_cast<ReturnStmt*>(stmt.get())) {
            TypePtr _result = visit(rs->value.get(), expected);
            if (expected && !(_result <= expected))
                throw TypeError(
                    "Block returns type " + _result->show() + "\033[31m incompatible with expected type " + expected->show(),
                    rs->start(),
                    rs->length()
                );
            
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
        throw TypeError(
            "Lambda expression cannot be of type " + expected->show() + "\033[31m",
            expr.start(),
            expr.length()
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

    if (expected && expr.params->params.size() != expectedParams.size())
        throw TypeError(
            "Lambda expression has " + std::to_string(expr.params->params.size()) + " parameters, expected " + std::to_string(expectedParams.size()),
            expr.params->start,
            expr.params->length
        );

    std::vector<TypePtr> params;
    for (size_t i = 0; i < expr.params->params.size(); i++) {
        auto& param = expr.params->params[i];
        
        if (param->type) {
            TypePtr annotation = visit(param->type.get());
            if (expected && !(expectedParams[i] <= annotation))
                throw TypeError(
                    "Lambda parameter annotation " + annotation->show() + "\033[31m doesn't generalize " + param->decl->type->show() + "\033[0m",
                    param->type->start(),
                    param->type->length()
                );

            param->decl->type = annotation;
            params.push_back(annotation);
            continue;
        }
        
        if (!expected)
            throw TypeError(
                "Lambda parameter type for " + param->id->show() + " could not be inferred",
                param->id->start(),
                param->id->length()
            );
        
        param->decl->type = expectedParams[i];
        params.push_back(expectedParams[i]);
    }

    TypePtr out;
    if (expected)
        out = visit(expr.body.get(), lambda->out);
    else
        out = visit(expr.body.get(), nullptr);
    
    if (expected && !(out <= lambda->out))
        throw TypeError(
            "Lambda body returns wrong type",
            expr.body->start(),
            expr.body->length()
        );
    
    result = std::make_shared<const LambdaType>(TupleType::toTuple(params), out);
}

void TypeChecker::visit(ApplExpr& expr) {
    TypePtr fun = visit(expr.fun.get(), nullptr);
    auto lambda = dynamic_cast<const LambdaType*>(fun.get());
    if (!lambda)
        throw TypeError("Called expression of type " + fun->show() + "\033[0m must be a function",
            expr.fun->start(), expr.fun->length()
        );

    TypePtr arg = visit(expr.arg.get(), lambda->arg);
    if (!(arg <= lambda->arg))
        throw TypeError("Function argument type " + arg->show() + "\033[0m is incompatible with " + lambda->arg->show(),
            expr.arg->start(), expr.arg->length()
        );
    
    if (expected && !(lambda->out <= expected))
        throw TypeError("Function return type " + arg->show() + "\033[0m is incompatible with " + lambda->arg->show(),
            expr.arg->start(), expr.arg->length()
        );
    
    result = lambda->out;
}

TypePtr TypeChecker::visit(Expr* expr) {
    expr->accept(*this);
    return result;
}

TypePtr TypeChecker::visit(Expr* expr, TypePtr _expected) {
    TypePtr previous = expected;
    expected = _expected;
    TypePtr _result = visit(expr);
    message(expr->show() + ": " + _result->show() + (_expected ? " (expected: " + _expected->show() + ")" : ""));
    expected = previous;
    return _result;
}

bool TypeChecker::typeCheck(Expr* expr, TypePtr _expected) {
    return visit(expr, _expected) <= _expected;
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
    TypePtr previous = expected;
    expected = nullptr;
    expr->accept(*this);
    expected = previous;
    return result;
}

#pragma endregion