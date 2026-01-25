#include <iostream>
#include <filesystem>
#include <fstream>
#include <cstring>
#include "compiler.hpp"

int main(int argc, const char** argv) {

    if (argc == 1) {
        Compiler compiler("demo.spectr");
        compiler.runFile("demo.spectr");
        return 0;
    }

    if (strcmp(argv[1], "session") == 0) {
        Compiler compiler("session");
        compiler.runSession(std::cin);
    }

    return 1;
}
