#pragma once

#include <string>
#include <iostream>
#include "lexer.hpp"
#include "parser.hpp"
#include "name_resolution.hpp"

class Compiler {
public:
    void runFile(const std::string& path);
    void runCode(const std::string& code);
    void runSession(const std::istream& stream);

    Compiler(const std::string& path) : path(path) {}

private:
    std::string path;
    Lexer lexer = Lexer(this->path);
    Parser parser = Parser(this->path);
    Context ctx;
    std::vector<size_t> offsets;
    NameResolver nameResolver = NameResolver(ctx, this->path, this->offsets);
};