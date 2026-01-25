#pragma once

#include <memory>
#include <set>
#include "type_expression.hpp"

enum class TypeKind {
    Prim,
    List,
    Tuple,
    Option,
    Function
};

struct Type {
    virtual ~Type() = default;
    virtual TypeKind kind() const = 0;
    virtual int compare(const Type& other) const = 0;
    virtual std::string show() const = 0;
};

using TypePtr = std::shared_ptr<const Type>;

struct TypeLess {
    bool operator()(const TypePtr& a, const TypePtr& b) const {
        return a->compare(*b) < 0;
    }
};

struct PrimType : Type {
    const Prim prim;
    TypeKind kind() const override { return TypeKind::Prim; }

    int compare(const Type& other) const override {
        if (auto *o = dynamic_cast<const PrimType*>(&other))
            return int(prim) - int(o->prim);
        
        return int(kind()) - int(other.kind());
    }

    std::string show() const override {
        return primTypeColor + primNames.at(prim) + "\033[0m";
    }

    PrimType(Prim prim) : prim(prim) {}
};

struct ListType : Type {
    const TypePtr type;
    TypeKind kind() const override { return TypeKind::List; }

    int compare(const Type& other) const override {
        if (auto *o = dynamic_cast<const ListType*>(&other)) {
            return type->compare(*o->type);
        }

        return int(kind()) - int(other.kind());
    }

    std::string show() const override {
        return typeConColor + "{" + type->show() + typeConColor + "}\033[0m";
    }

    ListType(const TypePtr& type) : type(type) {}
};

struct TupleType : Type {
    const std::vector<TypePtr> types;
    TypeKind kind() const override { return TypeKind::Tuple; }

    int compare(const Type& other) const override {
        if (auto *o = dynamic_cast<const TupleType*>(&other)) {
            if (types.size() != o->types.size())
                return types.size() < o->types.size() ? -1 : 1;
            
            for (size_t i = 0; i < types.size(); ++i) {
                int c = types[i]->compare(*o->types[i]);
                if (c != 0) return c;
            }

            return 0;
        }

        return int(kind()) - int(other.kind());
    }

    std::string show() const override {
        std::string s = typeConColor + "(";
        for (auto it = types.begin(); it != types.end(); ++it) {
            s += (*it)->show() + typeConColor + (std::next(it) != types.end() || it == types.begin() ? ", " : "");
        }

        return s + typeConColor + ")\033[0m";
    }

    TupleType(const std::vector<TypePtr>& types) : types(types) {}
};

struct OptionType : Type {
    const std::set<TypePtr, TypeLess> options;
    TypeKind kind() const override { return TypeKind::Option; }

    int compare(const Type& other) const override {
        if (auto *o = dynamic_cast<const OptionType*>(&other)) {
            if (options.size() != o->options.size())
                return options.size() < o->options.size() ? -1 : 1;
            
            auto it1 = options.begin();
            auto it2 = o->options.begin();
            for (; it1 != options.end(); ++it1, ++it2) {
                int c = (*it1)->compare(**it2);
                if (c != 0) return c;
            }

            return 0;
        }

        return int(kind()) - int(other.kind());
    }

    std::string show() const override {
        std::string s = "";
        bool isNullable = false;
        for (auto it = options.begin(); it != options.end(); ++it) {
            if (auto prim = std::dynamic_pointer_cast<const PrimType>(*it)) {
                if (prim->prim == Prim::Void) {
                    isNullable = true;
                    continue;
                }
            }
            s += (*it)->show() + (std::next(it) != options.end() ? typeConColor + "|" : "");
        }

        if (isNullable)
            s = typeConColor + "(" + s + typeConColor + ")?";
        
        return s + "\033[0m";
    }

    static std::set<TypePtr, TypeLess> merge(const std::vector<TypePtr>& opts) {
        std::set<TypePtr, TypeLess> merged;
        for (auto it = opts.begin(); it != opts.end(); ++it) {
            if (auto opt = std::dynamic_pointer_cast<const OptionType>(*it)) {
                for (auto o : opt->options)
                    merged.insert(o);
            }
            else
                merged.insert(*it);
        }

        if (merged.size() <= 1) {
            // Option types must have at least two different options.
            // Later merge collapsing options to non-option types.
            throw std::exception();
        }
        return merged;
    }

    OptionType(const std::vector<TypePtr>& opts) : options(merge(opts)) {}
};

struct FunctionType : Type {
    const std::vector<TypePtr> params;
    const TypePtr out;
    // maybe additional pure flag here later?
    TypeKind kind() const override { return TypeKind::Function; }

    int compare(const Type& other) const override {
        if (auto *o = dynamic_cast<const FunctionType*>(&other)) {
            int oc = out->compare(*o->out);
            if (oc != 0) return oc;

            if (params.size() != o->params.size())
                return params.size() < o->params.size() ? -1 : 1;
            
            for (size_t i = 0; i < params.size(); ++i) {
                int c = params[i]->compare(*o->params[i]);
                if (c != 0) return c;
            }

            return 0;
        }

        return int(kind()) - int(other.kind());
    }

    std::string show() const override {
        std::string s = typeConColor + "(";
        for (auto it = params.begin(); it != params.end(); ++it) {
            s += (*it)->show() + (std::next(it) != params.end() ? typeConColor + ", " : "");
        }

        return s + typeConColor + ")->" + out->show();
    }

    FunctionType(const std::vector<TypePtr>& params, const TypePtr& out) : params(params), out(out) {};
};