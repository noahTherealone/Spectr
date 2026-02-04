#pragma once

#include <string>
#include <unordered_map>
#include "lexer.hpp"
#include "type.hpp"

const std::unordered_map<Prim, std::string> primNames = {
#define X(kw, tok, lit, pt) {Prim::pt, kw},
    PRIMITIVE_TYPES
#undef X
};

inline Prim primByTokenType(const TokenType type) {
    switch (type) {
#define X(kw, tok, lit, pt) case TokenType::tok: return Prim::pt;
        PRIMITIVE_TYPES
#undef X
        default: throw std::exception(); // shouldn't happen;
    }
}

class TypeExprVisitor;

struct TypeExpr {
    virtual ~TypeExpr() = default;
    virtual std::string show() const = 0;
    size_t start()  const { return start_; }
    size_t length() const { return length_; }

    virtual void accept(TypeExprVisitor& v) = 0;

protected:
    size_t start_;
    size_t length_;

    TypeExpr(size_t start, size_t length) : start_(start), length_(length) {}
    TypeExpr(const Token& tok) : TypeExpr(tok.index, tok.text.length()) {}
};

struct PrimTypeExpr;
struct AnyTypeExpr;
struct NamedTypeExpr;
struct ListTypeExpr;
struct TupleTypeExpr;
struct UnionTypeExpr;
struct LambdaTypeExpr;
struct StructTypeExpr;

class TypeExprVisitor {
public:
    virtual ~TypeExprVisitor() = default;

    virtual void visit(PrimTypeExpr& expr)   = 0;
    virtual void visit(AnyTypeExpr& expr)    = 0;
    virtual void visit(NamedTypeExpr& expr)  = 0;
    virtual void visit(ListTypeExpr& expr)   = 0;
    virtual void visit(TupleTypeExpr& expr)  = 0;
    virtual void visit(UnionTypeExpr& expr)  = 0;
    virtual void visit(LambdaTypeExpr& expr) = 0;
    virtual void visit(StructTypeExpr& expr) = 0;
};

struct PrimTypeExpr : TypeExpr {
    const Prim prim;
    std::string show() const override { return primTypeColor + primNames.at(prim) + "\033[0m"; }

    void accept(TypeExprVisitor& v) override { v.visit(*this); }

    PrimTypeExpr(const Token& tok) : TypeExpr(tok), prim(primByTokenType(tok.type)) {}
    PrimTypeExpr(Prim prim, size_t start, size_t length) : TypeExpr(start, length), prim(prim) {}
};

struct AnyTypeExpr : TypeExpr {
    std::string show() const override { return primTypeColor + "any\033[0m"; }
    void accept(TypeExprVisitor& v) override { v.visit(*this); }
    AnyTypeExpr(const Token& tok) : TypeExpr(tok) {}
};

struct TypeDecl;

struct NamedTypeExpr : TypeExpr {
    const std::string name;
    TypeDecl* decl;
    std::string show() const override { return primTypeColor + name + "\033[0m"; }

    void accept(TypeExprVisitor& v) override { v.visit(*this); }

    NamedTypeExpr(const Token& tok) : TypeExpr(tok), name(tok.text) {}
};

struct ListTypeExpr : TypeExpr {
    std::unique_ptr<TypeExpr> type;
    std::string show() const override { return typeConColor + "{" + type->show() + typeConColor + "}\033[0m"; }

    void accept(TypeExprVisitor& v) override { v.visit(*this); }

    ListTypeExpr(std::unique_ptr<TypeExpr> type, size_t start, size_t length) : TypeExpr(start, length), type(std::move(type)) {}
};

struct TupleTypeExpr : TypeExpr {
    std::vector<std::unique_ptr<TypeExpr>> types;
    std::string show() const override {
        std::string s = typeConColor + "(";
        for (auto it = types.begin(); it != types.end(); ++it) {
            s += (*it)->show() + typeConColor + (std::next(it) != types.end() || it == types.begin() ? ", " : "");
        }

        return s + typeConColor + ")\033[0m";
    }

    void accept(TypeExprVisitor& v) override { v.visit(*this); }

    TupleTypeExpr(std::vector<std::unique_ptr<TypeExpr>> types, size_t start, size_t length) : TypeExpr(start, length), types(std::move(types)) {}
};

struct UnionTypeExpr : TypeExpr {
    std::vector<std::unique_ptr<TypeExpr>> options;
    std::string show() const override {
        std::string s = typeConColor + "(";
        for (auto it = options.begin(); it != options.end(); ++it) {
            s += (*it)->show() + (std::next(it) != options.end() ? typeConColor + "|" : "");
        }
        
        return s + typeConColor + ")\033[0m";
    };

    void accept(TypeExprVisitor& v) override { v.visit(*this); }

    UnionTypeExpr(std::vector<std::unique_ptr<TypeExpr>> options) :
        TypeExpr(options.front()->start(), options.back()->start() - options.front()->start() + options.back()->length()),
        options(std::move(options)) {}
};

struct LambdaTypeExpr : TypeExpr {
    std::unique_ptr<TypeExpr> arg;
    std::unique_ptr<TypeExpr> out;
    std::string show() const override {
        return typeConColor + "(" + arg->show() + typeConColor + "->" + out->show() + typeConColor + ")\033[0m";
    }

    void accept(TypeExprVisitor& v) override { v.visit(*this); }

    LambdaTypeExpr(std::unique_ptr<TypeExpr> arg, std::unique_ptr<TypeExpr> out) :
        TypeExpr(arg->start(), out->start() - arg->start() + out->length()),
        arg(std::move(arg)), out(std::move(out)) {}
};

struct VarDeclStmt;

struct StructTypeExpr : TypeExpr {
    std::vector<std::unique_ptr<VarDeclStmt>> stmts;
    std::string show() const override;

    void accept(TypeExprVisitor& v) override { v.visit(*this); }
    StructTypeExpr(std::vector<std::unique_ptr<VarDeclStmt>> stmts, size_t start, size_t length);
};