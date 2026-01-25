#include <iostream>
#include "name_resolution.hpp"
#include "statement.hpp"

void NameResolver::resolveAST(const std::vector<std::unique_ptr<Stmt>>& ast) {
    std::cout << "NAME RESOLUTION>\n";

    for (const auto& stmt : ast) {
        std::cout << stmt->show() + "\n";
    }
}