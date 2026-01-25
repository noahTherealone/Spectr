#pragma once

#include "Lexer.h"
#include "Types.h"
#include <memory>
#include <string>

class _Interpreter;

struct Expr {
    size_t line = 0, column = 0;
    virtual std::string to_string() const = 0;
    virtual void accept(_Interpreter& backend) = 0;
    virtual ~Expr() = default;
};

struct BoolExpr : Expr {
    bool value;
    std::string to_string() const override { return std::to_string(value); }
    void accept(_Interpreter& backend) override;
    BoolExpr(bool v, size_t l, size_t c) : value(v) { line = l; column = c; }
};

struct NumExpr : Expr {
    float value;
    std::string to_string() const override { return std::to_string(value); }
    void accept(_Interpreter& backend) override;
    NumExpr(float v, size_t l, size_t c) : value(v) { line = l; column = c; }
};

struct IntExpr : Expr {
    int value;
    std::string to_string() const override { return std::to_string(value); }
    void accept(_Interpreter& backend) override;
    IntExpr(int v, size_t l, size_t c) : value(v) { line = l; column = c; }
};

struct StrExpr : Expr {
    std::string value;
    std::string to_string() const override { return "\"" + value + "\""; }
    void accept(_Interpreter& backend) override;
    StrExpr(const std::string& v, size_t l, size_t c) : value(v) { line = l; column = c; }
};

struct IdentifierExpr : Expr {
    std::string name;
    std::string to_string() const override { return name; }
    void accept(_Interpreter& backend) override;
    IdentifierExpr(const std::string& n, size_t l, size_t c) : name(n) { line = l; column = c; }
};

struct LabelledExpr : Expr {
    std::string label;
    std::unique_ptr<Expr> expr;
    std::string to_string() const override {
        return "'" + label + " " + expr->to_string();
    }
    void accept(_Interpreter& backend) override;
    LabelledExpr(const std::string& lab, std::unique_ptr<Expr> e, size_t l, size_t c)
        : label(lab), expr(std::move(e)) { line = l; column = c; }
};

struct UnaryExpr : Expr {
    std::unique_ptr<Expr> param;
    TokenType op;
    std::string to_string() const override {
        return std::string(view(op)) + param->to_string();
    }
    void accept(_Interpreter& backend) override;
    UnaryExpr(std::unique_ptr<Expr> p, TokenType o, size_t l, size_t c)
        : param(std::move(p)), op(o) { line = l; column = c; }
};

struct BinaryExpr : Expr {
    std::unique_ptr<Expr> left, right;
    TokenType op;
    std::string to_string() const override {
        return "(" + left->to_string() + " " + std::string(view(op)) + " " + right->to_string() + ")";
    }
    void accept(_Interpreter& backend) override;
    BinaryExpr(std::unique_ptr<Expr> lft, std::unique_ptr<Expr> rght, TokenType o, size_t l, size_t c)
        : left(std::move(lft)), right(std::move(rght)), op(o) { line = l; column = c; }
};

struct SignalExpr : Expr {
    std::unique_ptr<Expr> freq, ampl;
    std::string to_string() const override {
        return "(" + (freq != nullptr ? freq->to_string() : "<nil>") + ":" + ampl->to_string() + ")";
    }
    void accept(_Interpreter& backend) override;
    SignalExpr(std::unique_ptr<Expr> f, std::unique_ptr<Expr> a, size_t l, size_t c)
        : freq(std::move(f)), ampl(std::move(a)) { line = l; column = c; }
};

struct ListExpr : Expr {
    std::vector<std::unique_ptr<Expr>> list;
    std::string to_string() const override {
        std::string str = "{ ";
        for (size_t i = 0; i < list.size(); ++i)
            str.append(list[i]->to_string() + " ");
        
        str.append("}");
        return str;
    }
    void accept(_Interpreter& backend) override;
    ListExpr(std::vector<std::unique_ptr<Expr>> L, size_t l, size_t c)
        : list(std::move(L)) { line = l; column = c; }
};

struct TupleExpr : Expr {
    std::vector<std::unique_ptr<Expr>> tuple;
    std::string to_string() const override {
        std::string str = "(";
        for (size_t i = 0; i < tuple.size(); ++i)
            str.append((i != 0 ? ", " : "") + tuple[i]->to_string());
        
        str.append(")");
        return str;
    }
    void accept(_Interpreter& backend) override;
    TupleExpr(std::vector<std::unique_ptr<Expr>> T, size_t l, size_t c)
        : tuple(std::move(T)) { line = l; column = c; }
};

OscPrim to_osc_prim(TokenType t);

struct OscPrimExpr : Expr {
    OscPrim shape = OscPrim::Sine;
    std::unique_ptr<Expr> param; // signal or spectrum
    std::string to_string() const override {
        std::string str = "";
        if      (shape == OscPrim::Sine) str = "~sin~";
        else if (shape == OscPrim::Square) str = "~sqr~";
        else if (shape == OscPrim::Saw) str = "~saw~";
        return "(" + str + " " + param->to_string() + ")";
    }
    void accept(_Interpreter& backend) override;
    OscPrimExpr(OscPrim s, std::unique_ptr<Expr> p, size_t l, size_t c)
        : shape(s), param(std::move(p)) { line = l; column = c; }
};

enum class AssignMode {
    eager, lazy
};

struct AssignmentExpr : Expr {
    AssignMode mode;
    std::unique_ptr<Expr> id, value; // id should be an identifier
    std::string to_string() const override {
        return "(" + id->to_string() + (mode == AssignMode::eager ? " = " : " := ") + value->to_string() + ")";
    }
    void accept(_Interpreter& backend) override;
    AssignmentExpr(AssignMode m, std::unique_ptr<Expr> i, std::unique_ptr<Expr> v, size_t l, size_t c)
        : mode(m), id(std::move(i)), value(std::move(v)) { line = l; column = c; }
};

/*enum class DeclType {
    num,
    signal,
    spectr,
    osc
};

static DeclType to_decl(TokenType t) {
    switch (t) {
        case TokenType::NUM:    return DeclType::num;
        case TokenType::SIGNAL: return DeclType::signal;
        case TokenType::SPECTR: return DeclType::spectr;
        case TokenType::OSC:    return DeclType::osc;
        default: throw; // really shouldn't happen if used correctly
    }
}

static std::string decl_name(DeclType t) {
    switch (t) {
        case DeclType::num:    return "$num";
        case DeclType::signal: return "$signal";
        case DeclType::spectr: return "$spectr";
        case DeclType::osc:    return "$osc";
        default: throw; // really shouldn't happen if used correctly
    }
}*/

// parser-only expression type, not to be sent to the interpreter
struct TypeExpr : Expr {
    Type type;

    std::string to_string() const override {
        return typeName(type);
    }
    void accept(_Interpreter& backend) override; // doesn't do anything for now
    TypeExpr(Type t, size_t l, size_t c)
        : type(t) { line = l; column = c; }
};

struct DeclExpr : Expr {
    //DeclType type;
    Type type;
    std::unique_ptr<Expr> decl; // either assignment or identifier
    std::string to_string() const override {
        return typeName(type) + " " + decl->to_string();
    }
    void accept(_Interpreter& backend) override;
    DeclExpr(/*DeclType*/Type t, std::unique_ptr<Expr> d, size_t l, size_t c)
        : type(t), decl(std::move(d)) { line = l; column = c; }
};

struct BlockExpr : Expr {
    std::vector<std::unique_ptr<Expr>> expressions;
    std::string to_string() const override {
        std::string str = "Block [\n";
        for (int i = 0; i < expressions.size(); ++i) {
            str.append("  " + expressions[i]->to_string() + "\n");
        }
        str.append("]");
        return str;
    }
    void accept(_Interpreter& backend) override;
    BlockExpr(std::vector<std::unique_ptr<Expr>> e, size_t l, size_t c)
        : expressions(std::move(e)) { line = l; column = c; }
};

struct OutExpr : Expr {
    std::unique_ptr<Expr> value;
    std::string to_string() const override {
        return "Output << " + value->to_string();
    }
    void accept(_Interpreter& backend) override;
    OutExpr(std::unique_ptr<Expr> v, size_t l, size_t c)
        : value(std::move(v)) { line = l; column = c; }
};

struct SignatureExpr {
    std::vector<std::unique_ptr<DeclExpr>> args;

    std::string to_string() const {
        std::string str = "(";
        for (size_t i; i < args.size(); ++i) {
            if (i != 0) str.append(", ");
            str.append(args[i]->to_string());
        }

        return str;
    }
};

struct LambdaExpr : Expr {
    std::unique_ptr<Expr> signature;
    std::unique_ptr<Expr> body;

    std::string to_string() const override {
        return "$" + signature->to_string() + "->" + body->to_string();
    }
    void accept(_Interpreter& backend) override;
    LambdaExpr(std::unique_ptr<Expr> s, std::unique_ptr<Expr> b, size_t l, size_t c)
        : signature(std::move(s)), body(std::move(b)) { line = l; column = c; }
};

struct FuncApplExpr : Expr {
    std::unique_ptr<Expr> func;
    std::vector<std::unique_ptr<Expr>> args;

    std::string to_string() const override {
        std::string str = "(" + func->to_string() + ")(";
        for (size_t i = 0; i < args.size(); i++) {
            if (i > 0) str.append(",");
            str.append(args[i]->to_string());
        }

        return str + ")";
    }
    void accept(_Interpreter& backend) override;
    FuncApplExpr(std::unique_ptr<Expr> f, std::vector<std::unique_ptr<Expr>> a)
        : func(std::move(f)) { 
            for (size_t i = 0; i < a.size(); i++)
                args.push_back(std::move(a[i]));
        }
};

struct PlaybackExpr : Expr {
    std::unique_ptr<Expr> osc, signal;
    std::string to_string() const override {
        return "Playback: " + osc->to_string() + " " + (signal ? signal->to_string() : "default");
    }
    void accept(_Interpreter& backend) override;
    PlaybackExpr(std::unique_ptr<Expr> o, std::unique_ptr<Expr> s, size_t l, size_t c)
        : osc(std::move(o)), signal(std::move(s)) { line = l; column = c; }
    PlaybackExpr(std::unique_ptr<Expr> o, std::nullptr_t, size_t l, size_t c)
        : osc(std::move(o)), signal(nullptr) { line = l; column = c; }
};

struct ReleaseExpr : Expr {
    std::unique_ptr<std::string> label;
    std::string to_string() const override {
        return "Release '" + (label ? *label : "_");
    }
    void accept(_Interpreter& backend) override;
    ReleaseExpr(std::unique_ptr<std::string> s, size_t l, size_t c)
        : label(std::move(s)) { line = l; column = c; }
};