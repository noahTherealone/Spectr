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

    Context() = default;

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
struct AliasDeclStmt;
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
    void resolveAST(const std::vector<std::unique_ptr<Stmt>>& ast);

    explicit NameResolver(Context& ctx, const std::string& path, const std::vector<size_t>& offsets) :
        ctx(ctx), path(path), offsets(offsets), currentScope(nullptr) { pushScope(); depth = 0; }

private:
    const std::string& path;
    const std::vector<size_t>& offsets;
    Context& ctx;
    std::vector<std::unique_ptr<Scope>> scopes;
    Scope* currentScope;
    size_t depth;

    void pushScope();
    void popScope();

    template<typename D, typename... Args>
    D* declare(const std::string& name, Args&&... args) {
        static_assert(std::is_base_of_v<Decl, D>, "D must derive from Decl");
        D* decl = ctx.makeDecl<D>(std::forward<Args>(args)...);

        if (currentScope->contains(name))
            throw NameError("Tried to redefine '" + name + "'", decl->start, decl->length);
        
        currentScope->bind(name, decl);
        return decl;
    }

    void message(const std::string& msg) const;

    void visit(IfStmt& stmt) override;
    void visit(VarDeclStmt& stmt) override;
    void visit(ReferenceDeclStmt& stmt) override;
    void visit(AssignmentStmt& stmt) override;
    void visit(AliasDeclStmt& stmt) override;
    void visit(ReturnStmt& stmt) override;
    void visit(ExprStmt& stmt) override;

    void visit(IdentifierExpr& expr) override;
    void visit(AttributeExpr& expr) override;
    void visit(VoidExpr& expr) override;
    void visit(BooleanExpr& expr) override;
    void visit(IntExpr& expr) override;
    void visit(NumExpr& expr) override;
    void visit(StrExpr& expr) override;
    void visit(BinaryExpr& expr) override;
    void visit(TernaryExpr& expr) override;
    void visit(TupleExpr& expr) override;
    void visit(BlockExpr& expr) override;
    void visit(ParamsExpr& expr) override;
    void visit(LambdaExpr& expr) override;
};