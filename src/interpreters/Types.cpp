#include "Types.h"

bool TypeBase::canAssume(const std::shared_ptr<const TypeBase>& other) const {
    if (auto* variant = dynamic_cast<const VariantType*>(other.get())) {
        for (auto opt : variant->options) {
            if (!canAssume(opt)) return false;
        }
    }

    return true;
};