#pragma once

#include <string>
#include <memory>
#include "type.hpp"

struct Decl {
    virtual ~Decl() = default;
    std::string name;
    size_t start;
    size_t length;

    Decl(const std::string& name, size_t start, size_t length) :
        name(name), start(start), length(length) {}
};

struct VarDecl : Decl {
    TypePtr type;

    VarDecl(const std::string& name, size_t start, size_t length) : Decl(name, start, length) {}
};

struct ParamDecl : Decl {
    TypePtr type;

    ParamDecl(const std::string& name, size_t start, size_t length) : Decl(name, start, length) {}
};

struct FunDecl : Decl {

};

struct TypeDecl : Decl {
    TypeDecl(const std::string& name, size_t start, size_t length) : Decl(name, start, length) {}
};

struct AliasDecl : Decl {

};

struct GenericTypeDecl : Decl {
    TypePtr superType;
};

enum class SymbolKind {
    Variable,
    Type,
    Generic
};

struct Symbol {
    std::string name;
    Decl* decl;
};

struct Stmt;

class NameResolver {
public:
    void resolveAST(const std::vector<std::unique_ptr<Stmt>>& ast);
};