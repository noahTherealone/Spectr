#include "Interpreter.hpp"

void Interpreter::evalExpr(const BoolExpr& expr) {
    setValue(expr.value);
}

void Interpreter::evalExpr(const NumExpr& expr) {
    setValue(expr.value);
}

void Interpreter::evalExpr(const IntExpr& expr) {
    setValue(expr.value);
}

void Interpreter::evalExpr(const StrExpr& expr) {
    setValue(expr.value);
}

void Interpreter::evalExpr(const IdentifierExpr& expr) {
    // later manage scope here as well
    getVar(expr.name);
}

void Interpreter::evalExpr(const LabelledExpr& expr) {
    if (auto play = dynamic_cast<PlaybackExpr*>(expr.expr.get())) {
        pushPlaybackEvent(*play, &expr.label);
    }
    else
        runtimeError("Labelled an unsupported kind of expression with '" + expr.label + ".", expr.line, expr.column);
}

void Interpreter::evalExpr(const UnaryExpr& expr) {
    expr.param->accept(*this);
    if (expr.op == TokenType::Plus) {
        auto value = getValue<float>();
        time += tempo * value;
        println("Timestamp: +" + std::to_string(value) + " (@" + std::to_string(time) + ")");
    }
    else if (expr.op == TokenType::At) {
        auto value = getValue<float>();
        time = value;
        println("Timestamp: @" + std::to_string(time));
    }
    else if (isType(NUM_TYPE)) {
        if (expr.op == TokenType::Minus)
            setValue(-getValue<float>());
        else if (isType(NUM_TYPE)) {
            auto value = getValue<float>();
            if (value != 0.0f) {
                setValue<float>(1.0f / value);
                return;
            }

            runtimeError("Can't divide by zero.", expr.line, expr.column);
        }
    }
}

void Interpreter::evalExpr(const BinaryExpr& expr) {
    expr.left->accept(*this);
    if (isType(NUM_TYPE)) {
        auto left = getValue<float>();
        expr.right->accept(*this);
        if (isType(NUM_TYPE)) {
            auto right = getValue<float>();
            switch (expr.op) {
                case TokenType::Plus:
                    setValue<float>(left + right);
                    break;
                case TokenType::Minus:
                    setValue<float>(left - right);
                    break;
                case TokenType::Star:
                    setValue<float>(left * right);
                    break;
                case TokenType::Slash:
                    setValue<float>(left / right);
                    break;
            }
            println("Computed num: " + std::to_string(getValue<float>()));
        }
        else if (isType(SIG_TYPE)) {
            auto right = getValue<Signal>();
            switch (expr.op) {
                case TokenType::Plus:
                    setValue<Spectrum>(left + right);
                    println("Computed spectrum: " + getValue<Spectrum>().to_string());
                    break;
                case TokenType::Star:
                    setValue<Signal>(left * right);
                    println("Computed signal: " + getValue<Signal>().to_string());
                    break;
            }
        }
        else if (isType(SPECTR_TYPE)) {
            auto right = getValue<Spectrum>();
            switch (expr.op) {
                case TokenType::Plus:
                    setValue<Spectrum>(left + right);
                    break;
                case TokenType::Star:
                    setValue<Spectrum>(left * right);
                    break;
            }
            println("Computed spectrum: " + getValue<Spectrum>().to_string());
        }
    }
    else if (isType(SIG_TYPE)) {
        auto left = getValue<Signal>();
        expr.right->accept(*this);
        if (isType(NUM_TYPE)) {
            auto right = getValue<float>();
            switch (expr.op) {
                case TokenType::Plus:
                    setValue<Spectrum>(left + right);
                    println("Computed spectrum: " + getValue<Spectrum>().to_string());
                    break;
                case TokenType::Star:
                    setValue<Signal>(left * right);
                    println("Computed signal: " + getValue<Signal>().to_string());
                    break;
                case TokenType::Slash:
                    setValue<Signal>(left / right);
                    println("Computed signal: " + getValue<Signal>().to_string());
                    break;
            }
        }
        else if (isType(SIG_TYPE)) {
            auto right = getValue<Signal>();
            switch (expr.op) {
                case TokenType::Plus:
                    setValue<Spectrum>(left + right);
                    println("Computed spectrum: " + getValue<Spectrum>().to_string());
                    break;
                case TokenType::Star:
                    setValue<Signal>(left * right);
                    println("Computed signal: " + getValue<Signal>().to_string());
                    break;
            }
        }
        else if (isType(SPECTR_TYPE)) {
            auto right = getValue<Spectrum>();
            switch (expr.op) {
                case TokenType::Plus:
                    setValue<Spectrum>(left + right);
                    break;
                case TokenType::Star:
                    setValue<Spectrum>(left * right);
                    break;
            }
            println("Computed spectrum: " + getValue<Spectrum>().to_string());
        }
    }
    else if (isType(SPECTR_TYPE)) {
        auto left = getValue<Spectrum>();
        expr.right->accept(*this);
        if (isType(NUM_TYPE)) {
            auto right = getValue<float>();
            switch (expr.op) {
                case TokenType::Plus:
                    setValue<Spectrum>(left + right);
                    break;
                case TokenType::Star:
                    setValue<Spectrum>(left * right);
                    break;
                case TokenType::Slash:
                    setValue<Spectrum>(left / right);
                    break;
            }
            println("Computed spectrum: " + getValue<Spectrum>().to_string());
        }
        else if (isType(SIG_TYPE)) {
            auto right = getValue<Signal>();
            switch (expr.op) {
                case TokenType::Plus:
                    setValue<Spectrum>(left + right);
                    break;
                case TokenType::Star:
                    setValue<Spectrum>(left * right);
                    break;
            }
            println("Computed spectrum: " + getValue<Spectrum>().to_string());
        }
        else if (isType(SPECTR_TYPE)) {
            auto right = getValue<Spectrum>();
            switch (expr.op) {
                case TokenType::Plus:
                    setValue<Spectrum>(left + right);
                    break;
                case TokenType::Star:
                    setValue<Spectrum>(left * right);
                    break;
            }
            println("Computed spectrum: " + getValue<Spectrum>().to_string());
        }
    }
    else if (isType(OSC_TYPE)) {
        if (expr.op == TokenType::Plus) {
            auto left = getValue<std::shared_ptr<Oscillator>>();
            expr.right->accept(*this);
            auto right = getValue<std::shared_ptr<Oscillator>>();
            if (auto l = dynamic_cast<WavetableOsc*>(left.get())) {
                if (auto r = dynamic_cast<WavetableOsc*>(right.get())) {
                    auto result = *l + *r;
                    setValue<std::shared_ptr<Oscillator>>(std::make_shared<CompoundOsc>(result));
                }
                else if (auto r = dynamic_cast<CompoundOsc*>(right.get())) {
                    auto result = *l + *r;
                    setValue<std::shared_ptr<Oscillator>>(std::make_shared<CompoundOsc>(result));
                }
            }
            else if (auto l = dynamic_cast<CompoundOsc*>(left.get())) {
                if (auto r = dynamic_cast<WavetableOsc*>(right.get())) {
                    auto result = *l + *r;
                    setValue<std::shared_ptr<Oscillator>>(std::make_shared<CompoundOsc>(result));
                }
                else if (auto r = dynamic_cast<CompoundOsc*>(right.get())) {
                    auto result = *l + *r;
                    setValue<std::shared_ptr<Oscillator>>(std::make_shared<CompoundOsc>(result));
                }
            }
            else throw; // invalid left operand

            println("Computed oscillator: " + getValue<std::shared_ptr<Oscillator>>()->to_string());
        }
    }
    else {
        runtimeError("Invalid operands on binary expression.", expr.line, expr.column);
    }
}

void Interpreter::evalExpr(const SignalExpr& expr) {
    expr.freq->accept(*this);
    auto freq = getValue<float>();
    expr.ampl->accept(*this);
    auto ampl = getValue<float>();

    setValue<Signal>(Signal(freq, { ampl, 0.0f }));
    println("Signal: " + getValue<Signal>().to_string());
}

void Interpreter::evalExpr(const ListExpr& expr) {
    List list;
    for (int i = 0; i < expr.list.size(); ++i) {
        expr.list[i]->accept(*this);
        if(!currentVal) {
            runtimeError("Elements of lists must evaluate to something", expr.list[i]->line, expr.list[i]->column);
            continue;
        }

        auto val = *currentVal;
        list.push_back(val);
    }

    setValue(list);
    println(typeName(currentVal->type()) + ": " + currentVal->to_string());
}

void Interpreter::evalExpr(const TupleExpr& expr) {
    Tuple tuple;
    for (int i = 0; i < expr.tuple.size(); ++i) {
        expr.tuple[i]->accept(*this);
        if(!currentVal) {
            runtimeError("Elements of tuples must evaluate to something", expr.tuple[i]->line, expr.tuple[i]->column);
            continue;
        }

        auto val = *currentVal;
        tuple.push_back(val);
    }

    setValue(tuple);
    println(typeName(currentVal->type()) + ": " + currentVal->to_string());
}

void Interpreter::evalExpr(const OscPrimExpr& expr) {
    expr.param->accept(*this);
    if (isType(SIG_TYPE)) {
        Signal signal = getValue<Signal>();
        setValue<std::shared_ptr<Oscillator>>(std::make_shared<WavetableOsc>(signal, expr.shape, 44100));
        println("Primitive osc from signal: " + getValue<std::shared_ptr<Oscillator>>()->to_string());
        return;
    }
    else if (isType(SPECTR_TYPE)) {
        Spectrum spectrum = getValue<Spectrum>();
        setValue<std::shared_ptr<Oscillator>>(std::make_shared<CompoundOsc>(spectrum, expr.shape, 44100));
        println("Primitive osc from spectrum: " + getValue<std::shared_ptr<Oscillator>>()->to_string());
        return;
    }

    runtimeError("Expected signal or spectrum as oscillator argument.", expr.param->line, expr.param->column);
}

void Interpreter::evalExpr(const AssignmentExpr& expr) {
    expr.value->accept(*this);

    if (auto* id = dynamic_cast<IdentifierExpr*>(expr.id.get())) {
        setVar(id->name);
        setValue(nullptr);
        return;
    }

    runtimeError("Assignment requires an identifier to assign to", expr.id->line, expr.id->column);
    setValue(nullptr);
    throw;
    // but later maybe add pattern matching for tuples or something
}

void Interpreter::evalExpr(const DeclExpr& expr) {
    AssignmentExpr* assign;
    IdentifierExpr* id;
    if (assign = dynamic_cast<AssignmentExpr*>(expr.decl.get())) {
        if (!(id = dynamic_cast<IdentifierExpr*>(assign->id.get()))) {
            runtimeError(
                "Expected identifier for declaration of type '" + typeName(expr.type) + "'.",
                assign->line, assign->column
            );
            return;
        }
    }
    else {
        if (!(id = dynamic_cast<IdentifierExpr*>(expr.decl.get()))) {
            runtimeError(
                "Expected assignment or identifier for declaration of type '" + typeName(expr.type) + "'.",
                expr.line, expr.column
            );
            return;
        }
    }

    if (variables.contains(id->name)) {
        runtimeError(
            "Tried declaring already taken name '" + id->name + "'.",
            expr.line, expr.column
        );
        return;
    }

    if (!assign) clearValue();
    else         assign->value.get()->accept(*this);

    declVar(expr.type, id->name);
}

void Interpreter::evalExpr(const BlockExpr& expr) {
    for (int i = 0; i < expr.expressions.size(); ++i) {
        expr.expressions[i]->accept(*this);
        if (dynamic_cast<OutExpr*>(expr.expressions[i].get()))
            return; // maybe mark later expressions as unreached
    }

    setValue(nullptr);
}

void Interpreter::evalExpr(const OutExpr& expr) {
    if (expr.value)
        expr.value->accept(*this);
    else
        setValue(nullptr);
}

void Interpreter::evalExpr(const FuncApplExpr& expr) {
    expr.func->accept(*this);
    if (!isFunc()) {
        runtimeError("Expects function for function application.", expr.line, expr.column);
        // maybe add exceptions for certain types to make them callable,
        // e.g. {$T->S}
        throw;
    }
}

void Interpreter::evalExpr(const PlaybackExpr& expr) {
    pushPlaybackEvent(expr);
}

void Interpreter::evalExpr(const ReleaseExpr& expr) {
    eventStream->release(expr.label.get(), time + (sessionTime ? *sessionTime : 0.0));
}

void Interpreter::pushPlaybackEvent(const PlaybackExpr& expr, const std::string* label) {
    expr.osc->accept(*this);
    auto osc = getValue<std::shared_ptr<Oscillator>>();
    Signal signal;
    if (expr.signal) {
        expr.signal->accept(*this);
        signal = getValue<Signal>();
    }
    else signal = osc->reference;

    auto event = PlaybackEvent(std::move(osc), signal, time + (sessionTime ? *sessionTime : 0.0), label);
    println("Playback event: @" + std::to_string(event.onset) + " "
        + event.osc->to_string() + ", " + event.signal.to_string());
    
    eventStream->push(event);
    setValue(nullptr);
}

void Interpreter::declVar(Type type, const std::string& name) {

    if (!type) {
        runtimeError("Declared variables need a type!", 0, 0);
        throw;
    }

    if (!currentVal) {
        if (const Value* value = defaultValue(type)) {
            variables.emplace(name, *value);
            std::cout << typeName(variables.at(name).type()) + " '" + name + "' <<< " + variables.at(name).to_string() + " (default value)" << std::endl;
            return;
        }

        runtimeError("Type " + typeName(type) + " needs an initialization value", 0, 0);
        throw;
    }

    if (type != currentVal->type()) {
        runtimeError("Tried assigning " + typeName(type) + " '" + name + "' value of wrong type " + typeName(currentVal->type()), 0, 0);
        throw;
    }

    variables.emplace(name, *currentVal);
    println(typeName(variables.at(name).type()) + " '" + name + "' <<< " + variables.at(name).to_string());
}

void Interpreter::getVar(const std::string& name) {
    if (!variables.contains(name)) {
        runtimeError("Tried accessing undeclared identifier '" + name + "'.", /*expr.line*/0, /*expr.column*/0);
        setValue(nullptr);
        return;
    }

    currentVal = std::make_unique<Value>(variables.at(name));
    println(typeName(variables.at(name).type()) + " '" + name + "' > " + variables.at(name).to_string());
}

void Interpreter::setVar(const std::string& name) {
    if (!variables.contains(name)) {
        runtimeError("Tried assigning undeclared identifier '" + name + "'", 0, 0);
        throw;
    }

    Type type = variables.at(name).type();

    if (!currentVal) {
        runtimeError("Assignment value must evaluate to something", 0, 0);
        throw;
    }

    try {
        variables.at(name) = *currentVal;
        println(typeName(variables.at(name).type()) + " '" + name + "' << " + variables.at(name).to_string());
    } catch (const std::runtime_error& e) {
        runtimeError(e.what(), 0, 0);
        throw;
    }
}

void Interpreter::println(const std::string& msg) {
    if (logSettings->logOutput && !logSettings->hideAll)
        std::cout << msg << std::endl;
}

void Interpreter::runtimeError(const std::string& msg, size_t line, size_t col) {
    std::cout << "\033[0;31mRuntime error at ("
        << std::to_string(line) << ":" << std::to_string(col) << "): "
        << msg << "\033[0m" << std::endl;
}