#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <charconv>
#include "parser.hpp"

Token* Parser::peek() {
    return index < tokens.size() ? &tokens[index] : nullptr;
}

Token* Parser::next() {
    return index < tokens.size() ? &tokens[index++] : nullptr;
}

#pragma region Expressions

int lbp(const Token& tok) {
    switch (tok.type) {
#define X(sym, tok, op, lbp, rbp) case TokenType::tok: return lbp;
        BINARY_OPERATORS
#undef X

        case TokenType::IF: return 2;
        default: return 0;
    }
}

int rbp(const Token& tok) {
    switch (tok.type) {
#define X(sym, tok, op, lbp, rbp) case TokenType::tok: return rbp;
        BINARY_OPERATORS
#undef X
        default: throw std::exception(); // non-operators shouldn't bind to things from the left
    }
}

std::unique_ptr<Expr> Parser::parseExpr(int rbp) {
    Token *tok = peek();
    if (!tok || tok->type == TokenType::LineBreak) return nullptr;
    next();

    auto left = nud(*tok);
    while (true) {
        tok = peek();
        if (!tok || lbp(*tok) <= rbp) break;

        left = led(std::move(left), *next());
    }

    return left;
}

std::unique_ptr<Expr> Parser::nud(const Token& tok) {
    switch (tok.type) {
        case TokenType::Identifier:
            return std::make_unique<IdentifierExpr>(tok);
        case TokenType::Nil:
            return std::make_unique<VoidExpr>(tok);
        case TokenType::False:
            return std::make_unique<BooleanExpr>(tok);
        case TokenType::True:
            return std::make_unique<BooleanExpr>(tok);
        case TokenType::IntLiteral:
            return std::make_unique<IntExpr>(tok);
        case TokenType::NumLiteral:
            return std::make_unique<NumExpr>(tok);
        case TokenType::StrLiteral:
            return std::make_unique<StrExpr>(tok);
        case TokenType::LParen:
            return parseParen(tok.index);
        case TokenType::LBracket:
            return parseBlock(tok.index);
        default:
            throw SyntaxError("Unexpected token at expression start", tok.index, tok.text.length());
    }
}

std::unique_ptr<Expr> Parser::led(std::unique_ptr<Expr> left, const Token& tok) {
    if (std::find(binaryOps.begin(), binaryOps.end(), tok.type) != binaryOps.end()) {
        if (tok.type == TokenType::RightArrow)
            return parseLambda(std::make_unique<Params>(std::move(left)));
        
        auto right = parseExpr(rbp(tok));
        if (!right)
            throw SyntaxError("Expected expression to the right of binary operator", tok.index + tok.text.length(), 0);

        return std::make_unique<BinaryExpr>(std::move(left), std::move(right), tok);
    }
    else if (tok.type == TokenType::IF) {
        auto condition = parseExpr();
        if (!condition)
            throw SyntaxError("Expected condition after infix \"if\"", tok.index + tok.text.length(), 0);
        
        
        Token *elseTok = next();
        if (!elseTok)
            throw SyntaxError("Expected \"else\" keyword for ternary operator", tokens.back().index + tokens.back().text.length(), 0);
        else if (elseTok->type != TokenType::ELSE)
            throw SyntaxError("Expected \"else\" keyword for ternary operator", elseTok->index, elseTok->text.length());
        
        auto alternative = parseExpr();
        if (!alternative)
            throw SyntaxError("Expected alternative for ternary operator", tokens[index-1].index + tokens[index-1].text.length(), 0);
        
        return std::make_unique<TernaryExpr>(std::move(left), std::move(condition), std::move(alternative));
    }

    throw SyntaxError("Unexpected token inside expression", tok.index, tok.text.length());
}

std::unique_ptr<LambdaExpr> Parser::parseLambda(std::unique_ptr<Params> params) {
    if (!peek())
        throw SyntaxError("Expected lambda body", tokens.back().index + tokens.back().text.length(), 0);

    //std::unique_ptr<ParamsExpr> params;
    
    /*if (auto p = dynamic_cast<ParamsExpr*>(left.get())) {
        params.reset(static_cast<ParamsExpr*>(left.release()));
    }
    else if (auto tuple = dynamic_cast<TupleExpr*>(left.get())) {
        params = std::make_unique<ParamsExpr>(
            std::unique_ptr<TupleExpr>(static_cast<TupleExpr*>(left.release()))
        );
    }
    else if (auto id = dynamic_cast<IdentifierExpr*>(left.get())) {
        params = std::make_unique<ParamsExpr>(
            std::unique_ptr<IdentifierExpr>(static_cast<IdentifierExpr*>(left.release()))
        );
    }
    else
        throw SyntaxError("Invalid parameter expression", left->start(), left->length());*/

    if (peek()->type == TokenType::LBracket) {
        auto body = parseBlock(next()->index);
        return std::make_unique<LambdaExpr>(std::move(params), std::move(body));
    }

    Token *exprStart = peek();
    auto expr = parseExpr();
    if (!expr) {
        throw SyntaxError("Expected expression for lambda return value", exprStart->index, exprStart->text.length());
    }

    return std::make_unique<LambdaExpr>(
        std::move(params),
        std::make_unique<BlockExpr>(std::make_unique<ReturnStmt>(std::move(expr), exprStart->index))
    );
}

std::unique_ptr<Expr> Parser::parseParen(size_t start) {
    if (!peek()) {
        throw SyntaxError("Unclosed parentheses", tokens.back().index + tokens.back().text.length(), 0);
    }

    if (peek()->type == TokenType::RParen) {
        return std::make_unique<VoidExpr>(start, peek()->index - start + next()->text.length());
    }

    if (peek()->type == TokenType::Identifier && index + 1 < tokens.size() && tokens[index+1].type == TokenType::TypeMarker) {
        return parseParams(start);
    }

    size_t exprStart = peek()->index;
    auto expr = parseExpr();
    if (!expr) {
        skipToLineBreak();
        throw SyntaxError("Expected expression", exprStart, 0);
    }

    if (!peek()) {
        throw SyntaxError("Unclosed parentheses", tokens.back().index + tokens.back().text.length(), 0);
    }

    if (peek()->type == TokenType::RParen) {
        next();
        return std::move(expr);
    }

    expect(TokenType::Comma);
    if (!peek()) {
        throw SyntaxError("Unclosed parentheses", tokens.back().index + tokens.back().text.length(), 0);
    }

    std::vector<std::unique_ptr<Expr>> exprns;
    exprns.push_back(std::move(expr));

    while (true) {
        if (peek()->type == TokenType::RParen) {
            return std::make_unique<TupleExpr>(std::move(exprns), start, peek()->index - start + next()->text.length());
        }

        exprStart = peek()->index;
        expr = parseExpr();
        if (!expr) {
            skipToLineBreak();
            throw SyntaxError("Expected expression", exprStart, 0);
        }

        exprns.push_back(std::move(expr));

        if (!peek()) {
            throw SyntaxError("Unclosed parentheses", tokens.back().index + tokens.back().text.length(), 0);
        }

        if (peek()->type == TokenType::RParen) {
            return std::make_unique<TupleExpr>(std::move(exprns), start, peek()->index - start + next()->text.length());
        }

        expect(TokenType::Comma);

        if (!peek()) {
            throw SyntaxError("Unclosed parentheses", tokens.back().index + tokens.back().text.length(), 0);
        }
    }
}

std::unique_ptr<LambdaExpr> Parser::parseParams(size_t start) {
    auto params = std::make_unique<Params>();
    while (true) {
        Token& nameTok = expect(TokenType::Identifier);
        std::unique_ptr<IdentifierExpr> name = std::make_unique<IdentifierExpr>(nameTok);
        if (!peek()) {
            throw SyntaxError("Unclosed params", tokens.back().index + tokens.back().text.length(), 0);
        }

        expect(TokenType::TypeMarker);
        auto type = parseTypeExpr();
        params->params.push_back(std::make_unique<Param>(std::move(name), std::move(type)));
        if (!peek()) {
            throw SyntaxError("Unclosed params", tokens.back().index + tokens.back().text.length(), 0);
        }

        if (peek()->type == TokenType::RParen) {
            params->start = start;
            params->length = peek()->index - start + next()->text.length();
            if (!peek() || peek()->type != TokenType::RightArrow)
                throw SyntaxError("Expected -> symbol", peek()->index, peek()->text.length());
            
            next();
            return parseLambda(std::move(params));
        }

        expect(TokenType::Comma);
    }
}

std::unique_ptr<BlockExpr> Parser::parseBlock(size_t start) {
    std::vector<std::unique_ptr<Stmt>> stmts;
    std::unique_ptr<Stmt> stmt;
    while (peek() && peek()->type != TokenType::RBracket) {
        Token *tok = peek();
        stmt = parseStatement();
        if (!stmt)
            throw SyntaxError("Expected statement inside block", peek()->index, peek()->text.length());

        stmts.push_back(std::move(stmt));
        if (!peek())
            throw SyntaxError("Unclosed code-block", tokens[index-1].index + tokens[index-1].text.length(), 0);
        
        skipLineBreaks();
        if (peek()->type == TokenType::RBracket)
            break;
    }

    if (!peek())
        throw SyntaxError("Uncloded code-block", tokens.back().index + tokens.back().text.length(), 0);

    if (stmts.size() == 0)
        throw SyntaxError("Empty code-block", start, peek()->index - start + peek()->text.length());

    next();
    return std::make_unique<BlockExpr>(std::move(stmts), start, tokens[index-1].index - start + tokens[index-1].text.length());
}

#pragma region Type expressions

inline int typeLbp(TokenType type) {
    switch (type) {
        case TokenType::Union:      return 20;
        case TokenType::RightArrow: return 10;
        default:                    return 0;
    }
}

std::unique_ptr<TypeExpr> Parser::parseTypeExpr(int rbp) {
    Token *tok = next();
    if (!tok || tok->type == TokenType::LineBreak) throw SyntaxError("Expected token to parse type", 0, 0);

    auto left = typeNud(*tok);
    while (peek()) {
        if (typeLbp(peek()->type) <= rbp) break;
        left = typeLed(std::move(left), *next());
    }

    return left;
}

std::unique_ptr<TypeExpr> Parser::typeNud(const Token& tok) {
    if (std::find(primTypes.begin(), primTypes.end(), tok.type) != primTypes.end())
        return std::make_unique<PrimTypeExpr>(tok);
    
    switch (tok.type) {
        case TokenType::LParen: {
            auto type = parseTypeExpr();
            if (!peek()) {
                throw SyntaxError("Unclosed parentheses in type expression", tokens[index-1].index + tokens[index-1].text.length(), 0);
            }

            if (peek()->type == TokenType::RParen) {
                next();
                return std::move(type);
            }

            std::vector<std::unique_ptr<TypeExpr>> types;
            types.push_back(std::move(type));
            while (true) {
                if (!peek()) {
                    throw SyntaxError("Unclosed parentheses in type expression", tokens[index-1].index + tokens[index-1].text.length(), 0);
                }

                if (peek()->type == TokenType::RParen) {
                    break;
                }

                if (peek()->type != TokenType::Comma) {
                    throw SyntaxError("Unexpected token in type expression", peek()->index, peek()->text.length());
                }

                next();

                if (peek()->type == TokenType::RParen) {
                    break;
                }

                types.push_back(parseTypeExpr());
            }

            if (peek()->type != TokenType::RParen) {
                throw SyntaxError("Unclosed parentheses in type expression", peek()->index + next()->text.length(), 0);
            }

            return std::make_unique<TupleTypeExpr>(std::move(types), tok.index, peek()->index - tok.index + next()->text.length());
        }
        case TokenType::LBrace: {
            auto type = parseTypeExpr();
            if (!peek() || peek()->type != TokenType::RBrace) {
                throw SyntaxError("Unclosed list type brace", tokens[index-1].index + tokens[index-1].text.length(), 0);
            }

            return std::make_unique<ListTypeExpr>(std::move(type), tok.index, peek()->index - tok.index + next()->text.length());
        }
    }
    
    throw SyntaxError("Could not parse type", tok.index, tok.text.length());
}

std::unique_ptr<TypeExpr> Parser::typeLed(std::unique_ptr<TypeExpr> left, const Token& tok) {
    switch (tok.type) {
        case TokenType::Union: {
            auto right = parseTypeExpr(20);
            std::vector<std::unique_ptr<TypeExpr>> options;

            if (auto opt = dynamic_cast<OptionTypeExpr*>(left.get())) {
                for (auto &o : opt->options) {
                    options.push_back(std::move(o));
                }
            }
            else {
                options.push_back(std::move(left));
            }

            if (auto opt = dynamic_cast<OptionTypeExpr*>(right.get())) {
                for (auto &o : opt->options) {
                    options.push_back(std::move(o));
                }
            }
            else {
                options.push_back(std::move(right));
            }

            return std::make_unique<OptionTypeExpr>(std::move(options));
        }
        case TokenType::RightArrow: {
            auto out = parseTypeExpr(5);
            if (auto params = dynamic_cast<TupleTypeExpr*>(left.get())) {
                size_t start = params->types.front()->start();
                return std::make_unique<FunctionTypeExpr>(std::move(params->types), std::move(out), start);
            }

            size_t start = left->start();
            std::vector<std::unique_ptr<TypeExpr>> params;
            params.push_back(std::move(left));
            return std::make_unique<FunctionTypeExpr>(std::move(params), std::move(out), start);
        }
    }

    throw std::exception(); // this shouldn't happen
}

#pragma endregion

#pragma endregion

#pragma region Statements

std::unique_ptr<Stmt> Parser::matchExplicitVarDecl(std::unique_ptr<IdentifierExpr> lhs, const Token& typeMarker) {
    Token *typeStart = peek();
    auto type = parseTypeExpr();

    if (!type) {
        throw std::exception(); // should not happen, because parseTypeExpr should throw when it's unable to parse a type
    }

    Token *tok = next();
    if (!tok || tok->type == TokenType::LineBreak) {
        return std::make_unique<VarDeclStmt>(std::move(lhs), std::move(type), nullptr);
    }
    
    if (tok->type != TokenType::Assign) {
        size_t start = tok->index;
        size_t end;
        while ((tok = next()) && tok->type != TokenType::LineBreak)
            end = tok->index + tok->text.length();

        throw SyntaxError("Expected = after type marking", start, end - start);
    }

    return matchVarDecl(std::move(lhs), std::move(type), *tok);
}

std::unique_ptr<Stmt> Parser::matchInferredVarDecl(std::unique_ptr<IdentifierExpr> lhs, const Token& assignmentOp) {
    return matchVarDecl(std::move(lhs), nullptr, assignmentOp);
}

std::unique_ptr<Stmt> Parser::matchVarDecl(std::unique_ptr<IdentifierExpr> lhs, std::unique_ptr<TypeExpr> type, const Token& assignmentOp) {
    auto value = parseExpr();
    if (!value)
        throw SyntaxError("Expected expression for declaration", assignmentOp.index + assignmentOp.text.length(), 0);

    Token* tok = next();
    if ((tok != nullptr) && tok->type != TokenType::LineBreak) {
        throw SyntaxError("Unexpected token after declaration statement", tok->index, tok->text.length());
    }
    
    return std::make_unique<VarDeclStmt>(std::move(lhs), std::move(type), std::move(value));
}

std::unique_ptr<Stmt> Parser::matchAssignment(std::unique_ptr<Expr> lhs, const Token& sgn) {
    std::string name;
    if (sgn.type == TokenType::Assign)
        name = "assignment";
    else if (sgn.type == TokenType::ReferenceAssign)
        name = "reference declaration";
    else
        throw SyntaxError("what the hell just happened", sgn.index, sgn.text.length()); // this shouldn't happen

    auto value = parseExpr();
    if (!value) {
        throw SyntaxError("Expected expression for " + name, sgn.index + sgn.text.length(), 0);
    }

    Token *tok = next();
    if ((tok != nullptr) && tok->type != TokenType::LineBreak) {
        throw SyntaxError("Unexpected token after " + name, tok->index, tok->text.length());
    }

    if (sgn.type == TokenType::Assign)
        return std::make_unique<AssignmentStmt>(std::move(lhs), std::move(value));
    else
        return std::make_unique<ReferenceDeclStmt>(std::move(lhs), std::move(value));
}

std::unique_ptr<Stmt> Parser::matchIf() {

    // if
    Token *ifTok = next();
    auto condition = parseExpr();
    if (!condition)
        throw SyntaxError("Expected if-condition", ifTok->index + ifTok->text.length(), 0);
    
    auto body = matchBody();
    IfChain ifChain;
    ifChain.emplace_back(std::move(condition), std::move(body));

    while (peek() && peek()->type == TokenType::LineBreak)
        next();

    // elif*
    while (peek() && peek()->type == TokenType::ELIF) {
        Token *tok = next();
        condition = parseExpr();
        if (!condition)
            throw SyntaxError("Expected elif-condition", tok->index + tok->text.length(), 0);
        
        body = matchBody();
        ifChain.emplace_back(std::move(condition), std::move(body));
    }

    while (peek() && peek()->type == TokenType::LineBreak)
        next();

    // else?
    std::unique_ptr<BlockExpr> elseCase;
    if (peek() && peek()->type == TokenType::ELSE) {
        next();
        body = matchBody();
        elseCase = std::move(body);
    }

    return std::make_unique<IfStmt>(std::move(ifChain), std::move(elseCase), ifTok->index, tokens[index-1].index - ifTok->index + tokens[index-1].text.length());
}

std::unique_ptr<BlockExpr> Parser::matchBody() {
    Token *tok = next();
    if (!tok)
        throw SyntaxError("Expected code body", tokens.back().index + tokens.back().text.length(), 0);

    if (tok->type == TokenType::LBracket) {
        return parseBlock(tok->index);
    }

    auto stmt = parseStatement();
    if (!stmt)
        throw SyntaxError("Expected statement or code-block", tok->index, tok->text.length());
    
    return std::make_unique<BlockExpr>(std::move(stmt));
}

std::unique_ptr<Stmt> Parser::matchExpr() {
    size_t start = peek()->index;
    auto expr = parseExpr();
    if (!expr)
        throw SyntaxError("Expected expression", start, 0);
    
    Token *tok = next();
    if (!tok || tok->type == TokenType::LineBreak)
        return std::make_unique<ExprStmt>(std::move(expr));

    switch (tok->type) {
        /*case TokenType::TypeMarker:
            if (auto id = dynamic_cast<IdentifierExpr*>(expr.get())) {
                auto idPtr = std::unique_ptr<IdentifierExpr>(
                    static_cast<IdentifierExpr*>(expr.release())
                );

                return matchExplicitVarDecl(std::move(idPtr), *tok);
            }
            else
                throw SyntaxError("lhs of explicit declaration must be an identifier", expr->start(), expr->length());
        case TokenType::TypeInferredAssign:
            if (auto id = dynamic_cast<IdentifierExpr*>(expr.get())) {
                auto idPtr = std::unique_ptr<IdentifierExpr>(
                    static_cast<IdentifierExpr*>(expr.release())
                );

                return matchInferredVarDecl(std::move(idPtr), *tok);
            }
            else
                throw SyntaxError("lhs of inferred declaration must be an identifier", expr->start(), expr->length()); */
        case TokenType::Assign:
            return matchAssignment(std::move(expr), *tok);
        case TokenType::ReferenceAssign:
            return matchAssignment(std::move(expr), *tok);
        case TokenType::RBracket:
            index--;
            return std::make_unique<ExprStmt>(std::move(expr));
        default:
            std::cout << "We're here\n";
            size_t after = tok->index;
            while ((tok = next()) && tok->type != TokenType::LineBreak) {}
            if (tok)
                throw SyntaxError("Unexpected tokens after expression", after, tok->index - after + tok->text.length() - 1);
            else
                throw SyntaxError("Unexpected tokens after expression", after, 10000000);
    }
}

std::unique_ptr<Stmt> Parser::parseStatement() {

    while (peek()->type == TokenType::LineBreak) {
        if (!next()) return nullptr;
    }

    Token *tok = peek();
    if (tok->type == TokenType::IF) {
        return matchIf();
    }
    else if (tok->type == TokenType::RETURN) {
        next();
        auto expr = parseExpr();
        return std::make_unique<ReturnStmt>(std::move(expr), tok->index);
    }
    else if (tok->type == TokenType::Identifier) {
        next();
        if (!peek()) {
            return std::make_unique<ExprStmt>(std::make_unique<IdentifierExpr>(*tok));
        }

        if (peek()->type == TokenType::TypeMarker) {
            auto idPtr = std::make_unique<IdentifierExpr>(*tok);
            return matchExplicitVarDecl(std::move(idPtr), *next());
        }

        if (peek()->type == TokenType::TypeInferredAssign) {
            auto idPtr = std::make_unique<IdentifierExpr>(*tok);
            return matchInferredVarDecl(std::move(idPtr), *next());
        }

        index--;
        matchExpr();
    }
    else {
        return matchExpr();
    }

    throw SyntaxError("Invalid statement shape", tok->index, peek()->index - tok->index + peek()->text.length());
}

#pragma endregion

#pragma region General

Token& Parser::expect(TokenType type) {
    if (!peek()) {
        throw SyntaxError("Expected token", tokens.back().index + tokens.back().text.length(), 0);
    }

    if (peek()->type != type) {
        size_t start = peek()->index;
        size_t length = next()->text.length();
        skipToLineBreak();
        throw SyntaxError("Expected token of certain type", start, length);
    }

    return *next();
}

void Parser::skipToLineBreak() {
    while (peek() && next()->type != TokenType::LineBreak) {}
    skipLineBreaks();
}

void Parser::skipLineBreaks() {
    while (peek() && peek()->type == TokenType::LineBreak) { next(); }
}

std::vector<std::unique_ptr<Stmt>> Parser::parse() {
    index = 0;

    std::vector<std::unique_ptr<Stmt>> stmts;
    while (peek()) {
        try {
            auto stmt = parseStatement();
            if (stmt) {
                stmts.push_back(std::move(stmt));
            }
            else {
                break;
            }
        }
        catch (const SyntaxError& err) {
            auto it = std::upper_bound(lineOffsets.begin(), lineOffsets.end(), err.start);
            size_t line = it - lineOffsets.begin() - 1;
            size_t column = err.start - lineOffsets[line];
            std::cout << "\033[31mSyntaxError at " + path + " (" + std::to_string(line+1) + ":" + std::to_string(column+1) + "): " + err.msg + "\033[0m\n";
        }
    }

    return std::move(stmts);
}

std::vector<std::unique_ptr<Stmt>> Parser::parseToks(const std::vector<Token>& toks, const std::vector<size_t>& offsets) {
    tokens = toks;
    lineOffsets = offsets;
    return parse();
}

#pragma endregion