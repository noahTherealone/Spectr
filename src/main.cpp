#include <iostream>
#include <filesystem>
#include <fstream>
#include "parser.hpp"
#include "name_resolution.hpp"

void runCode(const std::string& code, const std::string& path) {
    std::vector<Token> tokens;
    std::vector<size_t> offsets;
    Lexer lexer = Lexer(path);
    lexer.tokenize(code, tokens, offsets);

    std::cout << "\033[1m\033[95mTokenized code>\033[0m\n";
    for (Token token : tokens)
        std::cout << "\033[90m | \033[0m" + token.show() + (token.type == TokenType::LineBreak ? "\n" : "");

    Parser parser = Parser(path);
    auto ast = parser.parseToks(tokens, offsets);

    std::cout << "\n\033[1m\033[94mParsed code>\033[0m\n";
    for (auto& stmt : ast) {
        std::cout << stmt->show() + "\n";
    }

    Context ctx = Context();
    NameResolver nameResolver(ctx);
    nameResolver.resolveAST(ast, path, offsets);
}

void runFile(const std::string& path) {
    std::ifstream file(path);

    if (!file.is_open()) {
        std::cerr << "\033[0;31mCould not open file '" << path << "'.\033[0m" << std::endl;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string code = buffer.str();

    runCode(code, path);
}

int main() {
    runFile("demo.spectr");
}
