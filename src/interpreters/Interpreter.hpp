#pragma once

#include "Algebra.hpp"
#include "Expr.h"
#include "Types.h"
#include "PlaybackEvents.h"
#include <unordered_map>
#include <iostream>
#include <memory>
#include <any>

class _Interpreter {
protected:
    std::unique_ptr<IContainer> currentValue;
    Type currentType;
    //std::unique_ptr<Value> currentVal;
    
public:
    //std::vector<std::unique_ptr<PlaybackEvent>>* events;
    PlaybackEventStream* eventStream;
    std::unordered_map<std::string, Type> varTypes;

    template<typename T>
    void setValue(T value) {
        currentValue = std::make_unique<Container<T>>(Container<T>(std::move(value)));
        currentType = getType<T>();
    }

    template<typename T>
    T& getValue() const { 
        auto ptr = dynamic_cast<Container<T>*>(currentValue.get());
        if (!ptr) throw std::bad_cast();
        return (*ptr).data;
    }

    template<typename T>
    T* tryGetValue() const {
        return dynamic_cast<Container<T>*>(currentValue.get());
    }

    std::any getValueAsAny() const {
        return std::any(currentValue.get());
    }

    //template<typename T>
    //bool is() const {
    //    return currentValue && currentValue->type() == typeid(T);
    //}

    bool isType(Type type) {
        return currentType && currentType->equals(type);
    }

    bool isFunc() {
        return currentType && dynamic_cast<LambdaType*>(currentType.get());
    }

    bool isEmpty() { return !currentType; }
    

    IContainer* getIValue() const { return currentValue.get(); }
    void setIValue(std::unique_ptr<IContainer> value) { currentValue = std::move(value); }

    virtual void evalExpr(const BoolExpr& expr) = 0;
    virtual void evalExpr(const NumExpr& expr) = 0;
    virtual void evalExpr(const IntExpr& expr) = 0;
    virtual void evalExpr(const SignalExpr& expr) = 0;
    virtual void evalExpr(const ListExpr& expr) = 0;
    virtual void evalExpr(const OscPrimExpr& expr) = 0;
    virtual void evalExpr(const IdentifierExpr& expr) = 0;
    virtual void evalExpr(const LabelledExpr& expr) = 0;
    virtual void evalExpr(const UnaryExpr& expr) = 0;
    virtual void evalExpr(const BinaryExpr& expr) = 0;
    virtual void evalExpr(const AssignmentExpr& expr) = 0;
    virtual void evalExpr(const DeclExpr& expr) = 0;
    virtual void evalExpr(const BlockExpr& expr) = 0;
    virtual void evalExpr(const OutExpr& expr) = 0;
    virtual void evalExpr(const FuncApplExpr& expr) = 0;
    virtual void evalExpr(const PlaybackExpr& expr) = 0;
    virtual void evalExpr(const ReleaseExpr& expr) = 0;

    virtual void pushPlaybackEvent(const PlaybackExpr& expr, const std::string* label = nullptr) = 0;

    //virtual bool availableName(const std::string& name) = 0;

    // expects to set value before calling
    virtual void declVar(Type type, const std::string& name) = 0;
    virtual void getVar(const std::string& name) = 0;
    virtual void setVar(const std::string& name) = 0;

    virtual void parserError(const std::string& msg, size_t line, size_t col) = 0;
    virtual void runtimeError(const std::string& msg, size_t line, size_t col) = 0;

    _Interpreter() = default;
    _Interpreter(PlaybackEventStream* eStreamAdress, double* gTime=nullptr)
        : eventStream(eStreamAdress) { }
    
    virtual ~_Interpreter() = default;
};

class Interpreter : public _Interpreter {
protected:
    double tempo = 1.0, time = 0.0;
    double* sessionTime;
    Signal master;
    std::unordered_map<std::string, bool> bools;
    std::unordered_map<std::string, int> ints;
    std::unordered_map<std::string, float> nums;
    std::unordered_map<std::string, std::unique_ptr<Signal>> signals;
    std::unordered_map<std::string, std::unique_ptr<Spectrum>> spectra;
    std::unordered_map<std::string, std::unique_ptr<Oscillator>> oscillators;
    std::unordered_map<std::string, std::any> vars;

    //std::unordered_map<std::string, Value> variables;

public:
    void evalExpr(const BoolExpr& expr) override;
    void evalExpr(const NumExpr& expr) override;
    void evalExpr(const IntExpr& expr) override;
    void evalExpr(const SignalExpr& expr) override;
    void evalExpr(const ListExpr& expr) override;
    void evalExpr(const OscPrimExpr& expr) override;
    void evalExpr(const IdentifierExpr& expr) override;
    void evalExpr(const LabelledExpr& expr) override;
    void evalExpr(const UnaryExpr& expr) override;
    void evalExpr(const BinaryExpr& expr) override;
    void evalExpr(const AssignmentExpr& expr) override;
    void evalExpr(const DeclExpr& expr) override;
    void evalExpr(const BlockExpr& expr) override;
    void evalExpr(const OutExpr& expr) override;
    void evalExpr(const FuncApplExpr& expr) override;
    void evalExpr(const PlaybackExpr& expr) override;
    void evalExpr(const ReleaseExpr& expr) override;

    void pushPlaybackEvent(const PlaybackExpr& expr, const std::string* label = nullptr) override;

    // bool availableName(const std::string& name) override;

    // expects to set value before calling
    void declVar(Type type, const std::string& name) override;
    void getVar(const std::string& name) override;
    void setVar(const std::string& name) override;

    void parserError(const std::string& msg, size_t line, size_t col) override;
    void runtimeError(const std::string& msg, size_t line, size_t col) override;

    Interpreter(/*std::vector<std::unique_ptr<PlaybackEvent>>* eventAdress*/)/* { events = eventAdress; }*/ = default;
    Interpreter(PlaybackEventStream* eStreamAdress, double* gTime=nullptr) { eventStream = eStreamAdress; sessionTime = gTime; }
};