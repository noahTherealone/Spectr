#pragma once

#include <memory>
#include <set>
#include <algorithm>
#include "type_expression.hpp"

enum class TypeKind {
    INVALID,
    Prim,
    List,
    Tuple,
    Option,
    Function,
    Struct,
    Any
};

struct Type {
    virtual ~Type() = default;
    virtual TypeKind kind() const = 0;
    virtual int compare(const Type& other) const = 0;
    virtual std::string show() const = 0;
};

struct InvalidType : Type {
    TypeKind kind() const override { return TypeKind::INVALID; }
    int compare(const Type& other) const override;
    std::string show() const override;
    
    InvalidType() = default;
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
    int compare(const Type& other) const override;
    std::string show() const override;

    PrimType(Prim prim) : prim(prim) {}
};

static const TypePtr INVALID_TYPE = std::make_shared<InvalidType>();

static const TypePtr VOID_TYPE = std::make_shared<PrimType>(Prim::Void);
static const TypePtr BOOL_TYPE = std::make_shared<PrimType>(Prim::Bool);
static const TypePtr INT_TYPE  = std::make_shared<PrimType>(Prim::Int);
static const TypePtr NUM_TYPE  = std::make_shared<PrimType>(Prim::Num);
static const TypePtr STR_TYPE  = std::make_shared<PrimType>(Prim::Str);

struct ListType : Type {
    const TypePtr type;
    TypeKind kind() const override { return TypeKind::List; }
    int compare(const Type& other) const override;
    std::string show() const override;

    ListType(const TypePtr& type) : type(type) {}
};

struct TupleType : Type {
    const std::vector<TypePtr> types;
    TypeKind kind() const override { return TypeKind::Tuple; }
    int compare(const Type& other) const override;
    std::string show() const override;

    static TypePtr toTuple(const std::vector<TypePtr>& types) {
        if (types.size() == 0) return VOID_TYPE;
        if (types.size() == 1) return *types.begin();
        return std::make_shared<TupleType>(types);
    }

    TupleType(const std::vector<TypePtr>& types) : types(types) {}
};

struct UnionType : Type {
    const std::set<TypePtr, TypeLess> options;
    TypeKind kind() const override { return TypeKind::Option; }
    int compare(const Type& other) const override;
    std::string show() const override;

    static std::set<TypePtr, TypeLess> merge(const std::vector<TypePtr>& opts) {
        std::set<TypePtr, TypeLess> merged;
        for (auto it = opts.begin(); it != opts.end(); ++it) {
            if (auto opt = std::dynamic_pointer_cast<const UnionType>(*it)) {
                for (auto o : opt->options)
                    merged.insert(o);
            }
            else if (*it)
                merged.insert(*it);
        }

        return merged;
    }

    static TypePtr fromOptions(const std::vector<TypePtr>& opts) {
        auto set = UnionType::merge(opts);
        if (set.size() == 0)
            return VOID_TYPE;

        if (set.size() == 1)
            return *set.begin();
        
        return std::make_shared<const UnionType>(set);
    }

    UnionType(const std::set<TypePtr, TypeLess>& options) : options(options) {}
};

struct LambdaType : Type {
    const TypePtr arg;
    const TypePtr out;
    // maybe additional pure flag here later?
    TypeKind kind() const override { return TypeKind::Function; }
    int compare(const Type& other) const override;
    std::string show() const override;

    LambdaType(const TypePtr& arg, const TypePtr& out) : arg(arg), out(out) {};
};

struct StructType : Type {
    std::unique_ptr<std::string> name;
    TypePtr super;
    std::unordered_map<std::string, TypePtr> fields;
    TypeKind kind() const override { return TypeKind::Struct; }
    
    static std::vector<std::string> sortedKeys(const std::unordered_map<std::string, TypePtr>& m) {
        std::vector<std::string> keys;
        keys.reserve(m.size());
        for (auto& [k, _] : m) keys.push_back(k);
        std::sort(keys.begin(), keys.end());
        return keys;
    }

    int compare(const Type& other) const override {
        if (auto o = dynamic_cast<const StructType*>(&other)) {
            if (name && o->name && *name != *o->name)
                return *name < *o->name ? -1 : 1;

            int d = (o->super == nullptr) - (super == nullptr);
            if (d != 0) return d;

            if (super) {
                d = super->compare(*o->super);
                if (d != 0) return d;
            }

            d = fields.size() - o->fields.size();
            if (d != 0) return d;

            auto keysA = sortedKeys(fields);
            auto keysB = sortedKeys(o->fields);

            for (size_t i = 0; i < keysA.size(); i++) {
                if (keysA[i] != keysB[i])
                    return keysA[i] < keysB[i] ? -1 : 1;
                
                d = fields.at(keysA[i])->compare(*o->fields.at(keysB[i]));
                if (d != 0) return d;
            }
        }

        return int(kind()) - int(other.kind());
    }

    StructType(const std::string& name, TypePtr super, std::unordered_map<std::string, TypePtr> fields) :
        name(name.size() > 0 ? std::make_unique<std::string>(name) : nullptr), super(super), fields(fields) {}
};

struct AnyType : Type {
    TypeKind kind() const override { return TypeKind::Any; }
    int compare(const Type& other) const override {
        return int(kind()) - int(other.kind());
    }

    std::string show() const override {
        return primTypeColor + "any\033[0m";
    }

    AnyType() {};
};

static const TypePtr ANY_TYPE  = std::make_shared<AnyType>();

// Inheritance operator: a <= b means a is a subtype of b
bool operator<=(const TypePtr& a, const TypePtr& b);