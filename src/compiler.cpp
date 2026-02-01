#include <iostream>
#include <fstream>
#include <sstream>
#include "compiler.hpp"

void Compiler::runCode(const std::string& code) {
    std::vector<Token> tokens;
    lexer.tokenize(code, tokens, offsets);

    std::cout << "\033[1m\033[95mTokenized code>\033[0m\n";
    for (Token token : tokens)
        std::cout << "\033[90m | \033[0m" + token.show() + (token.type == TokenType::LineBreak ? "\n" : "");

    std::cout << "\n\n";

    auto ast = parser.parseToks(tokens, offsets);

    std::cout << "\033[1m\033[94mParsed code>\033[0m\n";
    for (auto& stmt : ast) {
        std::cout << stmt->show() + "\n";
    }

    std::cout << "\n";
    std::cout << "\033[1m\033[36mName resolution>\033[0m\n";

    nameResolver.resolveAST(ast);

    std::cout << "\n";
    std::cout << primTypeColor + "\033[1mType checking>\033[0m\n";

    typeChecker.typeCheckAST(ast);
}

void Compiler::runFile(const std::string& path) {
    std::ifstream file(path);

    if (!file.is_open()) {
        std::cerr << "\033[0;31mCould not open file '" << path << "'.\033[0m" << std::endl;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string code = buffer.str();

    std::string oldPath = this->path;
    std::vector<size_t> oldOffsets = this->offsets;
    this->path = path;
    runCode(code);
    this->path = oldPath;
    this->offsets = oldOffsets;
}

void Compiler::runSession(const std::istream& stream) {
    std::string line;
    while (true) {
        std::cout << "\033[95;1mInput>\033[0m\n";
        if (!std::getline(std::cin, line)) break;
        if (line == "\\quit") break;

        try {
            runCode(line);
        }
        catch (const std::exception& err) {
            std::cout << "\033[31m" << err.what() << "\033[0m" << std::endl;
        }
    }
}