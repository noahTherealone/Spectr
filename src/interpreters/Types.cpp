#include "Types.h"

/*
// this is an idea, ask chatgpt about it
std::any cppTypeGetter(Type type) {
    if (auto* simple = dynamic_cast<SimpleType*>(type.get())) {
        switch (simple->type) {
            case PrimType::Bool:
                return false;
            case PrimType::Int:
                return 0;
            case PrimType::Num:
                return 0.0f;
            //case PrimType::Osc:
            //    return std::make_shared<Oscillator>();
            case PrimType::Signal:
                return Signal();
            default:
                throw; // cry about it
        }
    }

    if(type->equals(SPECTR_TYPE))
        return Spectrum();
    
    if (auto* list = dynamic_cast<ListType*>(type.get())) {
        return std::vector<std::any>{ cppTypeGetter(list->element) };
    }

    throw; // cry
}
*/