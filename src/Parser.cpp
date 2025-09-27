#include "Parser.h"
#include <iostream>
#include <fstream>
#include <sstream>

std::unique_ptr<Expr> Parser::parseExpression(int rbp) {
    if (pos >= tokens.size()) { return nullptr; }
    Token tok = next();
    auto left = nud(tok);
    while (pos < tokens.size() && rbp < lbp(peek().type)) {
        tok = next();
        if (pos >= tokens.size()) {
            parserError("Expected token after '" + tok.text, tokens.back().line, tokens.back().column);
            throw;
        }
        left = led(tok, std::move(left));
    }

    return left;
}

Type Parser::parseTypeExpr(int rbp) {
    if (pos >= tokens.size()) { return nullptr; }
    Token tok = next();
    size_t startPos = pos;
    Type left = typeNud(tok);
    if (!left) {
        pos = startPos;
        return left;
    }
    while (pos < tokens.size() && rbp < typeLbp(peek().type)) {
        tok = next();
        if (pos >= tokens.size()) {
            parserError("Expected token after '" + tok.text, tokens.back().line, tokens.back().column);
            throw;
        }
        left = typeLed(tok, left);
    }

    return left;
}

std::unique_ptr<Expr> Parser::nud(Token tok) {
    switch (tok.type) {
        case TokenType::Number:
            return std::make_unique<NumExpr>(std::stof(tok.text), tok.line, tok.column);
        case TokenType::Integer:
            return std::make_unique<IntExpr>(std::stoi(tok.text), tok.line, tok.column);
        case TokenType::TRUE:
            return std::make_unique<BoolExpr>(true, tok.line, tok.column);
        case TokenType::FALSE:
            return std::make_unique<BoolExpr>(false, tok.line, tok.column);
        case TokenType::String:
            return std::make_unique<StrExpr>(tok.text, tok.line, tok.column);
        case TokenType::Identifier:
            return std::make_unique<IdentifierExpr>(tok.text, tok.line, tok.column);
        case TokenType::Label: {
            auto expr = parseExpression();
            return std::make_unique<LabelledExpr>(tok.text, std::move(expr), tok.line, tok.column);
        }
        case TokenType::Minus:
        case TokenType::Slash:
            return std::make_unique<UnaryExpr>(parseExpression(50), tok.type, tok.line, tok.column);
        case TokenType::Plus:
        case TokenType::At:
            return std::make_unique<UnaryExpr>(parseExpression(0), tok.type, tok.line, tok.column);
        case TokenType::Colon:
            return std::make_unique<SignalExpr>(nullptr, parseExpression(lbp(tok.type)), tok.line, tok.column);
        case TokenType::Out:
            return std::make_unique<OutExpr>(parseExpression(0), tok.line, tok.column);
        case TokenType::Type:
        case TokenType::Dollar: {
            pos--;
            Type type = parseTypeExpr();

            if (pos >= tokens.size()) {
                parserError("Expected declaration after type", 0, 0);
                throw;
            }

            auto decl = parseExpression();
            return std::make_unique<DeclExpr>(type, std::move(decl), tok.line, tok.column);
        }
        case TokenType::Sine:
        case TokenType::Square:
        case TokenType::Saw: {
            if (pos >= tokens.size())
                throw std::runtime_error("Expected expression after oscillator primitive keyword");

            auto param = parseExpression();
            return std::make_unique<OscPrimExpr>(to_osc_prim(tok.type), std::move(param), tok.line, tok.column);
        }
        case TokenType::LParen: {
            pos--;
            if (Type type = parseTypeExpr()) {
                std::cout << peek().text << std::endl;
                if (auto decl = parseExpression()) {
                    return std::make_unique<DeclExpr>(type, std::move(decl), tok.line, tok.column);
                }

                parserError("Expected declaration after type", tok.line, tok.column);
                throw;
            }

            std::vector<std::unique_ptr<Expr>> tuple;
            while (pos < tokens.size()) {
                tuple.push_back(parseExpression());
                if (next().type == TokenType::RParen) {
                    if (tuple.size() == 1)
                        return std::move(tuple[0]);
                    else
                        return std::make_unique<TupleExpr>(std::move(tuple), tok.line, tok.column);
                }
                
                pos--;
                if (next().type != TokenType::Comma) {
                    parserError("Elements of tuples must be separated with commas ','", tokens[pos-1].line, tokens[pos-1].column);
                    throw;
                }
            }

            parserError("Parentheses must be closed with ')'", tokens.back().line, tokens.back().column);
            throw;
        }
        case TokenType::LBrace: {
            pos--;
            if (Type type = parseTypeExpr()) {
                if (pos >= tokens.size()) {
                    parserError("Expected declaration after type", tok.line, tok.column);
                    throw;
                }

                auto decl = parseExpression();
                return std::make_unique<DeclExpr>(type, std::move(decl), tok.line, tok.column);
            }

            std::vector<std::unique_ptr<Expr>> list;
            while (pos < tokens.size()) {
                if (peek().type == TokenType::RBrace) {
                    next();
                    return std::make_unique<ListExpr>(std::move(list), tok.line, tok.column);
                }
                
                list.push_back(std::move(parseExpression()));
            }
            parserError(
                "Lists need to be closed with '}'", tokens.back().line, tokens.back().column
            );
        }
        case TokenType::LBracket: {
            std::vector<std::unique_ptr<Expr>> expressions;
            while (pos < tokens.size()) {
                if (peek().type == TokenType::RBracket) {
                    next();
                    return std::make_unique<BlockExpr>(std::move(expressions), tok.line, tok.column);
                }
                else if(peek().type != TokenType::LnBreak) {
                    expressions.push_back(std::move(parseExpression()));
                    continue;
                }
                
                next();
            }
            throw std::runtime_error(
                "Expected ']' at (" + 
                std::to_string(tokens.back().line) + ":" + 
                std::to_string(tokens.back().column) + ")"
            );
        }
        case TokenType::PLAY: {
            auto osc = parseExpression();
            std::unique_ptr<Expr> signal;
            if (pos >= tokens.size()
            || peek().type == TokenType::RParen
            || peek().type == TokenType::RBrace
            || peek().type == TokenType::RBracket)
                signal = nullptr;
            else
                signal = parseExpression();

            return std::make_unique<PlaybackExpr>(std::move(osc), std::move(signal), tok.line, tok.column);
        }
        case TokenType::RELEASE: {
            if (pos >= tokens.size())
                return std::make_unique<ReleaseExpr>(nullptr, tok.line, tok.column);
            else if (peek().type != TokenType::Label) {
                return std::make_unique<ReleaseExpr>(nullptr, tok.line, tok.column);
                //std::cout << "WHAT\n";
                //throw std::runtime_error("Unexpected token for release.");
            }
            
            return std::make_unique<ReleaseExpr>(std::make_unique<std::string>(next().text), tok.line, tok.column);
        }
    }

    parserError("Unexpected prefix token '" + tok.text + "'.", tok.line, tok.column);
    throw;
}

std::unique_ptr<Expr> Parser::led(Token tok, std::unique_ptr<Expr> left) {
    switch (tok.type) {
        case TokenType::Plus:
        case TokenType::Minus:
        case TokenType::Star:
        case TokenType::Slash: {
            auto right = parseExpression(lbp(tok.type));
            return std::make_unique<BinaryExpr>(std::move(left), std::move(right), tok.type, tok.line, tok.column);
        }
        case TokenType::Colon: {
            auto right = parseExpression(lbp(tok.type));
            return std::make_unique<SignalExpr>(std::move(left), std::move(right), tok.line, tok.column);
        }
        case TokenType::EAssign: {
            auto right = parseExpression(lbp(tok.type) - 1);
            return std::make_unique<AssignmentExpr>(AssignMode::eager, std::move(left), std::move(right), tok.line, tok.column);
        }
        case TokenType::LAssign: {
            auto right = parseExpression(lbp(tok.type) - 1);
            return std::make_unique<AssignmentExpr>(AssignMode::lazy, std::move(left), std::move(right), tok.line, tok.column);
        }
    }

    parserError("Unexpected infix token '" + tok.text + "'.", tok.line, tok.column);
    throw;
}

Type Parser::typeNud(Token tok) {
    switch (tok.type) {
        case TokenType::Type: {
            return fromString(tok.text);
        }
        case TokenType::LParen: {
            size_t startPos = pos;
            std::vector<Type> tuple;
            while (pos < tokens.size()) {
                if (Type elem = parseTypeExpr())
                    tuple.push_back(elem);
                else {
                    pos = startPos;
                    return nullptr;
                }

                if (next().type == TokenType::RParen) {
                    if (tuple.size() == 1)
                        return tuple[0];
                    else {
                        return std::make_shared<TupleType>(tuple);
                    }
                }

                pos--;
                if (next().type != TokenType::Comma) {
                    parserError("Expected ',' to separate types in tuple", tokens[pos-1].line, tokens[pos-1].column);
                    throw; //unexpected token
                }
            }

            parserError("Tuple type must be closed with ')'", tokens.back().line, tokens.back().column);
            throw;
        }
        case TokenType::LBrace: {
            size_t startPos = pos;
            if(Type inner = parseTypeExpr()) {
                if (pos >= tokens.size() || next().type != TokenType::RBrace) {
                    parserError("List types must be closed with '}'", tok.line, tok.column);
                    throw;
                }
                    
                return std::make_shared<ListType>(inner);
            }   
            
            pos = startPos;
            return nullptr;
        }
        case TokenType::Dollar: {
            std::shared_ptr<TupleType> input;
            if (peek().type != TokenType::Arrow) {
                Type first = parseTypeExpr();
                if (auto* in = dynamic_cast<const TupleType*>(first.get()))
                    input = std::make_shared<TupleType>(*in);
                else
                    input = std::make_shared<TupleType>(TupleType({ first }));
            }
            
            if (peek().type != TokenType::Arrow)
                return std::make_shared<LambdaType>(input, nullptr);
            
            next();
            Type output = parseTypeExpr();
            return std::make_shared<LambdaType>(input, output);
        }
    }

    return nullptr;
}

Type Parser::typeLed(Token tok, Type left) {
    switch (tok.type) {
        case TokenType::Vert:
            Type right = parseTypeExpr();
            return std::make_shared<VariantType>(left, right);
    }

    parserError("Unexpected type infix token '" + tok.text + "'.", tok.line, tok.column);
    throw;
}

int Parser::typeLbp(TokenType type) {
    return type == TokenType::Vert ? 10 : 0;
}

int Parser::lbp(TokenType type) {
    switch (type) {
        case TokenType::Plus:
        case TokenType::Minus:
            return 20;
        case TokenType::Star:
        case TokenType::Slash:
            return 30;
        case TokenType::Colon:
            return 40;
        case TokenType::EAssign:
        case TokenType::LAssign:
            return 10;
        default:
            return 0;
    }
}

void Parser::parseCode(const std::string& code) {
    Lexer lexer;
    tokens = lexer.tokenize(code);

    pos = 0;
    lastPos = 0;
    while (pos < tokens.size()) {
        if (tokens[pos].type == TokenType::LnBreak) {
            ++pos;
            ++lastPos;
            continue;
        }
        std::unique_ptr<Expr> parsed = parseExpression();

// ----- logs --------------------------

        if (logSettings->logRaw && !logSettings->hideAll) {
            size_t endc = pos < tokens.size() ? tokens[pos].pos : code.size();
            std::string rawCode = code.substr(tokens[lastPos].pos, endc - tokens[lastPos].pos);
            if (rawCode.back() != '\n') rawCode.append("\n");
            std::cout << "\033[1;34mRaw code>\033[0m" << std::endl << rawCode;
        }

        if (logSettings->logTokens && !logSettings->hideAll) {
            std::cout << "\033[1;34mTokenized code>\033[0m" << std::endl;

            for (size_t i = lastPos; i < pos; ++i)
                std::cout << tokens[i].to_string();
            std::cout << std::endl;
        }

        if (logSettings->logParsed && !logSettings->hideAll) {
            std::cout << "\033[1;34mParsed code>\033[0m" << std::endl;
            std::cout << parsed->to_string() << std::endl;
        }

        if (logSettings->logOutput && !logSettings->hideAll)
            std::cout << "\033[1;36mOutput>\033[0m" << std::endl;
        
        parsed->accept(*backend);

        if (logSettings->logOutput && !logSettings->hideAll)
            std::cout << std::endl;

        lastPos = pos;
    }
}

void Parser::parseFile(const std::string& path) {
    std::ifstream file(path);

    if (!file.is_open()) {
        std::cerr << "\033[0;31mCould not open file '" << path << "'.\033[0m" << std::endl;
        return;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string content = buffer.str();

    parseCode(content);
}

void Parser::parserError(const std::string& msg, size_t line, size_t col) {
    std::cout << "\033[0;31mParser error at (" << std::to_string(line) << ":" << std::to_string(col) << "): " << msg << "\033[0m" << std::endl;
}