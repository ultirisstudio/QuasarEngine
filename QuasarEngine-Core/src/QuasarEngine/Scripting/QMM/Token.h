#pragma once

#include <string>
#include <variant>

namespace QuasarEngine {
    struct SourceLocation { int line = 1; int col = 1; };

    enum class TokenType {
        LEFT_PAREN, RIGHT_PAREN,
        LEFT_BRACE, RIGHT_BRACE,
        COMMA, DOT, SEMICOLON,
        PLUS, MINUS, STAR, SLASH, PERCENT,
        BANG, EQUAL, LESS, GREATER,
        BANG_EQUAL, EQUAL_EQUAL, LESS_EQUAL, GREATER_EQUAL,
        AND_AND, OR_OR,
        IDENTIFIER, STRING, NUMBER,
        LET, FN, RETURN, IF, ELSE, WHILE, Q_TRUE, Q_FALSE, NIL,
        END_OF_FILE
    };

    struct Token {
        TokenType type{};
        std::string lexeme;
        SourceLocation loc{};
        std::variant<std::monostate, double, std::string, bool> literal{};
    };
}
