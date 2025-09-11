#pragma once

#include <string>
#include <vector>
#include <memory>
#include <utility>
#include <initializer_list>

#include "Token.h"
#include "AST.h"

namespace QuasarEngine {
    class Parser {
        const std::vector<Token> tokens; size_t current = 0; int fnDepth = 0;
        const Token& peek() const { return tokens[current]; }
        const Token& previous() const { return tokens[current - 1]; }
        bool isAtEnd() const { return peek().type == TokenType::END_OF_FILE; }
        bool check(TokenType t) const { return !isAtEnd() && peek().type == t; }
        const Token& advance() { if (!isAtEnd()) current++; return previous(); }
        bool match(std::initializer_list<TokenType> list) { for (auto t : list) { if (check(t)) { advance(); return true; } } return false; }
        [[noreturn]] void parseError(const Token& t, std::string_view msg) const { throw QError(t.loc, std::string(msg)); }
        const Token& consume(TokenType t, const char* message) { if (check(t)) return advance(); parseError(peek(), message); }
    public:
        explicit Parser(std::vector<Token> t) :tokens(std::move(t)) {}
        std::vector<StmtPtr> parse() { std::vector<StmtPtr> stmts; while (!isAtEnd()) stmts.push_back(declaration()); return stmts; }
    private:
        StmtPtr declaration() {
            if (match({ TokenType::FN })) return functionDecl();
            if (match({ TokenType::LET })) return letDecl();
            return statement();
        }

        StmtPtr functionDecl() {
            std::string name = consume(TokenType::IDENTIFIER, "Nom de fonction attendu").lexeme;
            auto [params, body] = fnTail();
            return std::make_shared<FunctionStmt>(name, std::move(params), std::move(body));
        }

        std::pair<std::vector<std::string>, std::shared_ptr<BlockStmt>> fnTail() {
            consume(TokenType::LEFT_PAREN, "'(' attendu après nom de fonction");
            std::vector<std::string> params;
            if (!check(TokenType::RIGHT_PAREN)) {
                do { params.push_back(consume(TokenType::IDENTIFIER, "Paramètre attendu").lexeme); } while (match({ TokenType::COMMA }));
            }
            consume(TokenType::RIGHT_PAREN, "')' attendu après paramètres");
            
            if (!match({ TokenType::LEFT_BRACE })) parseError(peek(), "Corps de fonction { ... } attendu");
            fnDepth++;
            auto body = blockRest();
            fnDepth--;
            return { std::move(params), body };
        }

        StmtPtr letDecl() {
            std::string name = consume(TokenType::IDENTIFIER, "Nom de variable attendu").lexeme;
            ExprPtr init = nullptr;
            if (match({ TokenType::EQUAL })) init = expression();
            consume(TokenType::SEMICOLON, "; attendu après déclaration");
            return std::make_shared<LetStmt>(name, init);
        }

        StmtPtr statement() {
            if (match({ TokenType::LEFT_BRACE })) return block();
            if (match({ TokenType::IF })) return ifStmt();
            if (match({ TokenType::WHILE })) return whileStmt();
            if (match({ TokenType::RETURN })) return returnStmt();
            return exprStmt();
        }

        StmtPtr block() { return blockRest(); }

        std::shared_ptr<BlockStmt> blockRest() {
            std::vector<StmtPtr> stmts;
            while (!check(TokenType::RIGHT_BRACE) && !isAtEnd()) stmts.push_back(declaration());
            consume(TokenType::RIGHT_BRACE, "'}' attendu pour fermer le bloc");
            return std::make_shared<BlockStmt>(std::move(stmts));
        }

        StmtPtr ifStmt() {
            consume(TokenType::LEFT_PAREN, "'(' après if attendu");
            auto cond = expression();
            consume(TokenType::RIGHT_PAREN, ") après condition attendu");
            auto thenB = statement();
            StmtPtr elseB = nullptr;
            if (match({ TokenType::ELSE })) elseB = statement();
            return std::make_shared<IfStmt>(cond, thenB, elseB);
        }

        StmtPtr whileStmt() {
            consume(TokenType::LEFT_PAREN, "'(' après while attendu");
            auto cond = expression();
            consume(TokenType::RIGHT_PAREN, ") après condition attendu");
            auto body = statement();
            return std::make_shared<WhileStmt>(cond, body);
        }

        StmtPtr returnStmt() {
            if (fnDepth <= 0) parseError(previous(), "'return' en dehors d'une fonction");
            ExprPtr value = nullptr;
            if (!check(TokenType::SEMICOLON)) value = expression();
            consume(TokenType::SEMICOLON, "; attendu après return");
            return std::make_shared<ReturnStmt>(value);
        }

        StmtPtr exprStmt() { auto e = expression(); consume(TokenType::SEMICOLON, "; attendu après expression"); return std::make_shared<ExprStmt>(e); }

        ExprPtr expression() { return assignment(); }

        ExprPtr assignment() {
            auto expr = logical_or();
            if (match({ TokenType::EQUAL })) {
                auto value = assignment();
                if (auto v = std::dynamic_pointer_cast<VariableExpr>(expr))
                    return std::make_shared<AssignExpr>(v->name, value);
                parseError(previous(), "Cible d'affectation invalide");
            }
            return expr;
        }

        ExprPtr logical_or() {
            auto expr = logical_and();
            while (match({ TokenType::OR_OR })) {
                auto op = previous().type; auto right = logical_and();
                expr = std::make_shared<LogicalExpr>(expr, op, right);
            }
            return expr;
        }
        ExprPtr logical_and() {
            auto expr = equality();
            while (match({ TokenType::AND_AND })) {
                auto op = previous().type; auto right = equality();
                expr = std::make_shared<LogicalExpr>(expr, op, right);
            }
            return expr;
        }

        ExprPtr equality() { auto expr = comparison(); while (match({ TokenType::BANG_EQUAL,TokenType::EQUAL_EQUAL })) { auto op = previous().type; auto right = comparison(); expr = std::make_shared<BinaryExpr>(expr, op, right); } return expr; }
        ExprPtr comparison() { auto expr = term(); while (match({ TokenType::LESS,TokenType::LESS_EQUAL,TokenType::GREATER,TokenType::GREATER_EQUAL })) { auto op = previous().type; auto right = term(); expr = std::make_shared<BinaryExpr>(expr, op, right); } return expr; }
        ExprPtr term() { auto expr = factor(); while (match({ TokenType::PLUS,TokenType::MINUS })) { auto op = previous().type; auto right = factor(); expr = std::make_shared<BinaryExpr>(expr, op, right); } return expr; }
        ExprPtr factor() { auto expr = unary();  while (match({ TokenType::STAR,TokenType::SLASH,TokenType::PERCENT })) { auto op = previous().type; auto right = unary();  expr = std::make_shared<BinaryExpr>(expr, op, right); } return expr; }

        ExprPtr unary() { if (match({ TokenType::BANG, TokenType::MINUS })) { auto op = previous().type; auto right = unary(); return std::make_shared<UnaryExpr>(op, right); } return call(); }

        ExprPtr call() {
            auto expr = primary();
            while (match({ TokenType::LEFT_PAREN })) {
                std::vector<ExprPtr> args;
                if (!check(TokenType::RIGHT_PAREN)) {
                    do { args.push_back(expression()); } while (match({ TokenType::COMMA }));
                }
                consume(TokenType::RIGHT_PAREN, ") attendu après arguments");
                expr = std::make_shared<CallExpr>(expr, std::move(args));
            }
            return expr;
        }

        ExprPtr primary() {
            if (match({ TokenType::NUMBER })) return std::make_shared<LiteralExpr>(std::get<double>(previous().literal));
            if (match({ TokenType::STRING })) return std::make_shared<LiteralExpr>(std::get<std::string>(previous().literal));
            if (match({ TokenType::Q_TRUE })) return std::make_shared<LiteralExpr>(true);
            if (match({ TokenType::Q_FALSE })) return std::make_shared<LiteralExpr>(false);
            if (match({ TokenType::NIL })) return std::make_shared<LiteralExpr>(nullptr);
            if (match({ TokenType::IDENTIFIER })) return std::make_shared<VariableExpr>(previous().lexeme);
            if (match({ TokenType::FN })) { auto [params, body] = fnTail(); return std::make_shared<FnExpr>(std::move(params), std::move(body)); }
            if (match({ TokenType::LEFT_PAREN })) { auto e = expression(); consume(TokenType::RIGHT_PAREN, ") manquante"); return e; }
            parseError(peek(), "Expression invalide");
            return nullptr;
        }
    };
}
