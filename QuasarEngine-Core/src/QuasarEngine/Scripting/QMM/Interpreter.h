#pragma once

#include <memory>
#include <string>
#include <variant>
#include <vector>

#include "AST.h"
#include "Environment.h"
#include "Value.h"

namespace QuasarEngine {
    class Interpreter {
    public:
        std::shared_ptr<Environment> globals = std::make_shared<Environment>();
        std::shared_ptr<Environment> env = globals;

        Value evaluate(const ExprPtr& e) {
            if (auto lit = std::dynamic_pointer_cast<LiteralExpr>(e)) return Value(lit->value);
            if (auto var = std::dynamic_pointer_cast<VariableExpr>(e)) return env->get(var->name);
            if (auto asg = std::dynamic_pointer_cast<AssignExpr>(e)) { auto v = evaluate(asg->value); if (!env->assign(asg->name, v)) throw QError({ 1,1 }, "Affectation sur variable inconnue: " + asg->name); return v; }
            if (auto un = std::dynamic_pointer_cast<UnaryExpr>(e)) return evalUnary(un);
            if (auto bin = std::dynamic_pointer_cast<BinaryExpr>(e)) return evalBinary(bin);
            if (auto log = std::dynamic_pointer_cast<LogicalExpr>(e)) return evalLogical(log);
            if (auto cal = std::dynamic_pointer_cast<CallExpr>(e)) return evalCall(cal);
            if (auto fn = std::dynamic_pointer_cast<FnExpr>(e)) return Value(std::make_shared<Function>(fn->params, fn->body, env));
            throw QError({ 1,1 }, "Expression non supportée (bug parser?)");
        }

        void execute(const StmtPtr& s) {
            if (auto es = std::dynamic_pointer_cast<ExprStmt>(s)) { evaluate(es->expr); return; }
            if (auto ls = std::dynamic_pointer_cast<LetStmt>(s)) { Value v; if (ls->initializer) v = evaluate(ls->initializer); env->define(ls->name, v); return; }
            if (auto bs = std::dynamic_pointer_cast<BlockStmt>(s)) { execBlock(bs->statements, std::make_shared<Environment>(env)); return; }
            if (auto is = std::dynamic_pointer_cast<IfStmt>(s)) { if (isTruthy(evaluate(is->cond))) execute(is->thenBranch); else if (is->elseBranch) execute(is->elseBranch); return; }
            if (auto ws = std::dynamic_pointer_cast<WhileStmt>(s)) { while (isTruthy(evaluate(ws->cond))) execute(ws->body); return; }
            if (auto fs = std::dynamic_pointer_cast<FunctionStmt>(s)) { auto fn = std::make_shared<Function>(fs->params, fs->body, env); env->define(fs->name, Value(fn)); return; }
            if (auto rs = std::dynamic_pointer_cast<ReturnStmt>(s)) { Value v; if (rs->value) v = evaluate(rs->value); throw ReturnJump{ v }; }
            throw QError({ 1,1 }, "Statement non supporté");
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
            throw QError({ 1,1 }, std::string("Attendu number dans ") + ctx);
        }

        Value evalUnary(const std::shared_ptr<UnaryExpr>& u) {
            Value r = evaluate(u->right);
            if (u->op == TokenType::MINUS) return Value(-asNumber(r, "unaire -"));
            if (u->op == TokenType::BANG)  return Value(!isTruthy(r));
            throw QError({ 1,1 }, "Opérateur unaire inconnu");
        }

        Value evalLogical(const std::shared_ptr<LogicalExpr>& l) {
            Value left = evaluate(l->left);
            if (l->op == TokenType::OR_OR) {
                if (isTruthy(left)) return Value(true); // short-circuit
                Value right = evaluate(l->right); return Value(isTruthy(right));
            }
            else {
                if (!isTruthy(left)) return Value(false);
                Value right = evaluate(l->right); return Value(isTruthy(right));
            }
        }

        Value evalBinary(const std::shared_ptr<BinaryExpr>& b) {
            Value l = evaluate(b->left); Value r = evaluate(b->right);
            switch (b->op) {
            case TokenType::PLUS:       return Value(asNumber(l, "+") + asNumber(r, "+"));
            case TokenType::MINUS:      return Value(asNumber(l, "-") - asNumber(r, "-"));
            case TokenType::STAR:       return Value(asNumber(l, "*") * asNumber(r, "*"));
            case TokenType::SLASH: { double denom = asNumber(r, "/"); if (denom == 0.0) throw QError({ 1,1 }, "Division par zéro"); return Value(asNumber(l, "/") / denom); }
            case TokenType::PERCENT: { double denom = asNumber(r, "%"); if (denom == 0.0) throw QError({ 1,1 }, "Modulo par zéro"); return Value(std::fmod(asNumber(l, "%"), denom)); }
            case TokenType::LESS:            return Value(asNumber(l, "<") < asNumber(r, "<"));
            case TokenType::LESS_EQUAL:      return Value(asNumber(l, "<=") <= asNumber(r, "<="));
            case TokenType::GREATER:         return Value(asNumber(l, ">") > asNumber(r, ">"));
            case TokenType::GREATER_EQUAL:   return Value(asNumber(l, ">=") >= asNumber(r, ">="));
            case TokenType::EQUAL_EQUAL:     return Value(valueEqual(l, r));
            case TokenType::BANG_EQUAL:      return Value(!valueEqual(l, r));
            default: throw QError({ 1,1 }, "Opérateur binaire inconnu");
            }
        }

        Value callUserFunction(const std::shared_ptr<Function>& fn, const std::vector<Value>& args) {
            auto newEnv = std::make_shared<Environment>(fn->closure);
            if (args.size() != fn->params.size()) throw QError({ 1,1 }, "Mauvais nombre d'arguments");
            for (size_t i = 0; i < args.size(); ++i) newEnv->define(fn->params[i], args[i]);
            try { execBlock(fn->body->statements, newEnv); }
            catch (const ReturnJump& rj) { return rj.value; }
            return Value();
        }

        static std::vector<Value::Prim> extractPrimArgs(const std::vector<Value>& values) {
            std::vector<Value::Prim> out; out.reserve(values.size());
            for (const auto& v : values) {
                if (!std::holds_alternative<Value::Prim>(v.data)) throw QError({ 1,1 }, "Seules les valeurs primitives sont passées aux natives");
                out.push_back(std::get<Value::Prim>(v.data));
            }
            return out;
        }

        Value evalCall(const std::shared_ptr<CallExpr>& c) {
            Value callee = evaluate(c->callee);
            std::vector<Value> args; args.reserve(c->args.size());
            for (auto& a : c->args) args.push_back(evaluate(a));

            if (std::holds_alternative<std::shared_ptr<Function>>(callee.data))
                return callUserFunction(std::get<std::shared_ptr<Function>>(callee.data), args);
            if (std::holds_alternative<std::shared_ptr<NativeFunction>>(callee.data)) {
                auto nf = std::get<std::shared_ptr<NativeFunction>>(callee.data);
                auto prim = (*nf)(extractPrimArgs(args));
                return Value(prim);
            }
            throw QError({ 1,1 }, "Tentative d'appel sur une valeur non appelable");
        }
    };
}
