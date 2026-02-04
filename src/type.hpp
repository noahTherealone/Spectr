#pragma once

#include <memory>
#include <set>
#include <algorithm>
#include <unordered_map>
#include <vector>
#include "base.hpp"

const std::string primTypeColor = "\033[32m";
const std::string typeConColor  = "\033[33m";

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

enum class Prim {
    Void,
    Bool,
    Int,
    Num,
    Str
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
    TypePtr super;
    std::unordered_map<std::string, TypePtr> fields;
    TypeKind kind() const override { return TypeKind::Struct; }
    
    std::vector<std::string> sortedKeys() const {
        std::vector<std::string> keys;
        keys.reserve(fields.size());
        for (auto& [k, _] : fields) keys.push_back(k);
        std::sort(keys.begin(), keys.end());
        return keys;
    }

    int compare(const Type& other) const override;
    std::string show() const override;

    static TypePtr fromFields(const std::unordered_map<std::string, TypePtr>& fields) {
        if (fields.size() == 0)
            return VOID_TYPE;
        
        return std::make_shared<StructType>(nullptr, fields);
    }

    StructType(TypePtr super, const std::unordered_map<std::string, TypePtr>& fields) :
        super(super), fields(fields) {}
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