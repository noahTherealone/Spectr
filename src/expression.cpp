#include <regex>
#include <iostream>
#include "expression.hpp"
#include "statement.hpp"
#include "parser.hpp"
#include "name_resolution.hpp"

std::string ListExpr::show() const {
    std::string s = exprColor + "{\033[0m";
    for (auto it = exprns.begin(); it != exprns.end(); ++it) {
        s += (*it)->show() + (std::next(it) != exprns.end() || it == exprns.begin() ? exprColor + ",\033[0m " : "");
    }

    return s + exprColor + "}\033[0m";
}

std::string TupleExpr::show() const {
    std::string s = exprColor + "(\033[0m";
    for (auto it = exprns.begin(); it != exprns.end(); ++it) {
        s += (*it)->show() + (std::next(it) != exprns.end() || it == exprns.begin() ? exprColor + ",\033[0m " : "");
    }

    return s + exprColor + ")\033[0m";
}

std::string BlockExpr::show() const {
    std::string s = "\n" + exprColor + "├─┬─block: [\033[0m\n";
    for (auto it = stmts.begin(); it != stmts.end(); ++it) {
        std::string indent = exprColor + ((std::next(it) == stmts.end()) ? "│ └─" : "│ ├─");
        std::string stmtText = indent + std::regex_replace((*it)->show(), std::regex("\n"), "\n" + exprColor + "│   ");
        s += stmtText + "\n";
    }

    return s + exprColor + "└───]\033[0m";
}

BlockExpr::BlockExpr(std::unique_ptr<Stmt> stmt) :
    Expr(stmt->start(), stmt->length()) {
        std::vector<std::unique_ptr<Stmt>> stmts_;
        stmts_.push_back(std::move(stmt));
        stmts = std::move(stmts_);
    }

Params::Params(std::unique_ptr<TupleExpr> tuple) {
    for (auto& expr : tuple->takeExprns()) {
        auto id = dynamic_cast<IdentifierExpr*>(expr.get());
        if (!id) {
            throw SyntaxError("Right now parameters must be identifiers", expr->start(), expr->length());
        }
        
        params.push_back(
            std::make_unique<Param>(
                std::unique_ptr<IdentifierExpr>(static_cast<IdentifierExpr*>(expr.release())),
                nullptr
            )
        );
    }
}

Params::Params(std::unique_ptr<IdentifierExpr> id) : start(id->start()), length(id->length()) {
    params.push_back(std::make_unique<Param>(std::move(id), nullptr));
}

Params::Params(std::unique_ptr<Expr> expr) {
    if (auto tuple = dynamic_cast<TupleExpr*>(expr.get())) {
        *this = Params(std::unique_ptr<TupleExpr>(static_cast<TupleExpr*>(expr.release())));
        return;
    }

    if (auto id = dynamic_cast<IdentifierExpr*>(expr.get())) {
        std::cout << "Hello\n";
        *this = Params(std::unique_ptr<IdentifierExpr>(static_cast<IdentifierExpr*>(expr.release())));
        return;
    }

    throw SyntaxError("Invalid parameter expression", expr->start(), expr->length());
}

std::string LambdaExpr::show() const {
    std::string s = "\n" + exprColor + "├─┬─lmbda: (\033[0m";
    for (auto it = params->params.begin(); it != params->params.end(); ++it) {
        s += it->get()->id->show() + exprColor + ": ";
        if (it->get()->type)
            s += it->get()->type->show();
        else
            s += "\033[31m?\033[0m";

        if (std::next(it) != params->params.end()) s += exprColor + ", ";
    }

    s += exprColor + ") -> [\n";

    for (auto it = body->stmts.begin(); it != body->stmts.end(); ++it) {
        std::string indent = exprColor + ((std::next(it) == body->stmts.end()) ? "│ └─" : "│ ├─");
        std::string stmtText = indent + std::regex_replace((*it)->show(), std::regex("\n"), "\n" + exprColor + "│   ");
        s += stmtText + "\n";
    }

    return s + exprColor + "└───]\033[0m";
}

LambdaExpr::LambdaExpr(std::unique_ptr<Params> params, std::unique_ptr<BlockExpr> body) :
    Expr(params->start, body->start() - params->start + body->length()), params(std::move(params)), body(std::move(body)) {}

LambdaExpr::LambdaExpr(std::unique_ptr<TupleExpr> params, std::unique_ptr<BlockExpr> body) :
    LambdaExpr(std::make_unique<Params>(std::move(params)), std::move(body)) {}