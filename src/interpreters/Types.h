#pragma once

#include "AudioTypes.h"
#include <memory>
#include <vector>
#include <unordered_map>
#include <regex>
#include <iostream> // remove later!
#include <any>
#include <variant>

enum class PrimType {
    Bool,
    Int,
    Num,
    Signal,
    Osc,
    String,
};

struct TypeBase {
    virtual ~TypeBase() = default;
    virtual std::shared_ptr<const TypeBase> clone() const = 0;
    virtual bool equals(const std::shared_ptr<const TypeBase>& other) const = 0;
    virtual bool canAssume(const std::shared_ptr<const TypeBase>& other) const;
};

using Type = std::shared_ptr<const TypeBase>;

inline bool operator==(const Type& a, const Type& b) {
    return a->equals(b);
}

inline bool operator!=(const Type& a, const Type& b) {
    return !(a == b);
}

struct SimpleType : TypeBase {
    const PrimType type;

    explicit SimpleType(PrimType t) : type(t) {}

    Type clone() const override {
        return std::make_shared<const SimpleType>(type);
    }

    bool equals(const Type& other) const override {
        if (auto* o = dynamic_cast<const SimpleType*>(other.get())) {
            return type == o->type;
        }
        return false;
    }

    bool canAssume(const Type& other) const override {
        if (!TypeBase::canAssume(other)) return false;

        if (auto* simple = dynamic_cast<const SimpleType*>(other.get()))
            return type == simple->type;
        
        return false;
    }
};

struct ListType : TypeBase {
    Type element;

    explicit ListType(Type e) : element(e) {}

    Type clone() const override {
        return std::make_shared<const ListType>(element->clone());
    }

    bool equals(const Type& other) const override {
        if (auto* o = dynamic_cast<const ListType*>(other.get())) {
            return element->equals(o->element);
        }
        return false;
    }

    bool canAssume(const Type& other) const override {
        if (!TypeBase::canAssume(other)) return false;

        if (auto* list = dynamic_cast<const ListType*>(other.get())) {
            return element->canAssume(list->element);
        }

        return false;
    }
};

struct VariantType : TypeBase {
    std::vector<Type> options;

    explicit VariantType(const std::vector<Type>& opts) {
        for (auto& o : opts) addOption(o->clone());
    }

    explicit VariantType(const Type& left, const Type& right) {
        if (auto* varLeft = dynamic_cast<const VariantType*>(left.get())) {
            options = varLeft->options;
            if (auto* varRight = dynamic_cast<const VariantType*>(right.get())) {
                for (Type t : varRight->options) addOption(t);
            }
            else
                addOption(right);
        }
        else if (auto* varRight = dynamic_cast<const VariantType*>(right.get())) {
            options = { left };
            for (Type t : varRight->options) addOption(t);
        }
        else options = { left, right };
    } 

    Type clone() const override {
        std::vector<Type> cloned;
        for (auto& o : options) cloned.push_back(o->clone());
        return std::make_shared<VariantType>(cloned);
    }

    bool equals(const Type& other) const override {
        if (auto* o = dynamic_cast<const VariantType*>(other.get())) {
            if (options.size() != o->options.size()) return false;
            for (size_t i = 0; i < options.size(); i++) {
                bool foundMatch = false;
                for (size_t j = 0; j < o->options.size(); j++) {
                    if (!options[i]->equals(o->options[j])) {
                        foundMatch = true;
                        break;
                    }
                }

                if (!foundMatch) return false;
            }
            return true;
        }
        return false;
    }

    bool canAssume(const Type& other) {
        if (!TypeBase::canAssume(other)) return false;

        for (auto match : options) {
            if (match->canAssume(other))
                return true;
        }

        return false;
    }

private:
    void addOption(const Type& opt) {
        for (auto type : options) {
            if (opt == type) return;
        }
        
        options.push_back(opt);
    }
};

struct TupleType : TypeBase {
    std::vector<Type> types;

    explicit TupleType(const std::vector<Type>& t) : types(t) {}

    Type clone() const override {
        return std::make_shared<const TupleType>(types);
    }

    bool equals(const Type& other) const override {
        if (auto* o = dynamic_cast<const TupleType*>(other.get())) {
            if (o->types.size() != types.size()) return false;
            for (size_t i = 0; i < types.size(); i++)
                if (!types[i]->equals(o->types[i])) return false;
            
            return true;
        }
        return false;
    }

    bool canAssume(const Type& other) const override {
        if (!TypeBase::canAssume(other)) return false;

        if (auto* tuple = dynamic_cast<const TupleType*>(other.get())) {
            if (types.size() != tuple->types.size()) return false;

            for (size_t i = 0; i < types.size(); i++) {
                if (!types[i]->canAssume(tuple->types[i]))
                    return false;
            }

            return true;
        }

        return false;
    }
};

struct LambdaType : TypeBase {
    std::shared_ptr<TupleType> input;
    Type output;

    explicit LambdaType(const std::shared_ptr<TupleType>& in, const Type& out) : input(in), output(out) {
        if (!in) {
            std::cout << "Lambdas need an input tuple!" << std::endl;
            // output may be nullptr though, which is distinct from potential future null type
            throw;
        }
    }

    Type clone() const override {
        return std::make_shared<const LambdaType>(input, output);
    }

    bool equals(const Type& other) const override {
        if (auto* o = dynamic_cast<const LambdaType*>(other.get())) {
            return input->equals(o->input) && output->equals(o->output);
        }

        return false;
    }

    bool canAssume(const Type& other) const override {
        if (!TypeBase::canAssume(other)) return false;

        if (auto* lambda = dynamic_cast<const LambdaType*>(other.get()))
            return input->canAssume(lambda->input) && (!output || output->canAssume(lambda->output));

        return false;
    }
};

static const Type BOOL_TYPE = std::make_shared<SimpleType>(SimpleType(PrimType::Bool));
static const Type INT_TYPE = std::make_shared<SimpleType>(SimpleType(PrimType::Int));
static const Type NUM_TYPE = std::make_shared<SimpleType>(SimpleType(PrimType::Num));
static const Type SIG_TYPE = std::make_shared<SimpleType>(SimpleType(PrimType::Signal));
static const Type OSC_TYPE = std::make_shared<SimpleType>(SimpleType(PrimType::Osc));
static const Type STR_TYPE = std::make_shared<SimpleType>(SimpleType(PrimType::String));
static const Type BOOLS_TYPE = std::make_shared<ListType>(ListType(BOOL_TYPE));
static const Type INTS_TYPE = std::make_shared<ListType>(ListType(INT_TYPE));
static const Type NUMS_TYPE = std::make_shared<ListType>(ListType(NUM_TYPE));
static const Type SPECTR_TYPE = std::make_shared<ListType>(ListType(SIG_TYPE));

static const std::unordered_map<std::string, Type> namedTypes({
    { "bool", BOOL_TYPE },
    { "int", INT_TYPE },
    { "num", NUM_TYPE },
    { "sig", SIG_TYPE },
    { "osc", OSC_TYPE },
    { "str", STR_TYPE },
    { "bools", BOOLS_TYPE },
    { "ints", INTS_TYPE },
    { "nums", NUMS_TYPE },
    { "spectr", SPECTR_TYPE }
});

inline Type fromString(const std::string& name) {
    if (namedTypes.contains(name)) return namedTypes.at(name);

    std::cout << "\033[0;31mINVALID TYPE NAME '" + name + "'\033[0m" << std::endl;
    throw;
    return nullptr;
}

inline std::string typeName(Type type) {
    for (auto it : namedTypes) {
        if (it.second == type)
            return it.first;
    }

    if (auto* list = dynamic_cast<const ListType*>(type.get()))
        return "{" + typeName(list->element) + "}";

    if (auto* variant = dynamic_cast<const VariantType*>(type.get())) {
        std::string name = typeName(variant->options[0]);
        for (size_t i = 1; i < variant->options.size(); i++)
            name.append("|" + typeName(variant->options[i]));
        
        return name;
    }

    if(auto* tuple = dynamic_cast<const TupleType*>(type.get())) {
        std::string name = "(" + typeName(tuple->types[0]);
        for (size_t i = 1; i < tuple->types.size(); i++)
            name.append("," + typeName(tuple->types[i]));
        
        return name + ")";
    }

    if(auto* lambda = dynamic_cast<const LambdaType*>(type.get())) {
        return "$" + typeName(lambda->input) + "->" + typeName(lambda->output);
    }

    return "UNKNOWN_TYPE";
}

template<typename>
struct is_std_vector : std::false_type {};

template<typename T, typename Alloc>
struct is_std_vector<std::vector<T, Alloc>> : std::true_type {
    using element_type = T;
};

inline Type mergeOptions(std::vector<Type> options) {
    if (options.size() > 1) {
        bool allEqual = true;
        for (size_t i = 1; i < options.size(); i++) {
            if (options[i-1] != options[i]) {
                allEqual = false;
                break;
            }
        }

        if (allEqual)
            return options[0];
        else
            return std::make_shared<const VariantType>(options);
    }

    if (options.size() == 1)
        return options[0];
    
    return nullptr;
}

template<typename T>
Type getType() {
    if constexpr (std::is_same_v<T, bool>)        return BOOL_TYPE;
    if constexpr (std::is_same_v<T, int>)         return INT_TYPE;
    if constexpr (std::is_same_v<T, float>)       return NUM_TYPE;
    if constexpr (std::is_same_v<T, std::string>) return STR_TYPE;
    if constexpr (std::is_same_v<T, Signal>)      return SIG_TYPE;
    if constexpr (std::is_same_v<T, std::shared_ptr<Oscillator>>) return OSC_TYPE;
    if constexpr (std::is_same_v<T, Spectrum>)    return SPECTR_TYPE;
    
    return nullptr;
}

