#include "type_expression.hpp"
#include "type.hpp"
#include "name_resolution.hpp"
#include "statement.hpp"

std::string StructTypeExpr::show() const {
    std::string s = typeConColor + "[\n";
    for (auto& stmt : stmts) {
        s += stmt->show() + "\n";
    }

    return s + typeConColor + "]\033[0m";
}

StructTypeExpr::StructTypeExpr(std::vector<std::unique_ptr<VarDeclStmt>> stmts, size_t start, size_t length) :
    TypeExpr(start, length), stmts(std::move(stmts)) {}