#pragma once

#include "Token.h"

#include <string>
#include <vector>
#include <unordered_map>
#include <cctype>
#include <optional>
#include <sstream>
#include <iostream>

namespace QuasarEngine {
    class Lexer {
        const std::string src;
        size_t start = 0, current = 0;
        int line = 1, col = 1;
        std::unordered_map<std::string, TokenType> keywords{
            {"let",TokenType::LET},{"fn",TokenType::FN},{"return",TokenType::RETURN},
            {"if",TokenType::IF},{"else",TokenType::ELSE},{"while",TokenType::WHILE},
            {"true",TokenType::Q_TRUE},{"false",TokenType::Q_FALSE},{"nil",TokenType::NIL}
        };
    public:
        explicit Lexer(std::string code) : src(std::move(code)) {}

        std::vector<Token> scanTokens() {
            std::vector<Token> tokens;
            while (!isAtEnd()) {
                start = current;
                auto t = scanToken();
                if (t.has_value()) tokens.push_back(*t);
            }
            tokens.push_back(Token{ TokenType::END_OF_FILE, "", {line,col}, {} });
            return tokens;
        }
    private:
        bool isAtEnd() const { return current >= src.size(); }
        char advance() {
            char c = src[current++];
            if (c == '\n') { line++; col = 1; }
            else { col++; }
            return c;
        }
        char peek() const { return isAtEnd() ? '\0' : src[current]; }
        char peekNext() const { return (current + 1 >= src.size()) ? '\0' : src[current + 1]; }
        bool match(char expected) { if (isAtEnd() || src[current] != expected) return false; current++; col++; return true; }

        static bool isAlpha(char c) {
            return std::isalpha((unsigned char)c) || c == '_' || c == '?' || c == '!' || c == '$';
        }
        static bool isAlphaNum(char c) { return isAlpha(c) || std::isdigit((unsigned char)c); }

        std::optional<Token> scanToken() {
            char c = advance();

            if (std::isspace((unsigned char)c)) return std::nullopt;

            if (c == '/' && peek() == '/') {
                while (peek() != '\n' && !isAtEnd()) advance();
                return std::nullopt;
            }

            if (c == '/' && peek() == '*') {
                advance();
                while (!isAtEnd()) {
                    if (peek() == '*' && peekNext() == '/') { advance(); advance(); break; }
                    advance();
                }
                return std::nullopt;
            }

            SourceLocation tokLoc{ line,col };
            switch (c) {
            case '(': return make(TokenType::LEFT_PAREN, tokLoc);
            case ')': return make(TokenType::RIGHT_PAREN, tokLoc);
            case '{': return make(TokenType::LEFT_BRACE, tokLoc);
            case '}': return make(TokenType::RIGHT_BRACE, tokLoc);
            case ',': return make(TokenType::COMMA, tokLoc);
            case '.': return make(TokenType::DOT, tokLoc);
            case ';': return make(TokenType::SEMICOLON, tokLoc);
            case '+': return make(TokenType::PLUS, tokLoc);
            case '-': return make(TokenType::MINUS, tokLoc);
            case '*': return make(TokenType::STAR, tokLoc);
            case '/': return make(TokenType::SLASH, tokLoc);
            case '%': return make(TokenType::PERCENT, tokLoc);
            case '!': return make(match('=') ? TokenType::BANG_EQUAL : TokenType::BANG, tokLoc);
            case '=': return make(match('=') ? TokenType::EQUAL_EQUAL : TokenType::EQUAL, tokLoc);
            case '<': return make(match('=') ? TokenType::LESS_EQUAL : TokenType::LESS, tokLoc);
            case '>': return make(match('=') ? TokenType::GREATER_EQUAL : TokenType::GREATER, tokLoc);
            case '&': if (match('&')) return make(TokenType::AND_AND, tokLoc);
                throw QError("'&' seul interdit, utilisez '&&'");
            case '|': if (match('|')) return make(TokenType::OR_OR, tokLoc);
                throw QError("'|' seul interdit, utilisez '||'");
            case '"': return stringToken(tokLoc);
            }
            if (std::isdigit((unsigned char)c)) return numberToken(c, tokLoc);
            if (isAlpha(c)) return identifierToken(c, tokLoc);
            std::ostringstream oss; oss << "Caractère inattendu '" << c << "'";
            throw QError(tokLoc, oss.str());
        }

        std::optional<Token> make(TokenType t, SourceLocation loc) {
            return Token{ t, std::string(src.begin() + (long)start, src.begin() + (long)current), loc, {} };
        }

        std::optional<Token> stringToken(SourceLocation loc) {
            std::u32string result; result.reserve(32);
            while (!isAtEnd() && peek() != '"') {
                char c = advance();
                if (c == '\\') {
                    char e = advance();
                    switch (e) {
                    case 'n': result.push_back('\n'); break;
                    case 't': result.push_back('\t'); break;
                    case 'r': result.push_back('\r'); break;
                    case '"': result.push_back('"'); break;
                    case '\\': result.push_back('\\'); break;
                    case 'u': {
                        int h = 0; for (int i = 0; i < 4; i++) { char d = advance(); if (!std::isxdigit((unsigned char)d)) throw QError(loc, "séquence \\u invalide"); h = h * 16 + (std::isdigit((unsigned char)d) ? d - '0' : (std::tolower((unsigned char)d) - 'a' + 10)); }
                        result.push_back((char32_t)h);
                        break;
                    }
                    default: result.push_back(e); break;
                    }
                }
                else {
                    result.push_back((unsigned char)c);
                }
            }
            if (isAtEnd()) throw QError(loc, "Chaîne non terminée");
            advance();
            std::string utf8; utf8.reserve(result.size());
            for (char32_t cp : result) {
                if (cp <= 0x7F) utf8.push_back(char(cp));
                else if (cp <= 0x7FF) { utf8.push_back(char(0xC0 | ((cp >> 6) & 0x1F))); utf8.push_back(char(0x80 | (cp & 0x3F))); }
                else if (cp <= 0xFFFF) { utf8.push_back(char(0xE0 | ((cp >> 12) & 0x0F))); utf8.push_back(char(0x80 | ((cp >> 6) & 0x3F))); utf8.push_back(char(0x80 | (cp & 0x3F))); }
                else { utf8.push_back(char(0xF0 | ((cp >> 18) & 0x07))); utf8.push_back(char(0x80 | ((cp >> 12) & 0x3F))); utf8.push_back(char(0x80 | ((cp >> 6) & 0x3F))); utf8.push_back(char(0x80 | (cp & 0x3F))); }
            }
            return Token{ TokenType::STRING, utf8, loc, utf8 };
        }

        static bool isNumSep(char c) { return c == '_'; }

        std::optional<Token> numberToken(char first, SourceLocation loc) {
            std::string num; num.push_back(first);
            auto grabDigits = [&]() { while (std::isdigit((unsigned char)peek()) || isNumSep(peek())) { char d = advance(); if (!isNumSep(d)) num.push_back(d); } };
            grabDigits();
            if (peek() == '.' && std::isdigit((unsigned char)peekNext())) { num.push_back(advance()); grabDigits(); }
            if (peek() == 'e' || peek() == 'E') { num.push_back(advance()); if (peek() == '+' || peek() == '-') num.push_back(advance()); if (!std::isdigit((unsigned char)peek())) throw QError(loc, "exposant mal formé"); grabDigits(); }
            return Token{ TokenType::NUMBER, num, loc, std::stod(num) };
        }

        std::optional<Token> identifierToken(char first, SourceLocation loc) {
            std::string id; id.push_back(first);
            while (isAlphaNum(peek())) id.push_back(advance());
            auto it = keywords.find(id);
            if (it != keywords.end()) return Token{ it->second, id, loc, {} };
            return Token{ TokenType::IDENTIFIER, id, loc, {} };
        }
    };
}
