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

void Interpreter::evalExpr(const IdentifierExpr& expr) {
    if (nums.contains(expr.name)) {
        setValue<float>(nums[expr.name]);
        std::cout << "Num '" << expr.name << "' > " << std::to_string(nums[expr.name]) << std::endl;
    } else if (signals.contains(expr.name)) {
        setValue<Signal>(*signals[expr.name]);
        std::cout << "Signal '" << expr.name << "' > " << (*signals[expr.name]).to_string() << std::endl;
    } else if (spectra.contains(expr.name)) {
        setValue<Spectrum>(*spectra[expr.name]);
        std::cout << "Spectrum '" << expr.name << "' > " << (*spectra[expr.name]).to_string() << std::endl;
    } else if (oscillators.contains(expr.name)) {
        setValue<std::unique_ptr<Oscillator>>(oscillators[expr.name]->clone());
        std::cout << "Oscillator '" << expr.name << "' > " << (*oscillators[expr.name]).to_string() << std::endl;
    }
    else
        runtimeError("Tried accessing undeclared identifier '" + expr.name + "'.", expr.line, expr.column);
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
        std::cout << "Timestamp: +" << std::to_string(value) << " (@" << std::to_string(time) << ")" << std::endl;
    }
    else if (expr.op == TokenType::At) {
        auto value = getValue<float>();
        time = value;
        std::cout << "Timestamp: @" << std::to_string(time) << std::endl;
    }
    else if (/*is<float>()*/isType(NUM_TYPE)) {
        if (expr.op == TokenType::Minus)
            setValue<float>(-getValue<float>());
        else if (/*is<float>()*/isType(NUM_TYPE)) {
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
    if (/*is<float>()*/isType(NUM_TYPE)) {
        auto left = getValue<float>();
        expr.right->accept(*this);
        if (/*is<float>()*/isType(NUM_TYPE)) {
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
            std::cout << "Computed num: " << std::to_string(getValue<float>()) << std::endl;
        }
        else if (/*is<Signal>()*/isType(SIG_TYPE)) {
            auto right = getValue<Signal>();
            switch (expr.op) {
                case TokenType::Plus:
                    setValue<Spectrum>(left + right);
                    std::cout << "Computed spectrum: " << getValue<Spectrum>().to_string() << std::endl;
                    break;
                case TokenType::Star:
                    setValue<Signal>(left * right);
                    std::cout << "Computed signal: " << getValue<Signal>().to_string() << std::endl;
                    break;
            }
        }
        else if (/*is<Spectrum>()*/isType(SPECTR_TYPE)) {
            auto right = getValue<Spectrum>();
            switch (expr.op) {
                case TokenType::Plus:
                    setValue<Spectrum>(left + right);
                    break;
                case TokenType::Star:
                    setValue<Spectrum>(left * right);
                    break;
            }
            std::cout << "Computed spectrum: " << getValue<Spectrum>().to_string() << std::endl;
        }
    }
    else if (/*is<Signal>()*/isType(SIG_TYPE)) {
        auto left = getValue<Signal>();
        expr.right->accept(*this);
        if (/*is<float>()*/isType(NUM_TYPE)) {
            auto right = getValue<float>();
            switch (expr.op) {
                case TokenType::Plus:
                    setValue<Spectrum>(left + right);
                    std::cout << "Computed spectrum: " << getValue<Spectrum>().to_string() << std::endl;
                    break;
                case TokenType::Star:
                    setValue<Signal>(left * right);
                    std::cout << "Computed signal: " << getValue<Signal>().to_string() << std::endl;
                    break;
                case TokenType::Slash:
                    setValue<Signal>(left / right);
                    std::cout << "Computed signal: " << getValue<Signal>().to_string() << std::endl;
                    break;
            }
        }
        else if (/*is<Signal>()*/isType(SIG_TYPE)) {
            auto right = getValue<Signal>();
            switch (expr.op) {
                case TokenType::Plus:
                    setValue<Spectrum>(left + right);
                    std::cout << "Computed spectrum: " << getValue<Spectrum>().to_string() << std::endl;
                    break;
                case TokenType::Star:
                    setValue<Signal>(left * right);
                    std::cout << "Computed signal: " << getValue<Signal>().to_string() << std::endl;
                    break;
            }
        }
        else if (/*is<Spectrum>()*/isType(SPECTR_TYPE)) {
            auto right = getValue<Spectrum>();
            switch (expr.op) {
                case TokenType::Plus:
                    setValue<Spectrum>(left + right);
                    break;
                case TokenType::Star:
                    setValue<Spectrum>(left * right);
                    break;
            }
            std::cout << "Computed spectrum: " << getValue<Spectrum>().to_string() << std::endl;
        }
    }
    else if (/*is<Spectrum>()*/isType(SPECTR_TYPE)) {
        auto left = getValue<Spectrum>();
        expr.right->accept(*this);
        if (/*is<float>()*/isType(NUM_TYPE)) {
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
            std::cout << "Computed spectrum: " << getValue<Spectrum>().to_string() << std::endl;
        }
        else if (/*is<Signal>()*/isType(SIG_TYPE)) {
            auto right = getValue<Signal>();
            switch (expr.op) {
                case TokenType::Plus:
                    setValue<Spectrum>(left + right);
                    break;
                case TokenType::Star:
                    setValue<Spectrum>(left * right);
                    break;
            }
            std::cout << "Computed spectrum: " << getValue<Spectrum>().to_string() << std::endl;
        }
        else if (/*is<Spectrum>()*/isType(SPECTR_TYPE)) {
            auto right = getValue<Spectrum>();
            switch (expr.op) {
                case TokenType::Plus:
                    setValue<Spectrum>(left + right);
                    break;
                case TokenType::Star:
                    setValue<Spectrum>(left * right);
                    break;
            }
            std::cout << "Computed spectrum: " << getValue<Spectrum>().to_string() << std::endl;
        }
    }
    else if (/*is<std::unique_ptr<Oscillator>>()*/isType(OSC_TYPE)) {
        if (expr.op == TokenType::Plus) {
            auto left = std::move(getValue<std::unique_ptr<Oscillator>>());
            expr.right->accept(*this);
            auto right = std::move(getValue<std::unique_ptr<Oscillator>>());
            if (auto l = dynamic_cast<WavetableOsc*>(left.get())) {
                if (auto r = dynamic_cast<WavetableOsc*>(right.get())) {
                    auto result = *l + *r;
                    setValue<std::unique_ptr<Oscillator>>(std::make_unique<CompoundOsc>(std::move(result)));
                }
                else if (auto r = dynamic_cast<CompoundOsc*>(right.get())) {
                    auto result = *l + *r;
                    setValue<std::unique_ptr<Oscillator>>(std::make_unique<CompoundOsc>(std::move(result)));
                }
            }
            else if (auto l = dynamic_cast<CompoundOsc*>(left.get())) {
                if (auto r = dynamic_cast<WavetableOsc*>(right.get())) {
                    auto result = *l + *r;
                    setValue<std::unique_ptr<Oscillator>>(std::make_unique<CompoundOsc>(std::move(result)));
                }
                else if (auto r = dynamic_cast<CompoundOsc*>(right.get())) {
                    auto result = *l + *r;
                    setValue<std::unique_ptr<Oscillator>>(std::make_unique<CompoundOsc>(std::move(result)));
                }
            }
            else throw; // invalid left operand

            std::cout << "Computed oscillator: "
                << getValue<std::unique_ptr<Oscillator>>()->to_string() << std::endl;
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
    std::cout << "Signal: " << getValue<Signal>().to_string() << std::endl;
}

void Interpreter::evalExpr(const ListExpr& expr) {
    Spectrum spec;
    for (int i = 0; i < expr.signals.size(); ++i) {
        expr.signals[i]->accept(*this);
        auto sig = getValue<Signal>();
        spec.push_back(sig);
    }

    setValue<Spectrum>(spec);
    std::cout << "Spectrum: " << getValue<Spectrum>().to_string() << std::endl;
}

void Interpreter::evalExpr(const OscPrimExpr& expr) {
    expr.param->accept(*this);
    if (/*is<Signal>()*/isType(SIG_TYPE)) {
        Signal signal = getValue<Signal>();
        setValue<std::unique_ptr<Oscillator>>(std::make_unique<WavetableOsc>(signal, expr.shape, 44100));
        std::cout << "Primitive osc from signal: "
            << getValue<std::unique_ptr<Oscillator>>()->to_string() << std::endl;
        return;
    }
    else if (/*is<Spectrum>()*/isType(SPECTR_TYPE)) {
        Spectrum spectrum = getValue<Spectrum>();
        setValue<std::unique_ptr<Oscillator>>(std::make_unique<CompoundOsc>(spectrum, expr.shape, 44100));
        std::cout << "Primitive osc from spectrum: "
            << getValue<std::unique_ptr<Oscillator>>()->to_string() << std::endl;
        return;
    }

    runtimeError("Expected signal or spectrum as oscillator argument.", expr.param->line, expr.param->column);
}

void Interpreter::evalExpr(const AssignmentExpr& expr) {

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

    if (/*!availableName*/varTypes.contains(id->name)) {
        runtimeError(
            "Tried declaring already taken name '" + id->name + "'.",
            expr.line, expr.column
        );
        return;
    }

    if (!assign) setValue<int*>(nullptr);
    else         assign->value.get()->accept(*this);

    declVar(expr.type, id->name);

    /*
    if (expr.type == DeclType::num) {
        float value = 0.0f;
        if (assign) {
            assign->value->accept(*this);
            value = getValue<float>();
        }

        nums.emplace(id->name, value);
        std::cout << "Num '" << id->name << "' <<< " << value << std::endl;
    }
    else if (expr.type == DeclType::signal) {
        Signal value = Signal::unit;
        if (assign) {
            assign->value->accept(*this);
            value = getValue<Signal>();
        }

        signals.emplace(id->name, std::make_unique<Signal>(value));
        std::cout << "Signal '" << id->name << "' <<< " << value.to_string() << std::endl;
    }
    else if (expr.type == DeclType::spectr) {
        Spectrum value = Spectrum::empty;
        if (assign) {
            assign->value->accept(*this);
            value = getValue<Spectrum>();
        }

        spectra.emplace(id->name, std::make_unique<Spectrum>(value));
        std::cout << "Spectrum '" << id->name << "' <<< " << value.to_string() << std::endl;
    }
    else if (expr.type == DeclType::osc) {
        if (assign) {
            assign->value->accept(*this);
            if (is<std::unique_ptr<Oscillator>>()) {
                auto reference = std::move(getValue<std::unique_ptr<Oscillator>>());
                oscillators.emplace(id->name, reference->clone()); // <- here it is
                std::cout << "Oscillator '" << id->name << "' <<< " << (*reference).to_string() << std::endl;
            }
            else {
                throw;
            }
        } 
    }*/
}

void Interpreter::evalExpr(const BlockExpr& expr) {
    for (int i = 0; i < expr.expressions.size(); ++i) {
        expr.expressions[i]->accept(*this);
        if (dynamic_cast<OutExpr*>(expr.expressions[i].get()))
            return; // maybe mark later expressions as unreached
    }

    setValue<int*>(nullptr);
}

void Interpreter::evalExpr(const OutExpr& expr) {
    if (expr.value)
        expr.value->accept(*this);
    else
        setValue<int*>(nullptr);
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
    auto osc = std::move(getValue<std::unique_ptr<Oscillator>>());
    Signal signal;
    if (expr.signal) {
        expr.signal->accept(*this);
        signal = getValue<Signal>();
    }
    else signal = osc->reference;

    auto event = PlaybackEvent(std::move(osc), signal, time + (sessionTime ? *sessionTime : 0.0), label);
    std::cout << "Playback event: @" << std::to_string(event.onset) << " "
        << event/*->*/.osc->to_string() << ", " << event/*->*/.signal.to_string() << std::endl;
    
    eventStream->push(event/*, label*/);
    setValue<int*>(nullptr);
}

//bool Interpreter::availableName(const std::string& name) {
//    return !(nums.contains(name) || signals.contains(name) || spectra.contains(name) || oscillators.contains(name));
//}

void Interpreter::declVar(Type type, const std::string& name) {
    //if (varTypes.contains(name)) {
    //    // that check should happen earlier, though, to have position data as well
    //    runtimeError("Tried declaring already declared variable '" + name + "'.", 0, 0);
    //    return;
    //}

    if (!type) {
        runtimeError("Declared variables need a type!", 0, 0);
        throw;
    }

    varTypes[name] = type;

    if (type->equals(BOOL_TYPE)) {
        bool value = false;
        if (/*is<float>()*/isType(BOOL_TYPE))
            value = getValue<bool>();
        else if (!/*is<int*>()*/isEmpty())
            runtimeError("Tried assigning " + typeName(type) + " '" + name + "' value of wrong type.", 0, 0); // keep track of code position later

        bools.emplace(name, value);
        std::cout << "Bool '" << name << "' <<< " << (value ? "true" : "false") << std::endl;
    }
    else if (type->equals(INT_TYPE)) {
        int value = 0;
        if (isType(INT_TYPE))
            value = getValue<int>();
        else if (isEmpty())
            runtimeError("Tried assigning " + typeName(type) + " '" + name + "' value of wrong type.", 0, 0); // keep track of code position later

        ints.emplace(name, value);
        std::cout << "Int '" << name << "' <<< " << value << std::endl;
    }
    else if (type->equals(NUM_TYPE)) {
        float value = 0.0f;
        if (/*is<float>()*/isType(NUM_TYPE))
            value = getValue<float>();
        else if (!/*is<int*>()*/isEmpty())
            runtimeError("Tried assigning " + typeName(type) + " '" + name + "' value of wrong type.", 0, 0); // keep track of code position later

        nums.emplace(name, value);
        std::cout << "Num '" << name << "' <<< " << value << std::endl;
    }
    else if (type->equals(SIG_TYPE)) {
        Signal value = Signal::unit;
        if (/*is<Signal>()*/isType(SIG_TYPE))
            value = getValue<Signal>();
        else if (!/*is<int*>()*/isEmpty())
            runtimeError("Tried assigning " + typeName(type) + " '" + name + "' value of wrong type.", 0, 0); // keep track of code position later


        signals.emplace(name, std::make_unique<Signal>(value));
        std::cout << "Signal '" << name << "' <<< " << value.to_string() << std::endl;
    }
    else if (type->equals(SPECTR_TYPE)) {
        Spectrum value = Spectrum::empty;
        if (/*is<Spectrum>()*/isType(SPECTR_TYPE))
            value = getValue<Spectrum>();
        else if (!/*is<int*>()*/isEmpty())
            runtimeError("Tried assigning " + typeName(type) + " '" + name + "' value of wrong type.", 0, 0); // keep track of code position later


        spectra.emplace(name, std::make_unique<Spectrum>(value));
        std::cout << "Spectrum '" << name << "' <<< " << value.to_string() << std::endl;
    }
    else if (type->equals(OSC_TYPE)) {
        if (!/*is<std::unique_ptr<Oscillator>>()*/isType(OSC_TYPE)) {
            if (/*is<int*>()*/isEmpty())
                runtimeError("Variable '" + name + "' of type " + typeName(type) + " needs value at declaration.", 0, 0); // keep track of code position later
            else
                runtimeError("Tried assigning " + typeName(type) + " '" + name + "' value of wrong type.", 0, 0); // keep track of code position later
            return;
        }

        auto reference = std::move(getValue<std::unique_ptr<Oscillator>>());
        oscillators.emplace(name, reference->clone());
        std::cout << "Oscillator '" << name << "' <<< " << (*reference).to_string() << std::endl;
    }
    else {
        std::any var = getValueAsAny();
        vars.emplace(name, var);
        std::cout << typeName(type) << " '" << name << "' <<< some value";
    }
    //else {
    //    runtimeError("Tried declaring currently unsupported type " + typeName(std::move(type)) + ".", 0, 0); // keep track of code position later
    //}
}

void Interpreter::getVar(const std::string& name) {

}

void Interpreter::setVar(const std::string& name) {

}

void Interpreter::parserError(const std::string& msg, size_t line, size_t col) {
    std::cout << "\033[0;31mParser error at ("
        << std::to_string(line) << ":" << std::to_string(col) << "): "
        << msg << "\033[0m" << std::endl;
}

void Interpreter::runtimeError(const std::string& msg, size_t line, size_t col) {
    std::cout << "\033[0;31mRuntime error at ("
        << std::to_string(line) << ":" << std::to_string(col) << "): "
        << msg << "\033[0m" << std::endl;
}