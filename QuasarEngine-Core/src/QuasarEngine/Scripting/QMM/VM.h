#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <variant>
#include <memory>
#include <random>
#include <sstream>
#include <ctime>
#include <cmath>
#include <limits>

#include "Interpreter.h"
#include "Parser.h"
#include "Lexer.h"

namespace QuasarEngine {
    class VM {
        Interpreter interp;
        std::mt19937 rng{ std::random_device{}() };
    public:
        VM() {
            registerFunction("print", [this](const std::vector<Value::Prim>& args)->Value::Prim {
                for (size_t i = 0; i < args.size(); ++i) {
                    const auto& a = args[i];
                    if (std::holds_alternative<std::nullptr_t>(a)) std::cout << "nil";
                    else if (std::holds_alternative<double>(a)) std::cout << std::get<double>(a);
                    else if (std::holds_alternative<std::string>(a)) std::cout << std::get<std::string>(a);
                    else if (std::holds_alternative<bool>(a)) std::cout << (std::get<bool>(a) ? "true" : "false");
                    if (i + 1 < args.size()) std::cout << " ";
                }
                std::cout << "\n"; return nullptr; });
            
            registerFunction("str", [](const std::vector<Value::Prim>& args)->Value::Prim {
                if (args.empty()) return std::string{};
                const auto& a = args[0];
                if (std::holds_alternative<std::nullptr_t>(a)) return std::string("nil");
                if (std::holds_alternative<double>(a)) { std::ostringstream o; o << std::get<double>(a); return o.str(); }
                if (std::holds_alternative<std::string>(a)) return std::get<std::string>(a);
                if (std::holds_alternative<bool>(a)) return std::string(std::get<bool>(a) ? "true" : "false");
                return std::string{};
                });
            
            registerFunction("len", [](const std::vector<Value::Prim>& args)->Value::Prim {
                if (args.empty() || !std::holds_alternative<std::string>(args[0])) return 0.0; return double(std::get<std::string>(args[0]).size()); });
            
            registerFunction("substr", [](const std::vector<Value::Prim>& args)->Value::Prim {
                if (args.size() < 2 || !std::holds_alternative<std::string>(args[0])) return std::string{};
                const auto& s = std::get<std::string>(args[0]); int i = int(args.size() > 1 && std::holds_alternative<double>(args[1]) ? std::get<double>(args[1]) : 0); int n = int(args.size() > 2 && std::holds_alternative<double>(args[2]) ? std::get<double>(args[2]) : (int)s.size() - i);
                if (i < 0) i = 0; if (n < 0) n = 0; if (i > (int)s.size()) return std::string{}; if (i + n > (int)s.size()) n = (int)s.size() - i; return s.substr((size_t)i, (size_t)n);
                });
            
            registerFunction("sin", [](const std::vector<Value::Prim>& a)->Value::Prim { return std::sin(a.empty() ? 0.0 : (std::holds_alternative<double>(a[0]) ? std::get<double>(a[0]) : 0.0)); });
            registerFunction("cos", [](const std::vector<Value::Prim>& a)->Value::Prim { return std::cos(a.empty() ? 0.0 : (std::holds_alternative<double>(a[0]) ? std::get<double>(a[0]) : 0.0)); });
            registerFunction("sqrt", [](const std::vector<Value::Prim>& a)->Value::Prim { double x = a.empty() ? 0.0 : (std::holds_alternative<double>(a[0]) ? std::get<double>(a[0]) : 0.0); if (x < 0) return std::numeric_limits<double>::quiet_NaN(); return std::sqrt(x); });
            registerFunction("rand", [this](const std::vector<Value::Prim>&)->Value::Prim { std::uniform_real_distribution<double> d(0.0, 1.0); return d(rng); });
            registerFunction("clock", [](const std::vector<Value::Prim>&)->Value::Prim { return double(std::clock()) / double(CLOCKS_PER_SEC); });
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
            if (stmts.size() != 1) throw QError({ 1,1 }, "evalExpr attend une seule expression");
            auto es = std::dynamic_pointer_cast<ExprStmt>(stmts[0]);
            if (!es) throw QError({ 1,1 }, "evalExpr attend une expression");
            try { return interp.evaluate(es->expr); }
            catch (const ReturnJump&) { throw QError({ 1,1 }, "return hors fonction"); }
        }
    };
}
