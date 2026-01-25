#include <regex>
#include "statement.hpp"
#include "expression.hpp"
#include "type_expression.hpp"

std::string IfStmt::show() const {
    std::string s;
    bool first = true;
    for (auto caseIt = cases.begin(); caseIt != cases.end(); ++caseIt) {
        s += stmtColor + (first ? "# IF :\033[0m " : "#ELIF:\033[0m ");
        std::string condition = (*caseIt).first->show();
        if (condition.find("\n") == std::string::npos)
            s += condition + "\n";
        else {
            condition = std::regex_replace(condition, std::regex("\n"), "\n" + stmtColor + "│   ");
            s += "\n" + stmtColor + "├───#COND:\033[0m " + condition + "\n";

        }

        std::string inner = "";
        bool lastCase = !elseCase && std::next(caseIt) == cases.end();
        for (auto stmtIt = (*caseIt).second->stmts.begin(); stmtIt != (*caseIt).second->stmts.end(); ++stmtIt) {
            std::string mainIndent;
            std::string innerIndent;
            bool firstStmt = stmtIt == (*caseIt).second->stmts.begin();
            bool lastStmt  = std::next(stmtIt) == (*caseIt).second->stmts.end();

            if (lastCase) {
                mainIndent  = stmtColor + "    ";
                if (firstStmt) {
                    if (lastStmt)
                        innerIndent = stmtColor + "└───";
                    else
                        innerIndent = stmtColor + "└─┬─";
                }
                else {
                    if (lastStmt)
                        innerIndent = stmtColor + "  └─";
                    else
                        innerIndent = stmtColor + "  ├─";
                }
            }
            else {
                if (firstStmt) {
                    mainIndent  = stmtColor + "│ │ ";
                    if (lastStmt)
                        innerIndent = stmtColor + "├───";
                    else
                        innerIndent = stmtColor + "├─┬─";
                }
                else {
                    mainIndent  = stmtColor + "│   ";
                    if (lastStmt)
                        innerIndent = stmtColor + "│ └─";
                    else
                        innerIndent = stmtColor + "│ ├─";
                }
            }

            std::string stmtText = std::regex_replace((*stmtIt)->show(), std::regex("\n"), "\n" + stmtColor + mainIndent);
            inner += innerIndent + stmtText;
            if (!lastCase) inner += "\n";
        }

        s += inner;
        first = false;
    }
    if (elseCase && elseCase->stmts.size() != 0) {
        s += stmtColor + "#ELSE:\033[0m\n";
        std::string inner = "";
        for (auto stmtIt = elseCase->stmts.begin(); stmtIt != elseCase->stmts.end(); ++stmtIt) {
            std::string stmtText = std::regex_replace((*stmtIt)->show(), std::regex("\n"), "\n" + stmtColor + "  │ ");
            std::string innerIndent;
            bool last = std::next(stmtIt) == elseCase->stmts.end();
            if (last)
                innerIndent = stmtColor + (stmtIt == elseCase->stmts.begin() ? "└───" : "  └─");
            else
                innerIndent = stmtColor + (stmtIt == elseCase->stmts.begin() ? "└─┬─" : "  ├─");
            inner += innerIndent + stmtText;
            if (!last) inner += "\n";
        }

        s += inner;
    }
    
    return s;
}

IfStmt::IfStmt(IfChain cases, std::unique_ptr<BlockExpr> elseCase, size_t start, size_t length) :
    Stmt(start, length), cases(std::move(cases)), elseCase(std::move(elseCase)) {}

std::string VarDeclStmt::show() const {
    std::string s = stmtColor + "#DECL: \033[0m" + decl->name + stmtColor + ":\033[0m " + type->show();
    if (value)
        s += stmtColor + " =\033[0m " + value->show();
    
    return s;
}

VarDeclStmt::VarDeclStmt(std::unique_ptr<IdentifierExpr> lhs, std::unique_ptr<TypeExpr> type, std::unique_ptr<Expr> value) :
    Stmt(lhs->start(), value->start() - lhs->start() + value->length()),
    lhs(std::move(lhs)), type(std::move(type)), value(std::move(value)),
    decl(std::make_unique<VarDecl>(lhs->name, lhs->start(), value->start() - lhs->start() + value->length())) {}

std::string TypeInferredDeclStmt::show() const {        
    return stmtColor + "#TDCL: \033[0m" + lhs->show() + stmtColor + " :=\033[0m " + value->show();
}

TypeInferredDeclStmt::TypeInferredDeclStmt(std::unique_ptr<Expr> lhs, std::unique_ptr<Expr> value) :
    Stmt(lhs->start(), value->start() - lhs->start() + value->length()),
    lhs(std::move(lhs)), value(std::move(value)) {}

std::string ReferenceDeclStmt::show() const {        
    return stmtColor + "#&DCL: \033[0m" + lhs->show() + stmtColor + " &=\033[0m " + value->show();
}

ReferenceDeclStmt::ReferenceDeclStmt(std::unique_ptr<Expr> lhs, std::unique_ptr<Expr> value) :
    Stmt(lhs->start(), value->start() - lhs->start() + value->length()),
    lhs(std::move(lhs)), value(std::move(value)) {}

std::string AssignmentStmt::show() const {        
    return stmtColor + "#ASGN: \033[0m" + lhs->show() + stmtColor + " =\033[0m " + value->show();
}

AssignmentStmt::AssignmentStmt(std::unique_ptr<Expr> lhs, std::unique_ptr<Expr> value) :
    Stmt(lhs->start(), value->start() - lhs->start() + value->length()),
    lhs(std::move(lhs)), value(std::move(value)) {}


std::string ReturnStmt::show() const {
    return stmtColor + "#RTRN: \033[0m" + (value ? value->show() : "\033[31mnil");
}

ReturnStmt::ReturnStmt(std::unique_ptr<Expr> value, size_t start) :
    Stmt(start, value->start() - start + value->length()), value(std::move(value)) {}

std::string ExprStmt::show() const {
    return stmtColor + "#EXPR: \033[0m" + expr->show();
}

ExprStmt::ExprStmt(std::unique_ptr<Expr> expr) :
    Stmt(expr->start(), expr->length()),
    expr(std::move(expr)) {}