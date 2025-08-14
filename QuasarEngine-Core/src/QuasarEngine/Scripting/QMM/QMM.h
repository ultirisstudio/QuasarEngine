#pragma once

#include <cassert>
#include <cctype>
#include <cmath>
#include <functional>
#include <iostream>
#include <limits>
#include <map>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>
#include <optional>

namespace QuasarEngine
{
    struct SourceLocation { int line = 1; int col = 1; };
    struct QError : std::runtime_error { using std::runtime_error::runtime_error; };

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
        TokenType type; std::string lexeme; SourceLocation loc; std::variant<std::monostate, double, std::string, bool> literal;
    };

    class Lexer {
        const std::string src; size_t start = 0, current = 0; int line = 1, col = 1;
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
        char advance() { char c = src[current++]; if (c == '\n') { line++; col = 1; } else col++; return c; }
        char peek() const { return isAtEnd() ? '\0' : src[current]; }
        char peekNext() const { return (current + 1 >= src.size()) ? '\0' : src[current + 1]; }
        bool match(char expected) { if (isAtEnd() || src[current] != expected) return false; current++; col++; return true; }

        static bool isAlpha(char c) { return std::isalpha((unsigned char)c) || c == '_'; }
        static bool isAlphaNum(char c) { return isAlpha(c) || std::isdigit((unsigned char)c); }

        std::optional<Token> scanToken() {
            char c = advance();
            
            if (std::isspace((unsigned char)c)) return std::nullopt;
            if (c == '/' && peek() == '/') { while (peek() != '\n' && !isAtEnd()) advance(); return std::nullopt; }

            SourceLocation loc{ line,col };
            switch (c) {
            case '(': return make(TokenType::LEFT_PAREN, loc);
            case ')': return make(TokenType::RIGHT_PAREN, loc);
            case '{': return make(TokenType::LEFT_BRACE, loc);
            case '}': return make(TokenType::RIGHT_BRACE, loc);
            case ',': return make(TokenType::COMMA, loc);
            case '.': return make(TokenType::DOT, loc);
            case ';': return make(TokenType::SEMICOLON, loc);
            case '+': return make(TokenType::PLUS, loc);
            case '-': return make(TokenType::MINUS, loc);
            case '*': return make(TokenType::STAR, loc);
            case '%': return make(TokenType::PERCENT, loc);
            case '!': return make(match('=') ? TokenType::BANG_EQUAL : TokenType::BANG, loc);
            case '=': return make(match('=') ? TokenType::EQUAL_EQUAL : TokenType::EQUAL, loc);
            case '<': return make(match('=') ? TokenType::LESS_EQUAL : TokenType::LESS, loc);
            case '>': return make(match('=') ? TokenType::GREATER_EQUAL : TokenType::GREATER, loc);
            case '&': if (match('&')) return make(TokenType::AND_AND, loc); break;
            case '|': if (match('|')) return make(TokenType::OR_OR, loc); break;
            case '"': return stringToken(loc);
            }
            if (std::isdigit((unsigned char)c)) return numberToken(c, loc);
            if (isAlpha(c)) return identifierToken(c, loc);
            std::ostringstream oss; oss << "Caractère inattendu '" << c << "'";
            throw QError(oss.str());
        }

        std::optional<Token> make(TokenType t, SourceLocation loc) {
            return Token{ t, src.substr(start, current - start), loc, {} };
        }
        std::optional<Token> stringToken(SourceLocation loc) {
            std::string s;
            while (peek() != '"' && !isAtEnd()) {
                char c = advance();
                if (c == '\\') {
                    char n = advance();
                    if (n == '"') s.push_back('"'); else if (n == 'n') s.push_back('\n'); else s.push_back(n);
                }
                else s.push_back(c);
            }
            if (isAtEnd()) throw QError("Chaîne non terminée");
            advance();
            return Token{ TokenType::STRING, s, loc, s };
        }
        std::optional<Token> numberToken(char first, SourceLocation loc) {
            std::string num; num.push_back(first);
            while (std::isdigit((unsigned char)peek())) num.push_back(advance());
            if (peek() == '.' && std::isdigit((unsigned char)peekNext())) {
                num.push_back(advance());
                while (std::isdigit((unsigned char)peek())) num.push_back(advance());
            }
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

    struct Expr; struct Stmt; struct Function; struct Environment;
    using ExprPtr = std::shared_ptr<Expr>; using StmtPtr = std::shared_ptr<Stmt>;

    struct Expr { virtual ~Expr() = default; };
    struct Stmt { virtual ~Stmt() = default; };

    struct LiteralExpr : Expr {
        std::variant<std::nullptr_t, double, std::string, bool> value;

        template<typename T>
        explicit LiteralExpr(T v) : value(std::move(v)) {}
    };

    struct VariableExpr : Expr { std::string name; explicit VariableExpr(std::string n) :name(std::move(n)) {} };
    struct AssignExpr : Expr { std::string name; ExprPtr value; AssignExpr(std::string n, ExprPtr v) :name(std::move(n)), value(std::move(v)) {} };
    struct UnaryExpr : Expr { TokenType op; ExprPtr right; UnaryExpr(TokenType o, ExprPtr r) :op(o), right(std::move(r)) {} };
    struct BinaryExpr : Expr { ExprPtr left; TokenType op; ExprPtr right; BinaryExpr(ExprPtr l, TokenType o, ExprPtr r) :left(std::move(l)), op(o), right(std::move(r)) {} };
    struct CallExpr : Expr { std::string callee; std::vector<ExprPtr> args; CallExpr(std::string c, std::vector<ExprPtr> a) :callee(std::move(c)), args(std::move(a)) {} };

    struct ExprStmt : Stmt { ExprPtr expr; explicit ExprStmt(ExprPtr e) :expr(std::move(e)) {} };
    struct LetStmt : Stmt { std::string name; ExprPtr initializer; LetStmt(std::string n, ExprPtr i) :name(std::move(n)), initializer(std::move(i)) {} };
    struct BlockStmt : Stmt { std::vector<StmtPtr> statements; explicit BlockStmt(std::vector<StmtPtr> s) :statements(std::move(s)) {} };
    struct IfStmt : Stmt { ExprPtr cond; StmtPtr thenBranch; StmtPtr elseBranch; IfStmt(ExprPtr c, StmtPtr t, StmtPtr e) :cond(std::move(c)), thenBranch(std::move(t)), elseBranch(std::move(e)) {} };
    struct WhileStmt : Stmt { ExprPtr cond; StmtPtr body; WhileStmt(ExprPtr c, StmtPtr b) :cond(std::move(c)), body(std::move(b)) {} };

    struct FunctionStmt : Stmt {
        std::string name; std::vector<std::string> params; std::shared_ptr<BlockStmt> body; FunctionStmt(std::string n, std::vector<std::string> p, std::shared_ptr<BlockStmt> b)
            : name(std::move(n)), params(std::move(p)), body(std::move(b)) {}
    };
    struct ReturnStmt : Stmt { ExprPtr value; explicit ReturnStmt(ExprPtr v) :value(std::move(v)) {} };

    class Parser {
        const std::vector<Token> tokens; size_t current = 0;
        const Token& peek() const { return tokens[current]; }
        const Token& previous() const { return tokens[current - 1]; }
        bool isAtEnd() const { return peek().type == TokenType::END_OF_FILE; }
        bool check(TokenType t) const { return !isAtEnd() && peek().type == t; }
        const Token& advance() { if (!isAtEnd()) current++; return previous(); }
        bool match(std::initializer_list<TokenType> list) { for (auto t : list) { if (check(t)) { advance(); return true; } } return false; }
        const Token& consume(TokenType t, const char* message) { if (check(t)) return advance(); throw QError(message); }

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
            consume(TokenType::LEFT_PAREN, "'(' attendu après le nom de fonction");
            std::vector<std::string> params;
            if (!check(TokenType::RIGHT_PAREN)) {
                do { params.push_back(consume(TokenType::IDENTIFIER, "Paramètre attendu").lexeme); } while (match({ TokenType::COMMA }));
            }
            consume(TokenType::RIGHT_PAREN, "')' attendu après paramètres");
            auto body = std::dynamic_pointer_cast<BlockStmt>(statement());
            if (!body) throw QError("Le corps de fonction doit être un bloc { ... }");
            return std::make_shared<FunctionStmt>(name, params, body);
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

        StmtPtr block() {
            std::vector<StmtPtr> stmts;
            while (!check(TokenType::RIGHT_BRACE) && !isAtEnd()) stmts.push_back(declaration());
            consume(TokenType::RIGHT_BRACE, "'}' attendu pour fermer le bloc");
            return std::make_shared<BlockStmt>(std::move(stmts));
        }

        StmtPtr ifStmt() {
            consume(TokenType::LEFT_PAREN, "'(' après if attendu");
            auto cond = expression();
            consume(TokenType::RIGHT_PAREN, "')' après condition attendu");
            auto thenB = statement();
            StmtPtr elseB = nullptr;
            if (match({ TokenType::ELSE })) elseB = statement();
            return std::make_shared<IfStmt>(cond, thenB, elseB);
        }

        StmtPtr whileStmt() {
            consume(TokenType::LEFT_PAREN, "'(' après while attendu");
            auto cond = expression();
            consume(TokenType::RIGHT_PAREN, "')' après condition attendu");
            auto body = statement();
            return std::make_shared<WhileStmt>(cond, body);
        }

        StmtPtr returnStmt() {
            ExprPtr value = nullptr;
            if (!check(TokenType::SEMICOLON)) value = expression();
            consume(TokenType::SEMICOLON, "; attendu après return");
            return std::make_shared<ReturnStmt>(value);
        }

        StmtPtr exprStmt() { auto e = expression(); consume(TokenType::SEMICOLON, "; attendu après expression"); return std::make_shared<ExprStmt>(e); }

        ExprPtr expression() { return assignment(); }

        ExprPtr assignment() {
            auto expr = logic_or();
            if (match({ TokenType::EQUAL })) {
                auto value = assignment();
                if (auto v = std::dynamic_pointer_cast<VariableExpr>(expr))
                    return std::make_shared<AssignExpr>(v->name, value);
                throw QError("Cible d'affectation invalide");
            }
            return expr;
        }

        ExprPtr logic_or() { auto expr = logic_and(); while (match({ TokenType::OR_OR })) { auto op = previous().type; auto right = logic_and(); expr = std::make_shared<BinaryExpr>(expr, op, right); } return expr; }
        ExprPtr logic_and() { auto expr = equality(); while (match({ TokenType::AND_AND })) { auto op = previous().type; auto right = equality(); expr = std::make_shared<BinaryExpr>(expr, op, right); } return expr; }
        ExprPtr equality() { auto expr = comparison(); while (match({ TokenType::BANG_EQUAL,TokenType::EQUAL_EQUAL })) { auto op = previous().type; auto right = comparison(); expr = std::make_shared<BinaryExpr>(expr, op, right); } return expr; }
        ExprPtr comparison() { auto expr = term(); while (match({ TokenType::LESS,TokenType::LESS_EQUAL,TokenType::GREATER,TokenType::GREATER_EQUAL })) { auto op = previous().type; auto right = term(); expr = std::make_shared<BinaryExpr>(expr, op, right); } return expr; }
        ExprPtr term() { auto expr = factor(); while (match({ TokenType::PLUS,TokenType::MINUS })) { auto op = previous().type; auto right = factor(); expr = std::make_shared<BinaryExpr>(expr, op, right); } return expr; }
        ExprPtr factor() { auto expr = unary(); while (match({ TokenType::STAR,TokenType::SLASH,TokenType::PERCENT })) { auto op = previous().type; auto right = unary(); expr = std::make_shared<BinaryExpr>(expr, op, right); } return expr; }
        ExprPtr unary() { if (match({ TokenType::BANG, TokenType::MINUS })) { auto op = previous().type; auto right = unary(); return std::make_shared<UnaryExpr>(op, right); } return call(); }

        ExprPtr call() {
            if (match({ TokenType::IDENTIFIER })) {
                std::string name = previous().lexeme;
                if (match({ TokenType::LEFT_PAREN })) {
                    std::vector<ExprPtr> args;
                    if (!check(TokenType::RIGHT_PAREN)) {
                        do { args.push_back(expression()); } while (match({ TokenType::COMMA }));
                    }
                    consume(TokenType::RIGHT_PAREN, ") attendu après arguments");
                    return std::make_shared<CallExpr>(name, args);
                }
                return std::make_shared<VariableExpr>(name);
            }
            if (match({ TokenType::LEFT_PAREN })) { auto e = expression(); consume(TokenType::RIGHT_PAREN, ") manquante"); return e; }
            if (match({ TokenType::NUMBER })) return std::make_shared<LiteralExpr>(std::get<double>(previous().literal));
            if (match({ TokenType::STRING })) return std::make_shared<LiteralExpr>(std::get<std::string>(previous().literal));
            if (match({ TokenType::Q_TRUE })) return std::make_shared<LiteralExpr>(true);
            if (match({ TokenType::Q_FALSE })) return std::make_shared<LiteralExpr>(false);
            if (match({ TokenType::NIL })) return std::make_shared<LiteralExpr>(nullptr);
            throw QError("Expression invalide");
        }
    };

    struct Function;
    using NativeFunction = std::function<std::variant<std::nullptr_t, double, std::string, bool>(const std::vector<std::variant<std::nullptr_t, double, std::string, bool>>&)>;

    struct Value {
        using Prim = std::variant<std::nullptr_t, double, std::string, bool>;
        std::variant<Prim, std::shared_ptr<Function>, std::shared_ptr<NativeFunction>> data;

        Value() : data(Prim{ std::nullptr_t{} }) {}
        Value(Prim v) :data(std::move(v)) {}
        Value(std::shared_ptr<Function> f) :data(std::move(f)) {}
        Value(std::shared_ptr<NativeFunction> nf) :data(std::move(nf)) {}

        bool isPrim() const { return std::holds_alternative<Prim>(data); }
        Prim& prim() { return std::get<Prim>(data); }
        const Prim& prim() const { return std::get<Prim>(data); }
    };

    static std::string toString(const Value& v) {
        if (std::holds_alternative<Value::Prim>(v.data)) {
            const auto& p = std::get<Value::Prim>(v.data);
            if (std::holds_alternative<std::nullptr_t>(p)) return "nil";
            if (std::holds_alternative<double>(p)) { std::ostringstream oss; oss << std::get<double>(p); return oss.str(); }
            if (std::holds_alternative<std::string>(p)) return std::get<std::string>(p);
            if (std::holds_alternative<bool>(p)) return std::get<bool>(p) ? "true" : "false";
        }
        if (std::holds_alternative<std::shared_ptr<Function>>(v.data)) return "<fn>";
        return "<native>";
    }

    struct Environment : std::enable_shared_from_this<Environment> {
        std::unordered_map<std::string, Value> values;
        std::shared_ptr<Environment> enclosing;

        explicit Environment(std::shared_ptr<Environment> parent = nullptr) :enclosing(std::move(parent)) {}

        void define(const std::string& name, Value v) { values[name] = std::move(v); }

        bool assign(const std::string& name, Value v) {
            if (values.count(name)) { values[name] = std::move(v); return true; }
            if (enclosing) return enclosing->assign(name, std::move(v));
            return false;
        }

        Value get(const std::string& name) {
            if (values.count(name)) return values[name];
            if (enclosing) return enclosing->get(name);
            throw QError("Variable non définie: " + name);
        }
    };

    struct Function {
        std::vector<std::string> params; std::shared_ptr<BlockStmt> body; std::shared_ptr<Environment> closure;
        Function(std::vector<std::string> p, std::shared_ptr<BlockStmt> b, std::shared_ptr<Environment> e)
            : params(std::move(p)), body(std::move(b)), closure(std::move(e)) {}
    };

    struct ReturnJump { Value value; };

    class Interpreter {
    public:
        std::shared_ptr<Environment> globals = std::make_shared<Environment>();
        std::shared_ptr<Environment> env = globals;

        Value evaluate(const ExprPtr& e) {
            if (auto lit = std::dynamic_pointer_cast<LiteralExpr>(e)) return Value(lit->value);
            if (auto var = std::dynamic_pointer_cast<VariableExpr>(e)) return env->get(var->name);
            if (auto asg = std::dynamic_pointer_cast<AssignExpr>(e)) { auto v = evaluate(asg->value); if (!env->assign(asg->name, v)) throw QError("Affectation sur variable inconnue: " + asg->name); return v; }
            if (auto un = std::dynamic_pointer_cast<UnaryExpr>(e)) return evalUnary(un);
            if (auto bin = std::dynamic_pointer_cast<BinaryExpr>(e)) return evalBinary(bin);
            if (auto call = std::dynamic_pointer_cast<CallExpr>(e)) return evalCall(call);
            throw QError("Expression non supportée (bug parser?)");
        }

        void execute(const StmtPtr& s) {
            if (auto es = std::dynamic_pointer_cast<ExprStmt>(s)) { evaluate(es->expr); return; }
            if (auto ls = std::dynamic_pointer_cast<LetStmt>(s)) { Value v; if (ls->initializer) v = evaluate(ls->initializer); env->define(ls->name, v); return; }
            if (auto bs = std::dynamic_pointer_cast<BlockStmt>(s)) { execBlock(bs->statements, std::make_shared<Environment>(env)); return; }
            if (auto is = std::dynamic_pointer_cast<IfStmt>(s)) { if (isTruthy(evaluate(is->cond))) execute(is->thenBranch); else if (is->elseBranch) execute(is->elseBranch); return; }
            if (auto ws = std::dynamic_pointer_cast<WhileStmt>(s)) { while (isTruthy(evaluate(ws->cond))) execute(ws->body); return; }
            if (auto fs = std::dynamic_pointer_cast<FunctionStmt>(s)) { auto fn = std::make_shared<Function>(fs->params, fs->body, env); env->define(fs->name, Value(fn)); return; }
            if (auto rs = std::dynamic_pointer_cast<ReturnStmt>(s)) { Value v; if (rs->value) v = evaluate(rs->value); throw ReturnJump{ v }; }
            throw QError("Statement non supporté");
        }

        void execBlock(const std::vector<StmtPtr>& stmts, std::shared_ptr<Environment> newEnv) {
            auto prev = env; env = newEnv; try { for (const auto& s : stmts) execute(s); }
            catch (...) { env = prev; throw; } env = prev;
        }

    private:
        static bool isTruthy(const Value& v) {
            if (std::holds_alternative<Value::Prim>(v.data)) {
                const auto& p = std::get<Value::Prim>(v.data);
                if (std::holds_alternative<std::nullptr_t>(p)) return false;
                if (std::holds_alternative<bool>(p)) return std::get<bool>(p);
                if (std::holds_alternative<double>(p)) return std::get<double>(p) != 0.0;
                if (std::holds_alternative<std::string>(p)) return !std::get<std::string>(p).empty();
            }
            return true;
        }

        static double asNumber(const Value& v, const char* ctx) {
            if (std::holds_alternative<Value::Prim>(v.data) && std::holds_alternative<double>(std::get<Value::Prim>(v.data)))
                return std::get<double>(std::get<Value::Prim>(v.data));
            throw QError(std::string("Attendu number dans ") + ctx);
        }

        static bool equal(const Value& a, const Value& b) { return toString(a) == toString(b); }

        Value evalUnary(const std::shared_ptr<UnaryExpr>& u) {
            Value r = evaluate(u->right);
            if (u->op == TokenType::MINUS) return Value(-asNumber(r, "unaire -"));
            if (u->op == TokenType::BANG) return Value(!isTruthy(r));
            throw QError("Opérateur unaire inconnu");
        }

        Value evalBinary(const std::shared_ptr<BinaryExpr>& b) {
            Value l = evaluate(b->left); Value r = evaluate(b->right);
            switch (b->op) {
            case TokenType::PLUS: {
                if (l.isPrim() && r.isPrim()) {
                    const auto& lp = std::get<Value::Prim>(l.data); const auto& rp = std::get<Value::Prim>(r.data);
                    if (std::holds_alternative<double>(lp) && std::holds_alternative<double>(rp)) return Value(std::get<double>(lp) + std::get<double>(rp));
                    return Value(toString(l) + toString(r));
                }
                return Value(toString(l) + toString(r));
            }
            case TokenType::MINUS: return Value(asNumber(l, "-") - asNumber(r, "-"));
            case TokenType::STAR: return Value(asNumber(l, "*") * asNumber(r, "*"));
            case TokenType::SLASH: return Value(asNumber(l, "/") / asNumber(r, "/"));
            case TokenType::PERCENT: return Value(std::fmod(asNumber(l, "%"), asNumber(r, "%")));
            case TokenType::LESS: return Value(asNumber(l, "<") < asNumber(r, "<"));
            case TokenType::LESS_EQUAL: return Value(asNumber(l, "<=") <= asNumber(r, "<="));
            case TokenType::GREATER: return Value(asNumber(l, ">") > asNumber(r, ">"));
            case TokenType::GREATER_EQUAL: return Value(asNumber(l, ">=") >= asNumber(r, ">="));
            case TokenType::EQUAL_EQUAL: return Value(equal(l, r));
            case TokenType::BANG_EQUAL: return Value(!equal(l, r));
            case TokenType::AND_AND: return Value(isTruthy(l) && isTruthy(r));
            case TokenType::OR_OR: return Value(isTruthy(l) || isTruthy(r));
            default: throw QError("Opérateur binaire inconnu");
            }
        }

        Value callUserFunction(const std::shared_ptr<Function>& fn, const std::vector<Value>& args) {
            auto newEnv = std::make_shared<Environment>(fn->closure);
            if (args.size() != fn->params.size()) throw QError("Mauvais nombre d'arguments");
            for (size_t i = 0; i < args.size(); ++i) newEnv->define(fn->params[i], args[i]);
            try { execBlock(fn->body->statements, newEnv); }
            catch (const ReturnJump& rj) { return rj.value; }
            return Value();
        }

        Value evalCall(const std::shared_ptr<CallExpr>& c) {
            Value callee = env->get(c->callee);
            std::vector<Value> args; args.reserve(c->args.size());
            for (auto& a : c->args) args.push_back(evaluate(a));

            if (std::holds_alternative<std::shared_ptr<Function>>(callee.data))
                return callUserFunction(std::get<std::shared_ptr<Function>>(callee.data), args);
            if (std::holds_alternative<std::shared_ptr<NativeFunction>>(callee.data)) {
                auto nf = std::get<std::shared_ptr<NativeFunction>>(callee.data);
                // Convert back to Value
                auto prim = (*nf)(extractPrimArgs(args));
                return Value(prim);
            }
            throw QError("Tentative d'appel sur une valeur non appelable");
        }

        static std::vector<Value::Prim> extractPrimArgs(const std::vector<Value>& values) {
            std::vector<Value::Prim> out; out.reserve(values.size());
            for (const auto& v : values) {
                if (!std::holds_alternative<Value::Prim>(v.data)) throw QError("Seules les valeurs primitives sont passées aux natives");
                out.push_back(std::get<Value::Prim>(v.data));
            }
            return out;
        }
    };

    class VM {
        Interpreter interp;
    public:
        VM() {
            registerFunction("print", [](const std::vector<Value::Prim>& args)->Value::Prim {
                for (size_t i = 0; i < args.size(); ++i) {
                    const auto& a = args[i];
                    if (std::holds_alternative<std::nullptr_t>(a)) std::cout << "nil";
                    else if (std::holds_alternative<double>(a)) std::cout << std::get<double>(a);
                    else if (std::holds_alternative<std::string>(a)) std::cout << std::get<std::string>(a);
                    else if (std::holds_alternative<bool>(a)) std::cout << (std::get<bool>(a) ? "true" : "false");
                    if (i + 1 < args.size()) std::cout << " ";
                }
                std::cout << "\n"; return nullptr; });
        }

        void registerFunction(const std::string& name, NativeFunction fn) {
            interp.globals->define(name, Value(std::make_shared<NativeFunction>(std::move(fn))));
        }

        void eval(const std::string& source) {
            Lexer lex(source); auto tokens = lex.scanTokens();
            Parser parser(tokens); auto stmts = parser.parse();
            for (auto& s : stmts) interp.execute(s);
        }

        Value evalExpr(const std::string& exprSource) {
            Lexer lex(exprSource + ";"); auto tokens = lex.scanTokens();
            Parser parser(tokens); auto stmts = parser.parse();
            if (stmts.size() != 1) throw QError("evalExpr attend une seule expression");
            auto es = std::dynamic_pointer_cast<ExprStmt>(stmts[0]);
            if (!es) throw QError("evalExpr attend une expression");
            return interp.evaluate(es->expr);
        }
    };
}