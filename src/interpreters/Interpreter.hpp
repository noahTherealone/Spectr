#pragma once

#include "Expr.h"
#include "Types.h"
#include "Values.h"
#include "PlaybackEvents.h"
#include "LogSettings.h"
#include <unordered_map>
#include <iostream>
#include <memory>
#include <variant>

class _Interpreter {
protected:
    std::unique_ptr<Value> currentVal;
    LogSettings* logSettings;
    
public:
    PlaybackEventStream* eventStream;

    void setValue(std::nullptr_t) {
        currentVal = nullptr;
    }

    template<typename T>
    void setValue(T value) {
        currentVal = std::make_unique<Value>(value);
    }

    void clearValue() {
        currentVal = nullptr;
    }

    template<typename T>
    T& getValue() { 
        if (!currentVal) {
            runtimeError("No value to access", 0, 0);
            throw;
        }

        auto val = std::get_if<T>(&currentVal->data);
        if (!val) {
            runtimeError("Couldn't cast types", 0, 0);
            throw;
        }

        return *val;
    }

    bool isType(Type type) {
        return currentVal && currentVal->type() == type;
    }

    bool isFunc() {
        return currentVal && dynamic_cast<const LambdaType*>(currentVal->type().get());
    }

    bool isEmpty() { return !currentVal; }

    virtual void evalExpr(const BoolExpr& expr) = 0;
    virtual void evalExpr(const NumExpr& expr) = 0;
    virtual void evalExpr(const IntExpr& expr) = 0;
    virtual void evalExpr(const StrExpr& expr) = 0;
    virtual void evalExpr(const SignalExpr& expr) = 0;
    virtual void evalExpr(const ListExpr& expr) = 0;
    virtual void evalExpr(const TupleExpr& expr) = 0;
    virtual void evalExpr(const OscPrimExpr& expr) = 0;
    virtual void evalExpr(const IdentifierExpr& expr) = 0;
    virtual void evalExpr(const LabelledExpr& expr) = 0;
    virtual void evalExpr(const UnaryExpr& expr) = 0;
    virtual void evalExpr(const BinaryExpr& expr) = 0;
    virtual void evalExpr(const AssignmentExpr& expr) = 0;
    virtual void evalExpr(const DeclExpr& expr) = 0;
    virtual void evalExpr(const BlockExpr& expr) = 0;
    virtual void evalExpr(const OutExpr& expr) = 0;
    virtual void evalExpr(const LambdaExpr& expr) = 0;
    virtual void evalExpr(const FuncApplExpr& expr) = 0;
    virtual void evalExpr(const PlaybackExpr& expr) = 0;
    virtual void evalExpr(const ReleaseExpr& expr) = 0;

    virtual void pushPlaybackEvent(const PlaybackExpr& expr, const std::string* label = nullptr) = 0;

    // expects to set value before calling
    virtual void declVar(Type type, const std::string& name) = 0;
    virtual void getVar(const std::string& name) = 0;
    virtual void setVar(const std::string& name) = 0;

    virtual void println(const std::string& msg) = 0;
    //virtual void parserError(const std::string& msg, size_t line, size_t col) = 0;
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

    std::unordered_map<std::string, Value> variables;

public:
    void evalExpr(const BoolExpr& expr) override;
    void evalExpr(const NumExpr& expr) override;
    void evalExpr(const IntExpr& expr) override;
    void evalExpr(const StrExpr& expr) override;
    void evalExpr(const SignalExpr& expr) override;
    void evalExpr(const ListExpr& expr) override;
    void evalExpr(const TupleExpr& expr) override;
    void evalExpr(const OscPrimExpr& expr) override;
    void evalExpr(const IdentifierExpr& expr) override;
    void evalExpr(const LabelledExpr& expr) override;
    void evalExpr(const UnaryExpr& expr) override;
    void evalExpr(const BinaryExpr& expr) override;
    void evalExpr(const AssignmentExpr& expr) override;
    void evalExpr(const DeclExpr& expr) override;
    void evalExpr(const BlockExpr& expr) override;
    void evalExpr(const OutExpr& expr) override;
    void evalExpr(const LambdaExpr& expr) override;
    void evalExpr(const FuncApplExpr& expr) override;
    void evalExpr(const PlaybackExpr& expr) override;
    void evalExpr(const ReleaseExpr& expr) override;

    void pushPlaybackEvent(const PlaybackExpr& expr, const std::string* label = nullptr) override;

    // expects to set value before calling
    void declVar(Type type, const std::string& name) override;
    void getVar(const std::string& name) override;
    void setVar(const std::string& name) override;

    void println(const std::string& msg) override;
    //void parserError(const std::string& msg, size_t line, size_t col) override;
    void runtimeError(const std::string& msg, size_t line, size_t col) override;

    Interpreter() = default;
    Interpreter(PlaybackEventStream* eStreamAdress, LogSettings* ls, double* gTime=nullptr) {
        eventStream = eStreamAdress;
        logSettings = ls;
        sessionTime = gTime;
    }
};