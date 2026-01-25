#pragma once

#include "Types.h"
#include "Expr.h"

struct Value;

struct List : public std::vector<Value> {
    using std::vector<Value>::vector;
};

struct Tuple : public std::vector<Value> {
    using std::vector<Value>::vector;
};

template<typename T>
Type getType(T obj) {
    if constexpr (std::is_same_v<T, List>) {
        if (obj.size() > 0) {
            std::vector<Type> options;
            for (size_t i = 0; i < obj.size(); i++)
                options.push_back(obj[i].type());
            
            return std::make_shared<ListType>(mergeOptions(options));
        }
        else
            return std::make_shared<ListType>(INT_TYPE);
    }

    if constexpr (std::is_same_v<T, Tuple>) {
        if (obj.size() > 1) {
            std::vector<Type> elements;
            for (size_t i = 0; i < obj.size(); i++)
                elements.push_back(obj[i].type());
            
            return std::make_shared<TupleType>(elements);
        }
        else if (obj.size() == 1)
            return getType(obj[0]);
        else {
            std::cout << "Tuples must have elements!" << std::endl;
            throw;
        }
    }

    return getType<T>();
}

struct Lambda {
    std::vector<std::shared_ptr<DeclExpr>> signature;
    std::shared_ptr<Expr> body; // maybe make into unique_ptr eventually

    Lambda() = default;
    Lambda(std::unique_ptr<Expr> sig, std::unique_ptr<Expr> bod) {
        auto s = dynamic_cast<TupleExpr*>(sig.get());
        if (!s) {
            std::cout << "Lambda signature must be tuple expression!" << std::endl;
            throw;
        }

        for (size_t i = 0; i < s->tuple.size(); ++i) {
            auto decl = dynamic_cast<DeclExpr*>(s->tuple[i].get());
            if (!decl) {
                std::cout << "Lambda signature entries must be declarations!" << std::endl;
                throw;
            }

            signature.push_back(std::make_shared<DeclExpr>(decl));
        }

        body = std::make_shared<Expr>(bod.get());
    }
};

struct Value {
private:
    Type _type;

public:
    Type type() const { return _type; }

    using Data = std::variant<
        bool,
        int,
        float,
        std::string,
        Signal,
        Spectrum,
        std::shared_ptr<Oscillator>,
        List,
        Tuple,
        Lambda
    >;

    Data data;

    std::string to_string() const {
        return std::visit(overloaded {
            [](bool b) { return std::string(b ? "true" : "false"); },
            [](int i) { return std::to_string(i); },
            [](float f) { return std::to_string(f); },
            [](const std::string& str) { return "\"" + str + "\""; },
            [](const Signal& sig) { return sig.to_string(); },
            [](const Spectrum& spec) { return spec.to_string(); },
            [](const std::shared_ptr<Oscillator>& o) { return o->to_string(); },
            [](const List& list) {
                std::string str;
                str += "{ ";
                for (const auto& v : list) {
                    str += v.to_string() + " ";
                }
                str += "}";
                return str;
            },
            [](const Tuple& tuple) {
                std::string str;
                str += "(";
                bool first = true;
                for (const auto& v : tuple) {
                    if (!first) str += ", ";
                    str += v.to_string();
                    first = false;
                }
                str += ")";
                return str;
            },
            [](const Lambda& lambda) {
                return lambda.body ? std::string("some code lulz") : std::string("no code found :(((");
            }
        }, data);
    }


    // Helper struct to allow multiple lambdas in std::visit
    template<class... Ts> struct overloaded : Ts... { using Ts::operator()...; };
    template<class... Ts> overloaded(Ts...) -> overloaded<Ts...>;

    Value(const Value& other) : _type(other.type()) {
        data = other.data;/*std::visit([&](auto&& arg) -> Data {
            using T = std::decay_t<decltype(arg)>;
            // we probably no longer need this because we're using shared pointers now
            if constexpr (std::is_same_v<T, std::shared_ptr<Oscillator>>) {
                if (arg) {
                    return std::shared_ptr<Oscillator>(arg->clone().release());
                } else {
                    return std::shared_ptr<Oscillator>{};
                }
            } else {
                return arg; // normal copy for everything else
            }
        }, other.data);*/
    }

    Value& operator=(const Value& other) {
        if (this == &other) return *this;
        if (!_type->canAssume(other.type()))
            throw std::runtime_error("Can't assign value of type " + typeName(other.type()) + " to variable of type " + typeName(_type));

        data = other.data;/*std::visit(
            [](auto&& arg) -> Data {
                using T = std::decay_t<decltype(arg)>;
                if constexpr (std::is_same_v<T, std::shared_ptr<Oscillator>>) {
                    if (arg) {
                    return std::shared_ptr<Oscillator>(arg->clone().release());
                    } else {
                        return std::shared_ptr<Oscillator>{};
                    }
                } else {
                    return arg;
                }
            },
            other.data
        );*/

        return *this;
    }

    Value(Value&& other) noexcept : _type(other.type()), data(std::move(other.data)) { }

    template<typename T>
    Value(const T& d) : _type(getType(d)), data(d) { }
};

struct TypeEqual {
    bool operator()(const Type& a, const Type& b) const {
        return a == b;
    }
};

static const Value DEFAULT_BOOL(false);
static const Value DEFAULT_INT(0);
static const Value DEFAULT_NUM(0.0f);


static const std::unordered_map<Type, Value, std::hash<Type>, TypeEqual> defaults{
    { BOOL_TYPE, DEFAULT_BOOL },
    { INT_TYPE, DEFAULT_INT },
    { NUM_TYPE, DEFAULT_NUM }
};

inline const Value* defaultValue(const Type& type) {
    if (type == BOOL_TYPE)
        return &DEFAULT_BOOL;
    if (type == INT_TYPE)
        return &DEFAULT_INT;
    if (type == NUM_TYPE)
        return &DEFAULT_NUM;

    if (auto* list = dynamic_cast<const ListType*>(type.get())) {
        auto value = new Value(List{ *defaultValue(list->element) });
        std::visit(
            [&](auto&& arg) {
                using T = std::decay_t<decltype(arg)>;
                if constexpr (std::is_same_v<T, List>)
                    arg.clear();
            },
            value->data
        );
        return value;
    }

    if (auto* tuple = dynamic_cast<const TupleType*>(type.get())) {
        Tuple values;
        for (auto elem : tuple->types) {
            if (auto* def = defaultValue(elem)) {
                values.push_back(*def);
            }
            else
                return nullptr;
        }

        return new Value(values);
    }
    
    return nullptr;
}