#include "type.hpp"

// Inheritance operator: a <= b means a is a subtype of b
bool operator<=(const TypePtr& a, const TypePtr& b) {
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
            
            if (aLambda->params.size() != bLambda->params.size())
                return false;
            
            for (size_t i = 0; i < aLambda->params.size(); i++) {
                if (!(bLambda->params[i] <= aLambda->params[i]))
                    return false;
            }

            return true;
        }

        return false;
    }

    return false;
};
