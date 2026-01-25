#pragma once

#include <string>
#include <memory>
#include "type.hpp"
#include "statement.hpp"
#include "expression.hpp"

struct NameError : SpectrError {
    using SpectrError::SpectrError;
};

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

struct Context {
    template<typename D, typename... Args>
    D* makeDecl(Args&&... args) {
        static_assert(std::is_base_of_v<Decl, D>, "D must derive from Decl");
        auto d = std::make_unique<D>(std::forward<Args>(args)...);
        D* raw = d.get();
        decls.push_back(std::move(d));
        return raw;
    }

private:
    std::vector<std::unique_ptr<Decl>> decls;
};

class Scope {
public:
    // searches only itself
    bool contains(const std::string& name) const {
        return symbols.contains(name);
    }

    // recurses to parents
    Decl* lookup(const std::string& name) const {
        const Scope* current = this;
        while (current) {
            auto it = current->symbols.find(name);
            if (it != current->symbols.end())
                return it->second;
        
            current = current->parent;
        }

        return nullptr;
    }

    Scope(Scope* parent) : parent(parent) {}
    Scope() : Scope(nullptr) {}

private:
    friend class NameResolver;
    void bind(const std::string& name, Decl* decl) {
        symbols[name] = decl;
    }

    Scope* parent;
    std::unordered_map<std::string, Decl*> symbols;
};

struct Stmt;
struct IfStmt;
struct VarDeclStmt;
struct TypeInferredDeclStmt;
struct ReferenceDeclStmt;
struct AssignmentStmt;
struct TypeDeclStmt;
struct ReturnStmt;
struct ExprStmt;

struct IdentifierExpr;
struct AttributeExpr;
struct VoidExpr;
struct BooleanExpr;
struct IntExpr;
struct NumExpr;
struct StrExpr;
struct BinaryExpr;
struct TernaryExpr;
struct TupleExpr;
struct BlockExpr;
struct ParamsExpr;
struct LambdaExpr;

class StmtVisitor;
class ExprVisitor;

class NameResolver : public StmtVisitor, public ExprVisitor {
public:
    void resolveAST(const std::vector<std::unique_ptr<Stmt>>& ast, const std::string& path, const std::vector<size_t>& offsets);

    explicit NameResolver(Context& ctx) : ctx(ctx) { pushScope(); }

private:
    Context& ctx;
    Scope* currentScope;

    void pushScope();
    void popScope();

    template<typename D, typename... Args>
    D* declare(const std::string& name, Args&&... args) {
        static_assert(std::is_base_of_v<Decl, D>, "D must derive from Decl");
        D* decl = ctx.makeDecl<D>(std::forward<Args>(args)...);

        if (currentScope->contains(name))
            throw NameError("Variable redefinition", decl->start, decl->length);
        
        currentScope->bind(name, decl);
        return decl;
    }

    void visit(IfStmt& stmt) override;
    void visit(VarDeclStmt& stmt) override;
    void visit(ReferenceDeclStmt& stmt) override;
    void visit(AssignmentStmt& stmt) override;
    void visit(TypeDeclStmt& stmt) override;
    void visit(ReturnStmt& stmt) override;
    void visit(ExprStmt& stmt) override;

    void visit(IdentifierExpr& stmt) override;
    void visit(AttributeExpr& stmt) override;
    void visit(VoidExpr& stmt) override;
    void visit(BooleanExpr& stmt) override;
    void visit(IntExpr& stmt) override;
    void visit(NumExpr& stmt) override;
    void visit(StrExpr& stmt) override;
    void visit(BinaryExpr& stmt) override;
    void visit(TernaryExpr& stmt) override;
    void visit(TupleExpr& stmt) override;
    void visit(BlockExpr& stmt) override;
    void visit(ParamsExpr& stmt) override;
    void visit(LambdaExpr& stmt) override;
};