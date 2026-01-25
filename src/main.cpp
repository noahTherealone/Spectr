#include <iostream>
#include <filesystem>
#include "parser.hpp"

int main() {
    Parser parser = Parser();
    auto ast = parser.parseFile("demo.spectr");
    for (auto& stmt : ast) {
        std::cout << stmt->show() + "\n";
    }

    NameResolver nameResolver = NameResolver();
    nameResolver.resolveAST(ast);
}
