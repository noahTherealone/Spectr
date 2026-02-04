#include "type.hpp"
#include "type_expression.hpp"

int InvalidType::compare(const Type& other) const {
    if (auto *o = dynamic_cast<const InvalidType*>(&other))
        return 0;
    
    return -1;
}

std::string InvalidType::show() const {
    return "\033[31mINVALID_TYPE\033[0m";
}

int PrimType::compare(const Type& other) const {
    if (auto *o = dynamic_cast<const PrimType*>(&other))
        return int(prim) - int(o->prim);
    
    return int(kind()) - int(other.kind());
}

std::string PrimType::show() const {
    return primTypeColor + primNames.at(prim) + "\033[0m";
}

int ListType::compare(const Type& other) const {
    if (auto *o = dynamic_cast<const ListType*>(&other)) {
        return type->compare(*o->type);
    }

    return int(kind()) - int(other.kind());
}

std::string ListType::show() const {
    return typeConColor + "{" + type->show() + typeConColor + "}\033[0m";
}

int TupleType::compare(const Type& other) const {
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

std::string TupleType::show() const {
    std::string s = typeConColor + "(";
    for (auto it = types.begin(); it != types.end(); ++it) {
        s += (*it)->show() + typeConColor + (std::next(it) != types.end() || it == types.begin() ? ", " : "");
    }

    return s + typeConColor + ")\033[0m";
}

int UnionType::compare(const Type& other) const {
    if (auto *o = dynamic_cast<const UnionType*>(&other)) {
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

std::string UnionType::show() const {
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

int LambdaType::compare(const Type& other) const {
    if (auto *o = dynamic_cast<const LambdaType*>(&other)) {
        int oc = out->compare(*o->out);
        if (oc != 0) return oc;

        int ac = arg->compare(*o->arg);
        if (ac != 0) return ac;

        return 0;
    }

    return int(kind()) - int(other.kind());
}

std::string LambdaType::show() const {
    return typeConColor + "(" + arg->show() + typeConColor + "->" + out->show() + typeConColor + ")\033[0m";
}

int StructType::compare(const Type& other) const {
    if (auto o = dynamic_cast<const StructType*>(&other)) {
        int d = (o->super == nullptr) - (super == nullptr);
        if (d != 0) return d;

        if (super) {
            d = super->compare(*o->super);
            if (d != 0) return d;
        }

        d = fields.size() - o->fields.size();
        if (d != 0) return d;

        auto keysA = sortedKeys();
        auto keysB = o->sortedKeys();

        for (size_t i = 0; i < keysA.size(); i++) {
            if (keysA[i] != keysB[i])
                return keysA[i] < keysB[i] ? -1 : 1;
            
            d = fields.at(keysA[i])->compare(*o->fields.at(keysB[i]));
            if (d != 0) return d;
        }
    }

    return int(kind()) - int(other.kind());
}

std::string StructType::show() const {
    std::string s = typeConColor + "[ ";
    auto keys = sortedKeys();
    for (auto it = keys.begin(); it != keys.end(); it++) {
        s += "\033[0m" + *it + typeConColor + ": " + fields.at(*it)->show();
        if (std::next(it) != keys.end())
            s += typeConColor + ", ";
    }

    return s + typeConColor + " ]\033[0m";
};

// Inheritance operator: a <= b means a is a subtype of b
bool operator<=(const TypePtr& a, const TypePtr& b) {
    if (b->compare(*ANY_TYPE) == 0 || b->compare(*INVALID_TYPE) == 0) return true;

    if (auto bOptions = dynamic_cast<const UnionType*>(b.get())) {
        if (auto aOptions = dynamic_cast<const UnionType*>(a.get())) {
            for (auto aIt = aOptions->options.begin(); aIt != aOptions->options.end(); aIt++) {
                for (auto bIt = bOptions->options.begin(); bIt != bOptions->options.end(); bIt++) {
                    if (*aIt <= *bIt)
                        break;
                    
                    if (std::next(bIt) == bOptions->options.end())
                        return false;
                }
            }

            return true;
        }

        for (auto it = bOptions->options.begin(); it != bOptions->options.end(); it++) {
            if (a <= *it) return true;
        }

        return false;
    }

    if (auto aPrim = dynamic_cast<const PrimType*>(a.get())) {
        if (auto bPrim = dynamic_cast<const PrimType*>(b.get()))
            return aPrim->prim == bPrim->prim;
        
        return false;
    }

    if (auto aList = dynamic_cast<const ListType*>(a.get())) {
        if (auto bList = dynamic_cast<const ListType*>(b.get()))
            return aList->type <= bList->type && bList->type <= aList->type;
        
        return false;
    }

    if (auto aTuple = dynamic_cast<const TupleType*>(a.get())) {
        if (auto bTuple = dynamic_cast<const TupleType*>(b.get())) {
            if (aTuple->types.size() != bTuple->types.size())
                return false;
            
            for (size_t i = 0; i < aTuple->types.size(); i++) {
                if (!(aTuple->types[i] <= bTuple->types[i]))
                    return false;
            }
        }

        return true;
    }

    if (auto aLambda = dynamic_cast<const LambdaType*>(a.get())) {
        if (auto bLambda = dynamic_cast<const LambdaType*>(b.get())) {
            if (!(aLambda->out <= bLambda->out))
                return false;
            
            if (!(bLambda->arg <= aLambda->arg))
                return false;

            return true;
        }

        return false;
    }

    if (const StructType* aStruct = dynamic_cast<const StructType*>(a.get())) {
        if (const StructType* bStruct = dynamic_cast<const StructType*>(b.get())) {
            for (auto key : aStruct->sortedKeys()) {
                if (!bStruct->fields.contains(key))
                    return false;
                
                if (aStruct->fields.at(key)->compare(*bStruct->fields.at(key)) != 0)
                    return false;
            }

            return true;
        }
    }

    return false;
};
