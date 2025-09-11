#pragma once

#include <memory>
#include <string>
#include <variant>
#include <vector>

#include "Token.h"

namespace QuasarEngine {
    struct Expr; struct Stmt; struct Function; struct Environment;
    using ExprPtr = std::shared_ptr<Expr>; using StmtPtr = std::shared_ptr<Stmt>;

    struct Expr { virtual ~Expr() = default; };
    struct Stmt { virtual ~Stmt() = default; };

    struct LiteralExpr : Expr {
        std::variant<std::nullptr_t, double, std::string, bool> value;
        template<typename T> explicit LiteralExpr(T v) : value(std::move(v)) {}
    };
    struct VariableExpr : Expr { std::string name; explicit VariableExpr(std::string n) :name(std::move(n)) {} };
    struct AssignExpr : Expr { std::string name; ExprPtr value; AssignExpr(std::string n, ExprPtr v) :name(std::move(n)), value(std::move(v)) {} };
    struct UnaryExpr : Expr { TokenType op; ExprPtr right; UnaryExpr(TokenType o, ExprPtr r) :op(o), right(std::move(r)) {} };
    struct BinaryExpr : Expr { ExprPtr left; TokenType op; ExprPtr right; BinaryExpr(ExprPtr l, TokenType o, ExprPtr r) :left(std::move(l)), op(o), right(std::move(r)) {} };
    struct LogicalExpr : Expr { ExprPtr left; TokenType op; ExprPtr right; LogicalExpr(ExprPtr l, TokenType o, ExprPtr r) :left(std::move(l)), op(o), right(std::move(r)) {} };
    struct CallExpr : Expr { ExprPtr callee; std::vector<ExprPtr> args; CallExpr(ExprPtr c, std::vector<ExprPtr> a) :callee(std::move(c)), args(std::move(a)) {} };
    struct FnExpr : Expr { std::vector<std::string> params; std::shared_ptr<struct BlockStmt> body; FnExpr(std::vector<std::string> p, std::shared_ptr<struct BlockStmt> b) :params(std::move(p)), body(std::move(b)) {} };

    struct ExprStmt : Stmt { ExprPtr expr; explicit ExprStmt(ExprPtr e) :expr(std::move(e)) {} };
    struct LetStmt : Stmt { std::string name; ExprPtr initializer; LetStmt(std::string n, ExprPtr i) :name(std::move(n)), initializer(std::move(i)) {} };
    struct BlockStmt : Stmt { std::vector<StmtPtr> statements; explicit BlockStmt(std::vector<StmtPtr> s) :statements(std::move(s)) {} };
    struct IfStmt : Stmt { ExprPtr cond; StmtPtr thenBranch; StmtPtr elseBranch; IfStmt(ExprPtr c, StmtPtr t, StmtPtr e) :cond(std::move(c)), thenBranch(std::move(t)), elseBranch(std::move(e)) {} };
    struct WhileStmt : Stmt { ExprPtr cond; StmtPtr body; WhileStmt(ExprPtr c, StmtPtr b) :cond(std::move(c)), body(std::move(b)) {} };
    struct FunctionStmt : Stmt { std::string name; std::vector<std::string> params; std::shared_ptr<BlockStmt> body; FunctionStmt(std::string n, std::vector<std::string> p, std::shared_ptr<BlockStmt> b) :name(std::move(n)), params(std::move(p)), body(std::move(b)) {} };
    struct ReturnStmt : Stmt { ExprPtr value; explicit ReturnStmt(ExprPtr v) :value(std::move(v)) {} };
}
