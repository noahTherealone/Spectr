#pragma once
#include <memory>
#include <type_traits>
#include <typeindex>
#include <stdexcept>
#include <iostream>
#include "AudioTypes.h"

struct IContainer { virtual ~IContainer() = default; virtual std::type_index type() const = 0; };
template<typename T>
struct Container : IContainer {
    T data;
    Container(T d) : data(std::move(d)) { }
    std::type_index type() const override { return typeid(T); }
};

// ---- helpers ----

template<typename L, typename R, typename Op>
static std::unique_ptr<IContainer> try_binary_op(const IContainer& a, const IContainer& b, Op&& op) {
    std::cout << "trying lhs as " << typeid(L).name()
          << " actual " << a.type().name() << "\n";

    auto lp = dynamic_cast<const Container<L>*>(&a);
    if (!lp) return nullptr;
    auto rp = dynamic_cast<const Container<R>*>(&b);
    if (!rp) return nullptr;

    using Res = std::decay_t<decltype(op(lp->data, rp->data))>;
    return std::make_unique<Container<Res>>(op(lp->data, rp->data));
}

// Recursively try all LHS types against all RHS types.
template<typename Op>
static std::unique_ptr<IContainer> dispatch_lhs_impl(const IContainer&, const IContainer&, Op&&) {
    return nullptr; // end of recursion
}

template<typename Op, typename L, typename... Ls>
static std::unique_ptr<IContainer> dispatch_lhs_impl(const IContainer& a, const IContainer& b, Op&& op) {
    
    if (auto r = try_binary_op<L, float   >(a, b, op)) return r;
    if (auto r = try_binary_op<L, Signal  >(a, b, op)) return r;
    if (auto r = try_binary_op<L, Spectrum>(a, b, op)) return r;
    
    return dispatch_lhs_impl<Op, Ls...>(a, b, std::forward<Op>(op));
}

template<typename Op>
static std::unique_ptr<IContainer> dispatch_binary(const IContainer& a, const IContainer& b, Op&& op) {
    // Extend this list when you add more runtime value types:
    return dispatch_lhs_impl<Op, float, Signal, Spectrum>(a, b, std::forward<Op>(op));
}

// ---- the public API you’ll call from backends ----

inline std::unique_ptr<IContainer> multiplyValues(const IContainer& a, const IContainer& b) {
    auto res = dispatch_binary(a, b, [](auto&& x, auto&& y) { return x * y; });
    if (!res) throw std::runtime_error("unsupported operand types for '*'");
    return res;
}

// (optional) addValues, subtractValues, divideValues can be identical
// except for the lambda (x + y), (x - y), (x / y).
