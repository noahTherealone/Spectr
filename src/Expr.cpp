#include "Interpreter.hpp"

void BoolExpr::accept(_Interpreter& backend)       { backend.evalExpr(*this); }
void NumExpr::accept(_Interpreter& backend)        { backend.evalExpr(*this); }
void IntExpr::accept(_Interpreter& backend)        { backend.evalExpr(*this); }
void IdentifierExpr::accept(_Interpreter& backend) { backend.evalExpr(*this); }
void LabelledExpr::accept(_Interpreter& backend)   { backend.evalExpr(*this); }
void UnaryExpr::accept(_Interpreter& backend)      { backend.evalExpr(*this); }
void BinaryExpr::accept(_Interpreter& backend)     { backend.evalExpr(*this); }
void SignalExpr::accept(_Interpreter& backend)     { backend.evalExpr(*this); }
void ListExpr::accept(_Interpreter& backend)       { backend.evalExpr(*this); }
void OscPrimExpr::accept(_Interpreter& backend)    { backend.evalExpr(*this); }
void AssignmentExpr::accept(_Interpreter& backend) { backend.evalExpr(*this); }
void TypeExpr::accept(_Interpreter& backend)       { }
void DeclExpr::accept(_Interpreter& backend)       { backend.evalExpr(*this); }
void BlockExpr::accept(_Interpreter& backend)      { backend.evalExpr(*this); }
void OutExpr::accept(_Interpreter& backend)        { backend.evalExpr(*this); }
void FuncApplExpr::accept(_Interpreter& backend)   { backend.evalExpr(*this); }
void PlaybackExpr::accept(_Interpreter& backend)   { backend.evalExpr(*this); }
void ReleaseExpr::accept(_Interpreter& backend)    { backend.evalExpr(*this); }

OscPrim to_osc_prim(TokenType t) {
    switch (t) {
        case TokenType::Sine:   return OscPrim::Sine;
        case TokenType::Square: return OscPrim::Square;
        case TokenType::Saw:    return OscPrim::Saw;
        default: throw; // really shouldn't happen if used correctly
    }
}